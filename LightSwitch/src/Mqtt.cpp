/**
 * DIY Smart Home - light switch
 * MQTT client
 * 2021 Łukasz Łasek
 */
#include "Mqtt.h"
#include "ManualSwitch.h"

extern CManualSwitch g_swLight1;
extern CManualSwitch g_swLight2;
extern CManualSwitch g_swLight3;

void CMqtt::MqttCb( char* topic, byte* payload, uint len ) {
    // @todo: implement, test code below
    Serial.printf( "Rcvd topic %s: '", topic );
    for( uint i = 0; i < len; i++ ) {
        Serial.print((char)payload[ i ]);
    }
    Serial.println( '\'' );

    if(( m_strSubTopicAct == topic ) && ( len > 1 )) {
        byte nSw = ( payload[ 0 ] - '0' );
        if(( nSw == 0 ) || ( nSw > 3 )) {
            m_mqtt.publish( m_strPubTopicAct.c_str(), "wtf?" );
            return;
        }
        CManualSwitch& rSw = ( nSw == 1 ) ? g_swLight1 : ( nSw == 2 ) ? g_swLight2 : g_swLight3;
        byte nAct = payload[ 1 ];

        switch( nAct )
        {
            case 's':
                rSw.OnShortClick();
                break;

            case 'l':
                rSw.OnLongClick();
                break;

            case 'd':
                rSw.OnDblClick();
                break;

            default:
                m_mqtt.publish( m_strPubTopicAct.c_str(), "woot?" );
                return;
        }
    }
    m_mqtt.publish( m_strPubTopicAct.c_str(), "mkay" );
}
