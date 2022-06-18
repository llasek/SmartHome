/**
 * Utilities library
 * 2021-2022 Łukasz Łasek
 */
#pragma once
#include <Arduino.h>

class CStringUtils
{
public:
    /**
     * Compare a string and byte buffer
     * 
     * @param[in]   a_psz   String to compare
     * @param[in]   a_nLen  Length of a_psz in bytes
     * @param[in]   payload Byte buffer to compare
     * @param[in]   len     Length of payload in bytes
     * 
     * @return  true if equal
     */
    static bool IsEqual( const char* a_psz, uint a_nLen, byte* payload, uint len )
    {
        return(( a_nLen == len ) && ( !memcmp( a_psz, payload, len )));
    }

    /**
     * Compare a string and byte buffer
     * 
     * @param[in]   a_rstr  String to compare
     * @param[in]   payload Byte buffer to compare
     * @param[in]   len     Length of payload in bytes
     * 
     * @return  true if equal
     */
    static bool IsEqual( String& a_rstr, byte* payload, uint len )
    {
        return IsEqual( a_rstr.c_str(), a_rstr.length(), payload, len );
    }

    /**
     * Check a prefix at a start of a payload buffer
     * 
     * @param[in]   a_rstrPrefix    Prefix string
     * @param[in]   payload         Byte buffer to begin with the prefix
     * @param[in]   len             Lengt of payload in bytes
     * 
     * @return  true if payload begins with the prefix
     */
    static bool BeginsWith( String& a_rstrPrefix, byte* payload, uint len )
    {
        uint nStrLen = a_rstrPrefix.length();
        return(( nStrLen <= len ) && ( !memcmp( a_rstrPrefix.c_str(), payload, nStrLen )));
    }

    /**
     * Check a prefix at a start of a payload buffer
     * 
     * @param[in]   a_psz           Prefix string
     * @param[in]   a_nLen          Length of the prefix string
     * @param[in]   payload         Byte buffer to begin with the prefix
     * @param[in]   len             Lengt of payload in bytes
     * 
     * @return  true if payload begins with the prefix
     */
    static bool BeginsWith( const char* a_psz, uint a_nLen, byte* payload, uint len )
    {
        return(( a_nLen <= len ) && ( !memcmp( a_psz, payload, a_nLen )));
    }

    /**
     * Convert a string into a base 10 uint16
     * 
     * @param[in]   payload     Input string
     * @param[in]   len         Length of the input string
     * 
     * @return  Base 10 uint16 value
     */
    static uint16_t AtoU16_10( byte *payload, uint len )
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

    /**
     * Convert a nibble char (i.e. half a byte, base 16) into a value
     * 
     * @param[in]   a_cNibble   Input nibble character
     * 
     * @return  Byte value
     */
    static byte NibbleToU8_16( char a_cNibble )
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

    /**
     * Convert a value to a nibble character
     * 
     * @param[in]   a_nNibble   Nibble value
     * 
     * @return  Nibble character
     */
    static const char U8ToNibble_16( byte a_nNibble )
    {
        return "0123456789abcdef"[ a_nNibble & 0xf ];
    }
};
