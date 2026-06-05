#include "system_logger.h"
#include <WiFi.h>

// Физическое выделение буфера логгера в ОЗУ (на 256 байт по вашему конфигу)
sets::Logger webLogger(256);

String getResetReason(RESET_REASON reason) {
    switch (reason) {
        case POWERON_RESET: return "POWERON_RESET (Включение питания)";
        case RTCWDT_SYS_RESET: return "RTCWDT_SYS_RESET (Аппаратный Watchdog)";
        case DEEPSLEEP_RESET: return "DEEPSLEEP_RESET (Выход из глубокого сна)";
        case TG0WDT_SYS_RESET: return "TG0WDT_SYS_RESET (Сторожевой таймер Task WDT)";
        case RTC_SW_SYS_RESET: return "RTC_SW_SYS_RESET (Программный сброс системы)";
        case RTC_SW_CPU_RESET: return "RTC_SW_CPU_RESET (Программный сброс CPU)";
        default: return "UNKNOWN_RESET (" + String(reason) + ")";
    }
}

void sysLog(const String& level, const String& module, const String& text) {
    String formattedLine = level + "[" + module + "] " + text;
    
    // Вывод в Монитор порта
    Serial.println(formattedLine);
    
    // Логируем СТРОГО в ОЗУ веб-интерфейса, не трогая файловую систему!
    webLogger.println(formattedLine); 
}

void logSystemHealth() {
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t maxBlock = ESP.getMaxAllocHeap();
    
    // Получаем MAC-адрес (BSSID) роутера, к которому подключены сейчас
    String currentBssid = WiFi.BSSIDstr();
    
    // Формируем детальное сообщение телеметрии
    String msg = "ОЗУ: " + String(freeHeap) + " Б (Блок: " + String(maxBlock) + " Б) | ";
    msg += "RSSI: " + String(WiFi.RSSI()) + " dBm | ";
    msg += "Роутер (BSSID): " + currentBssid; // <-- ДОБАВЛЕНО ТУТ
    
    if (freeHeap < 40000) {
        sysLog("warn:", "ДИАГНОСТИКА", msg);
    } else {
        sysLog("info:", "ДИАГНОСТИКА", msg);
    }
}

