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

#define MQTT_CMD_CH_ON      "ch_on"     // '_' is chan#
#define MQTT_CMD_CH_ON_LEN  5

#define MQTT_CMD_CH_OFF     "ch_off"    // '_' is chan#
#define MQTT_CMD_CH_OFF_LEN 6

#define MQTT_STAT_SIZE  (( MQTT_CMD_CH_ON_LEN > MQTT_CMD_CH_OFF_LEN ) ? MQTT_CMD_CH_ON_LEN : MQTT_CMD_CH_OFF_LEN )
#define DECL_MQTT_STAT( strName )           \
    String strName;                         \
    strName.reserve( MQTT_STAT_SIZE + 1 );  \
    strName = "ch";

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

    void Heartbeat( bool a_bForceSend )
    {
        m_tmHeartbeat.UpdateCur();
        if((( m_nHeartbeatIntvl > 0 ) && ( a_bForceSend )) || ( m_tmHeartbeat.Delta() >= m_nHeartbeatIntvl ))
        {
            PubStat( MQTT_STAT_ONLINE );
            m_tmHeartbeat.UpdateLast();
        }
    }

    bool PubStat( char a_nChanNo, bool a_bStateOn )
    {
        DECL_MQTT_STAT( strMqttStat );
        strMqttStat += a_nChanNo;
        strMqttStat += ( a_bStateOn ) ? "on" : "off";
        return PubStat( strMqttStat.c_str());
    }

    void loop();
    void MqttCb( char* topic, byte* payload, uint len );

protected:
    bool PubStat( const char* a_pszMsg )
    {
        DBGLOG2( "mqtt pub t:'%s' p:'%s'\n", m_strPubTopicStat.c_str(), a_pszMsg );
        return m_mqtt.publish( m_strPubTopicStat.c_str(), a_pszMsg );
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
