#include <Arduino.h>
#include <WiFi.h> // Добавлен для распознавания типа WiFiClient
#include <PubSubClient.h>

// Экспортируем сетевые клиенты для возможности диагностики
extern WiFiClient espClient;
extern PubSubClient mqttClient;

void initMQTT();
void loopMQTT();
void mqttPublishTrack(const char *trackInfo);
void mqttPublishStatus();
void mqttPublishPlaylist();
