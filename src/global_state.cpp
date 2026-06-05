#include "global_state.h"

// Физическое выделение памяти под переменные состояния
Station stationList[MAX_STATIONS];
int stationCount = 0;
int currentVolume = 12;
int currentBass = 0;
int currentTreble = 0;
int currentStationIdx = -1;
String currentStationName = "No Station";
String currentTrack = "No Title";
String customUrl = "";
bool changeStationFlag = false;
bool isRecording = false;
