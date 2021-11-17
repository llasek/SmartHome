/**
 * DIY Smart Home - light switch
 * WIFI helper class
 * 2021 Łukasz Łasek
 */
#include "WiFiHelper.h"
#include "Utils.h"

CWiFiHelper::CWiFiHelper()
    : m_nCurAP( 0 )
{
}

void CWiFiHelper::ReadCfg()
{
    if( m_nCurAP >= WIFI_AP_CNT )
        return;

    File file = LittleFS.open( FS_WIFI_CFG, "r" );
    if( file )
    {
        m_strHostname = CfgFileReadLine( file, "host" );
        m_nConnTimeout = CfgFileReadLine( file, "conn" ).toInt();

        const char* arrSsid[ WIFI_AP_CNT ] = { "ssid1", "ssid2" };
        m_strSsid = CfgFileReadLine( file, arrSsid[ m_nCurAP ]);

        const char* arrPwd[ WIFI_AP_CNT ] = { "pwd1", "pwd2" };
        m_strPwd = CfgFileReadLine( file, arrPwd[ m_nCurAP ]);

        file.close();

        DBGLOG5( "wifi cfg %d: ssid:'%s' pwd:'%s' timeo:%lus hostname:'%s'\n",
            m_nCurAP, m_strSsid.c_str(), m_strPwd.c_str(), m_nConnTimeout, m_strHostname.c_str());
        m_nConnTimeout *= 1000;
    }
    else
    {
        DBGLOG( "wifi cfg missing" );
    }
}

void CWiFiHelper::AlternateCfg()
{
    if( LittleFS.begin())
    {
        m_nCurAP = 1 - m_nCurAP;
        ReadCfg();
        LittleFS.end();
    }
}

String& CWiFiHelper::GetHostName()
{
    return m_strHostname;
}

void CWiFiHelper::Enable()
{
    Init( m_nConnTimeout );
    SetupSta( m_strSsid.c_str(), m_strPwd.c_str(), m_strHostname.c_str());
}

void CWiFiHelper::OnConnect()
{
    ArduinoOTA.setHostname( m_strHostname.c_str());
    ArduinoOTA.begin();
    DBGLOG1( "OTA begin: %s\n", ArduinoOTA.getHostname().c_str());
}

void CWiFiHelper::OnDisconnect()
{
    AlternateCfg();
    SetupSta( m_strSsid.c_str(), m_strPwd.c_str(), m_strHostname.c_str());
}

void CWiFiHelper::loop()
{
    ArduinoOTA.handle();
}
