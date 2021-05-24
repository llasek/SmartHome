/**
 * DIY Smart Home - light switch
 * MQTT client
 * 2021 Łukasz Łasek
 */
#include "Mqtt.h"
#include "ManualSwitch.h"

extern CManualSwitch g_swChan0;
extern CManualSwitch g_swChan1;
extern CManualSwitch g_swChan2;

void CMqtt::Heartbeat( bool a_bForceSend )
{
    m_tmHeartbeat.UpdateCur();
    if((( m_nHeartbeatIntvl > 0 ) && ( a_bForceSend )) || ( m_tmHeartbeat.Delta() >= m_nHeartbeatIntvl ))
    {
        PubStat( MQTT_STAT_ONLINE );
        m_tmHeartbeat.UpdateLast();
        g_swChan0.MqttPubStat();
        g_swChan1.MqttPubStat();
        g_swChan2.MqttPubStat();
    }
}

void CMqtt::loop()
{
    if( !m_bEnabled )
        return;

    if(( !m_mqtt.connected()) && ( m_mqtt.connect( m_strClientId.c_str())))
    {
        m_mqtt.subscribe( m_strSubTopicCmd.c_str());
        String strMqttSubTopicChan( m_strSubTopicCmd + MQTT_TOPIC_CHANNEL );
        m_mqtt.subscribe(( strMqttSubTopicChan + SW_CHANNEL_0 ).c_str());
        m_mqtt.subscribe(( strMqttSubTopicChan + SW_CHANNEL_1 ).c_str());
        m_mqtt.subscribe(( strMqttSubTopicChan + SW_CHANNEL_2 ).c_str());
        DBGLOG( "mqtt connected" );
        Heartbeat( true );
    }
    Heartbeat( false );
    m_mqtt.loop();
}

void CMqtt::MqttCb( char* topic, byte* payload, uint len )
{
    DBGLOG1( "Rcvd topic %s: '", topic );
    for( uint i = 0; i < len; i++ )
    {
        DBGLOG1( "%c", (char)payload[ i ]);
    }
    DBGLOG( '\'' );

    if( m_strSubTopicCmd == topic )
    {
        if(( len == MQTT_CMD_RESET_LEN ) && ( !memcmp( MQTT_CMD_RESET, payload, len )))
        {
            DBGLOG( "reset" );
            Disable();
            ESP.reset();
        }
        return;
    }

    CManualSwitch* pms = nullptr;
    if( GetChannelTopic( SW_CHANNEL_0, m_strSubTopicCmd ) == topic )
    {
        pms = &g_swChan0;
    }
    else if( GetChannelTopic( SW_CHANNEL_1, m_strSubTopicCmd ) == topic )
    {
        pms = &g_swChan1;
    }
    else if( GetChannelTopic( SW_CHANNEL_2, m_strSubTopicCmd ) == topic )
    {
        pms = &g_swChan2;
    }

    if( pms )
    {
        if(( len == MQTT_CMD_CH_ON_LEN ) && ( !memcmp( MQTT_CMD_CH_ON, payload, len )))
        {
            pms->OnShortClick();
        }
        else if(( len == MQTT_CMD_CH_OFF_LEN ) && ( !memcmp( MQTT_CMD_CH_OFF, payload, len )))
        {
            pms->OnLongClick();
        }
    }
}
