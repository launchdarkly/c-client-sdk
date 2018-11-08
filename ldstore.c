/*
 * wrapper for save and restore file like interface
 */

 #include <stdio.h>
 #include <stdlib.h>

 #include "ldapi.h"
 #include "ldinternal.h"

void *store_ctx;
LD_store_opener store_open;
LD_store_stringwriter store_writestring;
LD_store_stringreader store_readstring;
LD_store_closer store_close;

void
LD_store_setfns(void *context, LD_store_opener openfn, LD_store_stringwriter writefn,
    LD_store_stringreader readfn, LD_store_closer closefn)
{
    store_ctx = context;
    store_open = openfn;
    store_writestring = writefn;
    store_readstring = readfn;
    store_close = closefn;
}

void
LDi_savedata(const char *dataname, const char *username, const char *data)
{
    if (!store_writestring)
        return;
    char fullname[1024];
    snprintf(fullname, sizeof(fullname), "%s-%s", dataname, username);
    void *handle = store_open(store_ctx, fullname, "w", strlen(data));
    if (!handle)
        return;
    store_writestring(handle, data);
    store_close(handle);
}

char *
LDi_loaddata(const char *dataname, const char *username)
{
    char *data = NULL;

    if (!store_readstring)
        return NULL;
    char fullname[1024];
    snprintf(fullname, sizeof(fullname), "%s-%s", dataname, username);
    LDi_log(LD_LOG_INFO, "About to open abstract file %s", fullname);
    void *handle = store_open(store_ctx, fullname, "r", 0);
    if (!handle)
        return NULL;
    const char *s = store_readstring(handle);
    if (s)
        data = LDi_strdup(s);
    store_close(handle);
    return data;
}

/*
 * concrete implementation using standard C stdio
 */

struct stdio_store {
    FILE *fp;
    char *s;
};

void *
LD_store_fileopen(void *context, const char *name, const char *mode, size_t len)
{
    struct stdio_store *handle = LDAlloc(sizeof(*handle));
    if (!handle)
        return NULL;
    char filename[1024];
    snprintf(filename, sizeof(filename), "LD-%s.txt", name);
    handle->fp = fopen(filename, mode);
    if (!handle->fp) {
        LDi_log(LD_LOG_ERROR, "Failed to open %s", filename);
        LDFree(handle);
        return NULL;
    }
    handle->s = NULL;
    return handle;
}

bool
LD_store_filewrite(void *h, const char *data)
{
    struct stdio_store *handle = h;
    return fwrite(data, 1, strlen(data), handle->fp) == strlen(data);
}

const char *
LD_store_fileread(void *h)
{
    struct stdio_store *handle = h;
    char *buf;
    size_t bufsize, bufspace, buflen;

    buflen = 0;
    bufspace = bufsize = 4096;
    bufspace--; /* save a byte for nul */
    buf = malloc(bufsize);
    if (!buf)
        return NULL;

    while (true) {
        size_t amt = fread(buf + buflen, 1, bufspace, handle->fp);
        buflen += amt;
        if (amt < bufspace) {
            /* done */
            break;
        } else if (bufspace == 0) {
            /* grow */
            bufspace = bufsize;
            bufsize += bufspace;
            bufspace--;
            void *p = realloc(buf, bufsize);
            if (!p) {
                free(buf);
                return NULL;
            }
            buf = p;
        }
    }
    buf[buflen] = 0;
    handle->s = buf;
    return buf;
}

void
LD_store_fileclose(void *h)
{
    struct stdio_store *handle = h;
    fclose(handle->fp);
    free(handle->s);
    LDFree(handle);
}
