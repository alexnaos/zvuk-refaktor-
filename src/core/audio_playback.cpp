#include "audio_playback.h"
#include "../config.h"
#include "../global_state.h"
#include "../mqtt_module.h" 
#include "system_logger.h"
#include <SPI.h>

VS1053* player = nullptr;

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
        currentTrack = "Loading...";
        mqttPublishTrack(currentTrack.c_str());
        
        sysLog("info:", "ПЛЕЕР", "Подключение к: " + String(url));
        
        player->connecttohost(url);
        player->setVolume(currentVolume);
        
        // Сразу после запуска потока — диагностика
        logVS1053Registers();
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
static const unsigned long DIAG_LOG_INTERVAL_MS = 5000; // Лог каждые 5 секунд

void loopAudioPlayback() {
    if (player != nullptr && !isRecording) {
        player->loop();
        
        // Диагностика VS1053 каждые 5 секунд
        if (millis() - lastDiagLog > DIAG_LOG_INTERVAL_MS) {
            lastDiagLog = millis();
            logVS1053Registers();
        }
        
        // Автоматическая проверка "зависания" шины SPI декодера
        // Раз в 30 секунд читаем статус громкости. Если на проводах наводка — чип вернет > 254.
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