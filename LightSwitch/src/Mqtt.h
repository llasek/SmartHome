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

#define FS_MQTT_CFG     "mqtt_cfg"

#define MQTT_STAT_ONLINE    "online"
#define MQTT_STAT_OFFLINE   "offline"

#define MQTT_CMD_RESET      "reset"
#define MQTT_CMD_RESET_LEN  5

#define MQTT_CMD_CH_ON      "on"
#define MQTT_CMD_CH_ON_LEN  2

#define MQTT_CMD_CH_OFF     "off"
#define MQTT_CMD_CH_OFF_LEN 3

#define MQTT_CMD_CH_SHORT_TAP       "taps"  // + <cnt>
#define MQTT_CMD_CH_SHORT_TAP_LEN   4

#define MQTT_CMD_CH_LONG_TAP        "tapl"  // + <cnt>
#define MQTT_CMD_CH_LONG_TAP_LEN    4

#define MQTT_TOPIC_CHANNEL  "/ch"

class CMqtt
{
public:
    CMqtt() : m_mqtt( m_wc ), m_bEnabled( false ), m_pPayloadBuf( nullptr ), m_nPayloadBufLen( 0 ) {}

    void ReadCfg();

    void Enable();
    void Disable();

    bool PubStat( char a_nChannel, bool a_bStateOn );
    bool PubPriv( const char* a_pszBeacon );
    void PubHeartbeat( bool a_bForceSend );
    bool PubCmd( const char* a_pszPubTopic, const char* a_pszPayload );

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
    byte* m_pPayloadBuf;
    uint m_nPayloadBufLen;

    // cfg:
    String m_strServer, m_strClientId;
    uint16_t m_nPort, m_nConnTimeout, m_nHeartbeatIntvl;
    String m_strSubTopicCmd;
    String m_strPubTopicStat;
    String m_strPubSubTopicPriv;
};
