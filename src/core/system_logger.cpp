#include "system_logger.h"
#include <WiFi.h>

// Физическое выделение буфера логгера в ОЗУ для веб-интерфейса
sets::Logger webLogger(256);

// ВЫДЕЛЯЕМ ЭНЕРГОНЕЗАВИСИМУЮ RTC ПАМЯТЬ СТРОГО ПОД КРИТИЧЕСКИЙ ЛОГ
RTC_DATA_ATTR char rtcCrashBuffer[128] = "Нет данных о прошлом падении";
RTC_DATA_ATTR bool rtcHasCrashData = false;

String getResetReason(RESET_REASON reason) {
    switch (reason) {
        case POWERON_RESET:    return "POWERON_RESET (Включение питания)";
        case RTCWDT_SYS_RESET: return "RTCWDT_SYS_RESET (Аппаратный Watchdog роутера/питания)";
        case DEEPSLEEP_RESET:  return "DEEPSLEEP_RESET (Выход из глубокого сна)";
        case TG0WDT_SYS_RESET: return "TG0WDT_SYS_RESET (Сторожевой таймер Task WDT - ЗАВИС LOOP!)";
        case RTC_SW_SYS_RESET: return "RTC_SW_SYS_RESET (Программный сброс системы)";
        case RTC_SW_CPU_RESET: return "RTC_SW_CPU_RESET (Программный сброс CPU)";
        default:               return "UNKNOWN_RESET (" + String(reason) + ")";
    }
}

// Запись маркера текущего действия (вызывается перед опасными операциями)
void logCrashMarker(const char* stepInfo) {
    snprintf(rtcCrashBuffer, sizeof(rtcCrashBuffer), "%s", stepInfo);
    rtcHasCrashData = true;
}

// Вывод инфы о падении в веб-лог Гайвера при старте
void printPreviousCrashLog() {
    if (rtcHasCrashData) {
        webLogger.println("🚨 ПОСЛЕДНЕЕ ДЕЙСТВИЕ ПЕРЕД СБРОСОМ: " + String(rtcCrashBuffer));
        // Не очищаем маркер, чтобы его можно было прочитать в веб-интерфейсе
    }
}

void sysLog(const String& level, const String& module, const String& text) {
    String formattedLine = level + "[" + module + "] " + text;
    Serial.println(formattedLine);
    webLogger.println(formattedLine);
    
    // Дублируем важные ошибки в RTC буфер автоматического отслеживания падений
    if (level == "err:" || level == "warn:") {
        snprintf(rtcCrashBuffer, sizeof(rtcCrashBuffer), "[%s] %s", module.c_str(), text.c_str());
    }
}

void logSystemHealth() {
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t maxBlock = ESP.getMaxAllocHeap();
    String currentBssid = WiFi.BSSIDstr();
    
    // Делаем телеметрию максимально информативной (добавляем статус Wi-Fi)
    String wifiStatusStr = "UNKNOWN";
    if (WiFi.status() == WL_CONNECTED) wifiStatusStr = "CONNECTED";
    else if (WiFi.status() == WL_CONNECTION_LOST) wifiStatusStr = "LOST";
    else if (WiFi.status() == WL_DISCONNECTED) wifiStatusStr = "DISCONNECTED";

    String msg = "ОЗУ: " + String(freeHeap) + " Б (Блок: " + String(maxBlock) + " Б) | ";
    msg += "Статус Сети: " + wifiStatusStr + " (" + String(WiFi.RSSI()) + " dBm) | ";
    msg += "Роутер (BSSID): " + currentBssid;
    
    if (freeHeap < 40000) {
        sysLog("warn:", "ДИАГНОСТИКА", msg);
    } else {
        sysLog("info:", "ДИАГНОСТИКА", msg);
    }
}
