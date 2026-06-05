#include <Arduino.h>
#include <WiFi.h>
#include <Update.h>
#include <GyverDBFile.h>
#include "esp_task_wdt.h"
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

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    RESET_REASON r0 = rtc_get_reset_reason(0);
    sysLog("warn:", "АППАРАТ", "Причина прошлого сброса CPU: " + getResetReason(r0));
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
        delay(500);
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        sysLog("info:", "СЕТЬ", "Успешное подключение. IP: " + WiFi.localIP().toString());
        configTime(3600 * 3, 0, "pool.ntp.org", "ru.pool.ntp.org");
    } else {
        sysLog("err:", "СЕТЬ", "Ошибка Wi-Fi! Тайм-аут 10 секунд.");
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
    // Вызов обработчика веб-запросов Гайвера
    handleWebRequests();
    
    if (millis() - lastWiFiCheck > wifiCheckInterval) {
        lastWiFiCheck = millis();
        if (WiFi.status() != WL_CONNECTED) {
            sysLog("warn:", "СЕТЬ", "Связь потеряна! Переподключение...");
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        } else {
            // ДИНАМИЧЕСКИЙ РОУМИНГ (БЕЗ ХАРДКОДА MAC-АДРЕСА)
            // Если плата зацепилась за дальний роутер и сигнал хуже -82 dBm
            if (WiFi.RSSI() < -82) {
                sysLog("warn:", "РОУМИНГ", "Слишком слабый сигнал: " + String(WiFi.RSSI()) + " dBm. Ищу точку с лучшим приемом...");
                
                // Мягко отключаемся от текущего слабого роутера
                WiFi.disconnect();
                
                // Запускаем поиск заново. Сетевой стек ESP32 автоматически отсканирует
                // эфир вашей сети Sloboda100 и выберет роутер с максимальным уровнем сигнала.
                // При этом полное резервирование сети сохраняется!
                WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            }
        }
    }
    
    if (millis() - lastHealthCheck > 60000) {
        lastHealthCheck = millis();
        if (WiFi.status() == WL_CONNECTED) logSystemHealth();
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        loopMQTT();
        
        // БЕЗОПАСНАЯ ОТПРАВКА НАЗВАНИЯ ТРЕКА В MQTT
        // Переменная mqttTrackUpdateFlag создана глобально в audio_playback.cpp.
        extern bool mqttTrackUpdateFlag; 
        if (mqttTrackUpdateFlag) {
            mqttTrackUpdateFlag = false; // Мгновенно сбрасываем флаг
            mqttPublishTrack(currentTrack.c_str()); // Спокойно отправляем пакет вне аудио-прерывания
        }
    }
    
    // Главная аудио-задача: вызывается непрерывно и без задержек!
    loopAudioPlayback();
    loopAudioRecorder();
    
    if (changeStationFlag) {
        changeStationFlag = false;
        if (currentStationIdx >= 0 && currentStationIdx < stationCount) {
            String url = stationList[currentStationIdx].url;
            
            // Запуск аудио-потока делается напрямую. Движок Wolle VS1053 сам 
            // асинхронно обработает подключение и вернет ошибку, если сервер лежит.
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
