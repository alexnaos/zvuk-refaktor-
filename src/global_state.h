#pragma once
#include "config.h"

// Объявление разделяемых переменных состояния для всех модулей
extern Station stationList[MAX_STATIONS];
extern int stationCount;
extern int currentVolume;
extern int currentBass;
extern int currentTreble;
extern int currentStationIdx;
extern String currentStationName;
extern String currentTrack;
extern String customUrl;
extern bool changeStationFlag;
extern bool isRecording;
