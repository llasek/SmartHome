/**
 * Utilities library
 * 2021 Łukasz Łasek
 */
#pragma once
#include <Arduino.h>
#include <LittleFS.h>

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
bool StringEq( const char* a_psz, uint a_nLen, byte* payload, uint len );

/**
 * Compare a string and byte buffer
 * 
 * @param[in]   a_rstr  String to compare
 * @param[in]   payload Byte buffer to compare
 * @param[in]   len     Length of payload in bytes
 * 
 * @return  true if equal
 */
bool StringEq( String& a_rstr, byte* payload, uint len );

/**
 * Check a prefix at a start of a payload buffer
 * 
 * @param[in]   a_rstrPrefix    Prefix string
 * @param[in]   payload         Byte buffer to begin with the prefix
 * @param[in]   len             Lengt of payload in bytes
 * 
 * @return  true if payload begins with the prefix
 */
bool StringBeginsWith( String& a_rstrPrefix, byte* payload, uint len );

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
bool StringBeginsWith( const char* a_psz, uint a_nLen, byte* payload, uint len );

/**
 * Convert a string into a base 10 uint16
 * 
 * @param[in]   payload     Input string
 * @param[in]   len         Length of the input string
 * 
 * @return  Base 10 uint16 value
 */
uint16_t AtoU16_10( byte *payload, uint len );

/**
 * Convert a nibble char (i.e. half a byte, base 16) into a value
 * 
 * @param[in]   a_cNibble   Input nibble character
 * 
 * @return  Byte value
 */
byte NibbleToU8_16( char a_cNibble );

/**
 * Convert a value to a nibble character
 * 
 * @param[in]   a_nNibble   Nibble value
 * 
 * @return  Nibble character
 */
const char U8ToNibble_16( byte a_nNibble );

/**
 * Read a value from the config file.
 * 
 * Find a line containing the specified name/description. The line always begins with a '// ' prefix in the file.
 * Read the corresponding configuration entry value on the next line.
 * Return default if entry not found.
 * 
 * @param[in]   a_rFile         Opened configuration file
 * @param[in]   a_pszName       The beginning of a value name/description entry
 * @param[in]   a_pszDefault    The default value returned if no specified entry found
 * 
 * @return  The configuration value read
 */
String CfgFileReadLine( File& a_rFile, const char* a_pszName, const char* a_pszDefault = "" );
