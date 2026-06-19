#include "display_module.h"
#include "../config.h"
#include <Wire.h>
#include <WiFi.h>
#include <time.h>


Adafruit_SSD1306 display(128, 64, &Wire, -1);

void initDisplay() {
    Wire.begin(I2C_SDA, I2C_SCL);
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("[Экран]: Ошибка SSD1306!"));
        return;
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 20);
    display.println("--- INTERNET RADIO ---");
    display.display();
    Serial.println(F("[Экран]: Успешно запущен."));
}

// ============================================================================
// ОТРИСОВКА МЕНЮ ЭНКОДЕРА
// ============================================================================

void updateMenuDisplay(MenuMode mode, int value, int minVal, int maxVal, const char* label, const char* extra) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    
    // Верхняя строка: название режима
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(">> ");
    display.print(getModeName(mode));
    
    // Разделитель
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
    
    if (mode == MENU_PLAY) {
        // Режим воспроизведения: показываем станцию и трек
        display.setCursor(0, 14);
        display.print("ST: ");
        display.setTextSize(1);
        // label содержит имя станции, extra - трек
        display.println(utf8rus(String(label)));
        
        if (strlen(extra) > 0) {
            display.setCursor(0, 38);
            display.println(utf8rus(String(extra)));
        }
    } else if (mode == MENU_VOLUME) {
        // Громкость: текст + полоска
        display.setTextSize(2);
        display.setCursor(0, 20);
        display.print(utf8rus(String(label)));
        display.print(" ");
        display.print(value);
        
        // Полоска громкости
        int barWidth = map(value, minVal, maxVal, 0, 120);
        display.drawRect(4, 50, 120, 8, SSD1306_WHITE);
        display.fillRect(4, 50, barWidth, 8, SSD1306_WHITE);
        
    } else if (mode == MENU_STATIONS) {
        // Станции: номер + название
        display.setTextSize(1);
        display.setCursor(0, 14);
        display.print(utf8rus(String(label)));
        display.print(": ");
        display.print(value + 1); // нумерация с 1 для пользователя
        display.print("/");
        display.println(maxVal + 1);
        
        display.drawFastHLine(0, 26, 128, SSD1306_WHITE);
        
        display.setTextSize(1);
        display.setCursor(0, 32);
        if (extra != nullptr && strlen(extra) > 0) {
            display.println(utf8rus(String(extra)));
        } else {
            display.println("---");
        }
        display.setCursor(0, 50);
        display.println("> Click to select");
        
    } else if (mode == MENU_BASS || mode == MENU_TREBLE) {
        // Бас/Требл: текст + графическая полоска
        display.setTextSize(2);
        display.setCursor(0, 18);
        display.print(utf8rus(String(label)));
        
        // Значение по центру
        display.setCursor(70, 18);
        if (value >= 0) display.print("+");
        display.print(value);
        
        // Полоска (от -8 до +7 для treble, от 0 до 15 для bass)
        int range = maxVal - minVal;
        int barWidth = (range > 0) ? map(value, minVal, maxVal, 0, 116) : 0;
        display.drawRect(6, 48, 116, 8, SSD1306_WHITE);
        display.fillRect(6, 48, barWidth, 8, SSD1306_WHITE);
        
        // Метки min/max
        display.setTextSize(1);
        display.setCursor(6, 58);
        display.print(minVal);
        display.setCursor(106, 58);
        display.print(maxVal);
    }
    
    display.display();
}

String utf8rus(String source) {
    int i;
    String target = "";
    unsigned char a, b;
    for (i = 0; i < source.length(); i++) {
        a = source[i];
        if (a < 128) {
            target += (char)a;
        } else if (a == 208 || a == 209) {
            b = source[++i];
            if (a == 208 && b == 129) target += (char)161;
            if (a == 209 && b == 145) target += (char)177;
            if (a == 208 && b >= 144 && b <= 191) target += (char)(b + 48);
            if (a == 209 && b >= 128 && b <= 143) target += (char)(b + 112);
        }
    }
    return target;
}

void updateDisplay(const char* station, const char* track) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    struct tm timeinfo;
    char timeStr[6] = "--:--";
    if (getLocalTime(&timeinfo)) {
        strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
    }

    String ipStr = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "No Wi-Fi";

    display.setCursor(0, 0);
    display.print(ipStr);
    display.setCursor(98, 0);
    display.print(timeStr);
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);

    display.setCursor(0, 14);
    display.print("ST: ");
    display.println(utf8rus(String(station)));
    display.drawFastHLine(0, 32, 128, SSD1306_WHITE);

    display.setCursor(0, 38);
    display.println(utf8rus(String(track)));
    display.display();
}
