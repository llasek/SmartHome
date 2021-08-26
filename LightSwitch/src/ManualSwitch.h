/**
 * DIY Smart Home - light switch
 * Manual switch
 * 2021 Łukasz Łasek
 */
#pragma once
#include <Arduino.h>
#include <LittleFS.h>

#include "TouchBtn.h"
#include "Mqtt.h"
#include "Utils.h"
#include "dbg.h"



/// Config file path
#define FS_SW_CFG       "sw_cfg"



/// Switch is in a 'disabled' mode
#define SW_MODE_DISABLED    0

/// Switch is in an 'enabled' mode
#define SW_MODE_ENABLED     1

/// Switch is in a 'phantom' mode
#define SW_MODE_PHANTOM     2



/// Switch max id
#define SW_MAX_ID       64



/// Switch channel 0 in the mqtt channel name
#define SW_CHANNEL_0    '0'

/// Switch channel 1 in the mqtt channel name
#define SW_CHANNEL_1    '1'

/// Switch channel 2 in the mqtt channel name
#define SW_CHANNEL_2    '2'

/// Invalid switch channel
#define SW_CHANNEL_NA   0

/// Number of switch channels
#define SW_CHANNELS     3



/**
 * Manual light switch class
 * 
 * Manual light switch is a driver for an AC switch, controlled by a touch button.
 * There are 3 (SW_CHANNELS) switches available, distinguished by a channel number: 0/1/2.
 * 
 * One MCU pin is configured as a switch driver and outputs:
 * 1. LO when the switch is turned off,
 * 2. HI when the switch is turned on.
 * 
 * Another MCU pin is configured as a touch button input.
 * 
 * Manual light switch features the following modes:
 * 1. Enabled - a button tap cycles the on/off state of the switch driver output.
 * 2. Disabled - both button input pin and a switch output are disabled.
 * 3. Phantom - a button tap sends an MQTT group command; no AC switch is connected.
 * 
 * Recognized tap patterns:
 * 1. Single short tap - cycle the output on/off.
 * 2. Multi tap (N-times) - turn the output on for (N-1) minutes.
 * 3. Long tap - cycle the output on/off and simultaneously turn off (mask) other switches
 *    in the group - via a group command.
 * 
 * In the enabled mode each channel is assigned a unique id, corresponding to the bit# in the mask
 * (LSB first)). The id is in range 1-64.
 * Each channel is also assigned a 64-bit mask. The mask format is:
 *      0x0123456789abcdef
 * i.e. MQTT_CMD_MASK_LEN number of characters. The mask allows to identify all masked switches
 * upon the long tap event - i.e. the mask is then being sent in an MQTT group command.
 * The group command is received by all devices on the network. The mask is then tested against
 * all channel ids and only masked channels are turning off.
 * In the enabled mode the switch publishes its state upon change via its MQTT pub topic.
 * The states serviced by MQTT pub/sub topics are: on/off.
 * 
 * In phantom mode only an MQTT group command is sent instead of driving the actual switch output.
 * Group commands are received by all devices on the network. The group command carries a mask,
 * which is then checked against all channel ids by all switches in the enabled state. Only the masked
 * channels are executing the received MQTT group command.
 * This allows for one phantom switch to control multiple real switches.
 * 
 * The group command format is described in the Mqtt.h file.
 */
class CManualSwitch : public CTouchBtn
{
public:
    /**
     * Open the configuration file.
     * 
     * The configuration file is common for all switches.
     * 
     * @return  Opened file
     */
    static File OpenCfg();

    /**
     * Read a single switch configuration
     * 
     * @param[in]   a_rFile     Configuration file
     */
    void ReadCfg( File& a_rFile );



    /**
     * Enable the manual switch
     * 
     * Set up the button and a switch driver according to the configured mode
     * 
     * @param[in]   a_nChanNo   channel number
     */
    void Enable( uint8_t a_nChanNo );

    /**
     * Disable the manual switch
     * 
     * Disable the button and a switch driver.
     */
    void Disable();



    /**
     * Drive the switch output
     * 
     * @param[in]   a_bStateOn  true if HI, false if LO
     */
    void DriveSwitch( bool a_bStateOn );

    /**
     * Return the switch state
     * 
     * @return  true if HI, false if LO
     */
    bool GetSwitchState();



    /**
     * Main loop function.
     * 
     * Execute the main loop of the base class.
     * Handle the automatic switch turn off triggered by a long button tap.
     */
    void loop();



    /**
     * Handle the short button tap event.
     * 
     * In the enabled mode:
     * 1. Single short tap - cycle the output on/off and disable the auto-off timer.
     * 2. Multi tap (N-times) - turn the output on and confifure the auto-off timer for (N-1) minutes.
     * 3. Publish the switch on/off state via MQTT pub topic.
     * 
     * In the phantom mode only a MQTT_CMD_PH_SHORT_TAP group command is sent.
     * 
     * @param[in]   a_nCnt  Number of subsequent short taps.
     */
    virtual void OnShortTap( uint16_t a_nCnt );

    /**
     * Handle the long button tap event.
     * 
     * In the enabled mode:
     * 1. cycle the output on/off.
     * 2. Disable the auto-off timer.
     * 3. Publish the switch on/off state via MQTT pub topic.
     * 3. Sent a MQTT_CMD_SW_LONG_TAP group command to turn off all masked switches in the group.
     * 
     * In the phantom mode only a MQTT_CMD_PH_LONG_TAP group command is sent.
     */
    virtual void OnLongTap();



    /**
     * Handle the received MQTT group command.
     * 
     * Decode and execute the group command.
     * The following group commands are handled:
     * 1. MQTT_CMD_PH_SHORT_TAP
     * 2. MQTT_CMD_SW_LONG_TAP
     * 3. MQTT_CMD_SW_LONG_TAP
     * 
     * @param[in]   payload     MQTT message paylaod
     * @param[in]   len         Length of the payload
     */
    void OnGroupCmd( byte* payload, uint len );

    /**
     * Publish the switch on/off state via MQTT pub topic.
     */
    void MqttPubStat();

    /**
     * Send an MQTT group command with the current mask and arg (tap cnt).
     * 
     * @param[in]   a_pszMqttCmd    Base command to be sent
     * @param[in]   a_nArg          Numeric argument to be added at the end of a command (tap cnt)
     */
    void MqttSendGroupCmd( const char* a_pszMqttCmd, uint16_t a_nArg );



    /**
     * Return the configured channel name.
     * 
     * @return  SW_CHANNEL_0, SW_CHANNEL_1, SW_CHANNEL_2 or SW_CHANNEL_NA (invalid).
     */
    char GetChanNo();



protected:
    /**
     * Clear bits in the current group mask.
     * 
     * Note the current group mask is modified.
     * 
     * @param[in]   a_pszClearMask  Bits to be cleared - in a mask format
     */
    void GroupMaskClearBits( const char* a_pszClearMask );

    /**
     * Restore the original group mask.
     */
    void GroupMaskRestore();

    /**
     * Check the mask match against the channel id.
     * 
     * @param[in]   payload     Group mask received in a group cmd.
     * 
     * @return  true if the channel id is masked.
     */
    bool GroupMaskMatch( byte* payload );

    /**
     * Helper function to execute the group command.
     * 
     * 1. Test the group mask.
     * 2. Extract the command argument (tap cnt).
     * 3. Execute the action callback if channel id was masked.
     */
    void OnGroupMaskCmd( byte* payload, uint len, std::function< void( const char*, uint16_t )> a_fnAction );



    uint8_t m_nPinSwitch;       ///< AC switch driver pin
    uint8_t m_nPinSwitchVal;    ///< Current state of the AC switch driver pin
    CTimer m_tmAutoOff;         ///< Auto-off timer
    ulong m_nAutoOff;           ///< Threshold value for auto-off timer, 0:disabled

    uint8_t m_nMode;                    ///< Configured switch mode
    uint8_t m_nId;                      ///< Configured switch channel id (1-64, 0:disabled)
    String m_strMask;                   ///< Current switch group mask
    String m_strMaskCopy;               ///< Copy of the configured switch group mask

    static uint8_t Sm_arrPinIn[ SW_CHANNELS ];  ///< Input (touch btn) pin configuration for switch channels
    static uint8_t Sm_arrPinOut[ SW_CHANNELS ]; ///< Output (AC switch driver) pin configuration for switch channels
};
