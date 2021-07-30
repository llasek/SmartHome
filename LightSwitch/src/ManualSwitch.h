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



/// Switch mode 'disabled' configuration value
#define CFG_SW_MODE_DISABLED    "0"

/// Switch mode 'enabled' configuration value
#define CFG_SW_MODE_ENABLED     "1"



/// Switch is in a 'disabled' mode
#define SW_MODE_DISABLED    0

/// Switch is in an 'enabked' mode
#define SW_MODE_ENABLED     1

/// Switch is in a 'phantom' mode
#define SW_MODE_PHANTOM     2



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



/// Unknown tap beacon - to be ignored
#define SW_TAP_BEACON_CMD_IGNORE    0

/// 'Long tap' tap beacon cmd found in the private MQTT channel
#define SW_TAP_BEACON_CMD_LONG      'L'



/**
 * Manual light switch class
 * 
 * Manual light switch is a driver for an AC switch, controlled by a touch button.
 * There are 3 (SW_CHANNELS) switches available, distinguished by a channel number: 0/1/2.
 * 
 * 1 MCU pin is configured as a switch driver and outputs:
 * 1. LO when the switch is turned off,
 * 2. HI when the switch is turned on.
 * 
 * 1 MCU pin is configured as a touch button input.
 * 
 * Manual light switch features the following modes:
 * 1. Enabled - a button tap cycles the on/off state of the switch driver output.
 * 2. Disabled - both button input pin and a switch output are disabled.
 * 3. Phantom - a button tap sends an MQTT command; no AC switch is connected.
 * 
 * Recognized tap patterns:
 * 1. Single short tap - cycle the output on/off.
 * 2. Multi tap (N-times) - turn the output on for (N-1) minutes.
 * 3. Long tap - cycle the output on/off and simultaneously turn off all other switches
 *    in the group - via a tap beacon.
 * 
 * In the enabled mode the switch publishes its state upon change, via MQTT pub topic.
 * The states serviced by MQTT pub/sub topics are: on/off.
 * 
 * In phantom mode only an MQTT command to the real switch is sent instead of driving the actual
 * switch output. Also, no tap beacons are sent in phantom mode as the receiving (real) switch will
 * do that. The MQTT command is sent to the real switch directly via its MQTT subscription topic.
 * The topic name is derived from the phantom switch mode read from the config file. The mode should
 * contain a '0', '1', or the MQTT sub topic of the associated real switch.
 * The command format is:
 *      <cmd taps:short tap; tapl:long tap><tap count in decimal>
 * 
 * Tap beacon is a command sent over the private MQTT topic and to be received by all switches
 * available on the network. This enables the long tap action to turn off all other switches
 * in the group while changing the state of a local switch.
 * The tap beacon format is:
 *      <src switch group name><cmd L:long><src channel #><src hostname>
 * The switch group name, channel # and hostname are used to identify the sender of the command.
 * Currently, only the 'long tap' command is implemented, for which all receiving switches in the group
 * execute the command, except for the sender.
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
     * In the phantom mode the short tap command is sent directly to the MQTT sub topic of the associated real switch.
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
     * 3. Sent a long tap beacon to turn off all other switches in the group.
     * 
     * In the phantom mode the short tap command is sent directly to the MQTT sub topic of the associated real switch.
     */
    virtual void OnLongTap();



    /**
     * Handle the tap beacon received on a private MQTT topic.
     * 
     * Decode and execute the command from the beacon.
     * Currently only a long tap command is handled, which is:
     * 1. Turn off the switch,
     * 2. Disable the auto-off timer,
     * 3. Publish the switch on/off state via MQTT pub topic.
     * 
     * @param[in]   payload     MQTT message paylaod
     * @param[in]   len         Length of the payload
     */
    void OnMqttBeacon( byte* payload, uint len );

    /**
     * Publish the switch on/off state via MQTT pub topic.
     */
    void MqttPubStat();

    /**
     * In phantom mode: send a command to the associated real switch.
     * 
     * @param[in]   a_pszMqttCmd    Base command to be sent
     * @param[in]   a_nArg          Numeric argument to be added to the end of a command
     */
    void MqttSendPhantomCmd( const char* a_pszMqttCmd, uint16_t a_nArg );



    /**
     * Return the configured channel name.
     * 
     * @return  SW_CHANNEL_0, SW_CHANNEL_1, SW_CHANNEL_2 or SW_CHANNEL_NA (invalid).
     */
    char GetChanNo();



protected:
    /**
     * Construct and return a tap beacon.
     * 
     * @param[in]   a_nTapCmd   Tap command to be constructed.
     * 
     * @return  Tap beacon for a given command.
     */
    String GetTapBeacon( const char a_nTapCmd );

    /**
     * Decode a command from the beacon received.
     * 
     * A valid tap beacon for the receiving device consist of:
     * 1. A matching switch name,
     * 2. Tap command (1 character),
     * 3. Tap command arguments - for SW_TAP_BEACON_CMD_LONG:
     *    a. Switch channel number in base 10,
     *    b. Hostname of the sender.
     *    The receiving device differs in at least one of the above.
     * 
     * @param[in]   payload     Tap beacon received.
     * @param[in]   len         Length of the payload.
     * 
     * @return  Decoded tap cmd (e.g. SW_TAP_BEACON_CMD_LONG).
     *          SW_TAP_BEACON_CMD_IGNORE when not the receiver or decoder fault.
     */
    byte GetTapBeaconCmd( byte* payload, uint len );



    uint8_t m_nPinSwitch;       ///< AC switch driver pin
    uint8_t m_nPinSwitchVal;    ///< Current state of the AC switch driver pin
    CTimer m_tmAutoOff;         ///< Auto-off timer
    ulong m_nAutoOff;           ///< Threshold value for auto-off timer, 0:disabled

    uint8_t m_nMode;                    ///< Configured switch mode
    String m_strPhantomMqttPubTopic;    ///< MQTT pub topic for cmds sent in phantom mode - MQTT cmd sub topic of the receiver, e.g. sw/cmd/kitchen0/ch0
    String m_strGroupName;              ///< Configured switch group name, used in tap beacon

    static uint8_t Sm_arrPinIn[ SW_CHANNELS ];  ///< Input (touch btn) pin configuration for switch channels
    static uint8_t Sm_arrPinOut[ SW_CHANNELS ]; ///< Output (AC switch driver) pin configuration for switch channels
};
