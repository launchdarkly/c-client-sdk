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
#endif

#include <stdlib.h>

#include "ldapi.h"
#include "ldinternal.h"

/*
 * set a string value, copying the memory, and freeing the old.
 */
bool
LDSetString(char **const target, const char *const value)
{
    if (target) {
        LDFree(*target);
        if (value) {
            *target = LDi_strdup(value);
            if (*target) {
                return true;
            }
        } else {
            *target = NULL;
            return true;
        }
    }
    return false;
}

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
    LDi_mtxinit(&LDi_rngmtx);
    LDi_rngstate = time(NULL);
#endif
};

bool
LDi_random(unsigned int *const result)
{
    if(!result) { return false; }

#ifndef _WINDOWS
    LDi_mtxenter(&LDi_rngmtx);
    const int generated = rand_r(&LDi_rngstate);
    LDi_mtxleave(&LDi_rngmtx);
#else
    unsigned int generated;
    if (rand_s(&generated)) {
        return false;
    }
#endif

    *result = generated;
    return true;
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

ld_mutex_t LDi_allocmtx;
unsigned long long LD_allocations;
unsigned long long LD_frees;

void *
LDAlloc(size_t amt)
{
    void *v = malloc(amt);
    if (v) {
        LDi_mtxenter(&LDi_allocmtx);
        LD_allocations++;
        LDi_mtxleave(&LDi_allocmtx);
    }
    return v;
}

void
LDFree(void *v)
{
    if (v) {
        LDi_mtxenter(&LDi_allocmtx);
        LD_frees++;
        LDi_mtxleave(&LDi_allocmtx);
        free(v);
    }
}


char *
LDi_strdup(const char *src)
{
    char *cp = strdup(src);
    if (cp) {
        LDi_mtxenter(&LDi_allocmtx);
        LD_allocations++;
        LDi_mtxleave(&LDi_allocmtx);
    }
    return cp;
}


/*
 * some functions to help with threads.
 */
#ifndef _WINDOWS
/* posix */
void
LDi_condwait(pthread_cond_t *cond, pthread_mutex_t *mtx, int ms)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += ms / 1000;
    ts.tv_nsec += (ms % 1000) * 1000 * 1000;
    if (ts.tv_nsec > 1000 * 1000 * 1000) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000 * 1000 * 1000;
    }

    pthread_cond_timedwait(cond, mtx, &ts);
}

void
LDi_condsignal(pthread_cond_t *cond)
{
    pthread_cond_broadcast(cond);
}
#else
/* windows */
void
LDi_condwait(CONDITION_VARIABLE *cond, CRITICAL_SECTION *mtx, int ms)
{
    SleepConditionVariableCS(cond, mtx, ms);

}

void
LDi_condsignal(CONDITION_VARIABLE *cond)
{
    WakeAllConditionVariable(cond);
}

void
LDi_createthread(HANDLE *thread, LPTHREAD_START_ROUTINE fn, void *arg)
{
    DWORD id;
    *thread = CreateThread(NULL, 0, fn, arg, 0, &id);
}

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
        LDi_log(LD_LOG_ERROR, "LDi_usertojson failed in LDi_usertojsontext\n");
        return NULL;
    }

    char *const textuser = cJSON_PrintUnformatted(jsonuser);
    cJSON_Delete(jsonuser);

    if (!textuser) {
        LDi_log(LD_LOG_ERROR, "cJSON_PrintUnformatted failed in LDi_usertojsontext\n");
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
    if (readfile("/var/lib/dbus/machine-id", (unsigned char*)buffer, sizeof(buffer - 1)) == -1) {
        LDi_log(LD_LOG_ERROR, "LDi_deviceid failed to read /var/lib/dbus/machine-id\n");
        return NULL;
    }
  #elif _WIN32
    DWORD buffersize = sizeof(buffer) - 1; HKEY hkey; DWORD regtype = REG_SZ;

    const LSTATUS openstatus = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &hkey);

    if (openstatus != ERROR_SUCCESS) {
        LDi_log(LD_LOG_ERROR, "LDi_deviceid RegOpenKeyExA got %u\n", openstatus);
        return NULL;
    }

    const LSTATUS querystatus = RegQueryValueExA(hkey, "MachineGuid", NULL, &regtype, buffer, &buffersize);

    if (querystatus != ERROR_SUCCESS) {
        RegCloseKey(hkey);
        LDi_log(LD_LOG_ERROR, "LDi_deviceid RegGetValueA got %u\n", openstatus);
        return NULL;
    }

    RegCloseKey(hkey);
  #elif __APPLE__
    io_registry_entry_t entry = IORegistryEntryFromPath(kIOMasterPortDefault, "IOService:/");

    if (!entry) {
        LDi_log(LD_LOG_ERROR, "LDi_deviceid IORegistryEntryFromPath failed\n");
        return NULL;
    }

    CFStringRef uuid = (CFStringRef)IORegistryEntryCreateCFProperty(entry, CFSTR(kIOPlatformUUIDKey), kCFAllocatorDefault, 0);

    IOObjectRelease(entry);

    if (!uuid) {
        LDi_log(LD_LOG_ERROR, "LDi_deviceid IORegistryEntryCreateCFProperty failed\n");
        return NULL;
    }

    if (!CFStringGetCString(uuid, buffer, sizeof(buffer), kCFStringEncodingASCII)) {
        LDi_log(LD_LOG_ERROR, "LDi_deviceid CFStringGetCString failed\n");
        CFRelease(uuid); return NULL;
    }

    CFRelease(uuid);
  #elif __FreeBSD__
    if (readfile("/etc/hostid", (unsigned char*)buffer, sizeof(buffer) - 1) == -1) {
        LDi_log(LD_LOG_ERROR, "LDi_deviceid failed to read /etc/hostid\n");
        return NULL;
    }
  #else
    return NULL;
  #endif

    return LDi_strdup(buffer);
};
