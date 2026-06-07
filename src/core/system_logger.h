#pragma once
#include <Arduino.h>
#include <rom/rtc.h>
#include <SettingsGyver.h>

// Даем другим модулям знать, что объект логгера существует
extern sets::Logger webLogger;

// Объявления функций диагностики
String getResetReason(RESET_REASON reason);
void sysLog(const String& level, const String& module, const String& text);
void logSystemHealth();

// НОВЫЕ ФУНКЦИИ ДЛЯ ЧУДО-ЛОГА ПОСЛЕ ПАДЕНИЯ
void logCrashMarker(const char* stepInfo);
void printPreviousCrashLog();
