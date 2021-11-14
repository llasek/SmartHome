/**
 * Touch button base class
 * 2021 Łukasz Łasek
 */
#pragma once
#include <Arduino.h>
#include "Timer.h"



/**
 * Touch button class.
 * 
 * A touch button is an input device generating a logical HI signal when tapped/touched.
 * The signal must be debounced and connected to a selected GPIO pin.
 * An interrupt is configured to wake up the MCU and process the input.
 * 
 * A sample compatible device is a TTP223 touch sensor.
 * 
 * The following tapping patterns are implemented:
 * 1. Multiple short taps - multiple taps, lasting for no longer than a configured long tap duration,
 *    and no more than a configured double tap duration apart from the last one.
 * 2. Single long tap - lasting for at least a configured long tap duraction.
 * See the state machine diagram for details - TouchBtn.png
 */
class CTouchBtn
{
public:
    CTouchBtn() { m_state = EState::eDisabled; }

    /**
     * Enable the touch button.
     * 
     * Set the state to idle.
     * Configure duration for multi/double taps.
     * Configure the MCU input pin and attach an interrupt on state change.
     * 
     * @param[in]   a_nPin          MCU input pin
     * @param[in]   a_nLongTapMs    Long tap duration in ms
     * @param[in]   a_nDblTapMs     Double tap/next tap maximum delay since the last tap
     */
    void Enable( uint8_t a_nPin, uint16_t a_nLongTapMs, uint16_t a_nDblTapMs )
    {
        SetStateIdle();
        m_nLongTapMs = a_nLongTapMs;
        m_nDblTapMs = a_nDblTapMs;
        m_nPin = digitalPinToInterrupt( a_nPin );
        pinMode( m_nPin, INPUT );
        attachInterruptArg( m_nPin, reinterpret_cast< void (*)( void* )>( Isr ), this, CHANGE );
    }

    /**
     * Disable the touch button.
     * 
     * Set the state to disabled.
     * Detach the interrupt.
     */
    void Disable()
    {
        m_state = EState::eDisabled;
        detachInterrupt( m_nPin );
    }

    /**
     * Check if the button is enabled.
     * 
     * @return  True if enabled.
     */
    bool IsEnabled()
    {
        return m_state != EState::eDisabled;
    }

    /**
     * Main loop function.
     * 
     * Track the time passed since the last short tap finished.
     * Execute OnShortTap(tap cnt) callback when time threshold exceeded for a double tap pattern.
     */
    void loop()
    {
        noInterrupts();
        if( m_state == EState::eBtnReleased )
        {
            m_tmPress.UpdateCur();
            ulong nPressDuration = m_tmPress.Delta();
            if( nPressDuration >= m_nDblTapMs )
            {
                OnShortTap( m_nPressCnt );
                // SetStateIdle();
            }
        }
        interrupts();
    }

    /**
     * Short tap callback.
     * 
     * Set the state back to idle.
     * 
     * @param[in]   a_nCnt  The number of subsequent short taps.
     */
    virtual void OnShortTap( uint16_t a_nCnt )
    {
        SetStateIdle();
    }

    /**
     * Long tap callback.
     * 
     * Set the state back to idle.
     */
    virtual void OnLongTap()
    {
        SetStateIdle();
    }

protected:
    /**
     * Set the state to idle.
     * Reset the tap counter.
     */
    void SetStateIdle()
    {
        m_state = EState::eIdle;
        m_nPressCnt = 0;
    }

    /**
     * Set the state to button pressed.
     * Update the timestamp to track the duration of the tap.
     */
    void SetStateBtnPressed()
    {
        m_state = EState::eBtnPressed;
        m_tmPress.UpdateAll();
    }

    /**
     * Set the state to button released.
     * Increase the tap counter.
     * Update the timestamp to track the multitap sequence.
     */
    void SetStateBtnReleased()
    {
        m_state = EState::eBtnReleased;
        m_nPressCnt++;
        m_tmPress.UpdateAll();
    }

    /**
     * ISR for touch button state change.
     * 
     * Invoked whenever the logical state changes.
     * The input must be debounced.
     */
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
                    if(( m_nPressCnt > 0 ) || ( nPressDuration < m_nLongTapMs ) || ( m_nLongTapMs == 0 ))
                    {
                        SetStateBtnReleased();
                        // for m_nLongTapMs == 0 case, the OnShortTap() will be called from within loop()
                    }
                    else if(( m_nPressCnt == 0 ) && ( nPressDuration >= m_nLongTapMs ))
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
                    if( nPressDuration < m_nDblTapMs )
                        SetStateBtnPressed();
                }
                break;

            default:
                break;
        }
    }

    /**
     * ISR stub.
     */
    static void IRAM_ATTR Isr( CTouchBtn* a_pThis )
    {
        a_pThis->OnIsr();
    }

    /**
     * Touch button input pin state.
     */
    enum class EPinState : uint8_t
    {
        eBtnRelease = LOW,  ///< Falling edge -> btn rls
        eBtnPress = HIGH,   ///< Rising edge -> btn press
    };

    /**
     * Touch button state machine.
     */
    enum class EState
    {
        eIdle,
        eBtnPressed,
        eBtnReleased,
        eDisabled,
    };
    EState m_state;         ///< Internal state machine

    uint8_t m_nPin;         ///< MCU input pin
    uint16_t m_nLongTapMs;  ///< The minimal duration of the long tap in ms
    uint16_t m_nDblTapMs;   ///< The maximum time for the next tap in a multitap sequence
    uint16_t m_nPressCnt;   ///< The tap counter in a multitap sequence
    CTimer m_tmPress;       ///< Timer for long/multi tap
};
