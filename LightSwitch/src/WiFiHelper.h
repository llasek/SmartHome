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

#define FS_WIFI_CFG     "wifi_cfg"

class CWiFiHelper : public CWiFiHelperBase
{
public:
    void ReadCfg();

    String& GetHostName();

    void Enable();

    virtual void OnConnect();
    virtual void OnDisconnect();

    void loop();

protected:
    String m_strSsid, m_strPwd, m_strHostname;
    ulong m_nConnTimeout;
};
