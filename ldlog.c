#include <stdio.h>
#include <stdarg.h>

static void (*logfn)(const char *);
static int loglevel;

void
LD_SetLogFunction(int userlevel, void (userlogfn)(const char *))
{
    logfn = userlogfn;
    loglevel = userlevel;
}

void
LDi_log(int level, const char *fmt, ...)
{
    char buf [2048];
    va_list va;

    if (level > loglevel)
        return;

    va_start(va, fmt);
    vsnprintf(buf, sizeof(buf), fmt, va);
    va_end(va);

    if (logfn)
        logfn(buf);
}