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
