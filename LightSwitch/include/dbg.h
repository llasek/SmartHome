/**
 * DBG header
 * 2021 Łukasz Łasek
 */
#pragma once
#include <Arduino.h>

#ifdef DBG
    #define DBGLOG_( msg ) Serial.print( msg )
    #define DBGLOG( msg ) Serial.println( msg )
    #define DBGLOG1( fmt, arg1 ) Serial.printf( fmt, arg1 )
    #define DBGLOG2( fmt, arg1, arg2 ) Serial.printf( fmt, arg1, arg2 )
    #define DBGLOG3( fmt, arg1, arg2, arg3 ) Serial.printf( fmt, arg1, arg2, arg3 )
    #define DBGLOG4( fmt, arg1, arg2, arg3, arg4 ) Serial.printf( fmt, arg1, arg2, arg3, arg4 )
    #define DBGLOG5( fmt, arg1, arg2, arg3, arg4, arg5 ) Serial.printf( fmt, arg1, arg2, arg3, arg4, arg5 )

static inline void DbgLogSetup()
{
    Serial.begin( 115200 );
    Serial.println();
}
#else
    #define DBGLOG_( msg )
    #define DBGLOG( msg )
    #define DBGLOG1( fmt, arg1 )
    #define DBGLOG2( fmt, arg1, arg2 )
    #define DBGLOG3( fmt, arg1, arg2, arg3 )
    #define DBGLOG4( fmt, arg1, arg2, arg3, arg4 )
    #define DBGLOG5( fmt, arg1, arg2, arg3, arg4, arg5 )

static inline void DbgLogSetup()
{
}
#endif
