#include "playlist_parser.h"
#include "../config.h"
#include "../global_state.h"
#include "system_logger.h"
#include <LittleFS.h>

void loadPlaylist() {
    stationCount = 0;
    if (!LittleFS.exists("/stations.txt")) {
        sysLog("warn:", "ПАРСЕР", "Файл плейлиста /stations.txt отсутствует.");
        return;
    }
    
    File file = LittleFS.open("/stations.txt", "r");
    if (!file) {
        sysLog("err:", "ПАРСЕР", "Не удалось прочитать /stations.txt.");
        return;
    }

    bool isFirstLine = true;

    while (file.available() && stationCount < MAX_STATIONS) {
        String line = file.readStringUntil('\n');
        line.replace("\r", "");
        
        // ОЧИСТКА СКРЫТОГО BOM (Byte Order Mark) ДЛЯ ПЕРВОЙ СТРОКИ
        if (isFirstLine) {
            isFirstLine = false;
            if (line.length() >= 3 && (unsigned char)line[0] == 0xEF && (unsigned char)line[1] == 0xBB && (unsigned char)line[2] == 0xBF) {
                line = line.substring(3); // Отрезаем ровно 3 мусорных байта
                sysLog("info:", "ПАРСЕР", "Обнаружен и успешно удален скрытый UTF-8 BOM маркер.");
            }
            while (line.length() > 0 && (unsigned char)line[0] < 32) {
                line = line.substring(1);
            }
        }

        line.trim();
        if (line.length() < 5 || line.startsWith("//")) continue;

        int separatorIdx = line.indexOf('\t');
        if (separatorIdx != -1) {
            stationList[stationCount].name = line.substring(0, separatorIdx);
            stationList[stationCount].url = line.substring(separatorIdx + 1);
            stationList[stationCount].name.trim();
            stationList[stationCount].url.trim();
            stationCount++;
        }
    }
    file.close();
    sysLog("info:", "ПАРСЕР", "Загружено станций (разделитель таб): " + String(stationCount));

    if (stationCount > 0) {
        File haFile = LittleFS.open("/ha_playlist.txt", "w");
        if (haFile) {
            for (int i = 0; i < stationCount; i++) {
                haFile.print(stationList[i].name);
                haFile.print('\t');
                haFile.println(stationList[i].url);
            }
            haFile.close();
            sysLog("info:", "ПАРСЕР", "Файл /ha_playlist.txt успешно сгенерирован.");
        }
    }
}
