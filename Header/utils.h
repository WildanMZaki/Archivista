#ifndef UTILS_H
#define UTILS_H

#include <windows.h>
#include <winver.h>

static inline int ClampInt(int value, int minValue, int maxValue)
{
    if (value < minValue)
        return minValue;
    if (value > maxValue)
        return maxValue;
    return value;
}

static inline void GetAppVersion(char *buffer, size_t bufferSize) {

    if (!buffer || bufferSize == 0)
        return;

    lstrcpynA(buffer, "Unknown", (int)bufferSize);

    char exePath[MAX_PATH];

    if (!GetModuleFileNameA(NULL, exePath, MAX_PATH))
        return;

    DWORD dummy = 0;
    DWORD versionInfoSize = GetFileVersionInfoSizeA(exePath, &dummy);

    if (versionInfoSize == 0)
        return;

    BYTE *versionData = (BYTE *)malloc(versionInfoSize);
    if (!versionData)
        return;

    if (!GetFileVersionInfoA(exePath, 0, versionInfoSize, versionData))
    {
        free(versionData);
        return;
    }

    char *productVersion = NULL;
    UINT length = 0;

    if (VerQueryValueA(
            versionData,
            "\\StringFileInfo\\040904E4\\ProductVersion",
            (LPVOID *)&productVersion,
            &length))
    {
        lstrcpynA(buffer, productVersion, (int)bufferSize);
    }

    free(versionData);
}

#endif // UTILS_H
