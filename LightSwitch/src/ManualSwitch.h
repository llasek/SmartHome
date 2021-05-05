/**
 * DIY Smart Home - light switch
 * Manual switch
 * 2021 Łukasz Łasek
 */
#pragma once
#include <Arduino.h>
#include <LittleFS.h>

#include "TouchBtn.h"
#include "dbg.h"

/**
 * sw_cfg file:
 * 1: switch channel 1: mode: 0:disabled, 1:enabled, 2:phantom
 * 2: switch channel 1: long click ms
 * 3: switch channel 1: dbl click ms
 * 4: switch channel 2: mode: 0:disabled, 1:enabled, 2:phantom
 * 5: switch channel 2: long click ms
 * 6: switch channel 2: dbl click ms
 * 7: switch channel 3: mode: 0:disabled, 1:enabled, 2:phantom
 * 8: switch channel 3: long click ms
 * 9: switch channel 3: dbl click ms
 */
#define FS_SW_CFG       "sw_cfg"

#define PIN_OUT1    D1  // GPIO 5
#define PIN_OUT2    D2  // GPIO 4
#define PIN_OUT3    D8  // GPIO 15

#define PIN_IN1     D5  // GPIO 14
#define PIN_IN2     D6  // GPIO 12
#define PIN_IN3     D7  // GPIO 13

class CManualSwitch : public CTouchBtn {
public:
    static File OpenCfg() {
        return LittleFS.open( FS_SW_CFG, "r" );
    }

    void ReadCfg( File& a_rFile ) {
        if( a_rFile ) {
            m_nMode = a_rFile.readStringUntil( '\n' ).toInt();
            m_nLongClickMs = a_rFile.readStringUntil( '\n' ).toInt();
            m_nDblClickMs = a_rFile.readStringUntil( '\n' ).toInt();

            DBGLOG3( "sw cfg: swmod:%d long:%u dbl:%u\n",
                m_nMode, m_nLongClickMs, m_nDblClickMs );
        } else {
            m_nMode = 1;
            m_nLongClickMs = m_nDblClickMs = 250;
            DBGLOG( "sw cfg missing" );
        }
    }

    void Enable( uint8_t a_nPinBtn, uint8_t a_nPinSwitch ) {
        m_nPinSwitch = a_nPinSwitch;
        m_nPinSwitchVal = LOW;
        pinMode( m_nPinSwitch, OUTPUT );
        digitalWrite( m_nPinSwitch, m_nPinSwitchVal );
        CTouchBtn::Enable( a_nPinBtn, m_nLongClickMs, m_nDblClickMs );
    }

    virtual void OnShortClick() {
        CTouchBtn::OnShortClick();
        DBGLOG1( "short click pin#%d\n", m_nPin );
        digitalWrite( m_nPinSwitch, HIGH );
    }

    virtual void OnLongClick() {
        CTouchBtn::OnLongClick();
        DBGLOG1( "long click pin#%d\n", m_nPin );
        digitalWrite( m_nPinSwitch, LOW );
    }

    virtual void OnDblClick() {
        CTouchBtn::OnDblClick();
        DBGLOG1( "dbl click pin#%d\n", m_nPin );
        analogWrite( m_nPinSwitch, 100 );
    }

protected:
    uint8_t m_nPinSwitch, m_nPinSwitchVal;

    // cfg:
    uint8_t m_nMode;
    uint16_t m_nLongClickMs, m_nDblClickMs;
};
