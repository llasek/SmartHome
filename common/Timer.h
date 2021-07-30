/**
 * Timer utility class
 * 2021 Łukasz Łasek
 */
#pragma once
#include <Arduino.h>
#include <limits.h>



/**
 * Timer class.
 * 
 * Track the time passed between updates.
 */
class CTimer
{
public:
    CTimer()
    {
        UpdateAll();
    }

    /**
     * Update the current timestamp.
     */
    void UpdateCur()
    {
        m_tmCur = millis();
        if( m_tmCur < m_tmLast )
        {
            m_tmCur += ULONG_MAX - m_tmLast;
            m_tmCur += 1;
            m_tmLast = 0;
        }
    }

    /**
     * Update the last expire time.
     */
    void UpdateLast()
    {
        m_tmLast = m_tmCur;
    }

    /**
     * Update both current timestamp and last expire time.
     * 
     * This resets the timer.
     */
    void UpdateAll()
    {
        m_tmCur = m_tmLast = millis();
    }

    /**
     * Calculate the time elapsed between the last expire time and last update of current timestamp.
     */
    ulong Delta()
    {
        return m_tmCur - m_tmLast;
    }

private:
    ulong m_tmLast; ///< Timer last expire timestamp
    ulong m_tmCur;  ///< Current timestamp
};
