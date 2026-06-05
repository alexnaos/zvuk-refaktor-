#include "display_module.h"
#include "../config.h"        // <--- С выходом наверх!
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
