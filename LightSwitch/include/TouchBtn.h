/**
 * Touch button base class
 * 2021 Łukasz Łasek
 */
#pragma once
#include <Arduino.h>
#include "Timer.h"

class CTouchBtn
{
public:
    CTouchBtn() {}

    void Enable( uint8_t a_nPin, uint16_t a_nLongClickMs, uint16_t a_nDblClickMs )
    {
        m_state = EState::eIdle;
        m_nLongClickMs = a_nLongClickMs;
        m_nDblClickMs = a_nDblClickMs;
        m_nPin = digitalPinToInterrupt( a_nPin );
        pinMode( m_nPin, INPUT );
        attachInterruptArg( m_nPin, reinterpret_cast< void (*)( void* )>( Isr ), this, CHANGE );
    }

    void Disable()
    {
        m_state = EState::eDisabled;
        detachInterrupt( m_nPin );
    }

    bool IsEnabled()
    {
        return m_state != EState::eDisabled;
    }

    void OnIsr()
    {
        EPinState nPin = (EPinState)digitalRead( m_nPin );
        switch( m_state )
        {
            case EState::eIdle:
                if( nPin == EPinState::eBtnPress )
                {
                    m_state = EState::eBtn1Pressed;
                    m_tmSingleClick.UpdateAll();
                }
                break;

            case EState::eBtn1Pressed:
                if( nPin == EPinState::eBtnRelease )
                {
                    m_state = EState::eBtn1Released;
                    m_tmSingleClick.UpdateCur();
                    m_tmLastClick.UpdateAll();  // start dbl click timer
                }
                break;

            case EState::eBtn1Released:
                if( nPin == EPinState::eBtnPress )
                {
                    m_state = EState::eBtn2Pressed;
                }
                break;

            case EState::eBtn2Pressed:
                if( nPin == EPinState::eBtnRelease )
                {
                    m_state = EState::eBtn2Released;
                }
                break;

            case EState::eBtn2Released:
            default:
                break;
        }
    }

    virtual void loop()
    {
        m_tmLastClick.UpdateCur();
        ulong nSingleClickDelta = m_tmSingleClick.Delta();
        ulong nLastClickDelta = m_tmLastClick.Delta();
        switch( m_state )
        {
            case EState::eBtn1Released:
                if( nLastClickDelta > m_nDblClickMs )
                    OnShortClick();
                else if( nSingleClickDelta > m_nLongClickMs )
                    OnLongClick();
                break;

            case EState::eBtn2Released:
                OnDblClick();
                break;

            default:
                break;
        }
    }

    virtual void OnShortClick()
    {
        m_state = EState::eIdle;
    }

    virtual void OnLongClick()
    {
        m_state = EState::eIdle;
    }

    virtual void OnDblClick()
    {
        m_state = EState::eIdle;
    }

protected:
    static void ICACHE_RAM_ATTR Isr( CTouchBtn* a_pThis )
    {
        a_pThis->OnIsr();
    }

    enum class EPinState : uint8_t
    {
        eBtnRelease = LOW,  // falling edge -> btn rls
        eBtnPress = HIGH,   // rising edge -> btn press
    };

    enum class EState
    {
        eIdle,
        eBtn1Pressed,
        eBtn1Released,
        eBtn2Pressed,
        eBtn2Released,
        eDisabled,
    };
    EState m_state;

    uint8_t m_nPin;
    uint16_t m_nLongClickMs, m_nDblClickMs;
    CTimer m_tmSingleClick, m_tmLastClick;
};
