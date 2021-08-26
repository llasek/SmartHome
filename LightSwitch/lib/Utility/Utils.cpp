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

bool StringBeginsWith( const char* a_psz, uint a_nLen, byte* payload, uint len )
{
    return(( a_nLen <= len ) && ( !memcmp( a_psz, payload, a_nLen )));
}

bool StringBeginsWith( String& a_rstrPrefix, byte* payload, uint len )
{
    uint nStrLen = a_rstrPrefix.length();
    return(( nStrLen <= len ) && ( !memcmp( a_rstrPrefix.c_str(), payload, nStrLen )));
}

uint16_t AtoU16_10( byte *payload, uint len )
{
    uint16_t nRet = 0;
    for( uint nIdx = 0; nIdx < len; nIdx++ )
    {
        byte b = payload[ nIdx ];
        if(( b >= '0' ) && ( b <= '9' ))
        {
            nRet *= 10;
            nRet += ( b - '0' );
        }
        else
        {
            nRet = 0;
            break;
        }
    }
    return nRet;
}

byte NibbleToU8_16( char a_cNibble )
{
    if( a_cNibble <= '9' )
    {
        a_cNibble -= '0';
    }
    else if( a_cNibble <= 'F' )
    {
        a_cNibble -= 'A' - 0xa;
    }
    else
    {
        a_cNibble -= 'a' - 0xa;
    }
    return a_cNibble;
}

const char U8ToNibble_16( byte a_nNibble )
{
    return "0123456789abcdef"[ a_nNibble & 0xf ];
}

String CfgFileReadLine( File& a_rFile )
{
    a_rFile.readStringUntil( '\n' );
    a_rFile.readStringUntil( '\n' );
    return a_rFile.readStringUntil( '\n' );
}
