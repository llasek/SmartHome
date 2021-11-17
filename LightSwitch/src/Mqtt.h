/**
 * DIY Smart Home - light switch
 * MQTT client
 * 2021 Łukasz Łasek
 */
#pragma once
#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <LittleFS.h>

#include "Timer.h"
#include "dbg.h"



// MQTT QOS
#define MQTT_QOS_AT_MOST_ONCE   0
#define MQTT_QOS_AT_LEAST_ONCE  1
#define MQTT_QOS_EXACTLY_ONCE   2



/// Config file path
#define FS_MQTT_CFG     "mqtt_cfg"



/// Device online state - payload
#define MQTT_STAT_ONLINE    "online"

/// Device offline state - payload
#define MQTT_STAT_OFFLINE   "offline"



/// Device reset private cmd - payload
#define MQTT_CMD_RESET      "reset"

/// Device reset private cmd - payload len
#define MQTT_CMD_RESET_LEN  5



/// Channel on state - payload
#define MQTT_CMD_CH_ON      "on"

/// Channel on state - payload len
#define MQTT_CMD_CH_ON_LEN  2



/// Channel off state - payload
#define MQTT_CMD_CH_OFF     "off"

/// Channel off state - payload len
#define MQTT_CMD_CH_OFF_LEN 3



/// Tap cmd separator character
#define MQTT_CMD_SEPARATOR          "/"



/// Tap cmd mask length (64bit hex)
#define MQTT_CMD_MASK_LEN           18



/// Phantom short tap cmd - payload
#define MQTT_CMD_PH_SHORT_TAP       "phs"  // + '/' + <mask> + '/' + <cnt>

/// Phantom short tap cmd - payload len
#define MQTT_CMD_PH_SHORT_TAP_LEN   3



/// Phantom long tap cmd - payload
#define MQTT_CMD_PH_LONG_TAP        "phl"  // + '/' + <mask> + '/' + <cnt>

/// Phantom long tap cmd - payload len
#define MQTT_CMD_PH_LONG_TAP_LEN    3



/// Switch long tap cmd - payload
#define MQTT_CMD_SW_LONG_TAP        "swl"   // + '/' + <mask> + '/' + <cnt>

/// Switch long tap cmd - payload len
#define MQTT_CMD_SW_LONG_TAP_LEN    3



/// Channel topic name added to the device topic names
#define MQTT_TOPIC_CHANNEL  "/ch"



/**
 * MQTT client class.
 * 
 * Send and receive MQTT messages over the registered topics.
 */
class CMqtt
{
public:
    CMqtt() : m_mqtt( m_wc ), m_bEnabled( false ), m_pPayloadBuf( nullptr ), m_nPayloadBufLen( 0 ) {}

    /**
     * Read a configuration file.
     */
    void ReadCfg();

    /**
     * Enable MQTT.
     * 
     * Set WIFI client timeout, configure the MQTT server and msg recv callback.
     */
    void Enable();

    /**
     * Publish device offline status only.
     * 
     * The MQTT connection remains opened to allow for cmds on a private topic e.g. a reset.
     */
    void Disable();



    /**
     * Publish a on/off state of a channel over the channel state topic.
     * 
     * The MQTT message is retained.
     * 
     * @param[in]   a_nChannel  Channel: SW_CHANNEL_...
     * @param[in]   a_bStateOn  True if current state is on.
     * 
     * @return  True if status succesfully sent.
     */
    bool PubStat( char a_nChannel, bool a_bStateOn );

    /**
     * Publish a message over the device group channel.
     * 
     * The MQTT message is NOT retained.
     * 
     * @param[in]   a_pszMsg    Message to send.
     * 
     * @return  True if succesfully sent.
     */
    bool PubGroup( const char* a_pszMsg );

    /**
     * Publish (once) the initial device and channels state.
     * 
     * Send a device availability message (MQTT_STAT_ONLINE) over the device state topic.
     * Send an on/off state for all channels over respective channel state topics.
     */
    void PubInitState();



    /**
     * MQTT main loop function.
     * 
     * Test if the MQTT is enabled.
     * If connected to MQTT server, publish the initial state and run MQTT main loop.
     * If disconnected try to connect to the configured MQTT server.
     * Upon connect subscribe to:
     * 1. Device command subscription topic,
     * 2. All channels command subsctiption topics, i.e. <device cmd sub topic> + "/ch#"",
     * 3. Device private pub/sub topic,
     * 
     * Note the function will take the configured WIFI client connection timeout time to exit in case the MQTT server is unavailable.
     */
    void loop();

    /**
     * MQTT cmd dispatcher callback.
     * 
     * Dispatch the command received:
     * 1. Device cmd sub topic: MQTT_CMD_RESET,
     * 2. Device group pub sub topic: group cmds: MQTT_CMD_PH_SHORT_TAP, MQTT_CMD_PH_LONG_TAP, MQTT_CMD_SW_LONG_TAP
     * 3. Channel cmd sub topic: MQTT_CMD_CH_ON, MQTT_CMD_CH_OFF.
     * 
     * @param[in]   topic       MQTT topic of the incoming cmd.
     * @param[in]   payload     Cmd payload.
     * @param[in]   len         Length of the payload.
     */
    void MqttCb( char* topic, byte* payload, uint len );



protected:
    /**
     * Construct the channel pub/sub topic name from the corresponding device topic name.
     * 
     * The channel topic name is the <device topic name> + "/ch#"".
     * 
     * @param[in]   a_nChannel      Channel: SW_CHANNEL_...
     * @param[in]   a_rstrTopic     Device topic name.
     * 
     * @return  Channel topic name.
     */
    String GetChannelTopic( char a_nChannel, String& a_rstrTopic );



    /**
     * Publish a channel status over the channel status pub topic.
     * 
     * The MQTT message is retained.
     * 
     * @param[in]   a_nChannel  Channel: SW_CHANNEL_...
     * @param[in]   a_pszMsg    Message to publish.
     * 
     * @return  True if succesfully sent.
     */
    bool PubStat( char a_nChannel, const char* a_pszMsg );

    /**
     * Publish a device status over the device status pub topic.
     * 
     * The MQTT message is retained.
     * 
     * @param[in]   a_pszMsg    Message to publish.
     * 
     * @return  True if succesfully sent.
     */
    bool PubStat( const char* a_pszMsg );



    WiFiClient m_wc;        ///< WIFI client - controls connection timeout in the main loop function
    PubSubClient m_mqtt;    ///< The MQTT client    
    bool m_bEnabled;        ///< True if MQTT is enabled
    bool m_bInitStatSent;   ///< True if the initial state is sent
    CTimer m_tmInitStat;    ///< Initial state send timer
    byte* m_pPayloadBuf;    ///< Private buf for the received message in MQTT callback - see implementation for details
    uint m_nPayloadBufLen;  ///< Length of the private buf for the received message in MQTT callback



    // cfg:
    String m_strServer;             ///< Configured MQTT server IP or hostname
    String m_strClientId;           ///< Configured MQTT client id
    uint16_t m_nPort;               ///< Configured MQTT service port
    uint16_t m_nConnTimeout;        ///< Configured WIFI connection timeout
    ulong m_nInitStatDelayMs;       ///< Configured initial state pub delay (ms)
    String m_strSubTopicCmd;        ///< Configured device cmd subscription topic
    String m_strPubTopicStat;       ///< Configured device status publish topic
    String m_strPubSubTopicGrp;     ///< Configured device group pub/sub topic
};
