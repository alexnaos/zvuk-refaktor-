#pragma once
#include <Arduino.h>
#include "vs1053_ext.h"

extern VS1053* player;

void initAudio();
void startRadioStream(const char* url);
void updateVolume(int vol);
void updateTone(int bass, int treble);
void changeStation(int stationIndex);
void loopAudioPlayback();

// Остановка потока по инициативе пользователя (кнопка "Стоп", MQTT "stop"/"turnoff",
// запуск записи). Отключает автопереподключение, чтобы ensureStreamAlive()
// не поднимал поток, который пользователь остановил сам.
void notifyUserStop();

// Чистые объявления функций без extern "C", строго соответствующие библиотеке
void vs1053_showstreamtitle(const char *info);
void vs1053_showstreaminfo(const char *info);
