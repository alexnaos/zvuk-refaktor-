#include "audio_playback.h"
#include "../config.h"
#include "../global_state.h"
#include "../mqtt_module.h" 
#include "system_logger.h"
#include <SPI.h>
#include <WiFi.h>

VS1053* player = nullptr;

// ============================================================================
// АВТОВОССТАНОВЛЕНИЕ ПОТОКА (лечение "тихой смерти" через 2-5 часов работы)
// ============================================================================
// Библиотека vs1053_ext при потере потока пытается переподключиться сама
// (streamDetection -> connecttohost(m_lastHost)), НО при первой неудаче
// обнуляет m_lastHost и окончательно останавливается (m_f_running=false).
// Приложение должно само отслеживать состояние плеера и поднимать поток.
static String       currentStreamUrl = "";    // URL активного потока для автовосстановления
static bool         userStopRequested = false;// Поток остановлен пользователем — не переподключаться
static unsigned long lastReconnectAttempt = 0;
static int          reconnectAttempts = 0;

void notifyUserStop() {
    userStopRequested = true;
    reconnectAttempts = 0;
    sysLog("info:", "ПЛЕЕР", "Поток остановлен пользователем (автопереподключение отключено)");
}

// Периодический контроль: если плеер должен играть, но молчит — переподключаемся
static void ensureStreamAlive() {
    if (player == nullptr || isRecording) return;
    if (currentStreamUrl.length() == 0) return;   // нет запущенного потока
    if (userStopRequested) return;                // пользователь остановил сам
    if (WiFi.status() != WL_CONNECTED) return;    // сеть не готова — подождём Wi-Fi watchdog

    if (player->isRunning()) {
        // Поток жив (библиотека сама успешно переподключилась) — сбрасываем счётчик
        if (reconnectAttempts != 0) reconnectAttempts = 0;
        return;
    }

    // Плеер молчит при активном потоке — это и есть "тихая смерть"
    if (millis() - lastReconnectAttempt < STREAM_RECONNECT_INTERVAL_MS) return;
    lastReconnectAttempt = millis();
    reconnectAttempts++;

    if (reconnectAttempts > MAX_STREAM_RECONNECT_ATTEMPTS) {
        // Вероятная причина устойчивого отказа — фрагментация кучи (не выделяется
        // память под TCP/TLS). Перезагрузка очищает кучу и восстанавливает станцию из БД.
        sysLog("err:", "АВТОРЕКОННЕКТ", "Поток не восстановлен за " + String(MAX_STREAM_RECONNECT_ATTEMPTS) +
               " попыток. Перезагрузка для очистки ОЗУ...");
        delay(300);
        ESP.restart();
    }

    sysLog("warn:", "АВТОРЕКОННЕКТ", "Плеер остановлен (обрыв потока). Попытка #" +
           String(reconnectAttempts) + ": " + currentStreamUrl);

    bool ok = player->connecttohost(currentStreamUrl.c_str());
    if (ok) {
        player->setVolume(currentVolume);
        uint8_t toneData[] = { (uint8_t)currentTreble, 6, (uint8_t)currentBass, 5 };
        player->setTone(toneData);
        reconnectAttempts = 0;
        sysLog("info:", "АВТОРЕКОННЕКТ", "Поток успешно восстановлен");
    } else {
        sysLog("err:", "АВТОРЕКОННЕКТ", "Не удалось подключиться (осталось попыток: " +
               String(MAX_STREAM_RECONNECT_ATTEMPTS - reconnectAttempts) + ")");
    }
}


// ============================================================================
// ДИАГНОСТИЧЕСКИЙ ЛОГ VS1053
// ============================================================================

// Функция читает и выводит основные регистры VS1053 для диагностики
static void logVS1053Registers() {
    if (player == nullptr) {
        sysLog("err:", "VS1053", "player == nullptr, невозможно прочитать регистры");
        return;
    }
    
    // Проверка DREQ пина (если он всегда LOW - чип завис или не проинициализирован)
    int dreqLevel = digitalRead(VS1053_DREQ);
    
    uint16_t vol    = player->getVolume();
    int      bitrate = player->getBitRate();
    size_t bufFilled = player->bufferFilled();
    size_t bufFree   = player->bufferFree();
    uint32_t heapFree = ESP.getFreeHeap();
    uint32_t psramFree = ESP.getFreePsram();
    bool running = player->isRunning();
    int codec = player->getCodec();
    
    sysLog("diag:", "VS1053",
        String("DREQ=") + (dreqLevel ? "HIGH" : "LOW") +
        " | run=" + (running ? "YES" : "NO") +
        " | vol=" + vol +
        " | bitrate=" + bitrate + "kbps" +
        " | codec=" + player->getCodecname() +
        " | bufFill=" + bufFilled +
        " | bufFree=" + bufFree +
        " | heap=" + heapFree +
        " | psram=" + psramFree);
    
    // DREQ=LOW при работающем плеере — это НОРМАЛЬНО!
    // DREQ переключается HIGH/LOW постоянно: HIGH когда чип готов принять данные,
    // LOW когда чип занят декодированием. Если DREQ остаётся LOW >5 секунд — вот тогда проблема.
    // Предупреждение было ложным — DREQ циклически меняет состояние в процессе работы.
    // (убрано ложное срабатывание)
    
    // Проверка на зависание по громкости (если > 254 - SPI сбой)
    if (vol > 254) {
        sysLog("err:", "VS1053", "Аппаратный сбой SPI! getVolume()=" + String(vol) + " (норма < 254)");
    }
    
    // Если буфер пуст — поток не идёт
    if (bufFilled == 0 && running) {
        sysLog("warn:", "VS1053", "Буфер пуст, но плеер запущен! Нет аудиоданных из сети.");
    }
}

// ============================================================================
// СИСТЕМНЫЕ ПЕРЕХВАТЧИКИ БИБЛИОТЕКИ WOLLE VS1053 (ГЛУБОКАЯ ОТЛАДКА)
// ============================================================================

// Глобальный флаг для асинхронной отправки MQTT (чтобы не блокировать плеер)
// Определен в global_state.cpp
extern bool mqttTrackUpdateFlag;

void vs1053_showstreamtitle(const char *info) {
    if (info != nullptr) {
        currentTrack = String(info);
        currentTrack.trim();
        
        mqttTrackUpdateFlag = true;
        
        sysLog("info:", "ДЕКОДЕР", "Трек: " + currentTrack);
    }
}


void vs1053_showstreaminfo(const char *info) {
    if (info != nullptr) {
        Serial.printf("[ПОТОК INFO]: %s\n", info);
        sysLog("info:", "ПОТОК_ИНФО", String(info));
    }
}

void vs1053_commercial(const char *info) {
    if (info != nullptr) {
        sysLog("warn:", "СЕТЬ_БУФЕР", "Поток прерван! Ожидание данных. Код: " + String(info));
    }
}

void vs1053_eof_mp3(const char *info) {
    if (info != nullptr) {
        sysLog("err:", "ОБРЫВ_СВЯЗИ", "Удаленный сервер закрыл поток: " + String(info));
    }
    if (player != nullptr) {
        player->stop_mp3client();
    }
}

void vs1053_bitrate(const char *info) {
    if (info != nullptr) {
        sysLog("info:", "БИТРЕЙТ", "Скорость потока: " + String(info) + " kbps");
    }
}

// ============================================================================
// ОСНОВНЫЕ ФУНКЦИИ УПРАВЛЕНИЯ АУДИОСИСТЕМОЙ
// ============================================================================

void initAudio() {
    sysLog("info:", "VS1053", "Старт инициализации VS1053...");
    
    // Замер времени инициализации
    unsigned long tStart = millis();
    
    pinMode(VS1053_RST, OUTPUT);
    digitalWrite(VS1053_RST, LOW);
    delay(50);
    
    // Проверка DREQ до снятия сброса — должен быть LOW (чип в сбросе)
    int dreqBefore = digitalRead(VS1053_DREQ);
    digitalWrite(VS1053_RST, HIGH);
    delay(50);
    
    // DREQ после сброса: должен подняться в HIGH в течение 100-500мс
    int dreqAfter = digitalRead(VS1053_DREQ);
    
    sysLog("diag:", "VS1053", String("RST: DREQ до=") + (dreqBefore ? "HIGH" : "LOW") + 
        " после=" + (dreqAfter ? "HIGH" : "LOW"));

    player = new VS1053(VS1053_CS, VS1053_DCS, VS1053_DREQ, HSPI, PIN_SPI_MOSI, PIN_SPI_MISO, PIN_SPI_SCK);
    player->begin();
    
    // ИСПРАВЛЕНО (деградация через часы работы): таймаут подключения по умолчанию
    // в библиотеке всего 250 мс (HTTP). При кратковременной деградации сети/DNS
    // реконнект к станции не успевал установиться, и библиотека окончательно
    // останавливалась. Увеличиваем таймауты: HTTP 6 сек, HTTPS 16 сек.
    player->setConnectionTimeout(6000, 16000);
    
    unsigned long tInit = millis() - tStart;
    
    // Диагностика после begin()
    int chipVersion = player->printVersion();
    uint16_t volAfter = player->getVolume();
    bool isRunning = player->isRunning();
    size_t bufFilled = player->bufferFilled();
    uint32_t heapFree = ESP.getFreeHeap();
    uint32_t psramFree = ESP.getFreePsram();
    
    sysLog("diag:", "VS1053", String("initTime=") + tInit + "ms" +
        " | chipVer=" + chipVersion +
        " | vol=" + volAfter +
        " | running=" + (isRunning ? "YES" : "NO") +
        " | codec=" + player->getCodecname() +
        " | bufFill=" + bufFilled +
        " | heap=" + heapFree +
        " | psram=" + psramFree);
    
    // Если версия чипа 0 — SPI не работает
    if (chipVersion == 0) {
        sysLog("err:", "VS1053", "printVersion() вернул 0! SPI не работает!");
        sysLog("err:", "VS1053", "  Проверьте питание модуля (3.3В) и пины SPI");
    } else {
        sysLog("info:", "VS1053", String("Версия чипа: ") + chipVersion + " (VS1053 ожидается >0)");
    }
    
    // Если volume > 254 — SPI сбой при первом чтении
    if (volAfter > 254) {
        sysLog("err:", "VS1053", "Сбой SPI при первой установке громкости! vol=" + String(volAfter));
    }
    
    // Если DREQ не поднялся — проблема
    if (dreqAfter == 0) {
        sysLog("err:", "VS1053", "DREQ остаётся LOW после begin()! Чип не стартовал.");
    }
    
    player->setVolume(currentVolume);

    uint8_t toneData[] = { (uint8_t)currentTreble, 6, (uint8_t)currentBass, 5 };
    player->setTone(toneData);
    
    unsigned long tEnd = millis() - tStart;
    sysLog("info:", "VS1053", String("Инициализация завершена за ") + tEnd + "мс");
}

void startRadioStream(const char* url) {
    if (player != nullptr && !isRecording) {
        // Запоминаем URL для автовосстановления и сбрасываем состояние реконнекта
        currentStreamUrl = String(url);
        userStopRequested = false;
        reconnectAttempts = 0;
        lastReconnectAttempt = millis();
        
        currentTrack = "Loading...";
        mqttPublishTrack(currentTrack.c_str());
        
        sysLog("info:", "ПЛЕЕР", "Подключение к: " + String(url));
        
        // ИСПРАВЛЕНО: результат подключения теперь проверяется. Раньше при неудаче
        // устройство показывало "PLAYING", но поток не поднимался и не повторялся.
        bool ok = player->connecttohost(url);
        if (ok) {
            player->setVolume(currentVolume);
        } else {
            sysLog("err:", "ПЛЕЕР", "Подключение не удалось. Автоповтор через " +
                   String(STREAM_RECONNECT_INTERVAL_MS / 1000) + " сек.");
        }
        
        // Сразу после запуска потока — диагностика
        logVS1053Registers();
    }
}

void changeStation(int stationIndex) {
    extern Station stationList[MAX_STATIONS];
    extern int stationCount;
    extern int currentStationIdx;
    extern String currentStationName;
    extern bool changeStationFlag;
    
    if (stationIndex >= 0 && stationIndex < stationCount) {
        currentStationIdx = stationIndex;
        currentStationName = stationList[stationIndex].name;
        changeStationFlag = true; // main.cpp обработает в loop()
        sysLog("info:", "ЭНКОДЕР", "Выбрана станция: " + currentStationName);
    }
}

void updateVolume(int vol) {
    if (player != nullptr) {
        currentVolume = constrain(vol, VOLUME_MIN, VOLUME_MAX);
        player->setVolume(currentVolume);
        
        Serial.printf("[Звук]: Новая громкость: %d\n", currentVolume);
        sysLog("info:", "ЗВУК", "Новая громкость: " + String(currentVolume) + 
            " (getVolume=" + player->getVolume() + ")");
    }
}

void updateTone(int bass, int treble) {
    if (player != nullptr) {
        currentBass = constrain(bass, BASS_MIN, BASS_MAX);
        currentTreble = constrain(treble, TREBLE_MIN, TREBLE_MAX);
        
        uint8_t toneData[] = { (uint8_t)currentTreble, 6, (uint8_t)currentBass, 5 };
        player->setTone(toneData);
        
        Serial.printf("[Звук]: Эквалайзер -> БАС: %d, ТРЕБЛ: %d\n", currentBass, currentTreble);
        sysLog("info:", "ЗВУК", "Эквалайзер -> БАС: " + String(currentBass) + ", ТРЕБЛ: " + String(currentTreble));
    }
}

// Таймеры для диагностики в loop
static unsigned long lastDiagLog = 0;
// ИСПРАВЛЕНО: Увеличено с 5 до 30 секунд для уменьшения нагрузки на логгер и предотвращения
// переполнения буфера webLogger. Диагностика раз в 5 секунд была избыточной.
static const unsigned long DIAG_LOG_INTERVAL_MS = 30000; // Лог каждые 30 секунд

void loopAudioPlayback() {
    if (player != nullptr && !isRecording) {
        player->loop();
        
        // ГЛАВНОЕ ЛЕЧЕНИЕ "ТИХОЙ СМЕРТИ": если поток должен играть, но плеер
        // остановлен (сервер закрыл соединение, неудачный реконнект библиотеки,
        // реинициализация после SPI-сбоя) — поднимаем поток заново.
        ensureStreamAlive();
        
        // Диагностика VS1053 каждые 30 секунд
        if (millis() - lastDiagLog > DIAG_LOG_INTERVAL_MS) {
            lastDiagLog = millis();
            logVS1053Registers();
        }
        
        // Автоматическая проверка "зависания" шины SPI декодера
        // Раз в 30 секунд читаем статус громкости. Если на проводах наводка — чип вернет > 254.
        // ИСПРАВЛЕНО: после реинициализации чипа поток теперь НЕ теряется навсегда —
        // ensureStreamAlive() выше сам переподключит его на следующем цикле.
        static unsigned long lastHardwareCheck = 0;
        if (millis() - lastHardwareCheck > HARDWARE_CHECK_INTERVAL_MS) {
            lastHardwareCheck = millis();
            
            if (player->getVolume() > 254) {
                sysLog("err:", "КРИТ_СБОЙ", "Аппаратное зависание VS1053 (SPI). Перезапуск чипа...");
                player->stop_mp3client();
                player->begin(); // Быстрый софт-ресет регистров
            }
        }
    }
}