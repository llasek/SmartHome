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
        m_strPhantomMqttPubTopic = CfgFileReadLine( a_rFile );
        if( m_strPhantomMqttPubTopic == CFG_SW_MODE_DISABLED )
        {
            m_nMode = SW_MODE_DISABLED;
            m_strPhantomMqttPubTopic.clear();
        }
        else if( m_strPhantomMqttPubTopic == CFG_SW_MODE_ENABLED )
        {
            m_nMode = SW_MODE_ENABLED;
            m_strPhantomMqttPubTopic.clear();
        }
        else
        {
            m_nMode = SW_MODE_PHANTOM;
        }

        m_nLongTapMs = CfgFileReadLine( a_rFile ).toInt();
        m_nDblTapMs = CfgFileReadLine( a_rFile ).toInt();
        m_strGroupName = CfgFileReadLine( a_rFile );

        CfgFileReadLine( a_rFile );  // separator - empty double line

        DBGLOG5( "sw cfg: swmod:%d '%s' long:%u dbl:%u group:'%s'\n",
            m_nMode, m_strPhantomMqttPubTopic.c_str(), m_nLongTapMs, m_nDblTapMs, m_strGroupName.c_str());
    }
    else
    {
        m_nMode = SW_MODE_DISABLED;
        m_nLongTapMs = m_nDblTapMs = 0;
        DBGLOG( "sw cfg missing - disable" );
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
    if( m_nMode == SW_MODE_ENABLED )
    {
        digitalWrite( m_nPinSwitch, m_nPinSwitchVal );
    }
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
    switch( m_nMode )
    {
        case SW_MODE_ENABLED:
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
            break;

        case SW_MODE_PHANTOM:
            MqttSendPhantomCmd( MQTT_CMD_CH_SHORT_TAP, a_nCnt );
            break;

        default:
            break;
    }
}

void CManualSwitch::OnLongTap()
{
    CTouchBtn::OnLongTap();
    DBGLOG1( "long tap pin#%d\n", m_nPin );
    switch( m_nMode )
    {
        case SW_MODE_ENABLED:
            DriveSwitch( !GetSwitchState());
            m_nAutoOff = 0;
            MqttPubStat();
            g_mqtt.PubPriv( GetTapBeacon( SW_TAP_BEACON_CMD_LONG ).c_str());
            break;

        case SW_MODE_PHANTOM:
            MqttSendPhantomCmd( MQTT_CMD_CH_LONG_TAP, 1 );
            break;

        default:
            break;
    }
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
    if( m_nMode == SW_MODE_ENABLED )
    {
        g_mqtt.PubStat( GetChanNo(), GetSwitchState());
    }
}

void CManualSwitch::MqttSendPhantomCmd( const char* a_pszMqttCmd, uint16_t a_nArg )
{
    String strCmd( a_pszMqttCmd );
    strCmd += a_nArg;
    g_mqtt.PubMsg( m_strPhantomMqttPubTopic.c_str(), strCmd.c_str());
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

// tap beacon format is: <name><cmd L:long><src channel #><src hostname>
String CManualSwitch::GetTapBeacon( const char a_nTapCmd )
{
    return( m_strGroupName + a_nTapCmd + GetChanNo() + g_wifi.GetHostName());
}

byte CManualSwitch::GetTapBeaconCmd( byte* payload, uint len )
{
    if(( len > 0 ) && ( StringBeginsWith( m_strGroupName, payload, len )))
    {
        payload += m_strGroupName.length();
        len -= m_strGroupName.length();
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
