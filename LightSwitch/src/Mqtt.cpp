/**
 * DIY Smart Home - light switch
 * MQTT client
 * 2021-2022 Łukasz Łasek
 */
#include "Mqtt.h"
#include "ManualSwitch.h"
#include "WiFiHelper.h"
#include "CfgUtils.h"
#include "StringUtils.h"
#include "FwRev.h"

extern CWiFiHelper g_wifi;

extern CManualSwitch g_swChan0;
extern CManualSwitch g_swChan1;
extern CManualSwitch g_swChan2;

void CMqtt::ReadCfg()
{
    File file = LittleFS.open( FS_MQTT_CFG, "r" );
    if( file )
    {
        m_strServer = CConfigUtils::ReadValue( file, "srv" );
        m_nPort = CConfigUtils::ReadValue( file, "port" ).toInt();
        m_nConnTimeout = CConfigUtils::ReadValue( file, "conn" ).toInt();
        m_nInitStatDelayMs = CConfigUtils::ReadValue( file, "init" ).toInt();
        m_strClientId = CConfigUtils::ReadValue( file, "cli" );
        m_strSubTopicCmd = CConfigUtils::ReadValue( file, "sub" );
        m_strPubTopicStat = CConfigUtils::ReadValue( file, "pub" );
        m_strPubSubTopicGrp = CConfigUtils::ReadValue( file, "grp" );
        m_strPubTopicMgt = m_strSubTopicMgt = CConfigUtils::ReadValue( file, "mgt" );
        m_strPubTopicMgt += "/stat";
        m_strSubTopicMgt += "/cmd";

        DBGLOG4( "mqtt cfg: server:'%s' port:%u timeo:%u client-id:'%s' ",
            m_strServer.c_str(), m_nPort, m_nConnTimeout, m_strClientId.c_str());
        DBGLOG5( "init-delay:%lu cmd-sub:'%s' stat-pub:'%s' grp:'%s' mgt:'%s|stat'\n",
            m_nInitStatDelayMs, m_strSubTopicCmd.c_str(), m_strPubTopicStat.c_str(), m_strPubSubTopicGrp.c_str(), m_strSubTopicMgt.c_str());
    }
    else
    {
        DBGLOG( "mqtt cfg missing" );
    }
}

void CMqtt::Enable()
{
    if(( m_bEnabled )
        || ( m_strServer.isEmpty())
        || ( m_strClientId.isEmpty()))
        return;

    m_bEnabled = true;
    m_bInitStatSent = false;
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
    m_bEnabled = false;
    m_mqtt.disconnect();
}

bool CMqtt::PubStat( char a_nChannel, bool a_bStateOn )
{
    return PubStat( a_nChannel, ( a_bStateOn ) ? MQTT_CMD_CH_ON : MQTT_CMD_CH_OFF );
}

bool CMqtt::PubMgt( const char* a_pszMsg )
{
    return m_mqtt.publish( m_strPubTopicMgt.c_str(), a_pszMsg );
}

bool CMqtt::PubGroup( const char* a_pszMsg )
{
    return m_mqtt.publish( m_strPubSubTopicGrp.c_str(), a_pszMsg );
}

void CMqtt::PubInitState()
{
    if( m_bInitStatSent )
        return;

    /*
     * The delay here is a workaround for some MQTT brokers.
     * Upon a fast client disconnect/reconnect, the broker will realize the client
     * has disconnected upon reconnect and will send both retained LWT (offline)
     * and init state (online) at the same time.
     * In such case the receiver may see the messages in the reverse order.
     * 
     * The delay allows for the LWT message to be sent and received before
     * the init state message is sent.
     */
    m_tmInitStat.UpdateCur();
    if( m_tmInitStat.Delta() < m_nInitStatDelayMs )
        return;

    PubStat( MQTT_STAT_ONLINE );
    g_swChan0.MqttPubStat();
    g_swChan1.MqttPubStat();
    g_swChan2.MqttPubStat();

    OnMgtCmd((byte*)MQTT_CMD_MGT_DISCOVERY, MQTT_CMD_MGT_DISCOVERY_LEN );
    m_bInitStatSent = true;
}

void CMqtt::OnMgtCmd( byte* payload, uint len )
{
    String strHostName = g_wifi.GetHostName();
    if( CStringUtils::IsEqual( MQTT_CMD_MGT_DISCOVERY, MQTT_CMD_MGT_DISCOVERY_LEN, payload, len ))
    {
        String strResp( FW_REV_CURRENT );
        strResp += " ";
        strResp += strHostName;
        strResp += " ";
        strResp += g_wifi.GetIp();
        strResp += " ";
        strResp += g_wifi.GetMac();
        PubMgt( strResp.c_str());
    }
    else if( CStringUtils::BeginsWith( MQTT_CMD_MGT_RESET, MQTT_CMD_MGT_RESET_LEN, payload, len ))
    {
        payload += MQTT_CMD_MGT_RESET_LEN + 1;  // skip the separator
        len -= MQTT_CMD_MGT_RESET_LEN + 1;
        if( CStringUtils::IsEqual( strHostName, payload, len ))
        {
            ESP.reset();
        }
    }
}

void CMqtt::loop()
{
    if( !m_bEnabled )
        return;

    if( m_mqtt.connected())
    {
        PubInitState();
        m_mqtt.loop();
    }
    else if( m_mqtt.connect( m_strClientId.c_str(), m_strPubTopicStat.c_str(), MQTT_QOS_EXACTLY_ONCE, true, MQTT_STAT_OFFLINE ))
    {
        m_mqtt.subscribe( m_strSubTopicCmd.c_str());
        String strMqttSubTopicChan( m_strSubTopicCmd + MQTT_TOPIC_CHANNEL );
        m_mqtt.subscribe(( strMqttSubTopicChan + SW_CHANNEL_0 ).c_str());
        m_mqtt.subscribe(( strMqttSubTopicChan + SW_CHANNEL_1 ).c_str());
        m_mqtt.subscribe(( strMqttSubTopicChan + SW_CHANNEL_2 ).c_str());
        m_mqtt.subscribe( m_strPubSubTopicGrp.c_str());
        m_mqtt.subscribe( m_strSubTopicMgt.c_str());
        DBGLOG( "mqtt connected" );
        m_tmInitStat.UpdateAll();
        m_bInitStatSent = false;
    }
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

    if( m_strPubSubTopicGrp == topic )
    {
        g_swChan0.OnGroupCmd( pBuf, len );
        g_swChan1.OnGroupCmd( pBuf, len );
        g_swChan2.OnGroupCmd( pBuf, len );
        return;
    }
    else if( m_strSubTopicMgt == topic )
    {
        OnMgtCmd( pBuf, len );
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
        if( CStringUtils::IsEqual( MQTT_CMD_CH_ON, MQTT_CMD_CH_ON_LEN, pBuf, len ))
        {
            pms->SetState( true, 0 );
        }
        else if( CStringUtils::IsEqual( MQTT_CMD_CH_OFF, MQTT_CMD_CH_OFF_LEN, pBuf, len ))
        {
            pms->SetState( false, 0 );
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
    return m_mqtt.publish( strTopic.c_str(), a_pszMsg, true );
}

bool CMqtt::PubStat( const char* a_pszMsg )
{
    return PubStat( 0, a_pszMsg );
}
