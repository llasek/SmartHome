/**
 * DIY Smart Home - light switch
 * main program file
 * 2021 Łukasz Łasek
 */
#include <Arduino.h>
#include "WiFiHelper.h"
#include "ManualSwitch.h"
#include "Mqtt.h"
#include "dbg.h"

CWiFiHelper g_wifi;
CMqtt g_mqtt;

CManualSwitch g_swChan0;
CManualSwitch g_swChan1;
CManualSwitch g_swChan2;

/**
 * Enable all switches and MQTT client
 */
void EnableAll()
{
    DBGLOG( "Enable btns" );
    g_mqtt.Enable();
    g_swChan0.Enable( 0 );
    g_swChan1.Enable( 1 );
    g_swChan2.Enable( 2 );
}

/**
 * Disable all switches and MQTT client
 */
void DisableAll()
{
    DBGLOG( "Disable btns" );
    g_swChan0.Disable();
    g_swChan1.Disable();
    g_swChan2.Disable();
    g_mqtt.Disable();
}

/**
 * Setup the MCU
 */
void setup()
{
    // Setup debug log:
    DbgLogSetup();

    // Read all cfg files:
    if( LittleFS.begin())
    {
        File file = CManualSwitch::OpenCfg();
        g_swChan0.ReadCfg( file );
        g_swChan1.ReadCfg( file );
        g_swChan2.ReadCfg( file );
        file.close();

        g_wifi.ReadCfg();
        g_wifi.Enable();

        g_mqtt.ReadCfg();
        g_mqtt.Enable();

        LittleFS.end();
    }
    else
    {
        DBGLOG( "FS failed" );
    }

    // Enable all buttons and MQTT client:
    EnableAll();

    // Enable OTA FWU:
    ArduinoOTA.onStart(
        []()
        {
            DisableAll();
            if( ArduinoOTA.getCommand() == U_FLASH ) {
                DBGLOG( "OTA flash" );
            } else {
                DBGLOG( "OTA fs" );
            }
        });
    ArduinoOTA.onEnd(
        []()
        {
            DBGLOG( "OTA end" );    // will reboot
        });
    ArduinoOTA.onError(
        []( ota_error_t err )
        {
            EnableAll();
            DBGLOG1( "OTA err %u\n", err );
        });
}

/**
 * Main FW loop
 */
void loop()
{
    // Handle all the network services if connected.
    // Note this may reset the MCU if WIFI conn timeout was configured:
    if( g_wifi.Connected())
    {
        g_wifi.loop();
        g_mqtt.loop();
    }

    // Handle all switches:
    g_swChan0.loop();
    g_swChan1.loop();
    g_swChan2.loop();
}
