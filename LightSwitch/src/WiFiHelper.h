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
 * 4: hostname
 */
#define FS_WIFI_CFG     "wifi_cfg"

class CWiFiHelper : public CWiFiHelperBase
{
public:
    void ReadCfg();

    void Enable();

    virtual void OnConnect();
    virtual void OnDisconnect();

    void loop();

protected:
    String m_strSsid, m_strPwd, m_strHostname;
    ulong m_nConnTimeout;
};
