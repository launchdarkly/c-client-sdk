#include <math.h>
#include <stdio.h>

#ifndef _WINDOWS
#include <unistd.h>
#else
/* required for rng setup on windows must be before stdlib */
#define _CRT_RAND_S
#endif

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#include <stdlib.h>

#include <launchdarkly/api.h>

#include "ldinternal.h"

void
LDi_millisleep(int ms)
{
#ifndef _WINDOWS
    ms += 500;
    sleep(ms / 1000);
#else
    Sleep(ms);
#endif
}

#ifndef _WINDOWS
static unsigned int LDi_rngstate;
static ld_mutex_t   LDi_rngmtx;
#endif

void
LDi_initializerng(void)
{
#ifndef _WINDOWS
    LDi_mutex_init(&LDi_rngmtx);
    LDi_rngstate = time(NULL);
#endif
}

LDBoolean
LDi_randomhex(char *const buffer, const size_t buffersize)
{
    size_t            i;
    const char *const alphabet = "0123456789ABCDEF";

    for (i = 0; i < buffersize; i++) {
        unsigned int rng = 0;
        if (LDi_random(&rng)) {
            buffer[i] = alphabet[rng % 16];
        } else {
            return LDBooleanFalse;
        }
    }

    return LDBooleanTrue;
}
/*
 * some functions to help with threads.
 */
#ifdef _WINDOWS

static BOOL CALLBACK
OneTimeCaller(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *lpContext)
{
    void (*fn)(void) = Parameter;

    fn();
    return TRUE;
}

void
LDi_once(ld_once_t *once, void (*fn)(void))
{
    InitOnceExecuteOnce(once, OneTimeCaller, fn, NULL);
}
#endif

#if defined(__linux__) || defined(__FreeBSD__)
/* -1 on error, otherwise read size */
static int
readfile(
    const char *const    path,
    unsigned char *const buffer,
    size_t const         buffersize)
{
    long int filesize;

    FILE *const handle = fopen(path, "rb");

    if (!handle) {
        return -1;
    };

    if (fseek(handle, 0, SEEK_END)) {
        fclose(handle);
        return -1;
    }

    filesize = ftell(handle);

    if (filesize == -1) {
        fclose(handle);
        return -1;
    }

    if ((size_t)filesize > buffersize) {
        fclose(handle);
        return -1;
    }

    if (fseek(handle, 0, SEEK_SET)) {
        fclose(handle);
        return -1;
    }

    if (fread(buffer, 1, filesize, handle) != (size_t)filesize) {
        fclose(handle);
        return -1;
    }

    fclose(handle);

    return filesize;
}
#endif

char *
LDi_deviceid(void)
{
    char buffer[256];

#ifdef __linux__
    memset(buffer, 0, sizeof(buffer));

    if (readfile(
            "/var/lib/dbus/machine-id",
            (unsigned char *)buffer,
            sizeof(buffer) - 1) == -1)
    {
        LD_LOG(
            LD_LOG_ERROR,
            "LDi_deviceid failed to read /var/lib/dbus/machine-id");

        return NULL;
    }
#elif _WIN32
    DWORD   buffersize = sizeof(buffer) - 1;
    HKEY    hkey;
    DWORD   regtype = REG_SZ;
    LSTATUS openstatus, querystatus;

    memset(buffer, 0, sizeof(buffer));

    openstatus = RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\Cryptography",
        0,
        KEY_READ | KEY_WOW64_64KEY,
        &hkey);

    if (openstatus != ERROR_SUCCESS) {
        LD_LOG_1(LD_LOG_ERROR, "LDi_deviceid RegOpenKeyExA got %u", openstatus);

        return NULL;
    }

    querystatus = RegQueryValueExA(
        hkey,
        "MachineGuid",
        NULL,
        &regtype,
        (unsigned char *)buffer,
        &buffersize);

    if (querystatus != ERROR_SUCCESS) {
        RegCloseKey(hkey);

        LD_LOG_1(LD_LOG_ERROR, "LDi_deviceid RegGetValueA got %u", openstatus);

        return NULL;
    }

    RegCloseKey(hkey);
#elif __APPLE__

    /* Before macOS 12 Monterey, this was named Master instead of Main. */
    #if (MAC_OS_X_VERSION_MIN_REQUIRED < 120000)
        #define kIOMainPortDefault kIOMasterPortDefault
    #endif

    io_registry_entry_t entry;
    CFStringRef         uuid;

    memset(buffer, 0, sizeof(buffer));

    entry = IORegistryEntryFromPath(kIOMainPortDefault, "IOService:/");

    if (!entry) {
        LD_LOG(LD_LOG_ERROR, "LDi_deviceid IORegistryEntryFromPath failed");

        return NULL;
    }

    uuid = (CFStringRef)IORegistryEntryCreateCFProperty(
        entry, CFSTR(kIOPlatformUUIDKey), kCFAllocatorDefault, 0);

    IOObjectRelease(entry);

    if (!uuid) {
        LD_LOG(
            LD_LOG_ERROR,
            "LDi_deviceid IORegistryEntryCreateCFProperty failed");

        return NULL;
    }

    if (!CFStringGetCString(
            uuid, buffer, sizeof(buffer), kCFStringEncodingASCII)) {
        LD_LOG(LD_LOG_ERROR, "LDi_deviceid CFStringGetCString failed");

        CFRelease(uuid);
        return NULL;
    }

    CFRelease(uuid);
#elif __FreeBSD__
    memset(buffer, 0, sizeof(buffer));

    if (readfile("/etc/hostid", (unsigned char *)buffer, sizeof(buffer) - 1) ==
        -1) {
        LD_LOG(LD_LOG_ERROR, "LDi_deviceid failed to read /etc/hostid");

        return NULL;
    }
#else
    return NULL;
#endif

    return LDStrDup(buffer);
}
