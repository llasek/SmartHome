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



/// Channel short tap cmd - payload
#define MQTT_CMD_CH_SHORT_TAP       "taps"  // + <cnt>

/// Channel short tap cmd - payload len
#define MQTT_CMD_CH_SHORT_TAP_LEN   4



/// Channel long tap cmd - payload
#define MQTT_CMD_CH_LONG_TAP        "tapl"  // + <cnt>

/// Channel long tap cmd - payload len
#define MQTT_CMD_CH_LONG_TAP_LEN    4



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
     * @param[in]   a_nChannel  Channel: SW_CHANNEL_...
     * @param[in]   a_bStateOn  True if current state is on.
     * 
     * @return  True if status succesfully sent.
     */
    bool PubStat( char a_nChannel, bool a_bStateOn );

    /**
     * Publish a message over the device private channel.
     * 
     * @param[in]   a_pszMsg    Message to send.
     * 
     * @return  True if succesfully sent.
     */
    bool PubPriv( const char* a_pszMsg );

    /**
     * Publish a heartbeat over the device and channels state topics.
     * 
     * Heartbeat messages are periodically published states of the device and all channels.
     * Send a device availability message (MQTT_STAT_ONLINE) over the device state topic.
     * Send an on/off state for all channels over respective channel state topics.
     * 
     * @param[in]   a_bForceSend    Override the heartbeat period if true.
     */
    void PubHeartbeat( bool a_bForceSend );

    /**
     * Publish a message over a specified topic.
     * 
     * @param[in]   a_pszPubTopic   Topic to publish the message.
     * @param[in]   a_pszPayload    Message payload.
     * 
     * @return  True if succesfully sent.
     */
    bool PubMsg( const char* a_pszPubTopic, const char* a_pszPayload );



    /**
     * MQTT main loop function.
     * 
     * Test if the MQTT is enabled.
     * Try to connect to configured MQTT server if disconnected.
     * Upon connect subscribe to:
     * 1. Device command subscription topic,
     * 2. All channels command subsctiption topics, i.e. <device cmd sub topic> + "/ch#"",
     * 3. Device private pub/sub topic,
     * and send a heartbeat to notify device is online.
     * 
     * If connected, drive the heartbeat messages.
     * 
     * Note the function will take the configured WIFI client connection timeout time to exit in case the MQTT server is unavailable.
     */
    void loop();

    /**
     * MQTT cmd dispatcher callback.
     * 
     * Dispatch the command received:
     * 1. Device cmd sub topic: MQTT_CMD_RESET,
     * 2. Device private pub sub topic: channel beacon,
     * 3. Channel cmd sub topic: MQTT_CMD_CH_ON, MQTT_CMD_CH_OFF, MQTT_CMD_CH_SHORT_TAP (+tap count), MQTT_CMD_CH_LONG_TAP.
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
     * @param[in]   a_nChannel  Channel: SW_CHANNEL_...
     * @param[in]   a_pszMsg    Message to publish.
     * 
     * @return  True if succesfully sent.
     */
    bool PubStat( char a_nChannel, const char* a_pszMsg );

    /**
     * Publish a device status over the device status pub topic.
     * 
     * @param[in]   a_pszMsg    Message to publish.
     * 
     * @return  True if succesfully sent.
     */
    bool PubStat( const char* a_pszMsg );



    WiFiClient m_wc;        ///< WIFI client - controls connection timeout in the main loop function
    PubSubClient m_mqtt;    ///< The MQTT client    
    bool m_bEnabled;        ///< True if MQTT is enabled
    CTimer m_tmHeartbeat;   ///< Heartbeat timer
    byte* m_pPayloadBuf;    ///< Private buf for the received message in MQTT callback - see implementation for details
    uint m_nPayloadBufLen;  ///< Length of the private buf for the received message in MQTT callback



    // cfg:
    String m_strServer;             ///< Configured MQTT server IP or hostname
    String m_strClientId;           ///< Configured MQTT client id
    uint16_t m_nPort;               ///< Configured MQTT service port
    uint16_t m_nConnTimeout;        ///< Configured WIFI connection timeout
    uint16_t m_nHeartbeatIntvl;     ///< Configured heartbeat interval
    String m_strSubTopicCmd;        ///< Configured device cmd subscription topic
    String m_strPubTopicStat;       ///< Configured device status publish topic
    String m_strPubSubTopicPriv;    ///< Configured device private pub/sub topic
};
