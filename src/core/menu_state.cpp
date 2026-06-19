#include "menu_state.h"
#include "../config.h"
#include "../global_state.h"

// ============================================================================
// ЛОГИКА МЕНЮ: режимы, навигация по энкодеру
// ============================================================================

static MenuMode currentMode = MENU_PLAY;

// Значения для режимов
static int menuVolume;
static int menuBass;
static int menuTreble;
static int menuStationIdx;

// Для отображения информации о станции
extern Station stationList[MAX_STATIONS];
extern int stationCount;
extern String currentStationName;

// ============================================================================
// ИНИЦИАЛИЗАЦИЯ
// ============================================================================

void initMenu(int initialStationIdx, int initialVolume, int initialBass, int initialTreble) {
    currentMode = MENU_PLAY;
    menuStationIdx = initialStationIdx;
    menuVolume = initialVolume;
    menuBass = initialBass;
    menuTreble = initialTreble;
}

// ============================================================================
// ПОЛУЧИТЬ ТЕКУЩИЙ РЕЖИМ
// ============================================================================

MenuMode getCurrentMode() {
    return currentMode;
}

// ============================================================================
// ИМЯ РЕЖИМА ДЛЯ ДИСПЛЕЯ
// ============================================================================

const char* getModeName(MenuMode mode) {
    switch (mode) {
        case MENU_PLAY:     return "PLAY";
        case MENU_VOLUME:   return "VOLUME";
        case MENU_STATIONS: return "STATIONS";
        case MENU_BASS:     return "BASS";
        case MENU_TREBLE:   return "TREBLE";
        default:            return "???";
    }
}

// ============================================================================
// ПОЛУЧИТЬ ДАННЫЕ ДЛЯ ОТОБРАЖЕНИЯ (значение, мин/макс, подпись, доп.инфо)
// ============================================================================

void getMenuDisplayData(int& value, int& minVal, int& maxVal, const char*& label, const char*& extra) {
    switch (currentMode) {
        case MENU_PLAY:
            value = 0; minVal = 0; maxVal = 0;
            label = currentStationName.c_str();
            extra = "";
            break;
        case MENU_VOLUME:
            value = menuVolume;
            minVal = VOLUME_MIN;
            maxVal = VOLUME_MAX;
            label = "VOLUME";
            extra = "";
            break;
        case MENU_STATIONS:
            value = menuStationIdx;
            minVal = 0;
            maxVal = (stationCount > 0) ? stationCount - 1 : 0;
            label = "STATION";
            extra = (stationCount > 0 && menuStationIdx >= 0 && menuStationIdx < stationCount)
                ? stationList[menuStationIdx].name.c_str()
                : "---";
            break;
        case MENU_BASS:
            value = menuBass;
            minVal = BASS_MIN;
            maxVal = BASS_MAX;
            label = "BASS";
            extra = "";
            break;
        case MENU_TREBLE:
            value = menuTreble;
            minVal = TREBLE_MIN;
            maxVal = TREBLE_MAX;
            label = "TREBLE";
            extra = "";
            break;
    }
}

// ============================================================================
// ОБРАБОТКА ДЕЙСТВИЙ ЭНКОДЕРА
// ============================================================================

MenuAction processEncoderAction(EncoderDirection encDir, ButtonState btnState) {
    MenuAction action = {};
    action.nextMode = currentMode;
    action.needsStationChange = false;
    action.needsVolumeChange = false;
    action.needsToneChange = false;
    action.needsDisplay = false;

    // --- ОБРАБОТКА ДЛИННОГО НАЖАТИЯ (выход в PLAY из любого режима) ---
    if (btnState == BTN_LONG) {
        if (currentMode != MENU_PLAY) {
            currentMode = MENU_PLAY;
            action.nextMode = MENU_PLAY;
            action.needsDisplay = true;
            return action;
        }
        // Если уже в PLAY — ничего не делаем
        return action;
    }

    // --- ОБРАБОТКА КОРОТКОГО НАЖАТИЯ (переключение режимов) ---
    if (btnState == BTN_CLICK) {
        // Если в MENU_STATIONS и есть станции — применяем выбранную станцию
        if (currentMode == MENU_STATIONS && stationCount > 0) {
            action.needsStationChange = true;
            action.param = menuStationIdx;
            // Переключаемся в PLAY
            currentMode = MENU_PLAY;
            action.nextMode = MENU_PLAY;
            action.needsDisplay = true;
            return action;
        }
        
        // Иначе просто циклически переключаемся
        int next = ((int)currentMode + 1) % MENU_COUNT;
        currentMode = (MenuMode)next;
        action.nextMode = currentMode;
        action.needsDisplay = true;
        return action;
    }

    // --- ОБРАБОТКА ВРАЩЕНИЯ ---
    if (encDir != ENC_NONE) {
        int delta = (encDir == ENC_UP) ? 1 : -1;

        switch (currentMode) {
            case MENU_VOLUME:
                menuVolume = constrain(menuVolume + delta, VOLUME_MIN, VOLUME_MAX);
                action.needsVolumeChange = true;
                action.param = menuVolume;
                action.needsDisplay = true;
                break;

            case MENU_STATIONS:
                if (stationCount > 0) {
                    menuStationIdx = constrain(menuStationIdx + delta, 0, stationCount - 1);
                    action.needsDisplay = true;
                    // Пока не применяем — только после клика
                }
                break;

            case MENU_BASS:
                menuBass = constrain(menuBass + delta, BASS_MIN, BASS_MAX);
                action.needsToneChange = true;
                action.param = menuBass;
                action.needsDisplay = true;
                break;

            case MENU_TREBLE:
                menuTreble = constrain(menuTreble + delta, TREBLE_MIN, TREBLE_MAX);
                action.needsToneChange = true;
                action.param = menuTreble;
                action.needsDisplay = true;
                break;

            case MENU_PLAY:
            default:
                // В режиме PLAY вращение ничего не делает
                break;
        }
    }

    return action;
}