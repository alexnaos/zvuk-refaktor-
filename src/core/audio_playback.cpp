#include "audio_playback.h"
#include "../config.h"
#include "../global_state.h"
#include "../mqtt_module.h" 
#include "core/system_logger.h" // Добавлено для вывода отладки в веб-интерфейс
#include <SPI.h>

VS1053* player = nullptr;

// ============================================================================
// СИСТЕМНЫЕ ПЕРЕХВАТЧИКИ БИБЛИОТЕКИ WOLLE VS1053 (ГЛУБОКАЯ ОТЛАДКА)
// ============================================================================

// Глобальный флаг для асинхронной отправки MQTT (чтобы не блокировать плеер)
bool mqttTrackUpdateFlag = false;

void vs1053_showstreamtitle(const char *info) {
    if (info != nullptr) {
        currentTrack = String(info);
        currentTrack.trim();
        
        // ИСПРАВЛЕНО: Полностью убрана блокирующая функция mqttPublishTrack().
        // Теперь мы просто поднимаем флаг. Отправка произойдет в loop(),
        // когда процессор будет свободен от подкачки аудиоданных.
        mqttTrackUpdateFlag = true;
        
        // Логируем в ОЗУ для веб-интерфейса (работает мгновенно)
        sysLog("info:", "ДЕКОДЕР", "Трек: " + currentTrack);
    }
}


void vs1053_showstreaminfo(const char *info) {
    if (info != nullptr) {
        Serial.printf("[ПОТОК INFO]: %s\n", info);
        sysLog("info:", "ПОТОК_ИНФО", String(info));
    }
}

// ДОБАВЛЕНО: Срабатывает при критическом опустошении сетевого буфера (заикании)
void vs1053_commercial(const char *info) {
    if (info != nullptr) {
        sysLog("warn:", "СЕТЬ_БУФЕР", "Поток прерван! Ожидание данных. Код: " + String(info));
    }
}

// ДОБАВЛЕНО: Срабатывает, если TCP-сессия была разорвана со стороны сервера радио
void vs1053_eof_mp3(const char *info) {
    if (info != nullptr) {
        sysLog("err:", "ОБРЫВ_СВЯЗИ", "Удаленный сервер закрыл поток: " + String(info));
    }
    if (player != nullptr) {
        player->stop_mp3client();
    }
}

// ДОБАВЛЕНО: Отслеживание текущего битрейта (помогает выявить перегрузку Wi-Fi)
void vs1053_bitrate(const char *info) {
    if (info != nullptr) {
        sysLog("info:", "БИТРЕЙТ", "Скорость потока: " + String(info) + " kbps");
    }
}

// ============================================================================
// ОСНОВНЫЕ ФУНКЦИИ УПРАВЛЕНИЯ АУДИОСИСТЕМОЙ
// ============================================================================

void initAudio() {
    pinMode(VS1053_RST, OUTPUT);
    digitalWrite(VS1053_RST, LOW);
    delay(50);
    digitalWrite(VS1053_RST, HIGH);
    delay(50);

    // Сохранено строго в исходном виде
    player = new VS1053(VS1053_CS, VS1053_DCS, VS1053_DREQ, HSPI, PIN_SPI_MOSI, PIN_SPI_MISO, PIN_SPI_SCK);
    player->begin();
    player->setVolume(currentVolume);

    uint8_t toneData[] = { (uint8_t)currentTreble, 6, (uint8_t)currentBass, 5 };
    player->setTone(toneData);
    
    Serial.println("[Звук]: Модуль VS1053 успешно запущен.");
    sysLog("info:", "ЖЕЛЕЗО", "Модуль VS1053 успешно запущен.");
}

void startRadioStream(const char* url) {
    if (player != nullptr && !isRecording) {
        currentTrack = "Loading...";
        mqttPublishTrack(currentTrack.c_str());
        
        sysLog("info:", "ПЛЕЕР", "Подключение к: " + String(url));
        
        player->connecttohost(url);
        player->setVolume(currentVolume);
    }
}

void updateVolume(int vol) {
    if (player != nullptr) {
        currentVolume = constrain(vol, 0, 21);
        player->setVolume(currentVolume);
        
        Serial.printf("[Звук]: Новая громкость: %d\n", currentVolume);
        sysLog("info:", "ЗВУК", "Новая громкость: " + String(currentVolume));
    }
}

void updateTone(int bass, int treble) {
    if (player != nullptr) {
        currentBass = constrain(bass, 0, 15);
        currentTreble = constrain(treble, -8, 7);
        
        uint8_t toneData[] = { (uint8_t)currentTreble, 6, (uint8_t)currentBass, 5 };
        player->setTone(toneData);
        
        Serial.printf("[Звук]: Эквалайзер -> БАС: %d, ТРЕБЛ: %d\n", currentBass, currentTreble);
        sysLog("info:", "ЗВУК", "Эквалайзер -> БАС: " + String(currentBass) + ", ТРЕБЛ: " + String(currentTreble));
    }
}

void loopAudioPlayback() {
    if (player != nullptr && !isRecording) {
        player->loop();
        
        // ДОБАВЛЕНО: Автоматическая проверка "зависания" шины SPI декодера
        // Раз в 30 секунд читаем статус громкости. Если на проводах наводка — чип вернет > 254.
        static unsigned long lastHardwareCheck = 0;
        if (millis() - lastHardwareCheck > 30000) {
            lastHardwareCheck = millis();
            
            if (player->getVolume() > 254) {
                sysLog("err:", "КРИТ_СБОЙ", "Аппаратное зависание VS1053 (SPI). Перезапуск чипа...");
                player->stop_mp3client();
                player->begin(); // Быстрый софт-ресет регистров
            }
        }
    }
}
