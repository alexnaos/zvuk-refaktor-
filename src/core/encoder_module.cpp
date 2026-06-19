#include "encoder_module.h"
#include "../config.h"

// ============================================================================
// ЭНКОДЕР: обработка прерываний, антидребезг, распознавание кликов
// ============================================================================
// Используем классический алгоритм с конечным автоматом для энкодера.
// Без GyverEncoder — своя лёгкая реализация на прерываниях.
// ============================================================================

static volatile int encoderPos = 0;
static volatile uint8_t lastState = 0;

// Для кнопки
static volatile bool btnPressed = false;
static volatile unsigned long btnPressTime = 0;
static volatile bool btnHandled = false;
static volatile ButtonState btnResult = BTN_NONE;

// Флаг что было вращение
static volatile bool encoderMoved = false;
static volatile int encoderDelta = 0;

// ============================================================================
// ОБРАБОТЧИК ПРЕРЫВАНИЯ ЭНКОДЕРА
// ============================================================================

static void IRAM_ATTR encoderISR() {
    uint8_t clk = digitalRead(ENC_CLK);
    uint8_t dt  = digitalRead(ENC_DT);
    uint8_t state = (clk << 1) | dt;

    static uint8_t prevState = 0;
    static int tempPos = 0;

    if (state != prevState) {
        // Таблица переходов для энкодера (полушаговый режим)
        // Получаем приращение: -1, 0, или +1
        int8_t transitions[16] = {
            0, -1, 1, 0,
            1, 0, 0, -1,
            -1, 0, 0, 1,
            0, 1, -1, 0
        };
        int idx = (prevState << 2) | state;
        tempPos += transitions[idx];
        prevState = state;

        // Каждые 4 перехода — полный шаг
        if (tempPos >= 4) {
            encoderDelta += 1;
            tempPos = 0;
            encoderMoved = true;
        } else if (tempPos <= -4) {
            encoderDelta -= 1;
            tempPos = 0;
            encoderMoved = true;
        }
    }
}

// ============================================================================
// ОБРАБОТЧИК ПРЕРЫВАНИЯ КНОПКИ
// ============================================================================

static void IRAM_ATTR buttonISR() {
    // Срабатывает по CHANGE
    // Фильтрация дребезга: проверяем через 20мс в loop
    btnPressed = true;
}

// ============================================================================
// ИНИЦИАЛИЗАЦИЯ
// ============================================================================

void initEncoder() {
    pinMode(ENC_CLK, INPUT_PULLUP);
    pinMode(ENC_DT, INPUT_PULLUP);
    pinMode(ENC_SW, INPUT_PULLUP);

    // Прерывания на CHANGE для энкодера
    attachInterrupt(digitalPinToInterrupt(ENC_CLK), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_DT), encoderISR, CHANGE);
    
    // Прерывание на CHANGE для кнопки (обработаем дребезг в loop)
    attachInterrupt(digitalPinToInterrupt(ENC_SW), buttonISR, CHANGE);
}

// ============================================================================
// ЧТЕНИЕ ВРАЩЕНИЯ (сбрасывает флаг)
// ============================================================================

EncoderDirection readEncoder() {
    if (encoderMoved) {
        encoderMoved = false;
        int delta = encoderDelta;
        encoderDelta = 0;
        if (delta > 0) return ENC_UP;
        if (delta < 0) return ENC_DOWN;
    }
    return ENC_NONE;
}

// ============================================================================
// ЧТЕНИЕ КНОПКИ (сбрасывает флаг)
// ============================================================================

ButtonState readButton() {
    ButtonState result = btnResult;
    btnResult = BTN_NONE;
    return result;
}

// ============================================================================
// ОБРАБОТКА В LOOP: антидребезг кнопки, распознавание клик/длинное нажатие
// ============================================================================

void loopEncoder() {
    const unsigned long debounceMs = 30;
    const unsigned long longPressMs = 1000;

    if (btnPressed) {
        btnPressed = false;
        static unsigned long lastChangeTime = 0;
        static int lastStableState = HIGH;

        int currentState = digitalRead(ENC_SW);
        unsigned long now = millis();

        // Антидребезг
        if (now - lastChangeTime > debounceMs) {
            if (currentState == LOW && lastStableState == HIGH) {
                // Нажатие
                btnPressTime = now;
                btnHandled = false;
            } else if (currentState == HIGH && lastStableState == LOW) {
                // Отпускание
                if (!btnHandled) {
                    unsigned long pressDuration = now - btnPressTime;
                    if (pressDuration < longPressMs) {
                        btnResult = BTN_CLICK;  // короткое нажатие
                    }
                    btnHandled = true;
                }
            }
            lastStableState = currentState;
        }
        lastChangeTime = now;
    }

    // Проверка длинного нажатия (если кнопка всё ещё нажата)
    static unsigned long lastLongCheck = 0;
    if (millis() - lastLongCheck > 100) {
        lastLongCheck = millis();
        if (digitalRead(ENC_SW) == LOW && !btnHandled) {
            unsigned long pressDuration = millis() - btnPressTime;
            if (pressDuration >= longPressMs) {
                btnResult = BTN_LONG;
                btnHandled = true;
            }
        }
    }
}