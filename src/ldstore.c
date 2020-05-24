/*
 * wrapper for save and restore file like interface
 */

 #include <stdio.h>
 #include <stdlib.h>

 #include "ldapi.h"
 #include "ldinternal.h"

void *store_ctx = NULL;
LD_store_stringwriter store_writestring = NULL;
LD_store_stringreader store_readstring = NULL;

void
LD_store_setfns(void *const context, LD_store_stringwriter writefn,
    LD_store_stringreader readfn)
{
    store_ctx = context;
    store_writestring = writefn;
    store_readstring = readfn;
}

void
LDi_savedata(const char *const dataname, const char *const username, const char *const data)
{
    if (!store_writestring) { return; }

    char fullname[1024];

    if (snprintf(fullname, sizeof(fullname), "%s-%s", dataname, username) < 0) {
        LD_LOG(LD_LOG_ERROR, "LDi_savedata snprintf failed"); return;
    }

    LD_LOG_1(LD_LOG_INFO, "About to open abstract file %s", fullname);

    store_writestring(store_ctx, fullname, data);
}

char *
LDi_loaddata(const char *dataname, const char *username)
{
    if (!store_readstring) { return NULL; }

    char fullname[1024];

    if (snprintf(fullname, sizeof(fullname), "%s-%s", dataname, username) < 0) {
        LD_LOG(LD_LOG_ERROR, "LDi_loaddata snprintf failed"); return NULL;
    }

    LD_LOG_1(LD_LOG_INFO, "About to open abstract file %s", fullname);

    return store_readstring(store_ctx, fullname);
}

/*
 * concrete implementation using standard C stdio
 */

static FILE *
fileopen(const char *const name, const char *const mode)
{
    char filename[1024];

    if (snprintf(filename, sizeof(filename), "LD-%s.txt", name) < 0) {
        LD_LOG(LD_LOG_ERROR, "fileopen snprintf failed"); return NULL;
    }

    FILE *const handle = fopen(filename, mode);

    if (!handle) {
        LD_LOG_1(LD_LOG_ERROR, "Failed to open %s", filename);
        return NULL;
    }

    return handle;
}

bool
LD_store_filewrite(void *const context, const char *const name, const char *const data)
{
    FILE *const handle = fileopen(name, "w");

    if (!handle) { return false; }

    const size_t len = strlen(data);

    const bool success = fwrite(data, 1, len, handle) == len;

    fclose(handle);

    return success;
}

char *
LD_store_fileread(void *const context, const char *const name)
{
    FILE *const handle = fileopen(name, "r");

    if (!handle) { return NULL; }

    size_t bufsize = 4096, bufspace = 4095, buflen = 0;

    char *buf = LDAlloc(bufsize);

    if (!buf) { return NULL; }

    while (true) {
        const size_t amt = fread(buf + buflen, 1, bufspace, handle);

        buflen += amt;
        if (amt < bufspace) {
            /* done */
            break;
        } else if (bufspace == 0) {
            /* grow */
            bufspace = bufsize;
            bufsize += bufspace;
            bufspace--;

            void *const p = LDRealloc(buf, bufsize);

            if (!p) { LDFree(buf); return NULL; }

            buf = p;
        }
    }

    buf[buflen] = 0;

    fclose(handle);

    return buf;
}
