#include "audio_recorder.h"
#include "../global_state.h"
#include "audio_playback.h"
#include "system_logger.h"
#include "esp_task_wdt.h"
#include <LittleFS.h>

File recordFile;

// Лимит записи: ~1 МБ = 1024 * 1024 байт
const unsigned long RECORD_FILE_LIMIT = 1024 * 1024;

void startRecording() {
    if (player == nullptr || isRecording) return;
    sysLog("info:", "ЗАПИСЬ", "Запуск аппаратной записи звука...");
    
    // Удаляем старую запись, если она существует
    if (LittleFS.exists("/record.wav")) {
        LittleFS.remove("/record.wav");
    }
    
    // Метод стопа в библиотеке Wolle вызывается напрямую, он сам проверяет состояние
    player->stop_mp3client();
    
    recordFile = LittleFS.open("/record.wav", "w");
    if (!recordFile) {
        sysLog("err:", "ЗАПИСЬ", "Не удалось создать файл /record.wav");
        return;
    }
    isRecording = true;
}

void stopRecording() {
    if (!isRecording) return;
    isRecording = false;
    if (recordFile) {
        recordFile.close();
        sysLog("info:", "ЗАПИСЬ", "Аудиозапись успешно сохранена в /record.wav");
    }
}

void loopAudioRecorder() {
    if (!isRecording || !recordFile) return;
    
    // Сбрасываем WDT чтобы запись на Flash не вызвала перезагрузку
    esp_task_wdt_reset();
    
    // Защита от переполнения: если файл превысил лимит — останавливаем запись
    if (recordFile.size() >= RECORD_FILE_LIMIT) {
        sysLog("warn:", "ЗАПИСЬ", "Достигнут лимит файла записи. Остановка.");
        stopRecording();
        return;
    }
    
    // Временная заглушка для компиляции. Чтение регистров VS1053 для оцифровки 
    // будет реализовано через публичные методы плеера на этапе настройки битрейта.
    uint8_t dummyBuf[2] = {0, 0};
    recordFile.write(dummyBuf, 2); 
}
