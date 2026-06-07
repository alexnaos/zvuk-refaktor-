#include <Arduino.h>
#include <WiFi.h>
#include <Update.h>
#include <GyverDBFile.h>
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

unsigned long lastWiFiCheck = 0;
const unsigned long wifiCheckInterval = 10000;
unsigned long lastDisplayUpdate = 0;
unsigned long lastHealthCheck = 0;

// ПЕРЕМЕННЫЕ ДЛЯ БЕЗОПАСНОГО АСИНХРОННОГО РОУМИНГА
unsigned long lastRoamingAttempt = 0;
const unsigned long roamingDelay = 30000; // Не роумить чаще чем раз в 30 секунд

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    // УВЕЛИЧИВАЕМ ТАЙМАУТ ДО 30 СЕКУНД
    // 30 секунд — стандарт для устройств с OTA-обновлением по воздуху.
    // Защита от зависаний остается, но теперь WDT не прервет загрузку файлов.
    esp_task_wdt_init(30, true); 
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
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
        esp_task_wdt_reset(); // Кормим сторожа при старте
        delay(200);
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        sysLog("info:", "СЕТЬ", "Успешное подключение. IP: " + WiFi.localIP().toString());
        configTime(3600 * 3, 0, "pool.ntp.org", "ru.pool.ntp.org");
    } else {
        sysLog("err:", "СЕТЬ", "Ошибка Wi-Fi! Старт в автономном режиме.");
    }
    
    initWebServer();
    initMQTT();
    logSystemHealth();
    
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
    // Чистый, стандартный сброс без костылей
    esp_task_wdt_reset(); 

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
            if (WiFi.RSSI() < -82) {
                if (millis() - lastRoamingAttempt > roamingDelay) {
                    lastRoamingAttempt = millis();
                    sysLog("warn:", "РОУМИНГ", "Слабый сигнал: " + String(WiFi.RSSI()) + " dBm. Ищу лучшую точку...");
                    WiFi.disconnect(); 
                }
            }
        }
    }
    
    if (millis() - lastHealthCheck > 60000) {
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
    
    if (millis() - lastDisplayUpdate > 1000) {
        lastDisplayUpdate = millis();
        updateDisplay(currentStationName.c_str(), currentTrack.c_str());
    }
}
