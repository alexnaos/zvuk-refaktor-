#pragma once
#include <Adafruit_SSD1306.h>
#include "menu_state.h" // для MenuMode

extern Adafruit_SSD1306 display;

void initDisplay();
void updateDisplay(const char* station, const char* track);
void updateMenuDisplay(MenuMode mode, int value, int minVal, int maxVal, const char* label, const char* extra);
String utf8rus(String source);