/**
 * DIY Smart Home - light switch
 * Manual switch
 * 2021 Łukasz Łasek
 */
#include "ManualSwitch.h"
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

extern CMqtt g_mqtt;

// Tap operations:
#define SW_TAP_OP_TOGGLE            0   // Toggle output on/off. Arg: none
#define SW_TAP_OP_TOGGLE_MASK_OFF   1   // Toggle output on/off and send MQTT_CMD_GRP_TURN_OFF. Arg: mask
#define SW_TAP_OP_AUTO_OFF          2   // Turn on the output, setup auto-off timer for (TapCnt * <arg>) secs. Arg: timer step duratin in seconds
#define SW_TAP_OP_FORWARD           3   // Forward the tap event via MQTT_CMD_GRP_FWD_SHORT_TAP/MQTT_CMD_GRP_FWD_LONG_TAP. Arg: mask
#define SW_TAP_OP_DISABLE           4   // Fake op, no-op. Arg: none
#define SW_TAP_OPS                  4   // # of real ops, i.e. not including DISABLE
static_assert( SW_TAP_OP_DISABLE == SW_TAP_OPS, "Review all SW tap modes" );

// Cfg tap op name -> tap op (index) mapping:
static const char* Sg_arrCfgTapOps[ SW_TAP_OPS ] PROGMEM = { "tgle", "tgof", "aoff", "fwte" };

// Cfg tap event name -> tap event (index) mapping:
static const char* Sg_arrCfgTapEvents[ SW_TAP_EVENTS ] PROGMEM = { "ev-ss", "ev-sm", "ev-ls" };

// Cft tap event arg name -> tap event (index) mapping:
static const char* Sg_arrCfgTapEventArgs[ SW_TAP_EVENTS ] PROGMEM = { "arg-ss", "arg-sm", "arg-ls" };

void CManualSwitch::ReadCfg( uint8_t a_nChanNo )
{
    m_pszClearMask = NULL;
    m_nLongTapMs = m_nNextTapMs = 0;
    m_nChanNo = a_nChanNo;

    const char* arrCfgFile[ SW_CHANNELS ] = { FS_CH0_CFG, FS_CH1_CFG, FS_CH2_CFG };
    File file = LittleFS.open( arrCfgFile[ a_nChanNo ], "r" );
    if( file )
    {
        m_nId = CfgFileReadLine( file, "id" ).toInt();
        if( m_nId > SW_MAX_ID )
        {
            m_nId = 0;
        }

        m_nLongTapMs = CfgFileReadLine( file, "long" ).toInt();
        m_nNextTapMs = CfgFileReadLine( file, "next" ).toInt();

        bool bEnabled = ReadCfgTapEvent( file, SW_TAP_EVENT_SHORT_SINGLE );
        bEnabled |= ReadCfgTapEvent( file, SW_TAP_EVENT_SHORT_MULTI );
        bEnabled |= ReadCfgTapEvent( file, SW_TAP_EVENT_LONG_SINGLE );

        if( !bEnabled )
        {
            m_nChanNo += SW_CHANNELS;   // disable the channel
        }

        DBGLOG4( "sw ch%d cfg: id:%d long-ms:%u next-ms:%u ",
            m_nChanNo, m_nId, m_nLongTapMs, m_nNextTapMs );
        DBGLOG6( "ev: ss-op:%u -arg:'%s' sm-op:%u -arg:'%s' ls-op:%u -arg:'%s'\n",
            m_arrTapOps[ SW_TAP_EVENT_SHORT_SINGLE ], m_arrTapArgs[ SW_TAP_EVENT_SHORT_SINGLE ].c_str(),
            m_arrTapOps[ SW_TAP_EVENT_SHORT_MULTI ], m_arrTapArgs[ SW_TAP_EVENT_SHORT_MULTI ].c_str(),
            m_arrTapOps[ SW_TAP_EVENT_LONG_SINGLE ], m_arrTapArgs[ SW_TAP_EVENT_LONG_SINGLE ].c_str());
    }
    else
    {
        m_nChanNo += SW_CHANNELS;   // disable the channel
        DBGLOG1( "sw ch%d cfg missing - disable\n", m_nChanNo );
    }
}

bool CManualSwitch::IsDisabled()
{
    return m_nChanNo >= SW_CHANNELS;
}

bool CManualSwitch::ReadCfgTapEvent( File& a_rFile, uint8_t a_nTapEvent )
{
    m_arrTapArgs[ a_nTapEvent ] = CfgFileReadLine( a_rFile, Sg_arrCfgTapEvents[ a_nTapEvent ], Sg_arrCfgTapOps[ SW_TAP_OP_TOGGLE ]);
    for( uint16_t nOp = 0; nOp < SW_TAP_OPS; nOp++ )
    {
        if( m_arrTapArgs[ a_nTapEvent ] == Sg_arrCfgTapOps[ nOp ])
        {
            m_arrTapOps[ a_nTapEvent ] = nOp;
            switch( nOp )
            {
                case SW_TAP_OP_TOGGLE_MASK_OFF:
                case SW_TAP_OP_FORWARD:
                    m_arrTapArgs[ a_nTapEvent ] = CfgFileReadLine( a_rFile, Sg_arrCfgTapEventArgs[ a_nTapEvent ]);
                    if( m_arrTapArgs[ a_nTapEvent ].length() == MQTT_CMD_MASK_LEN )
                    {
                        // Unmask itself
                        if( m_nId )
                        {
                            uint8_t nId = m_nId - 1;
                            uint8_t nNibble = nId >> 2;         // same as: nId / 4
                            uint8_t nMask = 1 << ( nId & 3 );   // same as: 1 << ( nId % 4 )
                            byte nVal = NibbleToU8_16( m_arrTapArgs[ a_nTapEvent ][ 2 + ( 15 - nNibble )]);
                            nVal &= ~nMask;
                            m_arrTapArgs[ a_nTapEvent ][ 2 + ( 15 - nNibble )] = U8ToNibble_16( nVal );
                        }
                    }
                    else
                    {
                        continue;   // will set DISABLE
                    }
                    break;

                case SW_TAP_OP_AUTO_OFF:
                    m_arrTapArgs[ a_nTapEvent ] = CfgFileReadLine( a_rFile, Sg_arrCfgTapEventArgs[ a_nTapEvent ], "60" );
                    break;

                default:
                    m_arrTapArgs[ a_nTapEvent ].clear();
                    break;
            }
            return true;
        }
    }

    m_arrTapOps[ a_nTapEvent ] = SW_TAP_OP_DISABLE;
    m_arrTapArgs[ a_nTapEvent ].clear();
    return false;
}

void CManualSwitch::Enable()
{
    if( IsDisabled())
    {
        return;
    }

    m_tmAutoOff.UpdateAll();
    pinMode( Sm_arrPinOut[ m_nChanNo ], OUTPUT );
    DriveSwitch( false );
    CTouchBtn::Enable( Sm_arrPinIn[ m_nChanNo ], m_nLongTapMs, m_nNextTapMs );
}

void CManualSwitch::Disable()
{
    m_nChanNo += SW_CHANNELS;
    CTouchBtn::Disable();
}

void CManualSwitch::DriveSwitch( bool a_bStateOn )
{
    m_nPinSwitchVal = ( a_bStateOn ) ? HIGH : LOW;
    digitalWrite( Sm_arrPinOut[ m_nChanNo ], m_nPinSwitchVal );
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

void CManualSwitch::OnTap( uint8_t a_nTapEvent, uint16_t a_nTapCnt )
{
    switch( m_arrTapOps[ a_nTapEvent ])
    {
        case SW_TAP_OP_TOGGLE:
            SetState( !GetSwitchState(), 0 );
            break;

        case SW_TAP_OP_TOGGLE_MASK_OFF:
            SetState( !GetSwitchState(), 0 );
            MqttSendGroupCmd( MQTT_CMD_GRP_TURN_OFF, 1, m_arrTapArgs[ a_nTapEvent ].c_str());
            break;

        case SW_TAP_OP_AUTO_OFF:
            SetState( true, ((ulong)max( 1, a_nTapCnt - 1 )) * m_arrTapArgs[ a_nTapEvent ].toInt() * 1000 );
            break;

        case SW_TAP_OP_FORWARD:
            MqttSendGroupCmd(( a_nTapEvent == SW_TAP_EVENT_LONG_SINGLE ) ?  MQTT_CMD_GRP_FWD_LONG_TAP : MQTT_CMD_GRP_FWD_SHORT_TAP, a_nTapCnt, m_arrTapArgs[ a_nTapEvent ].c_str());
            break;

        default:
            break;
    }
}

void CManualSwitch::OnShortTap( uint16_t a_nCnt )
{
    CTouchBtn::OnShortTap( a_nCnt );
    DBGLOG2( "short tap x%d pin#%d\n", a_nCnt, m_nPin );
    OnTap(( a_nCnt == 1 ) ? SW_TAP_EVENT_SHORT_SINGLE : SW_TAP_EVENT_SHORT_MULTI, a_nCnt );
}

void CManualSwitch::OnLongTap()
{
    CTouchBtn::OnLongTap();
    DBGLOG1( "long tap pin#%d\n", m_nPin );
    OnTap( SW_TAP_EVENT_LONG_SINGLE, 1 );
}

void CManualSwitch::SetState( bool a_bStateOn, ulong a_nAutoOff )
{
    if( IsDisabled())
    {
        return;
    }

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
    if( StringBeginsWith( MQTT_CMD_GRP_FWD_LONG_TAP, MQTT_CMD_GRP_FWD_LONG_TAP_LEN, payload, len ))
    {
        payload += MQTT_CMD_GRP_FWD_LONG_TAP_LEN + 1;    // skip the separator
        len -= MQTT_CMD_GRP_FWD_LONG_TAP_LEN + 1;
        OnGroupMaskCmd( payload, len,
            [ this ]( const char* a_pMask, uint16_t a_nCnt )
            {
                /**
                 * The arriving group cmd may target multiple switches, which may mask each other, e.g.
                 * a group cmd arrives with mask 0x03, i.e. targeting switch id 1 and 2. In response:
                 * 1. SW1 id=1 mask:0x000e - switch id 1 will mask switches id 2..15.
                 * 2. SW2 id=2 mask:0x000d - switch id 2 will mask switches id 1, 3..15.
                 * In this scenario SW1 and SW2 will mask each other after executing the group cmd.
                 * To prevent this, the bits correspoinging to the arriving group cmd's mask bits will be cleared
                 * in the current group mask of a corresponding tap event of the switch channel responding
                 * to the group cmd. This will exclude both SW1 and SW2 from their mask.
                 * All other masked switches will receive the group command from SW1 and SW2.
                 */
                this->m_pszClearMask = a_pMask;
                this->OnLongTap();
                this->m_pszClearMask = NULL;
            });
    }
    else if( StringBeginsWith( MQTT_CMD_GRP_FWD_SHORT_TAP, MQTT_CMD_GRP_FWD_SHORT_TAP_LEN, payload, len ))
    {
        payload += MQTT_CMD_GRP_FWD_SHORT_TAP_LEN + 1;   // skip the separator
        len -= MQTT_CMD_GRP_FWD_SHORT_TAP_LEN + 1;
        OnGroupMaskCmd( payload, len,
            [ this ]( const char* a_pMask, uint16_t a_nCnt )
            {
                this->m_pszClearMask = a_pMask;
                this->OnShortTap( a_nCnt );
                this->m_pszClearMask = NULL;
            });
    }
    else if( StringBeginsWith( MQTT_CMD_GRP_TURN_OFF, MQTT_CMD_GRP_TURN_OFF_LEN, payload, len ))
    {
        payload += MQTT_CMD_GRP_TURN_OFF_LEN + 1;   // skip the separator
        len -= MQTT_CMD_GRP_TURN_OFF_LEN + 1;
        OnGroupMaskCmd( payload, len,
            [ this ]( const char* a_pMask, uint16_t a_nCnt )
            {
                this->SetState( false, 0 );
            });
    }
}

void CManualSwitch::MqttPubStat()
{
    if( IsDisabled())
    {
        return;
    }

    g_mqtt.PubStat( GetChanNo(), GetSwitchState());
}

void CManualSwitch::MqttSendGroupCmd( const char* a_pszMqttCmd, uint16_t a_nArg, const char* a_pszMask )
{
    String strMask( a_pszMask );
    String strCmd( a_pszMqttCmd );
    strCmd += MQTT_CMD_SEPARATOR;
    GroupMaskClearBits( strMask );
    strCmd += strMask;
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

void CManualSwitch::GroupMaskClearBits( String& a_rstrMask )
{
    if( !m_pszClearMask )
    {
        return;
    }

    for( int nIdx = 2; nIdx < MQTT_CMD_MASK_LEN; nIdx++ )
    {
        byte nVal = NibbleToU8_16( a_rstrMask[ nIdx ]);
        byte nMask = NibbleToU8_16( m_pszClearMask[ nIdx ]);
        a_rstrMask[ nIdx ] = U8ToNibble_16( nVal & ( ~nMask ));
    }
}

bool CManualSwitch::GroupMaskMatch( byte* payload )
{
    if( !m_nId )
    {
        return false;
    }

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
