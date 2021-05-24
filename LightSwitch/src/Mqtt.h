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
 * 6: mqtt sub topic: command
 * 7: mqtt pub topic: status
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

    void ReadCfg()
    {
        File file = LittleFS.open( FS_MQTT_CFG, "r" );
        if( file )
        {
            m_strServer = file.readStringUntil( '\n' );
            m_nPort = file.readStringUntil( '\n' ).toInt();
            m_nConnTimeout = file.readStringUntil( '\n' ).toInt();
            m_nHeartbeatIntvl = file.readStringUntil( '\n' ).toInt();
            m_strClientId = file.readStringUntil( '\n' );
            m_strSubTopicCmd = file.readStringUntil( '\n' );
            m_strPubTopicStat = file.readStringUntil( '\n' );

            DBGLOG5( "mqtt cfg: server:'%s' port:%u timeo:%u heartbeat:%u client-id:'%s' ",
                m_strServer.c_str(), m_nPort, m_nConnTimeout, m_nHeartbeatIntvl, m_strClientId.c_str());
            DBGLOG2( "cmd-sub:'%s' stat-pub:'%s'\n",
                m_strSubTopicCmd.c_str(), m_strPubTopicStat.c_str());
        }
        else
        {
            DBGLOG( "mqtt cfg missing" );
        }
    }

    void Enable()
    {
        if( m_bEnabled )
            return;

        m_bEnabled = true;
        m_wc.setTimeout( m_nConnTimeout );
        m_mqtt.setServer( m_strServer.c_str(), m_nPort );
        m_mqtt.setCallback(
            [ this ]( char* topic, byte* payload, uint len )
            {
                this->MqttCb( topic, payload, len );
            });
    }

    void Disable()
    {
        // Fake disable - report 'offline' only but don't actually disconnect mqtt to allow for a reset cmd in case of emergency
        // m_bEnabled = false;
        PubStat( MQTT_STAT_OFFLINE );
        // m_mqtt.disconnect();
    }

    bool PubStat( char a_nChannel, bool a_bStateOn )
    {
        return PubStat( a_nChannel, ( a_bStateOn ) ? MQTT_CMD_CH_ON : MQTT_CMD_CH_OFF );
    }

    void Heartbeat( bool a_bForceSend );
    void loop();
    void MqttCb( char* topic, byte* payload, uint len );

protected:
    String GetChannelTopic( char a_nChannel, String& a_rstrTopic )
    {
        String strTopic( a_rstrTopic );
        if( a_nChannel )
        {
            strTopic += MQTT_TOPIC_CHANNEL;
            strTopic += a_nChannel;
        }
        return strTopic;
    }

    bool PubStat( char a_nChannel, const char* a_pszMsg )
    {
        String strTopic = GetChannelTopic( a_nChannel, m_strPubTopicStat );
        DBGLOG2( "mqtt pub t:'%s' p:'%s'\n", strTopic.c_str(), a_pszMsg );
        return m_mqtt.publish( strTopic.c_str(), a_pszMsg );
    }

    bool PubStat( const char* a_pszMsg )
    {
        return PubStat( 0, a_pszMsg );
    }

    WiFiClient m_wc;
    PubSubClient m_mqtt;
    bool m_bEnabled;
    CTimer m_tmHeartbeat;

    // cfg:
    String m_strServer, m_strClientId;
    uint16_t m_nPort, m_nConnTimeout, m_nHeartbeatIntvl;
    String m_strSubTopicCmd;
    String m_strPubTopicStat;
};
