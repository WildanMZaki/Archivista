#ifndef ARCHIVISTA_CONFIG_H
#define ARCHIVISTA_CONFIG_H

#include <windows.h>

#define CONFIG_INI_NAME "config.ini"

// Write/read a string value to/from the configuration file
BOOL Config_WriteString(const char *section, const char *key, const char *value);
DWORD Config_ReadString(const char *section, const char *key, const char *defaultValue, char *outBuffer, DWORD bufferSize);

// Write/read an integer value to/from the configuration file
BOOL Config_WriteInt(const char *section, const char *key, int value);
int Config_ReadInt(const char *section, const char *key, int defaultValue);

#endif // ARCHIVISTA_CONFIG_H
