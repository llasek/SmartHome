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
#define FS_CH0_CFG          "ch0_cfg"
#define FS_CH1_CFG          "ch1_cfg"
#define FS_CH2_CFG          "ch2_cfg"



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



/// Short single tap event
#define SW_TAP_EVENT_SHORT_SINGLE   0

/// Short multi tap tap event
#define SW_TAP_EVENT_SHORT_MULTI    1

/// Long single tap event
#define SW_TAP_EVENT_LONG_SINGLE    2

/// Number of tap events
#define SW_TAP_EVENTS               3



/**
 * Manual light switch class
 * 
 * Manual light switch is a driver for a switch, controlled by a touch button.
 * There are 3 (SW_CHANNELS) switches available, distinguished by a channel number: 0/1/2.
 * 
 * One MCU pin is configured as a switch driver and is drived:
 * 1. LO when the switch is turned off,
 * 2. HI when the switch is turned on.
 * 
 * Another MCU pin is configured as a touch button input.
 * 
 * The switch is configured to respond to the following physical tap events:
 * 1. ev-ss - short single tap,
 * 2. ev-sm - short multi tap,
 * 3. ev-ls - long single tap.
 * 
 * Each tap event can be assigned an argument: arg-ss, arg-sm, arg-ls.
 * 
 * The response to each of the tap event can be configured to one of the following tap operations:
 * 1. tgle - toggle the on/off state of the switch driver output. This is the default.
 *    arg: none
 * 2. tgof - the same as above, but turn off (mask off) other switches in the group via a group command:
 *           MQTT_CMD_TURN_OFF.
 *    arg: 64-bit hexadecimal group mask (0x0123456789abcdef).
 * 3. aoff - turn the output on for max(taps#-1 (multi-tap), 1 (single tap)) * arg seconds.
 *    arg: auto-off timer step duration.
 * 4. fwte - don't drive the local output, only forward the tap event to configured remote switches
 *           via a corresponding MQTT group cmd: MQTT_CMD_FORWARD_SHORT_TAP, MQTT_CMD_FORWARD_LONG_TAP.
 *    arg: 64-bit hexadecimal group mask (0x0123456789abcdef).
 * 5. anything else - the tap event is disabled/ignored.
 * 
 * Each channel can be assigned a unique id, corresponding to a bit# (LSB first) in the mask sent
 * in a group command. A valid id is in the range of 1-64, while 0 denotes no id being assigned to the channel.
 * In such a case, the channel is not addressable via group commands.
 * 
 * The group command is received by all devices on the network. The mask is then tested against
 * all channel ids and only masked channels are responding to the command. This allows for one tap event
 * to control multiple remote switches.
 * The group command format is described in the Mqtt.h file.
 * 
 * The switch publishes its state upon change via its MQTT pub topic.
 * The states serviced by MQTT pub/sub topics are: on/off.
 */
class CManualSwitch : public CTouchBtn
{
public:
    /**
     * Read the configuration file corresponding to given channel
     * 
     * @param[in]   a_nChanNo   channel number
     */
    void ReadCfg( uint8_t a_nChanNo );

    /**
     * @brief Read the configured tap event
     * 
     * @param a_rFile       Opened cfg file.
     * @param a_nTapEvent   Tap event to configure: SW_TAP_EVENT_*
     * 
     * @return  true if a tap operation is configured for the tap event (i.e. not SW_TAP_OP_DISABLE).
     */
    bool ReadCfgTapEvent( File& a_rFile, uint8_t a_nTapEvent );



    /**
     * @brief Test if the switch is disabled, i.e. no tap events were configured and enabled.
     * 
     * @return true if the switch is disabled, false otherwise.
     */
    bool IsDisabled();

    /**
     * Enable the manual switch
     * 
     * Set up the button and a switch driver according to the configured mode
     */
    void Enable();

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
     * @brief Execute a tap operation corresponding to the tap event.
     * 
     * Map the tap event (i.e. physical single-/multi- short/long tap) to the configured tap operation.
     * Execute the operation.
     * 1. SW_TAP_OP_TOGGLE:
     *  a. Cycle the output on/off.
     *  b. Disable the auto-off timer.
     *  c. Publish the switch on/off state via MQTT pub topic.
     * 
     * 2. SW_TAP_OP_TOGGLE_MASK_OFF:
     *  a. Same as SW_TAP_OP_TOGGLE.
     *  b. Send the MQTT_CMD_TURN_OFF group command to turn off all masked switches in the group.
     * 
     * 3. SW_TAP_OP_AUTO_OFF:
     *  a. Turn the output on.
     *  b. Configure the auto-off timer for max( 1 (single tap), TapCnt - 1 (multi tap) ) seconds.
     *  c. Publish the switch on/off state via MQTT pub topic.
     * 
     * 4. SW_TAP_OP_FORWARD:
     *  a. Forward the tap event through the MQTT_CMD_FORWARD_SHORT_TAP/MQTT_CMD_FORWARD_LONG_TAP group command.
     * 
     * 5. SW_TAP_OP_DISABLE:
     *  a. no-op.
     * 
     * @param a_nTapEvent   The tap event: SW_TAP_EVENT_*.
     * @param a_nTapCnt     Number of subsequent taps.
     */
    void OnTap( uint8_t a_nTapEvent, uint16_t a_nTapCnt );

    /**
     * Receive and execute the short button tap event.
     * 
     * @param[in]   a_nCnt  Number of subsequent short taps.
     */
    virtual void OnShortTap( uint16_t a_nCnt );

    /**
     * Receive and execute the long button tap event.
     */
    virtual void OnLongTap();



    /**
     * @brief Set the switch state.
     * 
     * In the enabled mode:
     * 1. Set the output on/off state and configure the auto-off timer.
     * 2. Publish the switch on/off state via MQTT pub topic.
     * 
     * @param[in]   a_bStateOn  The on/off state to set: true:on, false:off.
     * @param[in]   a_nAutoOff  The auto-off timer in msec (0:disable).
     */
    void SetState( bool a_bStateOn, ulong a_nAutoOff );

    /**
     * Handle the received MQTT group command.
     * 
     * Decode and execute the group command. The following cmds are handled:
     * 1. MQTT_CMD_FORWARD_SHORT_TAP
     * 2. MQTT_CMD_FORWARD_LONG_TAP
     * 3. MQTT_CMD_TURN_OFF
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
     * @param[in]   a_pszMask       Configured tap event mask
     */
    void MqttSendGroupCmd( const char* a_pszMqttCmd, uint16_t a_nArg, const char* a_pszMask );



    /**
     * Return the configured channel name.
     * 
     * @return  SW_CHANNEL_0, SW_CHANNEL_1, SW_CHANNEL_2 or SW_CHANNEL_NA (invalid).
     */
    char GetChanNo();



protected:
    /**
     * Clear bits in the group mask.
     * 
     * Note the clearing mask is m_pszClearMask.
     * 
     * @param[in]   a_rstrMask  The mask to be cleared - in a mask format
     */
    void GroupMaskClearBits( String& a_rstrMask );

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



    uint8_t m_nChanNo;          ///< Configured channel number
    uint8_t m_nPinSwitchVal;    ///< Current state of the AC switch driver pin
    CTimer m_tmAutoOff;         ///< Auto-off timer
    ulong m_nAutoOff;           ///< Threshold value for auto-off timer, 0:disabled

    uint16_t m_arrTapOps[ SW_TAP_EVENTS ];  ///< Configured tap operations for all tap events
    String m_arrTapArgs[ SW_TAP_EVENTS ];   ///< Configured tap op args for all tap events

    uint8_t m_nId;              ///< Configured switch channel id (1-64:valid, 0:disabled)

    const char* m_pszClearMask; ///< Bit mask to be cleared in context of MqttSendGroupCmd()

    static uint8_t Sm_arrPinIn[ SW_CHANNELS ];  ///< Input (touch btn) pin configuration for switch channels
    static uint8_t Sm_arrPinOut[ SW_CHANNELS ]; ///< Output (AC switch driver) pin configuration for switch channels
};
