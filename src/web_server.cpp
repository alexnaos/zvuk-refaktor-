#include "web_server.h"
#include "web_interface.h"
#include "config.h"
#include "core/system_logger.h"
#include "esp_task_wdt.h"
#include <LittleFS.h>

// Физическое выделение памяти под БД и сервер (стр. 13-14 документации)
GyverDBFile db(&LittleFS, "/db.bin");
SettingsGyver sett("ESP32S2 RefaktoRadio", &db);

void initWebServer() {
    if (LittleFS.begin(true)) {
        db.begin();
        
        // РЕГИСТРАЦИЯ И ИНИЦИАЛИЗАЦИЯ ДЕФОЛТНЫХ КЛЮЧЕЙ БД СТРОГО ПО СУЩЕСТВУЮЩЕМУ МАКРОСУ
        // Метод .init() создает ячейку только если её еще нет, не перезаписывая старые данные
        db.init(SH("vol"), VOLUME_DEFAULT);
        db.init(SH("bass"), BASS_DEFAULT);
        db.init(SH("treble"), TREBLE_DEFAULT);
        db.init(SH("st_id"), 0);
        db.init(SH("c_url"), "");
        db.update(); // Принудительно сбрасываем структуру на Flash
        
        sysLog("info:", "СЕРВЕР", "База данных успешно проинициализирована начальными ключами.");
    } else {
        sysLog("err:", "СЕРВЕР", "КРИТИКА: LittleFS недоступен, БД не запущена!");
    }

    // Регистрация кастомного CSS через custom.js (инжектит стили в <head>)
    if (LittleFS.exists("/custom.js")) {
        sett.setCustomFile("/custom.js");
        sysLog("info:", "СЕРВЕР", "Кастомный CSS/JS файл загружен с LittleFS.");
    }

    // Настройка темы (Default =跟随 custom.js overrides)
    sett.config.theme = sets::Colors::Default;

    sett.begin();
    sett.onBuild(buildInterface);   
    sett.onUpdate(updateInterface); 
    sysLog("info:", "СЕРВЕР", "Веб-сервер успешно запущен.");
}

void handleWebRequests() {
    // Каждый раз, когда веб-сервер принимает пакет данных (включая куски OTA-прошивки),
    // мы нативно сбрасываем счетчик ядра WDT, предотвращая ложный ресет платы.
       esp_task_wdt_reset(); 
   
    sett.tick(); // Вызывается в главном loop
}
