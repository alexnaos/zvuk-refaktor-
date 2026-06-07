#pragma once
#include <Arduino.h>
#include <LittleFS.h>

// Настройки Wi-Fi (ИСПРАВЛЕНО: добавлен static против ошибок множественного определения)
static const char* WIFI_SSID = "Sloboda100";
static const char* WIFI_PASSWORD = "2716192023";

// Наша новая аппаратная схема пинов для Lolin S2 Mini
#define PIN_SPI_SCK 7
#define PIN_SPI_MISO 9
#define PIN_SPI_MOSI 11
#define VS1053_CS 12 // XCS
#define VS1053_DCS 14 // XDCS
#define VS1053_DREQ 13 // DREQ
#define VS1053_RST 18 // XRESET
#define I2C_SDA 33
#define I2C_SCL 35

// Константы для звука
#define VOLUME_MIN 0
#define VOLUME_MAX 21
#define BASS_MIN 0
#define BASS_MAX 15
#define TREBLE_MIN -8
#define TREBLE_MAX 7
#define VOLUME_DEFAULT 12
#define BASS_DEFAULT 0
#define TREBLE_DEFAULT 0

// Константы Wi-Fi
#define WIFI_TIMEOUT_MS 10000
#define WIFI_CHECK_INTERVAL_MS 10000
#define WIFI_ROAMING_DELAY_MS 30000
#define WIFI_RSSI_THRESHOLD -82

// Константы системы
#define HEALTH_CHECK_INTERVAL_MS 60000
#define DISPLAY_UPDATE_INTERVAL_MS 1000
#define HARDWARE_CHECK_INTERVAL_MS 30000
#define WDT_TIMEOUT_SEC 30

// Структура для динамического списка станций
struct Station {
 String name;
 String url;
};

// Резервируем до 50 станций в памяти
const int MAX_STATIONS = 50;

// Указываем, что сами данные физически лежат в main.cpp
extern Station stationList[MAX_STATIONS];
extern int stationCount; // <--- СТРОГО С extern! Никаких локальных дубликатов!


// Переменные для хранения настроек звука
extern int currentVolume;
extern int currentBass;
extern int currentTreble;

// Функции для работы с файловой системой
void initFileSystem();
void loadPlaylist();

// --- НАСТРОЙКИ MQTT ДЛЯ ОФИЦИАЛЬНОЙ ИНТЕГРАЦИИ YORADIO ---
// (ИСПРАВЛЕНО: добавлен static против ошибок множественного определения)
static const char* MQTT_SERVER = "192.168.1.23"; 
const int MQTT_PORT = 1883;
static const char* MQTT_USER = "";
static const char* MQTT_PASSWORD = "";

// Жестко фиксируем базовый топик для интеграции автора yoradio
#define MQTT_CLIENT_ID "homeradio" // Имя вашего радио
#define MQTT_BASE_TOPIC "yoradio/homeradio"
