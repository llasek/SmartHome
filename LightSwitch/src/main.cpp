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

CManualSwitch g_swLight1;
CManualSwitch g_swLight2;
CManualSwitch g_swLight3;

void setup() {
    Serial.begin( 115200 );
    Serial.println();

    if( LittleFS.begin()) {
        File file = CManualSwitch::OpenCfg();
        g_swLight1.ReadCfg( file );
        g_swLight2.ReadCfg( file );
        g_swLight3.ReadCfg( file );
        file.close();

        g_wifi.ReadCfg();
        g_wifi.Enable();

        g_mqtt.ReadCfg();
        g_mqtt.Enable();

        LittleFS.end();
    } else {
        DBGLOG( "FS failed" );
    }

    g_swLight1.Enable( PIN_IN1, PIN_OUT1 );
    g_swLight2.Enable( PIN_IN2, PIN_OUT2 );
    g_swLight3.Enable( PIN_IN3, PIN_OUT3 );

    // ArduinoOTA.onStart( []() {
    //     if( ArduinoOTA.getCommand() == U_FLASH ) {
    //         DBGLOG( "OTA flash" );
    //     } else {
    //         DBGLOG( "OTA fs" );
    //     }
    // });
    // ArduinoOTA.onEnd( []() {
    //     DBGLOG( "OTA end" );
    // });
    // ArduinoOTA.onProgress( []( uint prog, uint tot ) {
    // });
    // ArduinoOTA.onError( []( ota_error_t err ) {
    //     DBGLOG1( "OTA err %u\n", err );
    // });
}

void loop()
{
    if( g_wifi.Connected()) {
        g_wifi.loop();
        g_mqtt.loop();
    }
    g_swLight1.loop();
    g_swLight2.loop();
    g_swLight3.loop();
}
