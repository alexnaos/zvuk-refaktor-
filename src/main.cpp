#include <Arduino.h>
#include <WiFi.h>
#include <Update.h>
#include <GyverDBFile.h>
#include <ESPmDNS.h>
#include "esp_task_wdt.h" // Подключаем встроенный WDT
#include "config.h"
#include "global_state.h"
#include "web_server.h"
#include "mqtt_module.h"
#include "core/fs_module.h"
#include "core/playlist_parser.h"
#include "core/audio_playback.h"
#include "core/audio_recorder.h"
#include "core/display_module.h"
#include "core/system_logger.h"
#include "core/encoder_module.h"
#include "core/menu_state.h"

unsigned long lastWiFiCheck = 0;
const unsigned long wifiCheckInterval = WIFI_CHECK_INTERVAL_MS;
unsigned long lastDisplayUpdate = 0;
unsigned long lastHealthCheck = 0;

// ПЕРЕМЕННЫЕ ДЛЯ БЕЗОПАСНОГО АСИНХРОННОГО РОУМИНГА
unsigned long lastRoamingAttempt = 0;
const unsigned long roamingDelay = WIFI_ROAMING_DELAY_MS; // Не роумить чаще чем раз в 30 секунд

// Флаг: показывать дисплей с меню (энкодер) или стандартный (трек)
static volatile bool menuModeActive = false;

// === WATCHDOG АВТОВОССТАНОВЛЕНИЯ ПОСЛЕ КРАХА ===
// RTC-память сохраняется при перезагрузках WDT/сбоях
RTC_DATA_ATTR static unsigned long crashCount = 0;      // Счетчик последовательных падений
RTC_DATA_ATTR static unsigned long lastCrashTime = 0;    // Время последнего падения (uptime)

// Максимальное количество быстрых перезагрузок перед полным сбросом конфигурации
#define MAX_CRASH_BEFORE_RESET 3

// Порог "быстрого падения" — если краш произошел раньше чем через 120 секунд после старта
#define CRASH_WINDOW_SEC 120

// Защита от бесконечного цикла перезагрузок: если крашей > MAX до истечения окна — сбрасываем
void checkAndHandleCrashLoop() {
    unsigned long uptimeSec = millis() / 1000;
    
    // Если с последнего краша прошло больше CRASH_WINDOW_SEC — сбрасываем счетчик
    if (lastCrashTime > 0 && uptimeSec - lastCrashTime > CRASH_WINDOW_SEC) {
        crashCount = 0;
    }
    
    crashCount++;
    lastCrashTime = uptimeSec;
    
    if (crashCount > MAX_CRASH_BEFORE_RESET) {
        // Критический цикл перезагрузок — сбрасываем конфигурацию
        sysLog("err:", "WATCHDOG", "Обнаружен цикл крашей! Сброс конфигурации...");
        if (LittleFS.exists("/db.bin")) {
            LittleFS.remove("/db.bin");
            sysLog("info:", "WATCHDOG", "Файл БД удален. Настройки будут сброшены.");
        }
        crashCount = 0;
    }
    
    sysLog("warn:", "WATCHDOG", "Краш #" + String(crashCount) + 
           " (uptime=" + String(uptimeSec) + "с, окно=" + String(CRASH_WINDOW_SEC) + "с)");
}

// Периодический мониторинг здоровья системы
static unsigned long lastHealthMonitor = 0;
#define HEALTH_MONITOR_INTERVAL_MS 30000  // Каждые 30 секунд

void systemHealthMonitor() {
    unsigned long now = millis();
    if (now - lastHealthMonitor < HEALTH_MONITOR_INTERVAL_MS) return;
    lastHealthMonitor = now;
    
    uint32_t freeHeap = ESP.getFreeHeap();
    // ИСПРАВЛЕНО (деградация через часы работы): контролируем не только свободную
    // ОЗУ, но и максимальный непрерывный блок (maxAllocHeap). При фрагментации кучи
    // freeHeap может быть > 20 КБ, а непрерывный блок уже меньше, чем нужен для
    // TCP/TLS-соединения — именно поэтому радио "переставало принимать станции".
    uint32_t maxBlock = ESP.getMaxAllocHeap();
    
    // Если ОЗУ < 20 КБ или непрерывный блок < 20 КБ — критично, пытаемся освободить память
    if (freeHeap < 20000 || maxBlock < 20000) {
        sysLog("warn:", "WATCHDOG", "Критически мало ОЗУ: " + String(freeHeap) +
               " Б (макс. блок: " + String(maxBlock) + " Б). Очистка...");
        // Принудительно очищаем буфер логгера для освобождения памяти
        webLogger.clear();
        
        // Если всё еще мало — инициируем перезагрузку
        if (ESP.getFreeHeap() < 15000 || ESP.getMaxAllocHeap() < 15000) {
            sysLog("err:", "WATCHDOG", "ОЗУ исчерпано (free=" + String(ESP.getFreeHeap()) +
                   " Б, блок=" + String(ESP.getMaxAllocHeap()) + " Б). Аварийная перезагрузка...");
            delay(100);
            ESP.restart();
        }
    }
    
    // Мониторинг VS1053
    if (player != nullptr) {
        uint16_t vol = player->getVolume();
        if (vol > 254) {
            sysLog("err:", "WATCHDOG", "SPI сбой VS1053 (vol=" + String(vol) + "). Реинициализация...");
            player->stop_mp3client();
            player->begin();
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    // УВЕЛИЧИВАЕМ ТАЙМАУТ ДО 30 СЕКУНД
    // 30 секунд — стандарт для устройств с OTA-обновлением по воздуху.
    // Защита от зависаний остается, но теперь WDT не прервет загрузку файлов.
    esp_task_wdt_init(WDT_TIMEOUT_SEC, true); 
    esp_task_wdt_add(NULL); // Привязываем WDT к главному loop()

    RESET_REASON r0 = rtc_get_reset_reason(0);
    sysLog("warn:", "АППАРАТ", "Причина прошлого сброса CPU: " + getResetReason(r0));
    
    printPreviousCrashLog(); 
    sysLog("info:", "СТАРТ", "Запуск радио. Инициализация периферии...");
    
    initDisplay();
    initFileSystem();
    loadPlaylist();
    initAudio();
    
    WiFi.mode(WIFI_MODE_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    sysLog("info:", "СЕТЬ", "Подключение к Wi-Fi: " + String(WIFI_SSID));
    
    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < WIFI_TIMEOUT_MS) {
        esp_task_wdt_reset(); // Кормим сторожа при старте
        delay(200);
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.setHostname(MDNS_NAME);
        sysLog("info:", "СЕТЬ", "Успешное подключение. IP: " + WiFi.localIP().toString());
        configTime(3600 * 3, 0, "pool.ntp.org", "ru.pool.ntp.org");

        // mDNS
        if (MDNS.begin(MDNS_NAME)) {
            sysLog("info:", "MDNS", String(MDNS_NAME) + ".local");
            MDNS.addService("http", "tcp", 80);
        } else {
            sysLog("err:", "MDNS", "Ошибка запуска");
        }
    } else {
        sysLog("err:", "СЕТЬ", "Ошибка Wi-Fi! Старт в автономном режиме.");
    }
    
    initWebServer();
    initMQTT();
    
    // Проверка на цикл крашей и автосброс конфигурации при необходимости
    checkAndHandleCrashLoop();
    
    logSystemHealth();
    
    // Инициализация энкодера и меню
    initEncoder();
    initMenu(currentStationIdx, currentVolume, currentBass, currentTreble);
    sysLog("info:", "ЭНКОДЕР", "Энкодер инициализирован (GPIO 1,2,3)");

    if (db.has(SH("st_id"))) {
        int savedStationIdx = db[SH("st_id")].toInt();
        if (savedStationIdx >= 0 && savedStationIdx < stationCount) {
            currentStationIdx = savedStationIdx;
            currentStationName = stationList[currentStationIdx].name;
            sysLog("info:", "СТАРТ", "Восстановлена станция: " + currentStationName);
            updateDisplay(currentStationName.c_str(), "Connecting...");
            startRadioStream(stationList[currentStationIdx].url.c_str());
        }
    }
}

void loop() {
    // Обработка энкодера
    loopEncoder();
    EncoderDirection encDir = readEncoder();
    ButtonState btnState = readButton();
    
    if (encDir != ENC_NONE || btnState != BTN_NONE) {
        MenuAction action = processEncoderAction(encDir, btnState);
        
        // Применяем действия
        if (action.needsVolumeChange) {
            updateVolume(action.param);
        }
        if (action.needsToneChange) {
            // Для BASSA и TREBLE — вызываем updateTone с обоими параметрами
            // В menu_action param содержит изменённое значение,
            // второе значение остаётся прежним
            if (getCurrentMode() == MENU_BASS) {
                updateTone(action.param, currentTreble);
            } else if (getCurrentMode() == MENU_TREBLE) {
                updateTone(currentBass, action.param);
            }
        }
        if (action.needsStationChange) {
            changeStation(action.param);
        }
        if (action.needsDisplay) {
            menuModeActive = true; // Переключаем дисплей в режим меню
            // Обновляем дисплей с меню
            int val, mn, mx;
            const char* lbl;
            const char* ext;
            getMenuDisplayData(val, mn, mx, lbl, ext);
            updateMenuDisplay(getCurrentMode(), val, mn, mx, lbl, ext);
        }
    }
    
    // Автоматический выход из меню через 10 секунд бездействия
    static unsigned long lastMenuActivity = 0;
    if (menuModeActive) {
        if (encDir != ENC_NONE || btnState != BTN_NONE) {
            lastMenuActivity = millis();
        }
        if (millis() - lastMenuActivity > 10000) {
            menuModeActive = false;
        }
    }

    // Вызов обработчика веб-запросов Гайвера
    handleWebRequests();
    
    // Проверка связи и асинхронный роуминг
    if (millis() - lastWiFiCheck > wifiCheckInterval) {
        lastWiFiCheck = millis();
        
        if (WiFi.status() != WL_CONNECTED) {
            sysLog("warn:", "СЕТЬ", "Связь потеряна! Мягкое переподключение...");
            if (WiFi.status() == WL_DISCONNECTED || WiFi.status() == WL_IDLE_STATUS) {
                WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            }
        } else {
            if (WiFi.RSSI() < WIFI_RSSI_THRESHOLD) {
                if (millis() - lastRoamingAttempt > roamingDelay) {
                    lastRoamingAttempt = millis();
                    sysLog("warn:", "РОУМИНГ", "Слабый сигнал: " + String(WiFi.RSSI()) + " dBm. Ищу лучшую точку...");
                    WiFi.disconnect();
                    delay(500);
                    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
                }
            }
        }
    }
    
    if (millis() - lastHealthCheck > HEALTH_CHECK_INTERVAL_MS) {
        lastHealthCheck = millis();
        if (WiFi.status() == WL_CONNECTED) logSystemHealth();
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        loopMQTT();
        
        extern bool mqttTrackUpdateFlag;
        if (mqttTrackUpdateFlag) {
            mqttTrackUpdateFlag = false; 
            mqttPublishTrack(currentTrack.c_str()); 
        }
    }
    
    // Мониторинг здоровья системы (ОЗУ, SPI, автовосстановление)
    systemHealthMonitor();
    
    // Главная аудио-задача
    loopAudioPlayback();
    loopAudioRecorder();
    
    if (changeStationFlag) {
        changeStationFlag = false;
        if (currentStationIdx >= 0 && currentStationIdx < stationCount) {
            String url = stationList[currentStationIdx].url;
            updateDisplay(stationList[currentStationIdx].name.c_str(), "Connecting...");
            startRadioStream(url.c_str());
            updateDisplay(stationList[currentStationIdx].name.c_str(), "PLAYING");
            currentStationName = stationList[currentStationIdx].name;
        }
    }
    
    // Обновление дисплея: если активно меню — не перезаписываем меню стандартным экраном
    if (millis() - lastDisplayUpdate > DISPLAY_UPDATE_INTERVAL_MS) {
        lastDisplayUpdate = millis();
        if (!menuModeActive) {
            updateDisplay(currentStationName.c_str(), currentTrack.c_str());
        }
    }
}