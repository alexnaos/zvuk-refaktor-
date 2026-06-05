#include "web_interface.h"
#include "config.h"
#include "global_state.h"
#include "web_server.h"
#include "core/system_logger.h"
#include "core/audio_playback.h"
#include "core/audio_recorder.h"
#include "core/fs_module.h"
#include <Update.h>

// Регистрация ключей БД строго по вашей спецификации
DB_KEYS(kk, vol, bass, treble, st_id, c_url, txt_st, txt_tr, sys_log);

String getStationsOptions() {
    if (stationCount == 0) return "Нет станций";
    String list = "";
    for (int i = 0; i < stationCount; i++) {
        list += stationList[i].name;
        if (i < stationCount - 1) list += ";"; // Разделитель ';' по доке (стр. 8)
    }
    return list;
}

void buildInterface(sets::Builder &b) {
    // ---- ГРУППА: ТЕКУЩИЙ ПОТОК ----
    {
        sets::Group g(b, "Текущий поток");
        b.Label(kk::txt_st, "Станция", currentStationName);
        b.Label(kk::txt_tr, "Трек", currentTrack);
        
        if (b.Slider(kk::vol, "Громкость", 0, 21, 1)) {
            extern GyverDBFile db;
            updateVolume(db[kk::vol].toInt());
        }
        
        {
            sets::Buttons btns(b);
            if (b.Button("⏮ Назад")) {
                if (stationCount > 0) {
                    extern GyverDBFile db;
                    currentStationIdx = (currentStationIdx - 1 + stationCount) % stationCount;
                    db[kk::st_id] = currentStationIdx;
                    db.update();
                    customUrl = "";
                    changeStationFlag = true;
                }
            }
            if (b.Button("▶ Плей")) {
                if (currentStationIdx >= 0 && currentStationIdx < stationCount)
                    changeStationFlag = true;
            }
            if (b.Button("⏹ Стоп")) {
                changeStationFlag = false;
                if (player != nullptr) player->stop_mp3client();
            }
            if (b.Button("⏭ Вперед")) {
                if (stationCount > 0) {
                    extern GyverDBFile db;
                    currentStationIdx = (currentStationIdx + 1) % stationCount;
                    db[kk::st_id] = currentStationIdx;
                    db.update();
                    customUrl = "";
                    changeStationFlag = true;
                }
            }
        }
    }

    // ---- ГРУППА: НАСТРОЙКИ ТЕМБРА ----
    {
        sets::Group g(b, "Настройки тембра");
        bool toneChanged = false;
        if (b.Slider(kk::bass, "Усиление НЧ (Бас)", 0, 15, 1)) toneChanged = true;
        if (b.Slider(kk::treble, "Усиление ВЧ (Требл)", -8, 7, 1)) toneChanged = true;
        if (toneChanged) {
            extern GyverDBFile db;
            updateTone(db[kk::bass].toInt(), db[kk::treble].toInt());
        }
    }

    // ---- ГРУППА: ВЫБОР ИСТОЧНИКА ----
    {
        sets::Group g(b, "Выбор источника");
        if (b.Select(kk::st_id, "Выбрать станцию", getStationsOptions())) {
            extern GyverDBFile db;
            currentStationIdx = db[kk::st_id].toInt();
            customUrl = "";
            changeStationFlag = true;
        }
        if (b.Input(kk::c_url, "Кастомный URL")) {
            extern GyverDBFile db;
            customUrl = db[kk::c_url].toString();
            if (customUrl.length() > 5) {
                currentStationIdx = -1;
                changeStationFlag = true;
            }
        }
    }

    // ---- ГРУППА: УПРАВЛЕНИЕ ЗАПИСЬЮ ----
    {
        sets::Group g(b, "Управление записью (VS1053)");
        sets::Buttons btns(b);
        if (b.Button("🔴 Старт записи")) startRecording();
        if (b.Button("⬜ Стоп записи")) stopRecording();
    }

    // ---- ГРУППА: ДИАГНОСТИКА И ЛОГИ ----
    {
        sets::Group g(b, "Диагностика и Логи");
        
        // ИСПРАВЛЕНО: Полностью удален тяжелый блок чтения "LittleFS.open(/log.txt)"
        // Теперь логгер берет актуальные данные прямо из циклического буфера ОЗУ
        b.Log(kk::sys_log, webLogger, "Журнал событий");
        
        if (b.Button("Очистить историю логов")) {
            // Очищаем буфер в оперативной памяти
            // Примечание: так как файла на FS больше нет, метод просто сбрасывает ОЗУ-текст
            sysLog("info:", "СИСТЕМА", "История логов успешно очищена.");
            b.reload(); 
        }
    }
}

void updateInterface(sets::Updater &upd) {
    if (Update.isRunning()) return;
    
    upd.update(kk::txt_st, currentStationName);
    upd.update(kk::txt_tr, currentTrack);
    upd.update(kk::sys_log, webLogger); // Синхронизация лога по ОЗУ (стр. 10 доки)
}
