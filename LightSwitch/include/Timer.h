/**
 * Timer utility class
 * 2021 Łukasz Łasek
 */
#pragma once
#include <Arduino.h>
#include <limits.h>

class CTimer {
public:
    CTimer() {
        UpdateAll();
    }

    void UpdateCur() {
        m_tmCur = millis();
        if( m_tmCur < m_tmLast ) {
            m_tmCur += ULONG_MAX - m_tmLast;
            m_tmCur += 1;
            m_tmLast = 0;
        }
    }

    void UpdateLast() {
        m_tmLast = m_tmCur;
    }

    void UpdateAll() {
        m_tmCur = m_tmLast = millis();
    }

    ulong Delta() {
        return m_tmCur - m_tmLast;
    }

private:
    ulong m_tmLast, m_tmCur;
};
