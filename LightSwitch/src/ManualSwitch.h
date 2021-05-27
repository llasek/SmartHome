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

/**
 * sw_cfg file:
 * 1: switch channel 0: mode: 0:disabled, 1:enabled, 2:phantom
 * 2: switch channel 0: long tap ms, 0 = disabled
 * 3: switch channel 0: dbl tap ms, 0 = disabled
 * 4: switch channel 0: mqtt tap beacon
 * 5..8: switch channel 1
 * 9..12: switch channel 2
 */
#define FS_SW_CFG       "sw_cfg"

#define SW_MODE_DISABLED    0
#define SW_MODE_ENABLED     1
#define SW_MODE_PHANTOM     2   // @todo: implement

#define SW_CHANNEL_0    '0'
#define SW_CHANNEL_1    '1'
#define SW_CHANNEL_2    '2'
#define SW_CHANNEL_NA   0
#define SW_CHANNELS     3

// tap beacon format is: <beacon><cmd L:long><src channel #><src hostname>
#define SW_TAP_BEACON_CMD_IGNORE    0
#define SW_TAP_BEACON_CMD_LONG      'L'

class CManualSwitch : public CTouchBtn
{
public:
    static File OpenCfg();
    void ReadCfg( File& a_rFile );

    void Enable( uint8_t a_nChanNo );
    void Disable();

    void DriveSwitch( bool a_bStateOn );
    bool GetSwitchState();

    void loop();

    virtual void OnShortTap( uint16_t a_nCnt );
    virtual void OnLongTap();

    void OnMqttBeacon( byte* payload, uint len );
    void MqttPubStat();

    char GetChanNo();

protected:
    String GetTapBeacon( const char a_nTapCmd );
    byte GetTapBeaconCmd( byte* payload, uint len );

    uint8_t m_nPinSwitch, m_nPinSwitchVal;
    CTimer m_tmAutoOff;
    ulong m_nAutoOff;

    // cfg:
    uint8_t m_nMode;
    String m_strTapBeacon;

    static uint8_t Sm_arrPinIn[ SW_CHANNELS ];
    static uint8_t Sm_arrPinOut[ SW_CHANNELS ];
};
