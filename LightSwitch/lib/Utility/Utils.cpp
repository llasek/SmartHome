/**
 * Utilities library
 * 2021 Łukasz Łasek
 */
#include "Utils.h"

bool StringEq( const char* a_psz, uint a_nLen, byte* payload, uint len )
{
    return(( a_nLen == len ) && ( !memcmp( a_psz, payload, len )));
}

bool StringEq( String& a_rstr, byte* payload, uint len )
{
    return StringEq( a_rstr.c_str(), a_rstr.length(), payload, len );
}

bool StringAt0( String& a_rstr, byte* payload, uint len )
{
    uint nStrLen = a_rstr.length();
    return(( nStrLen <= len ) && ( !memcmp( a_rstr.c_str(), payload, nStrLen )));
}

String CfgFileReadLine( File& a_rFile )
{
    a_rFile.readStringUntil( '\n' );
    a_rFile.readStringUntil( '\n' );
    return a_rFile.readStringUntil( '\n' );
}
