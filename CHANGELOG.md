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