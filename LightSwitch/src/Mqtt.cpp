/**
 * DIY Smart Home - light switch
 * MQTT client
 * 2021 Łukasz Łasek
 */
#include "Mqtt.h"
#include "ManualSwitch.h"
#include "Utils.h"

extern CManualSwitch g_swChan0;
extern CManualSwitch g_swChan1;
extern CManualSwitch g_swChan2;

void CMqtt::ReadCfg()
{
    File file = LittleFS.open( FS_MQTT_CFG, "r" );
    if( file )
    {
        m_strServer = CfgFileReadLine( file );
        m_nPort = CfgFileReadLine( file ).toInt();
        m_nConnTimeout = CfgFileReadLine( file ).toInt();
        m_nHeartbeatIntvl = CfgFileReadLine( file ).toInt();
        m_strClientId = CfgFileReadLine( file );
        m_strSubTopicCmd = CfgFileReadLine( file );
        m_strPubTopicStat = CfgFileReadLine( file );
        m_strPubSubTopicPriv = CfgFileReadLine( file );

        DBGLOG5( "mqtt cfg: server:'%s' port:%u timeo:%u heartbeat:%u client-id:'%s' ",
            m_strServer.c_str(), m_nPort, m_nConnTimeout, m_nHeartbeatIntvl, m_strClientId.c_str());
        DBGLOG3( "cmd-sub:'%s' stat-pub:'%s', prv:'%s'\n",
            m_strSubTopicCmd.c_str(), m_strPubTopicStat.c_str(), m_strPubSubTopicPriv.c_str());
    }
    else
    {
        DBGLOG( "mqtt cfg missing" );
    }
}

void CMqtt::Enable()
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

void CMqtt::Disable()
{
    // Fake disable - report 'offline' only but don't actually disconnect mqtt to allow for a reset cmd in case of emergency
    // m_bEnabled = false;
    PubStat( MQTT_STAT_OFFLINE );
    // m_mqtt.disconnect();
}

bool CMqtt::PubStat( char a_nChannel, bool a_bStateOn )
{
    return PubStat( a_nChannel, ( a_bStateOn ) ? MQTT_CMD_CH_ON : MQTT_CMD_CH_OFF );
}

bool CMqtt::PubPriv( const char* a_pszBeacon )
{
    return m_mqtt.publish( m_strPubSubTopicPriv.c_str(), a_pszBeacon );
}

void CMqtt::PubHeartbeat( bool a_bForceSend )
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

bool CMqtt::PubCmd( const char* a_pszPubTopic, const char* a_pszPayload )
{
    return m_mqtt.publish( a_pszPubTopic, a_pszPayload );
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
        m_mqtt.subscribe( m_strPubSubTopicPriv.c_str());
        DBGLOG( "mqtt connected" );
        PubHeartbeat( true );
    }
    PubHeartbeat( false );
    m_mqtt.loop();
}

void CMqtt::MqttCb( char* topic, byte* payload, uint len )
{
    // Copy the buf for processing in all channels.
    // The PubSubClient implementation does use a single buffer for rx/tx.
    // In case multiple channels are executing (i.e. on a device private topic),
    // the mqtt status sent by one channel (tx) will overwrite the payload before the next channel executes (rx).
    byte* pBuf = m_pPayloadBuf;
    if( m_nPayloadBufLen < len )
    {
        byte* pNewBuf = (byte*)realloc( m_pPayloadBuf, len );
        if( pNewBuf )
        {
            pBuf = m_pPayloadBuf = pNewBuf;
            m_nPayloadBufLen = len;
        }
        else
        {
            pBuf = payload;
        }
    }
    if( pBuf != payload )
    {
        memcpy( pBuf, payload, len );
    }

    DBGLOG1( "Rcvd topic %s: '", topic );
    for( uint i = 0; i < len; i++ )
    {
        DBGLOG1( "%c", (char)pBuf[ i ]);
    }
    DBGLOG( '\'' );

    if( m_strSubTopicCmd == topic )
    {
        if( StringEq( MQTT_CMD_RESET, MQTT_CMD_RESET_LEN, pBuf, len ))
        {
            DBGLOG( "reset" );
            Disable();
            ESP.reset();
        }
        return;
    }
    else if( m_strPubSubTopicPriv == topic )
    {
        g_swChan0.OnMqttBeacon( pBuf, len );
        g_swChan1.OnMqttBeacon( pBuf, len );
        g_swChan2.OnMqttBeacon( pBuf, len );
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
        if(( StringEq( MQTT_CMD_CH_ON, MQTT_CMD_CH_ON_LEN, pBuf, len ))
            || ( StringEq( MQTT_CMD_CH_OFF, MQTT_CMD_CH_OFF_LEN, pBuf, len )))
        {
            pms->OnShortTap( 1 );
            return;
        }

        if(( len > MQTT_CMD_CH_SHORT_TAP_LEN ) && ( !memcmp( MQTT_CMD_CH_SHORT_TAP, pBuf, MQTT_CMD_CH_SHORT_TAP_LEN )))
        {
            uint16_t nCnt = 0;
            for( uint nIdx = MQTT_CMD_CH_SHORT_TAP_LEN; nIdx < len; nIdx++ )
            {
                byte b = pBuf[ nIdx ];
                if(( b >= '0' ) && ( b <= '9' ))
                {
                    nCnt *= 10;
                    nCnt += ( b - '0' );
                }
                else
                {
                    nCnt = 1;
                    break;
                }
            }
            pms->OnShortTap( nCnt );
            return;
        }

        if(( len > MQTT_CMD_CH_LONG_TAP_LEN ) && ( !memcmp( MQTT_CMD_CH_LONG_TAP, pBuf, MQTT_CMD_CH_LONG_TAP_LEN )))
        {
            pms->OnLongTap();
            return;
        }
    }
}

String CMqtt::GetChannelTopic( char a_nChannel, String& a_rstrTopic )
{
    String strTopic( a_rstrTopic );
    if( a_nChannel )
    {
        strTopic += MQTT_TOPIC_CHANNEL;
        strTopic += a_nChannel;
    }
    return strTopic;
}

bool CMqtt::PubStat( char a_nChannel, const char* a_pszMsg )
{
    String strTopic = GetChannelTopic( a_nChannel, m_strPubTopicStat );
    DBGLOG2( "mqtt pub t:'%s' p:'%s'\n", strTopic.c_str(), a_pszMsg );
    return m_mqtt.publish( strTopic.c_str(), a_pszMsg );
}

bool CMqtt::PubStat( const char* a_pszMsg )
{
    return PubStat( 0, a_pszMsg );
}
