#include "fs_module.h"
#include <LittleFS.h>

void initFileSystem() {
    if (!LittleFS.begin(true)) {
        Serial.println("[КРИТ]: Ошибка монтирования LittleFS!");
        return;
    }
    Serial.println("[СИСТЕМА]: LittleFS успешно смонтирован.");
}

void removeFile(const char* path) {
    if (LittleFS.exists(path)) {
        LittleFS.remove(path);
    }
}

bool fileExists(const char* path) {
    return LittleFS.exists(path);
}
