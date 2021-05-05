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

#include "dbg.h"

/**
 * mqtt_cfg file:
 * 1: mqtt server: hostname or ip
 * 2: mqtt port
 * 3: mqtt server conn timeout ms
 * 4: mqtt client id
 * 5: mqtt sub topic: action
 * 6: mqtt pub topic: action
 * 7: mqtt sub topic: mgmt
 * 8: mqtt pub topic: mgmt
 */
#define FS_MQTT_CFG     "mqtt_cfg"

class CMqtt {
public:
    CMqtt() : m_mqtt( m_wc ) {}

    void ReadCfg() {
        File file = LittleFS.open( FS_MQTT_CFG, "r" );
        if( file ) {
            m_strServer = file.readStringUntil( '\n' );
            m_nPort = file.readStringUntil( '\n' ).toInt();
            m_nConnTimeout = file.readStringUntil( '\n' ).toInt();
            m_strClientId = file.readStringUntil( '\n' );
            m_strSubTopicAct = file.readStringUntil( '\n' );
            m_strPubTopicAct = file.readStringUntil( '\n' );
            m_strSubTopicMgmt = file.readStringUntil( '\n' );
            m_strPubTopicMgmt = file.readStringUntil( '\n' );

            DBGLOG4( "mqtt cfg: server:'%s' port:%u timeo:%u client-id:'%s' ",
                m_strServer.c_str(), m_nPort, m_nConnTimeout, m_strClientId.c_str());
            DBGLOG4( "act-sub/pub:'%s'/'%s' mgmt-sub/pub:'%s'/'%s'\n",
                m_strSubTopicAct.c_str(), m_strPubTopicAct.c_str(), m_strSubTopicMgmt.c_str(), m_strPubTopicMgmt.c_str());
        } else {
            DBGLOG( "mqtt cfg missing" );
        }
    }

    void loop()
    {
        if( !m_mqtt.connected()) {
            if( m_mqtt.connect( m_strClientId.c_str())) {
                m_mqtt.subscribe( m_strSubTopicAct.c_str());
                m_mqtt.subscribe( m_strSubTopicMgmt.c_str());
                DBGLOG( "mqtt connected" );
            }
        }
        m_mqtt.loop();
    }

    void Enable() {
        m_wc.setTimeout( m_nConnTimeout );
        m_mqtt.setServer( m_strServer.c_str(), m_nPort );
        m_mqtt.setCallback([ this ]( char* topic, byte* payload, uint len ) {
                this->MqttCb( topic, payload, len );
            });
    }

    void MqttCb( char* topic, byte* payload, uint len );

protected:
    WiFiClient m_wc;
    PubSubClient m_mqtt;

    // cfg:
    String m_strServer, m_strClientId;
    uint16_t m_nPort, m_nConnTimeout;
    String m_strSubTopicAct, m_strPubTopicAct;
    String m_strSubTopicMgmt, m_strPubTopicMgmt;
};
