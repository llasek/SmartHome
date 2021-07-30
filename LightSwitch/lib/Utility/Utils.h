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
 * Read the next value from config file.
 * 
 * The config file layout is defined to contain a value every 3 lines.
 * The first 2 lines are skipped as a separator or comment.
 * The 3rd line is being returned as value read.
 * 
 * @param[in]   a_rFile     Opened configuration file
 * 
 * @return  the next configuration value read
 */
String CfgFileReadLine( File& a_rFile );
