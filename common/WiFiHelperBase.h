/**
 * WIFI helper base class
 * 2021 Łukasz Łasek
 */
#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "Timer.h"
#include "dbg.h"

/**
 * WIFI helper base class
 * 
 * Manage WIFI connection in the STA mode.
 * Issue callbacks on WIFI connection/disconnection events.
 * Issue the MCU reset on WIFI connection timeout.
 */
class CWiFiHelperBase
{
public:
    CWiFiHelperBase() {}

    /**
     * Initialize the instance
     * 
     * @param[in]   a_nConnTimeout  WIFI connection timeout or 0:disable timeout.
     *                              Timeout will result in MCU reset if enabled
     */
    void Init( ulong a_nConnTimeout = 0 )
    {
        m_nConnTimeout = a_nConnTimeout;

        // Configure WIFI callbacks to track and manage the WIFI connection:
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

    /**
     * Setup the STA mode and start a WIFI connection
     * 
     * @param[in]   a_pszSsid       WIFI SSID to connect to
     * @param[in]   a_pszPwd        WIFI password
     * @param[in]   a_pszHostname   Self DNS host name
     */
    void SetupSta( const char* a_pszSsid, const char* a_pszPwd, const char* a_pszHostname = nullptr )
    {
        DBGLOG( "Wifi STA setup" );
        WiFi.disconnect();
        WiFi.mode( WIFI_STA );
        if( a_pszHostname )
            WiFi.hostname( a_pszHostname );
        WiFi.begin( a_pszSsid, a_pszPwd );
    }

    /**
     * WIFI connected callback
     */
    virtual void OnConnect() {}

    /**
     * WIFI disconnected callback
     */
    virtual void OnDisconnect() {}

    /**
     * Test for the WIFI connection, track connection timeout and issue MCU reset
     * 
     * @return  true if WIFI is connected
     */
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
    bool m_bConn;           ///< Tracks current status of WIFI connection - true when connected
    ulong m_nConnTimeout;   ///< Configured WIFI connection timeout
    CTimer m_tm;            ///< Tracks WIFI connection timeout
    WiFiEventHandler m_evtConn, m_evtDisconn, m_evtGotIp, m_evtDhcpTimeout; ///< Internal WIFI events
};
