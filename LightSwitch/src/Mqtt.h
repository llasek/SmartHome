/**
 * DIY Smart Home - light switch
 * MQTT client
 * 2021 Łukasz Łasek
 */
#pragma once
#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <LittleFS.h>

#include "Timer.h"
#include "dbg.h"

/**
 * mqtt_cfg file:
 * 1: mqtt server: hostname or ip
 * 2: mqtt port
 * 3: mqtt server conn timeout ms
 * 4: mqtt heartbeat interval ms    (0: disable heartbeat)
 * 5: mqtt client id
 * 6: mqtt sub topic: device and channel command
 * 7: mqtt pub topic: device and channel status
 * 8: mqtt pub/sub topic: device private (for e.g. tap beacons)
 */
#define FS_MQTT_CFG     "mqtt_cfg"

#define MQTT_STAT_ONLINE    "online"
#define MQTT_STAT_OFFLINE   "offline"

#define MQTT_CMD_RESET      "reset"
#define MQTT_CMD_RESET_LEN  5

#define MQTT_CMD_CH_ON      "on"
#define MQTT_CMD_CH_ON_LEN  2

#define MQTT_CMD_CH_OFF     "off"
#define MQTT_CMD_CH_OFF_LEN 3

#define MQTT_TOPIC_CHANNEL  "/ch"

class CMqtt
{
public:
    CMqtt() : m_mqtt( m_wc ), m_bEnabled( false ) {}

    void ReadCfg();

    void Enable();
    void Disable();

    bool PubStat( char a_nChannel, bool a_bStateOn );
    bool PubPriv( const char* a_pszBeacon );
    void PubHeartbeat( bool a_bForceSend );

    void loop();
    void MqttCb( char* topic, byte* payload, uint len );

protected:
    String GetChannelTopic( char a_nChannel, String& a_rstrTopic );

    bool PubStat( char a_nChannel, const char* a_pszMsg );
    bool PubStat( const char* a_pszMsg );

    WiFiClient m_wc;
    PubSubClient m_mqtt;
    bool m_bEnabled;
    CTimer m_tmHeartbeat;

    // cfg:
    String m_strServer, m_strClientId;
    uint16_t m_nPort, m_nConnTimeout, m_nHeartbeatIntvl;
    String m_strSubTopicCmd;
    String m_strPubTopicStat;
    String m_strPubSubTopicPriv;
};
