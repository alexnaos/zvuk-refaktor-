#include "mqtt_module.h"
#include "config.h"
#include "global_state.h"
#include "core/system_logger.h"
#include "core/audio_playback.h"
#include <WiFi.h>

// Физическое выделение памяти под сетевые клиенты
WiFiClient espClient;
PubSubClient mqttClient(espClient);

const char *topic_command = MQTT_BASE_TOPIC "/command";
const char *topic_status = MQTT_BASE_TOPIC "/status";
const char *topic_playlist = MQTT_BASE_TOPIC "/playlist";
const char *topic_volume = MQTT_BASE_TOPIC "/volume";

String currentMqttTrack = "Radio Ready";
String currentMqttStatus = "playing";

void mqttPublishPlaylist() {
    if (!mqttClient.connected()) return;
    // Возвращен оригинальный путь получения файла через встроенный веб-сервер
    String playlistUrl = "http://" + WiFi.localIP().toString() + "/fetch?path=/ha_playlist.txt";
    mqttClient.publish(topic_playlist, playlistUrl.c_str(), true);
}

void mqttPublishStatus() {
    if (!mqttClient.connected()) return;
    
    int haVolume = (int)((currentVolume * 254) / 21);
    haVolume = constrain(haVolume, 0, 254);
    mqttClient.publish(topic_volume, String(haVolume).c_str(), true);

    String track = currentMqttTrack;
    track.replace("\"", "'");
    if (track.length() == 0) track = "Radio Ready";

    String station = "";
    if (currentStationIdx >= 0 && currentStationIdx < stationCount) {
        station = stationList[currentStationIdx].name;
    } else {
        station = currentStationName;
    }
    station.replace("\"", "'");
    if (station.length() == 0 || station == "No Station") station = "Sloboda Radio";

    int currentIdx = (currentStationIdx >= 0) ? currentStationIdx + 1 : 1;
    int isStatusPlaying = (currentMqttStatus == "playing") ? 1 : 0;

    // Сборка оригинального JSON YoRadio: строго "title" и "artist"
    String json = "{";
    json += "\"title\":\"" + track + "\",";
    json += "\"artist\":\"" + station + "\",";
    json += "\"name\":\"" + station + "\",";
    json += "\"on\":1,";
    json += "\"status\":" + String(isStatusPlaying) + ",";
    json += "\"station\":" + String(currentIdx);
    json += "}";

    mqttClient.publish(topic_status, json.c_str(), true);
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    String body = "";
    for (unsigned int i = 0; i < length; i++) body += (char)payload[i];
    body.trim();

    sysLog("info:", "MQTT", "Получена команда от Home Assistant: -> " + body);

    if (body.startsWith("vol ")) {
        int haVol = body.substring(4).toInt();
        updateVolume((haVol * 21) / 254);
    } else if (body.startsWith("play ")) {
        int targetIdx = body.substring(5).toInt() - 1;
        if (targetIdx >= 0 && targetIdx < stationCount) {
            currentStationIdx = targetIdx;
            customUrl = "";
            changeStationFlag = true;
            currentMqttStatus = "playing";
        }
    } else if (body == "next" && stationCount > 0) {
        currentStationIdx = (currentStationIdx + 1) % stationCount;
        customUrl = "";
        changeStationFlag = true;
        currentMqttStatus = "playing";
    } else if (body == "prev" && stationCount > 0) {
        currentStationIdx = (currentStationIdx - 1 + stationCount) % stationCount;
        customUrl = "";
        changeStationFlag = true;
        currentMqttStatus = "playing";
    } else if (body == "stop") {
        currentMqttStatus = "stopped";
        changeStationFlag = false;
        if (player != nullptr) player->stop_mp3client();
    } else if (body == "start" || body == "play") {
        currentMqttStatus = "playing";
        changeStationFlag = true;
    } else if (body == "turnoff") {
        currentMqttStatus = "stopped";
        changeStationFlag = false;
        if (player != nullptr) player->stop_mp3client();
    }
    mqttPublishStatus();
}

void connectMQTT() {
    if (!mqttClient.connected()) {
        Serial.print("[MQTT]: Подключение к брокеру...");
        if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
            Serial.println(" успешно!");
            mqttClient.subscribe(topic_command);
            mqttPublishPlaylist();
            mqttPublishStatus();
        } else {
            Serial.printf(" ошибка, rc=%d. Повтор через 5 сек.\n", mqttClient.state());
        }
    }
}

void initMQTT() {
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
}

void loopMQTT() {
    if (!mqttClient.connected()) {
        static unsigned long lastReconnect = 0;
        if (millis() - lastReconnect > 5000) {
            lastReconnect = millis();
            connectMQTT();
        }
    }
    mqttClient.loop();

    static unsigned long lastStatusTime = 0;
    if (millis() - lastStatusTime > 10000) {
        lastStatusTime = millis();
        mqttPublishStatus();
    }
}

void mqttPublishTrack(const char *trackInfo) {
    currentMqttTrack = String(trackInfo);
    currentMqttStatus = "playing";
    mqttPublishStatus();
}
