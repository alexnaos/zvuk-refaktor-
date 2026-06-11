# zvuk(refaktor) — ESP32-S2 Интернет-Радио

Интернет-радио на базе **Lolin S2 Mini (ESP32-S2)** с аппаратным декодером **VS1053**, OLED-дисплеем **SSD1306**, веб-интерфейсом и интеграцией с **Home Assistant** через MQTT.

## 📋 Характеристики

| Параметр | Значение |
|----------|----------|
| **Платформа** | ESP32-S2 (одноядерный, 240 MHz) |
| **Плата** | Lolin S2 Mini |
| **Фреймворк** | PlatformIO + Arduino |
| **Аудиочип** | VS1053 (MP3 / Ogg / WAV / FLAC) |
| **Дисплей** | SSD1306 128×64 (I²C) |
| **Файловая система** | LittleFS |
| **Веб-интерфейс** | SettingsGyver + GyverDBFile |
| **MQTT** | PubSubClient (топик `yoradio/homeradio`) |

## 🔧 Аппаратное подключение

### Lolin S2 Mini → VS1053 (SPI)

| VS1053 | Lolin S2 Mini |
|--------|---------------|
| SCK    | GPIO 7        |
| MISO   | GPIO 9        |
| MOSI   | GPIO 11       |
| XCS    | GPIO 12       |
| XDCS   | GPIO 14       |
| DREQ   | GPIO 13       |
| XRESET | GPIO 18       |

### Lolin S2 Mini → SSD1306 (I²C)

| SSD1306 | Lolin S2 Mini |
|---------|---------------|
| SDA     | GPIO 33       |
| SCL     | GPIO 35       |

## 📂 Структура проекта

```
zvuk(refaktor)/
├── platformio.ini              # Конфигурация сборки PlatformIO
├── CHANGELOG.md                # История изменений
├── SESSION_CONTEXT.md          # Контекст сессий разработки
├── data/                       # Файлы для загрузки в LittleFS
│   ├── stations.txt            # Плейлист радиостанций
│   └── рефакт.txt              # Описание рефакторинга
├── lib/
│   └── ESP32-vs1053_ext/       # Локальная библиотека VS1053
└── src/
    ├── config.h                # Конфигурация пинов, Wi-Fi, MQTT, константы
    ├── global_state.h          # extern-объявления глобальных переменных
    ├── global_state.cpp        # Определения глобальных переменных
    ├── main.cpp                # setup() + loop()
    ├── web_server.h/.cpp       # GyverDB + SettingsGyver (веб-сервер)
    ├── web_interface.h/.cpp    # Пользовательский веб-интерфейс
    ├── mqtt_module.h/.cpp      # MQTT-клиент для Home Assistant
    └── core/
        ├── audio_playback.h/.cpp    # VS1053 плеер (потоковое вещание)
        ├── audio_recorder.h/.cpp    # Запись звука с VS1053 в файл
        ├── display_module.h/.cpp    # OLED-дисплей SSD1306
        ├── system_logger.h/.cpp     # Системный логгер (ОЗУ-буфер + Serial)
        ├── fs_module.h/.cpp         # Файловая система LittleFS
        └── playlist_parser.h/.cpp   # Парсер плейлиста stations.txt
```

## 🚀 Функциональность

### 📻 Интернет-радио
- Потоковое воспроизведение MP3/Ogg через VS1053
- Плейлист до 50 станций, загружаемый из `stations.txt`
- Автоматическое восстановление последней станции при старте
- Поддержка кастомного URL станции через веб-интерфейс

### 🌐 Веб-интерфейс
- Управление громкостью (0–21)
- Настройка эквалайзера (Bass 0–15, Treble -8..7)
- Выбор станции из списка
- Кнопки: Назад, Плей, Стоп, Вперед
- Поле для кастомного URL станции
- Управление записью звука (Старт / Стоп)
- Журнал событий (системный лог)

### 🏠 Интеграция с Home Assistant (MQTT)
- Протокол **YoRadio** (`yoradio/homeradio`)
- **Топики:**
  - `yoradio/homeradio/command` — управление (vol N, play N, next, prev, stop, start)
  - `yoradio/homeradio/status` — статус (JSON: title, artist, name, station, status)
  - `yoradio/homeradio/volume` — уровень громкости (0–254)
  - `yoradio/homeradio/playlist` — URL файла плейлиста для HA

### 🔴 Запись звука
- Запись потока с VS1053 в файл на LittleFS
- Автоматическое ограничение размера файла (1 MB)
- Управление через веб-интерфейс

### 🛡️ Система диагностики
- Watchdog Timer (WDT) с таймаутом 30 с — защита от зависаний
- Мониторинг аппаратного состояния VS1053 (SPI-зависания)
- Проверка сигнала Wi-Fi и асинхронный роуминг при RSSI < -82 dBm
- Циклический буфер логов (256 записей) с выводом в веб-интерфейс
- Crash-лог: запись причины сброса и последнего сообщения перед сбоем

## ⚙️ Сборка и прошивка

### Требования
- **PlatformIO** (VSCode extension или CLI)

### Команды
```bash
# Сборка
pio run

# Прошивка через USB
pio run --target upload

# Прошивка по OTA (после первой USB-прошивки)
pio run --target upload --upload-port <IP_устройства>

# Монитор порта
pio device monitor
```

### Загрузка файловой системы
```bash
# Загрузка данных (stations.txt и др.) в LittleFS
pio run --target uploadfs
```

## 📝 Зависимости (lib_deps)

| Библиотека | Версия | Назначение |
|-----------|--------|------------|
| `gyverlibs/GyverDB` | 1.4.3 | База данных ключ-значение |
| `gyverlibs/Settings` | 1.3.16 | Веб-интерфейс и настройки |
| `knolleary/PubSubClient` | ^2.8 | MQTT-клиент |
| `adafruit/Adafruit GFX Library` | ^1.11.9 | Графика для OLED |
| `adafruit/Adafruit BusIO` | ^1.14.5 | Шина I²C/SPI для Adafruit |
| `adafruit/Adafruit SSD1306` | ^2.5.13 | Драйвер OLED SSD1306 |

> Также используется локальная библиотека `ESP32-vs1053_ext` (находится в `lib/`).

## 📊 Потребление ресурсов

| Ресурс | Использовано |
|--------|-------------|
| **RAM** | ~17.7% (58 КБ / 320 КБ) |
| **Flash** | ~85% (1.1 МБ / 1.3 МБ) |

## 📜 Формат плейлиста

Файл `data/stations.txt` (загружается в `/stations.txt` на LittleFS):

```
Название_станции<TAB>URL_потока
```

Разделитель — символ табуляции (`\t`). Автоматически генерируется `/ha_playlist.txt` для Home Assistant.

## 🔄 История версий

- **v1.1.0** (2026-06-07) — Code review и исправление ошибок
- **v1.0.0** — Первая стабильная версия

Полный список изменений — в [CHANGELOG.md](CHANGELOG.md).

## 📄 Лицензия

Проект распространяется по лицензии **MIT**. Подробнее — в файле [LICENSE](LICENSE).