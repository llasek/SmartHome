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

void CMqtt::loop()
{
    if( !m_bEnabled )
        return;

    if(( !m_mqtt.connected()) && ( m_mqtt.connect( m_strClientId.c_str())))
    {
        m_mqtt.subscribe( m_strSubTopicCmd.c_str());
        DBGLOG( "mqtt connected" );
        Heartbeat( true );
        g_swChan0.OnMqttConnected();
        g_swChan1.OnMqttConnected();
        g_swChan2.OnMqttConnected();
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

    if(( len == MQTT_CMD_RESET_LEN ) && ( !memcmp( MQTT_CMD_RESET, payload, len )))
    {
        DBGLOG( "reset" );
        Disable();
        ESP.reset();
    }

    if((( len == MQTT_CMD_CH_ON_LEN ) || ( len == MQTT_CMD_CH_OFF_LEN ))
        && ( payload[ 0 ] == 'c' )
        && ( payload[ 1 ] == 'h' ))
    {
        uint8_t nChanNo = payload[ 2 ] - '0';
        if( nChanNo < SW_CHANNELS ) {
            CManualSwitch& rSw = ( nChanNo == 0 ) ? g_swChan0 : ( nChanNo == 1 ) ? g_swChan1 : g_swChan2;
            bool bEnable = ( len == 5 ) && ( payload[ 3 ] == 'o' ) && ( payload[ 4 ] == 'n' );
            bool bDisable = ( len == 6 ) && ( payload[ 3 ] == 'o' ) && ( payload[ 4 ] == 'f' ) && ( payload[ 5 ] == 'f' );
            if( bEnable )
                rSw.OnShortClick();
            else if( bDisable )
                rSw.OnLongClick();
        }
    }
}
