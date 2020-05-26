#include <stdio.h>
#include <math.h>

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

#include "ldapi.h"
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
static ld_mutex_t LDi_rngmtx;
#endif

void
LDi_initializerng(){
#ifndef _WINDOWS
    LDi_mutex_init(&LDi_rngmtx);
    LDi_rngstate = time(NULL);
#endif
}

bool
LDi_randomhex(char *const buffer, const size_t buffersize)
{
    const char *const alphabet = "0123456789ABCDEF";

    for (size_t i = 0; i < buffersize; i++) {
        unsigned int rng = 0;
        if (LDi_random(&rng)) {
            buffer[i] = alphabet[rng % 16];
        }
        else {
            return false;
        }
    }

    return true;
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

void LDi_once(ld_once_t *once, void (*fn)(void))
{
    InitOnceExecuteOnce(once, OneTimeCaller, fn, NULL);
}
#endif

char *
LDi_usertojsontext(LDClient *const client, LDUser *const user, const bool redact)
{
    cJSON *const jsonuser = LDi_usertojson(client, user, redact);

    if (!jsonuser) {
        LD_LOG(LD_LOG_ERROR, "LDi_usertojson failed in LDi_usertojsontext");
        return NULL;
    }

    char *const textuser = cJSON_PrintUnformatted(jsonuser);
    cJSON_Delete(jsonuser);

    if (!textuser) {
        LD_LOG(LD_LOG_ERROR, "cJSON_PrintUnformatted failed in LDi_usertojsontext");
        return NULL;
    }

    return textuser;
}

/* -1 on error, otherwise read size */
static int
readfile(const char *const path, unsigned char *const buffer, size_t const buffersize)
{
    FILE *const handle = fopen(path, "rb");

    if (!handle) { return -1; };

    if (fseek(handle, 0, SEEK_END)) { fclose(handle); return -1; }

    const long int filesize = ftell(handle);

    if (filesize == -1) { fclose(handle); return -1; }

    if ((size_t)filesize > buffersize) { fclose(handle); return -1; }

    if (fseek(handle, 0, SEEK_SET)) { fclose(handle); return -1; }

    if (fread(buffer, 1, filesize, handle) != (size_t)filesize) {
        fclose(handle); return -1;
    }

    fclose(handle);

    return filesize;
}

char *
LDi_deviceid()
{
  char buffer[256]; memset(buffer, 0, sizeof(buffer));

  #ifdef __linux__
    if (readfile("/var/lib/dbus/machine-id", (unsigned char*)buffer, sizeof(buffer) - 1) == -1) {
        LD_LOG(LD_LOG_ERROR, "LDi_deviceid failed to read /var/lib/dbus/machine-id");
        return NULL;
    }
  #elif _WIN32
    DWORD buffersize = sizeof(buffer) - 1; HKEY hkey; DWORD regtype = REG_SZ;

    const LSTATUS openstatus = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &hkey);

    if (openstatus != ERROR_SUCCESS) {
        LD_LOG(LD_LOG_ERROR, "LDi_deviceid RegOpenKeyExA got %u", openstatus);
        return NULL;
    }

    const LSTATUS querystatus = RegQueryValueExA(hkey, "MachineGuid", NULL, &regtype, (unsigned char*)buffer, &buffersize);

    if (querystatus != ERROR_SUCCESS) {
        RegCloseKey(hkey);
        LD_LOG(LD_LOG_ERROR, "LDi_deviceid RegGetValueA got %u", openstatus);
        return NULL;
    }

    RegCloseKey(hkey);
  #elif __APPLE__
    io_registry_entry_t entry = IORegistryEntryFromPath(kIOMasterPortDefault, "IOService:/");

    if (!entry) {
        LD_LOG(LD_LOG_ERROR, "LDi_deviceid IORegistryEntryFromPath failed");
        return NULL;
    }

    CFStringRef uuid = (CFStringRef)IORegistryEntryCreateCFProperty(entry, CFSTR(kIOPlatformUUIDKey), kCFAllocatorDefault, 0);

    IOObjectRelease(entry);

    if (!uuid) {
        LD_LOG(LD_LOG_ERROR, "LDi_deviceid IORegistryEntryCreateCFProperty failed");
        return NULL;
    }

    if (!CFStringGetCString(uuid, buffer, sizeof(buffer), kCFStringEncodingASCII)) {
        LD_LOG(LD_LOG_ERROR, "LDi_deviceid CFStringGetCString failed");
        CFRelease(uuid); return NULL;
    }

    CFRelease(uuid);
  #elif __FreeBSD__
    if (readfile("/etc/hostid", (unsigned char*)buffer, sizeof(buffer) - 1) == -1) {
        LD_LOG(LD_LOG_ERROR, "LDi_deviceid failed to read /etc/hostid");
        return NULL;
    }
  #else
    return NULL;
  #endif

    return LDStrDup(buffer);
}
