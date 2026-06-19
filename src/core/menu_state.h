#pragma once
#include <Arduino.h>
#include "encoder_module.h"

// Режимы меню (порядок переключения по клику)
enum MenuMode {
    MENU_PLAY = 0,      // Воспроизведение (инфо)
    MENU_VOLUME,        // Громкость
    MENU_STATIONS,      // Выбор станции
    MENU_BASS,          // Эквалайзер: бас
    MENU_TREBLE,        // Эквалайзер: тембр
    MENU_COUNT
};

// Результат обработки (возвращается в main, чтобы выполнить действие)
struct MenuAction {
    MenuMode nextMode;  // режим, в который нужно переключиться
    int param;          // параметр: новая громкость/бас/тембр/индекс станции
    bool needsStationChange; // true если param — новый индекс станции
    bool needsVolumeChange;  // true если param — новая громкость
    bool needsToneChange;    // true если param — новые бас/тембр
    bool needsDisplay;       // true если нужно обновить дисплей
};

// Инициализация меню
void initMenu(int initialStationIdx, int initialVolume, int initialBass, int initialTreble);

// Обработка энкодера (вращение + кнопка)
MenuAction processEncoderAction(EncoderDirection encDir, ButtonState btnState);

// Получить текущий режим
MenuMode getCurrentMode();

// Получить строку текущего режима для дисплея
const char* getModeName(MenuMode mode);

// Получить текущие значения для отображения
void getMenuDisplayData(int& value, int& minVal, int& maxVal, const char*& label, const char*& extra);