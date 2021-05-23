/**
 * WIFI helper base class
 * 2021 Łukasz Łasek
 */
#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "Timer.h"
#include "dbg.h"

class CWiFiHelperBase
{
public:
    CWiFiHelperBase() {}

    void Init( const char* a_pszSsid, const char* a_pszPwd, ulong a_nConnTimeout = 0 )
    {
        m_pszSsid = a_pszSsid;
        m_pszPwd = a_pszPwd;
        m_nConnTimeout = a_nConnTimeout;

        m_evtConn = WiFi.onStationModeConnected(
            [ this ]( const WiFiEventStationModeConnected& arg )
            {
                DBGLOG1( "Wifi connected: %lu\n", m_tm.Delta());
            });
        m_evtDisconn = WiFi.onStationModeDisconnected(
            [ this ]( const WiFiEventStationModeDisconnected& arg )
            {
                DBGLOG( "Wifi disconnected" );
                m_bConn = false;
                OnDisconnect();
            });
        m_evtGotIp = WiFi.onStationModeGotIP(
            [ this ]( const WiFiEventStationModeGotIP& arg )
            {
                DBGLOG_( "Wifi ip: " );
                DBGLOG( WiFi.localIP());
                m_bConn = true;
                OnConnect();
            });
        m_evtDhcpTimeout = WiFi.onStationModeDHCPTimeout(
            [ this ]()
            {
                DBGLOG( "Wifi dhcp timeout" );
                m_bConn = false;
                OnDisconnect();
            });
        m_tm.UpdateAll();
    }

    void SetupSta()
    {
        DBGLOG( "Wifi STA setup" );
        WiFi.disconnect();
        WiFi.mode( WIFI_STA );
        WiFi.begin( m_pszSsid, m_pszPwd );
    }

    virtual void OnConnect() {}
    virtual void OnDisconnect() {}

    bool Connected()
    {
        m_tm.UpdateCur();
        if( m_bConn )
        {
            m_tm.UpdateLast();
            return true;
        }
        else
        {
            if(( m_nConnTimeout ) && ( m_tm.Delta() > m_nConnTimeout ))
            {
                DBGLOG1( "Wifi retry failed for %lu - issue reset\n", m_tm.Delta());
                ESP.reset();
            }
        }
        return false;
    }

private:
    bool m_bConn;
    ulong m_nConnTimeout;
    CTimer m_tm;
    const char* m_pszSsid;
    const char* m_pszPwd;
    WiFiEventHandler m_evtConn, m_evtDisconn, m_evtGotIp, m_evtDhcpTimeout;
};
