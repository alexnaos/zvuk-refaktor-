# CHANGELOG — zvuk(refaktor) / ESP32-S2 Radio

## [1.1.0] — 2026-06-07 — Code Review & Fixes

### Что было сделано
Полный code review проекта. Прочитаны все 16 файлов.

### Найденные и исправленные проблемы

#### 🔴 Критические (3)
1. **audio_playback.cpp:5** — `#include "core/system_logger.h"` → `#include "system_logger.h"` (файл уже в папке core)
2. **global_state.cpp** — отсутствовало определение `mqttTrackUpdateFlag` (был только extern в header)
3. **audio_recorder.cpp** — бесконечный рост файла записи (добавлен лимит 1MB)

#### 🟡 Серьезные (3)
4. **main.cpp** — Wi-Fi роуминг: после `WiFi.disconnect()` не было `WiFi.begin()`
5. **audio_recorder.cpp** — в `loopAudioRecorder()` не было `esp_task_wdt_reset()`
6. **mqtt_module.cpp** — дублирование состояния MQTT (не исправлено, требует рефакторинга)

#### 🟢 Косметические (3)
7. **web_server.cpp** — лишний пробел перед `#include`, отсутствовал `#include "config.h"`
8. **config.h** — все магические числа заменены на именованные константы
9. **Весь проект** — константы применены во всех файлах вместо хардкода

### Файлы, которые были изменены (8)
- `src/config.h` — добавлены константы
- `src/main.cpp` — константы + Wi-Fi роуминг
- `src/global_state.cpp` — mqttTrackUpdateFlag + константы
- `src/core/audio_playback.cpp` — include + константы
- `src/core/audio_recorder.cpp` — лимит + WDT
- `src/web_server.cpp` — include + пробел
- `src/web_interface.cpp` — константы
- `src/mqtt_module.cpp` — константы

### Компиляция
- **RAM:** 17.7% (58,092/327,680 байт)
- **Flash:** 85.2% (1,116,486/1,310,720 байт)
- **Git:** коммит `9553d11` — "fix: исправление ошибок кода и рефакторинг констант"


Реализована кастомизация дизайна вебстраницы библиотеки Settings **без модификации самой библиотеки**.

## Что сделано

### 1. Создан `data/custom.js`
Файл содержит JavaScript, который инжектирует кастомные CSS-стили в `<head>` страницы. Реализована **современная тёмная тема** с:

- **Глубокий тёмный фон** (`#080a10` / `#0f1117` / `#1a1d27`) — вместо серого стандартного
- **Акцентный цвет** `#00b4d8` (cyan) — для кнопок, слайдеров, переключателей
- **Glassmorphism-эффект** на кнопках (glow-тени)
- **Улучшенные слайдеры** — акцентный thumb с border и glow
- **Современные toggle-переключатели** с анимацией
- **Скруглённые карточки** (14px вместо 10px)
- **Заголовки групп** — uppercase, letter-spacing, smaller font
- **Системный шрифт** (Segoe UI / Roboto) вместо Verdana
- **Кастомный скроллбар** — тонкий, минималистичный
- **Стилизованные диалоги** с backdrop-blur
- **Лог-блок** с фоном и рамкой

### 2. Изменён `src/web_server.cpp`
Добавлена регистрация custom.js через хук библиотеки:
```cpp
if (LittleFS.exists("/custom.js")) {
    sett.setCustomFile("/custom.js");
}
sett.config.theme = sets::Colors::Default;
```

## Как это работает (архитектура)

1. При старте ESP32 читает `custom.js` с LittleFS и вычисляет hash
2. При запросе страницы сервер отправляет `custom_hash` клиенту
3. JS-клиент загружает `/custom.js`, кеширует в localStorage
4. При следующей загрузке страницы custom.js выполняется и создаёт `<style>` тег
5. CSS-переменные библиотеки (`--accent`, `--back`, `--tab` и т.д.) перезаписываются

## Как загрузить на плату

```bash
# Шаг 1: Загрузить данные (custom.js + stations.txt) на LittleFS
pio run -e lolin_s2_mini -t uploadfs

# Шаг 2: Загрузить прошивку
pio run -e lolin_s2_mini -t upload
```

## Как изменить дизайн в будущем

Просто отредактируйте `data/custom.js` и повторите `pio run -t uploadfs`. Библиотека Settings не затронута и обновляется отдельно.