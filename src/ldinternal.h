#ifndef C_CLIENT_LDINTERNAL_H
#define C_CLIENT_LDINTERNAL_H

#include "cJSON.h"
#ifndef _WINDOWS
#include <pthread.h>
#else
#include <windows.h>
#endif

#include "assertion.h"
#include "client.h"
#include "concurrency.h"
#include "config.h"
#include "utility.h"
#include "sse.h"
#include "event_processor.h"
#include "store.h"
#include "user.h"
#include "logging.h"

#ifndef _WINDOWS

#define ld_once_t pthread_once_t
#define LD_ONCE_INIT PTHREAD_ONCE_INIT
#define LDi_once(once, fn) pthread_once(once, fn)
/* windows */
#else

#define ld_once_t INIT_ONCE
#define LD_ONCE_INIT INIT_ONCE_STATIC_INIT
void LDi_once(ld_once_t *once, void (*fn)(void));
#endif

unsigned char * LDi_base64_encode(const unsigned char *src, size_t len,
	size_t *out_len);

void LDi_cancelread(int handle);
char *LDi_fetchfeaturemap(struct LDClient *client, int *response);

void LDi_readstream(struct LDClient *const client, int *response,
    struct LDSSEParser *const parser,
    void cbhandle(struct LDClient *client, int handle));

void LDi_sendevents(struct LDClient *client, const char *eventdata,
    const char *const payloadUUID, int *response);

void LDi_reinitializeconnection(struct LDClient *client);
void LDi_startstopstreaming(struct LDClient *client, bool stopstreaming);
void LDi_onstreameventput(struct LDClient *client, const char *data);
void LDi_onstreameventpatch(struct LDClient *client, const char *data);
void LDi_onstreameventdelete(struct LDClient *client, const char *data);

extern void (*LDi_statuscallback)(int);

void LDi_millisleep(int ms);
/* must be called exactly once before rng is used */
void LDi_initializerng();
/* returns true on success, may leave buffer dirty */
bool LDi_randomhex(char *buffer, size_t buffersize);
/* returns a unique device identifier, returns NULL on failure */
char *LDi_deviceid();

/* calls into the store interface */
void LDi_savedata(const char *dataname, const char *username, const char *data);
char *LDi_loaddata(const char *dataname, const char *username);

extern ld_mutex_t LDi_allocmtx;
extern ld_once_t LDi_earlyonce;
void LDi_earlyinit(void);

/* expects caller to own LDi_clientlock */
void LDi_updatestatus(struct LDClient *client, const LDStatus status);

THREAD_RETURN LDi_bgeventsender(void *const v);
THREAD_RETURN LDi_bgfeaturepoller(void *const v);
THREAD_RETURN LDi_bgfeaturestreamer(void *const v);

double LDi_calculateStreamDelay(const unsigned int retries);

#endif /* C_CLIENT_LDINTERNAL_H */
