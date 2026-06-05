#pragma once
#include <Adafruit_SSD1306.h>

extern Adafruit_SSD1306 display;

void initDisplay();
void updateDisplay(const char* station, const char* track);
String utf8rus(String source);
