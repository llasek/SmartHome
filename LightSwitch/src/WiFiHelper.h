/**
 * DIY Smart Home - light switch
 * WIFI helper class
 * 2021 Łukasz Łasek
 */
#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoOTA.h>

#include "WiFiHelperBase.h"
#include "dbg.h"

/**
 * wifi_cfg file:
 * 1: ssid
 * 2: pwd
 * 3: conn timeout in sec - reset
 */
#define FS_WIFI_CFG     "wifi_cfg"

class CWiFiHelper : public CWiFiHelperBase
{
public:
    void ReadCfg()
    {
        File file = LittleFS.open( FS_WIFI_CFG, "r" );
        if( file )
        {
            m_strSsid = file.readStringUntil( '\n' );
            m_strPwd = file.readStringUntil( '\n' );
            m_nConnTimeout = file.readStringUntil( '\n' ).toInt();
            file.close();

            DBGLOG3( "wifi cfg: ssid:'%s' pwd:'%s' timeo:%lus\n",
                m_strSsid.c_str(), m_strPwd.c_str(), m_nConnTimeout );
            m_nConnTimeout *= 1000;
        }
        else
        {
            DBGLOG( "wifi cfg missing" );
        }
    }

    void Enable()
    {
        Init( m_strSsid.c_str(), m_strPwd.c_str(), m_nConnTimeout );
        SetupSta();
    }

    virtual void OnConnect()
    {
        ArduinoOTA.begin();
        DBGLOG1( "OTA begin: %s\n", ArduinoOTA.getHostname().c_str());
    }

    virtual void OnDisconnect()
    {
        SetupSta();
    }

    void loop()
    {
        ArduinoOTA.handle();
    }

protected:
    String m_strSsid, m_strPwd;
    ulong m_nConnTimeout;
};
