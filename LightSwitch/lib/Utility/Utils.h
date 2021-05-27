/**
 * Utilities library
 * 2021 Łukasz Łasek
 */
#pragma once
#include <Arduino.h>

bool StringEq( const char* a_psz, uint a_nLen, byte* payload, uint len );
bool StringEq( String& a_rstr, byte* payload, uint len );
bool StringAt0( String& a_rstr, byte* payload, uint len );
