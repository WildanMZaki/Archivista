#include "../Header/config.h"
#include <stdio.h>

static void GetConfigIniPath(char *outPath) {
    GetModuleFileName(NULL, outPath, MAX_PATH);
    char *lastSlash = strrchr(outPath, '\\');
    if (lastSlash) {
        strcpy(lastSlash + 1, CONFIG_INI_NAME);
    } else {
        strcpy(outPath, CONFIG_INI_NAME);
    }
}

BOOL Config_WriteString(const char *section, const char *key, const char *value) {
    char iniPath[MAX_PATH];
    GetConfigIniPath(iniPath);
    return WritePrivateProfileString(section, key, value, iniPath);
}

DWORD Config_ReadString(const char *section, const char *key, const char *defaultValue, char *outBuffer, DWORD bufferSize) {
    char iniPath[MAX_PATH];
    GetConfigIniPath(iniPath);
    return GetPrivateProfileString(section, key, defaultValue, outBuffer, bufferSize, iniPath);
}

BOOL Config_WriteInt(const char *section, const char *key, int value) {
    char iniPath[MAX_PATH];
    char valStr[32];
    sprintf(valStr, "%d", value);
    GetConfigIniPath(iniPath);
    return WritePrivateProfileString(section, key, valStr, iniPath);
}

int Config_ReadInt(const char *section, const char *key, int defaultValue) {
    char iniPath[MAX_PATH];
    GetConfigIniPath(iniPath);
    return GetPrivateProfileInt(section, key, defaultValue, iniPath);
}
