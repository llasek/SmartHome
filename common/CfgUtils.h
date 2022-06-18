/**
 * Utilities library
 * 2021-2022 Łukasz Łasek
 */
#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include "StringUtils.h"

class CConfigUtils {
public:
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
    static String ReadValue( File& a_rFile, const char* a_pszName, const char* a_pszDefault = "" )
    {
        String strPrefix( a_pszName );
        a_rFile.seek( 0 );
        while( a_rFile.available())
        {
            String str = a_rFile.readStringUntil( '\n' );
            // Skip first 3 characters: '// '
            if(( str.length() > 3 ) && ( CStringUtils::BeginsWith( strPrefix, (byte*)str.c_str() + 3, str.length())))
            {
                return a_rFile.readStringUntil( '\n' );
            }
        }
        return a_pszDefault;
    }
};
