#include "global_state.h"

// Физическое выделение памяти под переменные состояния
Station stationList[MAX_STATIONS];
int stationCount = 0;
int currentVolume = VOLUME_DEFAULT;
int currentBass = BASS_DEFAULT;
int currentTreble = TREBLE_DEFAULT;
int currentStationIdx = -1;
String currentStationName = "No Station";
String currentTrack = "No Title";
String customUrl = "";
bool changeStationFlag = false;
bool isRecording = false;
bool mqttTrackUpdateFlag = false;
