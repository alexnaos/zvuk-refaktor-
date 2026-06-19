#pragma once
#include <Arduino.h>

// Направление вращения энкодера
enum EncoderDirection {
    ENC_NONE = 0,
    ENC_UP = 1,
    ENC_DOWN = -1
};

// Состояние кнопки энкодера
enum ButtonState {
    BTN_NONE = 0,
    BTN_CLICK = 1,      // короткое нажатие (< 1 сек)
    BTN_LONG = 2        // длинное нажатие (>= 1 сек)
};

void initEncoder();
EncoderDirection readEncoder();
ButtonState readButton();
void loopEncoder(); // вызывать в loop() для обработки прерываний