/*
 * wrapper for save and restore file like interface
 */

 #include <stdio.h>
 #include <stdlib.h>

 #include "ldapi.h"
 #include "ldinternal.h"

void *filer_ctx;
LD_filer_opener filer_open;
LD_filer_stringwriter filer_writestring;
LD_filer_stringreader filer_readstring;
LD_filer_closer filer_close;

void
LD_filer_setfns(void *context, LD_filer_opener openfn, LD_filer_stringwriter writefn,
    LD_filer_stringreader readfn, LD_filer_closer closefn)
{
    filer_ctx = context;
    filer_open = openfn;
    filer_writestring = writefn;
    filer_readstring = readfn;
    filer_close = closefn;
}

void
LDi_savedata(const char *dataname, const char *username, const char *data)
{
    if (!filer_writestring)
        return;
    char fullname[1024];
    snprintf(fullname, sizeof(fullname), "%s-%s", dataname, username);
    void *handle = filer_open(filer_ctx, fullname, "w", strlen(data));
    if (!handle)
        return;
    filer_writestring(handle, data);
    filer_close(handle);
}

char *
LDi_loaddata(const char *dataname, const char *username)
{
    char *data = NULL;
    
    if (!filer_readstring)
        return NULL;
    char fullname[1024];
    snprintf(fullname, sizeof(fullname), "%s-%s", dataname, username);
    LDi_log(15, "About to open abstract file %s\n", fullname);
    void *handle = filer_open(filer_ctx, fullname, "r", 0);
    if (!handle)
        return NULL;
    const char *s = filer_readstring(handle);
    if (s)
        data = LDi_strdup(s);
    filer_close(handle);
    return data;
}

/*
 * concrete implementation using standard C stdio
 */

struct stdio_filer {
    FILE *fp;
    char *s;
};

void *
LD_filer_fileopen(void *context, const char *name, const char *mode, size_t len)
{
    struct stdio_filer *handle = LDAlloc(sizeof(*handle));
    if (!handle)
        return NULL;
    char filename[1024];
    snprintf(filename, sizeof(filename), "LD-%s.txt", name);
    handle->fp = fopen(filename, mode);
    if (!handle->fp) {
        LDi_log(10, "Failed to open %s\n", filename);
        LDFree(handle);
        return NULL;
    }
    handle->s = NULL;
    return handle;
}

bool
LD_filer_filewrite(void *h, const char *data)
{
    struct stdio_filer *handle = h;
    return fwrite(data, 1, strlen(data), handle->fp) == strlen(data);
}

const char *
LD_filer_fileread(void *h)
{
    struct stdio_filer *handle = h;
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
    LDi_log(15, "Read %ld bytes of string\n", (long)buflen);
    return buf;
}

void
LD_filer_fileclose(void *h)
{
    struct stdio_filer *handle = h;
    fclose(handle->fp);
    free(handle->s);
    LDFree(handle);
}