#include "audio_recorder.h"
#include "../global_state.h"
#include "audio_playback.h"
#include "system_logger.h"
#include <LittleFS.h>

File recordFile;

void startRecording() {
    if (player == nullptr || isRecording) return;
    sysLog("info:", "ЗАПИСЬ", "Запуск аппаратной записи звука...");
    
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
    
    // Временная заглушка для компиляции. Чтение регистров VS1053 для оцифровки 
    // будет реализовано через публичные методы плеера на этапе настройки битрейта.
    uint8_t dummyBuf[2] = {0, 0};
    recordFile.write(dummyBuf, 2); 
}
