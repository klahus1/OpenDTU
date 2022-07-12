#include "MqttSettings.h"
#include "Configuration.h"
#include "WiFiSettings.h"
#include <AsyncMqttClient.h>
#include <Ticker.h>
#include <WiFi.h>

MqttSettingsClass::MqttSettingsClass()
    : mqttClient()
{
}

void MqttSettingsClass::WiFiEvent(WiFiEvent_t event)
{
    switch (event) {
#ifdef OLIMEX_ESP32_POE_LAN
    case ARDUINO_EVENT_ETH_GOT_IP:
        Serial.println(F("MQTT: ETH connected"));
        performConnect();
        break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
        Serial.println(F("MQTT: ETH lost connection"));
        mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
        break;
#else    
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.println(F("WiFi connected"));
        performConnect();
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println(F("WiFi lost connection"));
        mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
        break;
#endif
    }
}

void MqttSettingsClass::onMqttConnect(bool sessionPresent)
{
    Serial.println(F("Connected to MQTT."));
    CONFIG_T& config = Configuration.get();
    publish(config.Mqtt_LwtTopic, config.Mqtt_LwtValue_Online);
}

void MqttSettingsClass::onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
    Serial.println(F("Disconnected from MQTT."));

    mqttReconnectTimer.once(
        2, +[](MqttSettingsClass* instance) { instance->performConnect(); }, this);
}

void MqttSettingsClass::performConnect()
{
#ifdef OLIMEX_ESP32_POE_LAN
    if (Lan.isConnected() && Configuration.get().Mqtt_Enabled) {
#else
    if (WiFi.isConnected() && Configuration.get().Mqtt_Enabled) {
#endif
        Serial.println(F("Connecting to MQTT..."));
        CONFIG_T& config = Configuration.get();
        mqttClient.setServer(config.Mqtt_Hostname, config.Mqtt_Port);
        mqttClient.setCredentials(config.Mqtt_Username, config.Mqtt_Password);

        willTopic = String(config.Mqtt_Topic) + config.Mqtt_LwtTopic;
        mqttClient.setWill(willTopic.c_str(), 2, config.Mqtt_Retain, config.Mqtt_LwtValue_Offline);

        clientId = WiFiSettings.getApName();
        mqttClient.setClientId(clientId.c_str());

        mqttClient.connect();
    }
}

void MqttSettingsClass::performDisconnect()
{
    CONFIG_T& config = Configuration.get();
    publish(config.Mqtt_LwtTopic, config.Mqtt_LwtValue_Offline);
    mqttClient.disconnect();
}

void MqttSettingsClass::performReconnect()
{
    performDisconnect();

    mqttReconnectTimer.once(
        2, +[](MqttSettingsClass* instance) { instance->performConnect(); }, this);
}

bool MqttSettingsClass::getConnected()
{
    return mqttClient.connected();
}

void MqttSettingsClass::publish(String subtopic, String payload)
{
    String topic = Configuration.get().Mqtt_Topic;
    topic += subtopic;
    mqttClient.publish(topic.c_str(), 0, Configuration.get().Mqtt_Retain, payload.c_str());
}

void MqttSettingsClass::init()
{
    using namespace std::placeholders;
    WiFi.onEvent(std::bind(&MqttSettingsClass::WiFiEvent, this, _1));

    mqttClient.onConnect(std::bind(&MqttSettingsClass::onMqttConnect, this, _1));
    mqttClient.onDisconnect(std::bind(&MqttSettingsClass::onMqttDisconnect, this, _1));
}

MqttSettingsClass MqttSettings;
