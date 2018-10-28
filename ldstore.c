/*
 * wrapper for save and restore file like interface
 */

 #include <stdio.h>
 #include <stdlib.h>

 #include "ldapi.h"
 #include "ldinternal.h"

void
LD_store_setfns(LDClient *const client, void *const context, LD_store_opener openfn,
    LD_store_stringwriter writefn, LD_store_stringreader readfn, LD_store_closer closefn)
{
    client->store_ctx = context;
    client->store_open = openfn;
    client->store_writestring = writefn;
    client->store_readstring = readfn;
    client->store_close = closefn;
}

void
LDi_savedata(LDClient *const client, const char *const dataname, const char *const username, const char *const data)
{
    if (!client->store_writestring) { return; }

    char fullname[1024];
    snprintf(fullname, sizeof(fullname), "%s-%s", dataname, username);

    void *const handle = client->store_open(client->store_ctx, fullname, "w", strlen(data));
    if (!handle) { return; }

    client->store_writestring(handle, data);
    client->store_close(handle);
}

char *
LDi_loaddata(LDClient *const client, const char *const dataname, const char *const username)
{
    if (!client->store_readstring) { return NULL; }

    char fullname[1024];
    snprintf(fullname, sizeof(fullname), "%s-%s", dataname, username);
    LDi_log(LD_LOG_INFO, "About to open abstract file %s\n", fullname);

    void *const handle = client->store_open(client->store_ctx, fullname, "r", 0);
    if (!handle) { return NULL; }

    const char *const s = client->store_readstring(handle);

    client->store_close(handle);

    if (s) {
        return LDi_strdup(s);
    }
    else {
        return NULL;
    }
}

/*
 * concrete implementation using standard C stdio
 */

struct stdio_store {
    FILE *fp;
    char *s;
};

void *
LD_store_fileopen(LDClient *const client, void *const context,
    const char *const name, const char *const mode, const size_t len)
{
    struct stdio_store *const handle = LDAlloc(sizeof(*handle));
    if (!handle) { return NULL; }

    char filename[1024];
    snprintf(filename, sizeof(filename), "LD-%s.txt", name);

    handle->fp = fopen(filename, mode);
    if (!handle->fp) {
        LDi_log(LD_LOG_ERROR, "Failed to open %s\n", filename);
        LDFree(handle);
        return NULL;
    }

    handle->s = NULL;
    return handle;
}

bool
LD_store_filewrite(LDClient *const client, void *const h, const char *const data)
{
    struct stdio_store *const handle = h;
    return fwrite(data, 1, strlen(data), handle->fp) == strlen(data);
}

const char *
LD_store_fileread(LDClient *const client, void *const h)
{
    struct stdio_store *const handle = h;
    size_t bufsize = 4069, bufspace = 4069, buflen = 0;

    bufspace--; /* save a byte for nul */

    char *buf = malloc(bufsize);

    if (!buf) { return NULL; }

    while (true) {
        const size_t amt = fread(buf + buflen, 1, bufspace, handle->fp);

        buflen += amt;

        if (amt < bufspace) {
            /* done */
            break;
        } else if (bufspace == 0) {
            /* grow */
            bufspace = bufsize;
            bufsize += bufspace;
            bufspace--;

            void *const p = realloc(buf, bufsize);
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
LD_store_fileclose(LDClient *const client, void *const h)
{
    struct stdio_store *const handle = h;
    fclose(handle->fp);
    free(handle->s);
    LDFree(handle);
}
