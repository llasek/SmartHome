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



/// Configuration file
#define FS_WIFI_CFG     "wifi_cfg"



/**
 * WIFI connection helper class.
 * 
 * Read the config file.
 * Enable the WIFI, configure host name.
 * Enable OTA FWU on WIFI connect.
 * Reconnect on WIFI disconnect.
 */
class CWiFiHelper : public CWiFiHelperBase
{
public:
    CWiFiHelper();



    /**
     * Read the configuration file.
     */
    void ReadCfg();

    /**
     * Alternate between APs configured in the cfg file.
     * Read in the SSID and PWD.
     */
    void AlternateCfg();



    /**
     * Return the configured host name.
     * 
     * @return  Configured host hame.
     */
    String& GetHostName();



    /**
     * Configure and enable WIFI in STA mode.
     */
    void Enable();



    /**
     * WIFI connected callback.
     */
    virtual void OnConnect();

    /**
     * WIFI disconnected callback.
     */
    virtual void OnDisconnect();



    /**
     * Main loop function.
     * 
     * Execute OTA FWU.
     */
    void loop();

protected:
    String m_strSsid;       ///< Configured WIFI SSID
    String m_strPwd;        ///< Configured WIFI password
    String m_strHostname;   ///< Configured host name
    ulong m_nConnTimeout;   ///< Configured WIFI connection timeout for MCU reset, 0:disable
    int m_nCurAP;           ///< Current WIFI AP
};
