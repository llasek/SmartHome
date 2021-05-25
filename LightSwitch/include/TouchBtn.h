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
    CTouchBtn() { m_state = EState::eDisabled; }

    void Enable( uint8_t a_nPin, uint16_t a_nLongClickMs, uint16_t a_nDblClickMs )
    {
        SetStateIdle();
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

    void loop()
    {
        noInterrupts();
        if( m_state == EState::eBtnReleased )
        {
            m_tmPress.UpdateCur();
            ulong nPressDuration = m_tmPress.Delta();
            if( nPressDuration >= m_nDblClickMs )
            {
                OnShortTap( m_nPressCnt );
                // SetStateIdle();
            }
        }
        interrupts();
    }

    virtual void OnShortTap( uint16_t a_nCnt )
    {
        SetStateIdle();
    }

    virtual void OnLongTap()
    {
        SetStateIdle();
    }

protected:
    void SetStateIdle()
    {
        m_state = EState::eIdle;
        m_nPressCnt = 0;
    }

    void SetStateBtnPressed()
    {
        m_state = EState::eBtnPressed;
        m_tmPress.UpdateAll();
    }

    void SetStateBtnReleased()
    {
        m_state = EState::eBtnReleased;
        m_nPressCnt++;
        m_tmPress.UpdateAll();
    }

    void OnIsr()
    {
        EPinState nPin = (EPinState)digitalRead( m_nPin );
        switch( m_state )
        {
            case EState::eIdle:
                if( nPin == EPinState::eBtnPress )
                    SetStateBtnPressed();
                break;

            case EState::eBtnPressed:
                if( nPin == EPinState::eBtnRelease )
                {
                    m_tmPress.UpdateCur();
                    ulong nPressDuration = m_tmPress.Delta();
                    if(( m_nPressCnt > 0 ) || ( nPressDuration < m_nLongClickMs ) || ( m_nLongClickMs == 0 ))
                    {
                        SetStateBtnReleased();
                        // for m_nLongClickMs == 0 case, the OnShortTap() will be called from within loop()
                    }
                    else if(( m_nPressCnt == 0 ) && ( nPressDuration >= m_nLongClickMs ))
                    {
                        OnLongTap();
                        // SetStateIdle();
                    }
                }
                break;

            case EState::eBtnReleased:
                if( nPin == EPinState::eBtnPress )
                {
                    m_tmPress.UpdateCur();
                    ulong nPressDuration = m_tmPress.Delta();
                    if( nPressDuration < m_nDblClickMs )
                        SetStateBtnPressed();
                }
                break;

            default:
                break;
        }
    }

    static void ICACHE_RAM_ATTR Isr( CTouchBtn* a_pThis )
    {
        a_pThis->OnIsr();
    }

    enum class EPinState : uint8_t
    {
        eBtnRelease = LOW,  // falling edge -> btn rls
        eBtnPress = HIGH,   // rising edge -> btn press
    };

    // @todo: digitize and upload state machine
    enum class EState
    {
        eIdle,
        eBtnPressed,
        eBtnReleased,
        eDisabled,
    };
    EState m_state;

    uint8_t m_nPin;
    uint16_t m_nLongClickMs, m_nDblClickMs, m_nPressCnt;
    CTimer m_tmPress;
};
