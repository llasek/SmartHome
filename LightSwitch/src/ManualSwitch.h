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
#include "dbg.h"

/**
 * sw_cfg file:
 * 1: switch channel 0: mode: 0:disabled, 1:enabled, 2:phantom
 * 2: switch channel 0: long click ms
 * 3: switch channel 0: dbl click ms
 * 4..6: switch channel 1
 * 7..9: switch channel 2
 */
#define FS_SW_CFG       "sw_cfg"

#define SW_CHANNEL_0    '0'
#define SW_CHANNEL_1    '1'
#define SW_CHANNEL_2    '2'
#define SW_CHANNEL_NA   0
#define SW_CHANNELS     3

class CManualSwitch : public CTouchBtn
{
public:
    static File OpenCfg()
    {
        return LittleFS.open( FS_SW_CFG, "r" );
    }

    void ReadCfg( File& a_rFile )
    {
        if( a_rFile )
        {
            m_nMode = a_rFile.readStringUntil( '\n' ).toInt();
            m_nLongClickMs = a_rFile.readStringUntil( '\n' ).toInt();
            m_nDblClickMs = a_rFile.readStringUntil( '\n' ).toInt();

            DBGLOG3( "sw cfg: swmod:%d long:%u dbl:%u\n",
                m_nMode, m_nLongClickMs, m_nDblClickMs );
        }
        else
        {
            m_nMode = 1;
            m_nLongClickMs = m_nDblClickMs = 250;
            DBGLOG( "sw cfg missing" );
        }
    }

    void Enable( uint8_t a_nChanNo )
    {
        if( a_nChanNo >= SW_CHANNELS )
            return;

        m_nPinSwitch = Sm_arrPinOut[ a_nChanNo ];
        pinMode( m_nPinSwitch, OUTPUT );
        DriveSwitch( false );
        CTouchBtn::Enable( Sm_arrPinIn[ a_nChanNo ], m_nLongClickMs, m_nDblClickMs );   
    }

    void Disable()
    {
        CTouchBtn::Disable();
    }

    void DriveSwitch( bool a_bStateOn )
    {
        m_nPinSwitchVal = ( a_bStateOn ) ? HIGH : LOW;
        digitalWrite( m_nPinSwitch, m_nPinSwitchVal );
    }

    bool GetSwitchState()
    {
        return ( m_nPinSwitchVal == LOW ) ? false : true;
    }

    virtual void OnShortClick()
    {
        CTouchBtn::OnShortClick();
        DBGLOG1( "short click pin#%d\n", m_nPin );
        DriveSwitch( true );
        MqttPubStat();
    }

    virtual void OnLongClick()
    {
        CTouchBtn::OnLongClick();
        DBGLOG1( "long click pin#%d\n", m_nPin );
        DriveSwitch( false );
        MqttPubStat();
    }

    virtual void OnDblClick()
    {
        CTouchBtn::OnDblClick();
        DBGLOG1( "dbl click pin#%d\n", m_nPin );
        // @todo: demo, implement actions
        m_nPinSwitchVal = HIGH;
        analogWrite( m_nPinSwitch, 100 );
        MqttPubStat();
    }

    char GetChanNo();
    void MqttPubStat();

protected:
    uint8_t m_nPinSwitch, m_nPinSwitchVal;

    // cfg:
    uint8_t m_nMode;
    uint16_t m_nLongClickMs, m_nDblClickMs;

    static uint8_t Sm_arrPinIn[ SW_CHANNELS ];
    static uint8_t Sm_arrPinOut[ SW_CHANNELS ];
};
