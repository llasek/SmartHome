/**
 * DIY Smart Home - light switch
 * WIFI helper class
 * 2021 Łukasz Łasek
 */
#include "WiFiHelper.h"

void CWiFiHelper::ReadCfg()
{
    File file = LittleFS.open( FS_WIFI_CFG, "r" );
    if( file )
    {
        m_strSsid = file.readStringUntil( '\n' );
        m_strPwd = file.readStringUntil( '\n' );
        m_nConnTimeout = file.readStringUntil( '\n' ).toInt();
        m_strHostname = file.readStringUntil( '\n' );
        file.close();

        DBGLOG4( "wifi cfg: ssid:'%s' pwd:'%s' timeo:%lus hostname:'%s'\n",
            m_strSsid.c_str(), m_strPwd.c_str(), m_nConnTimeout, m_strHostname.c_str());
        m_nConnTimeout *= 1000;
    }
    else
    {
        DBGLOG( "wifi cfg missing" );
    }
}

String& CWiFiHelper::GetHostName()
{
    return m_strHostname;
}

void CWiFiHelper::Enable()
{
    Init( m_strSsid.c_str(), m_strPwd.c_str(), m_nConnTimeout );
    SetupSta( m_strHostname.c_str());
}

void CWiFiHelper::OnConnect()
{
    ArduinoOTA.setHostname( m_strHostname.c_str());
    ArduinoOTA.begin();
    DBGLOG1( "OTA begin: %s\n", ArduinoOTA.getHostname().c_str());
}

void CWiFiHelper::OnDisconnect()
{
    SetupSta( m_strHostname.c_str());
}

void CWiFiHelper::loop()
{
    ArduinoOTA.handle();
}
