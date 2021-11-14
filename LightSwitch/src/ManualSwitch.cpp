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
        // Read in the mode:
        m_strMask = CfgFileReadLine( a_rFile );
        m_nMode = SW_MODE_DISABLED;
        if( m_strMask.length() <= 2 )
        {
            m_nId = m_strMask.toInt();
            if(( m_nId > 0 ) && ( m_nId <= SW_MAX_ID ))
            {
                m_nMode = SW_MODE_ENABLED;
            }
        }
        else
        {
            m_nMode = SW_MODE_PHANTOM;
        }

        m_strMask = CfgFileReadLine( a_rFile );
        if( m_strMask.length() != MQTT_CMD_MASK_LEN )
        {
            m_nMode = SW_MODE_DISABLED;
        }
        else if( m_nMode == SW_MODE_ENABLED )
        {
            // Unmask itself
            uint8_t nId = m_nId - 1;
            uint8_t nNibble = nId >> 2;         // same as: nId / 4
            uint8_t nMask = 1 << ( nId & 3 );   // same as: 1 << ( nId % 4 )
            byte nVal = NibbleToU8_16( m_strMask[ 2 + ( 15 - nNibble )]);
            nVal &= ~nMask;
            m_strMask[ 2 + ( 15 - nNibble )] = U8ToNibble_16( nVal );
        }
        m_strMaskCopy = m_strMask;

        m_nLongTapMs = CfgFileReadLine( a_rFile ).toInt();
        m_nDblTapMs = CfgFileReadLine( a_rFile ).toInt();

        CfgFileReadLine( a_rFile );  // separator - empty double line

        DBGLOG5( "sw cfg: swmod:%d id:%d mask:'%s' long:%u dbl:%u\n",
            m_nMode, m_nId, m_strMask.c_str(), m_nLongTapMs, m_nDblTapMs);
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
            SetState( false, 0 );
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
                SetState( true, ((ulong)a_nCnt - 1 ) * 1000 * 60 ); // wait (tap cnt - 1) minutes
            }
            else
            {
                SetState( !GetSwitchState(), 0 );
            }
            break;

        case SW_MODE_PHANTOM:
            MqttSendGroupCmd( MQTT_CMD_PH_SHORT_TAP, a_nCnt );
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
            SetState( !GetSwitchState(), 0 );
            MqttSendGroupCmd( MQTT_CMD_SW_LONG_TAP, 1 );
            break;

        case SW_MODE_PHANTOM:
            MqttSendGroupCmd( MQTT_CMD_PH_LONG_TAP, 1 );
            break;

        default:
            break;
    }
}

void CManualSwitch::SetState( bool a_bStateOn, ulong a_nAutoOff )
{
    DriveSwitch( a_bStateOn );
    m_nAutoOff = a_nAutoOff;
    if( a_nAutoOff )
    {
        m_tmAutoOff.UpdateAll();
    }
    MqttPubStat();
}

void CManualSwitch::OnGroupCmd( byte* payload, uint len )
{
    if( m_nMode != SW_MODE_ENABLED )
    {
        return;
    }

    if( StringBeginsWith( MQTT_CMD_PH_LONG_TAP, MQTT_CMD_PH_LONG_TAP_LEN, payload, len ))
    {
        payload += MQTT_CMD_PH_LONG_TAP_LEN + 1;    // skip the separator
        len -= MQTT_CMD_PH_LONG_TAP_LEN + 1;
        OnGroupMaskCmd( payload, len,
            [ this ]( const char* a_pMask, uint16_t a_nCnt )
            {
                /**
                 * OnLongTap() will send an MQTT_CMD_SW_LONG_TAP group cmd with the current mask.
                 * The arriving MQTT_CMD_PH_LONG_TAP group cmd may target multiple switches,
                 * which will mask each other, e.g.
                 * MQTT_CMD_PH_LONG_TAP arrives with mask 0x03, i.e. targeting switch id 1 and 2.
                 * SW1 id=1 mask:0x000e - switch id 1 will mask (i.e. turn off) switches id 2..15 on long tap.
                 * SW2 id=2 mask:0x000d - switch id 2 will mask (i.e. turn off) switches id 1, 3..15 on long tap.
                 * In this scenario SW1 and SW2 will mask each other after executing OnLongTap().
                 * To prevent this, the arriving MQTT_CMD_PH_LONG_TAP cmd mask bits will be cleared
                 * in the current group mask of the switch channel responding to MQTT_CMD_PH_LONG_TAP.
                 * This will exclude both SW1 and SW2 from their mask.
                 * All other masked switches will receive the MQTT_CMD_SW_LONG_TAP from SW1 and SW2.
                 */
                this->GroupMaskClearBits( a_pMask );
                this->OnLongTap();
                this->GroupMaskRestore();
            });
    }
    else if( StringBeginsWith( MQTT_CMD_PH_SHORT_TAP, MQTT_CMD_PH_SHORT_TAP_LEN, payload, len ))
    {
        payload += MQTT_CMD_PH_SHORT_TAP_LEN + 1;   // skip the separator
        len -= MQTT_CMD_PH_SHORT_TAP_LEN + 1;
        OnGroupMaskCmd( payload, len,
            [ this ]( const char* a_pMask, uint16_t a_nCnt )
            {
                // OnShortTap() does not send a group cmd
                this->OnShortTap( a_nCnt );
            });
    }
    else if( StringBeginsWith( MQTT_CMD_SW_LONG_TAP, MQTT_CMD_SW_LONG_TAP_LEN, payload, len ))
    {
        payload += MQTT_CMD_SW_LONG_TAP_LEN + 1;    // skip the separator
        len -= MQTT_CMD_SW_LONG_TAP_LEN + 1;
        OnGroupMaskCmd( payload, len,
            [ this ]( const char* a_pMask, uint16_t a_nCnt )
            {
                this->SetState( false, 0 );
            });
    }
}

void CManualSwitch::MqttPubStat()
{
    if( m_nMode == SW_MODE_ENABLED )
    {
        g_mqtt.PubStat( GetChanNo(), GetSwitchState());
    }
}

void CManualSwitch::MqttSendGroupCmd( const char* a_pszMqttCmd, uint16_t a_nArg )
{
    String strCmd( a_pszMqttCmd );
    strCmd += MQTT_CMD_SEPARATOR;
    strCmd += m_strMask;
    strCmd += MQTT_CMD_SEPARATOR;
    strCmd += a_nArg;
    g_mqtt.PubGroup( strCmd.c_str());
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

void CManualSwitch::GroupMaskClearBits( const char* a_pszClearMask )
{
    for( int nIdx = 2; nIdx < MQTT_CMD_MASK_LEN; nIdx++ )
    {
        byte nVal = NibbleToU8_16( m_strMask[ nIdx ]);
        byte nMask = NibbleToU8_16( a_pszClearMask[ nIdx ]);
        m_strMask[ nIdx ] = U8ToNibble_16( nVal & ( ~nMask ));
    }
}

void CManualSwitch::GroupMaskRestore()
{
    m_strMask = m_strMaskCopy;
}

bool CManualSwitch::GroupMaskMatch( byte* payload )
{
    uint8_t nId = m_nId - 1;
    uint8_t nNibble = nId >> 2;         // same as: nId / 4
    uint8_t nMask = 1 << ( nId & 3 );   // same as: 1 << ( nId % 4 )
    uint8_t nCmdMask = NibbleToU8_16( payload[ 2 + ( 15 - nNibble )]);  // 2: skip '0x', 15:reverse nibbles (LSB nibble at the last index)
    return ( nMask & nCmdMask ) == nMask;
}

void CManualSwitch::OnGroupMaskCmd( byte* payload, uint len, std::function< void( const char*, uint16_t )> a_fnAction )
{
    if( len > MQTT_CMD_MASK_LEN + 1 )
    {
        if( GroupMaskMatch( payload ))
        {
            const char* pMask = (const char*)payload;
            payload += MQTT_CMD_MASK_LEN + 1;
            len -= MQTT_CMD_MASK_LEN + 1;
            uint16_t nCnt = AtoU16_10( payload, len );
            a_fnAction( pMask, nCnt );
        }
    }
}
