/**
 * DIY Smart Home - light switch
 * Manual switch
 * 2021 Łukasz Łasek
 */
#include "ManualSwitch.h"
#include "WiFiHelper.h"
#include "Mqtt.h"
#include "Utils.h"

#define PIN_IN0     D5  // GPIO 14
#define PIN_IN1     D6  // GPIO 12
#define PIN_IN2     D7  // GPIO 13
uint8_t CManualSwitch::Sm_arrPinIn[ SW_CHANNELS ] = { PIN_IN0, PIN_IN1, PIN_IN2 };

#define PIN_OUT0    D1  // GPIO 5
#define PIN_OUT1    D2  // GPIO 4
#define PIN_OUT2    D8  // GPIO 15
uint8_t CManualSwitch::Sm_arrPinOut[ SW_CHANNELS ] = { PIN_OUT0, PIN_OUT1, PIN_OUT2 };

extern CWiFiHelper g_wifi;
extern CMqtt g_mqtt;

File CManualSwitch::OpenCfg()
{
    return LittleFS.open( FS_SW_CFG, "r" );
}

void CManualSwitch::ReadCfg( File& a_rFile )
{
    if( a_rFile )
    {
        m_nMode = a_rFile.readStringUntil( '\n' ).toInt();
        m_nLongTapMs = a_rFile.readStringUntil( '\n' ).toInt();
        m_nDblTapMs = a_rFile.readStringUntil( '\n' ).toInt();
        m_strTapBeacon = a_rFile.readStringUntil( '\n' );

        DBGLOG4( "sw cfg: swmod:%d long:%u dbl:%u tapbcn:'%s'\n",
            m_nMode, m_nLongTapMs, m_nDblTapMs, m_strTapBeacon.c_str());
    }
    else
    {
        m_nMode = 1;
        m_nLongTapMs = m_nDblTapMs = 250;
        DBGLOG( "sw cfg missing" );
    }
}

void CManualSwitch::Enable( uint8_t a_nChanNo )
{
    if(( a_nChanNo >= SW_CHANNELS ) || ( m_nMode == SW_MODE_DISABLED ))
        return;

    m_tmAutoOff.UpdateAll();
    m_nPinSwitch = Sm_arrPinOut[ a_nChanNo ];
    pinMode( m_nPinSwitch, OUTPUT );
    DriveSwitch( false );
    CTouchBtn::Enable( Sm_arrPinIn[ a_nChanNo ], m_nLongTapMs, m_nDblTapMs );   
}

void CManualSwitch::Disable()
{
    CTouchBtn::Disable();
}

void CManualSwitch::DriveSwitch( bool a_bStateOn )
{
    m_nPinSwitchVal = ( a_bStateOn ) ? HIGH : LOW;
    digitalWrite( m_nPinSwitch, m_nPinSwitchVal );
}

bool CManualSwitch::GetSwitchState()
{
    return ( m_nPinSwitchVal == LOW ) ? false : true;
}

void CManualSwitch::loop()
{
    CTouchBtn::loop();
    if( m_nAutoOff )
    {
        m_tmAutoOff.UpdateCur();
        if( m_tmAutoOff.Delta() >= m_nAutoOff )
        {
            DriveSwitch( false );
            m_nAutoOff = 0;
            MqttPubStat();
        }
    }
}

void CManualSwitch::OnShortTap( uint16_t a_nCnt )
{
    CTouchBtn::OnShortTap( a_nCnt );
    DBGLOG2( "short tap x%d pin#%d\n", a_nCnt, m_nPin );
    if( a_nCnt > 1 )
    {
        DriveSwitch( true );
        m_nAutoOff = ((ulong)a_nCnt - 1 ) * 1000 * 60;    // wait (tap cnt - 1) minutes
        m_tmAutoOff.UpdateAll();
    }
    else
    {
        DriveSwitch( !GetSwitchState());
        m_nAutoOff = 0;
    }
    MqttPubStat();
}

void CManualSwitch::OnLongTap()
{
    CTouchBtn::OnLongTap();
    DBGLOG1( "long tap pin#%d\n", m_nPin );
    DriveSwitch( !GetSwitchState());
    m_nAutoOff = 0;
    MqttPubStat();
    g_mqtt.PubPriv( GetTapBeacon( SW_TAP_BEACON_CMD_LONG ).c_str());
}

void CManualSwitch::OnMqttBeacon( byte* payload, uint len )
{
    byte nTapCmd = GetTapBeaconCmd( payload, len );
    if( nTapCmd == SW_TAP_BEACON_CMD_LONG )
    {
        DriveSwitch( false );
        m_nAutoOff = 0;
        MqttPubStat();
    }
}

void CManualSwitch::MqttPubStat()
{
    g_mqtt.PubStat( GetChanNo(), GetSwitchState());
}

char CManualSwitch::GetChanNo()
{
    switch( m_nPin )
    {
        case PIN_IN0:
            return SW_CHANNEL_0;

        case PIN_IN1:
            return SW_CHANNEL_1;

        case PIN_IN2:
            return SW_CHANNEL_2;
    }
    return SW_CHANNEL_NA;
}

// tap beacon format is: <beacon><cmd L:long><src channel #><src hostname>
String CManualSwitch::GetTapBeacon( const char a_nTapCmd )
{
    return( m_strTapBeacon + a_nTapCmd + GetChanNo() + g_wifi.GetHostName());
}

byte CManualSwitch::GetTapBeaconCmd( byte* payload, uint len )
{
    if(( len > 0 ) && ( StringAt0( m_strTapBeacon, payload, len )))
    {
        payload += m_strTapBeacon.length();
        len -= m_strTapBeacon.length();
        if( len > 2 )
        {
            byte nTapCmd = *(payload++);
            if( nTapCmd == SW_TAP_BEACON_CMD_LONG )
            {
                byte nChanNo = *(payload++);
                if(( nChanNo != GetChanNo())
                    || ( !StringEq( g_wifi.GetHostName(), payload, len - 2 )))
                {
                    return SW_TAP_BEACON_CMD_LONG;
                }
            }
        }
    }
    return SW_TAP_BEACON_CMD_IGNORE;
}
