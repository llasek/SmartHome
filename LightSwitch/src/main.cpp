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
    g_swChan0.Enable();
    g_swChan1.Enable();
    g_swChan2.Enable();
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
        g_swChan0.ReadCfg( 0 );
        g_swChan1.ReadCfg( 1 );
        g_swChan2.ReadCfg( 2 );

        g_wifi.ReadCfg();
        g_mqtt.ReadCfg();

        LittleFS.end();

        g_wifi.Enable();
        g_mqtt.Enable();
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
