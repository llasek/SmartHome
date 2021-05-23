/**
 * DIY Smart Home - light switch
 * Manual switch
 * 2021 Łukasz Łasek
 */
#include "ManualSwitch.h"

#define PIN_IN0     D5  // GPIO 14
#define PIN_IN1     D6  // GPIO 12
#define PIN_IN2     D7  // GPIO 13
uint8_t CManualSwitch::Sm_arrPinIn[ SW_CHANNELS ] = { PIN_IN0, PIN_IN1, PIN_IN2 };

#define PIN_OUT0    D1  // GPIO 5
#define PIN_OUT1    D2  // GPIO 4
#define PIN_OUT2    D8  // GPIO 15
uint8_t CManualSwitch::Sm_arrPinOut[ SW_CHANNELS ] = { PIN_OUT0, PIN_OUT1, PIN_OUT2 };

extern CMqtt g_mqtt;

char CManualSwitch::GetChanNo()
{
    switch( m_nPin )
    {
        case PIN_IN0:
            return '0';

        case PIN_IN1:
            return '1';

        case PIN_IN2:
            return '2';
    }
    return 'x';
}

void CManualSwitch::MqttPubStat( bool a_bStateOn )
{
    g_mqtt.PubStat( GetChanNo(), a_bStateOn );
}

void CManualSwitch::OnMqttConnected()
{
    g_mqtt.PubStat( GetChanNo(), GetSwitchState());
}
