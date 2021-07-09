#pragma once

#include <stddef.h>

#include <launchdarkly/boolean.h>

typedef LDBoolean (*ld_sse_dispatch)(
    const char *const name, const char *const body, void *const context);

struct LDSSEParser
{
    char *          buffer;
    size_t          bufferSize;
    char *          eventName;
    char *          eventBody;
    ld_sse_dispatch dispatch;
    void *          context;
};

void
LDSSEParserInitialize(
    struct LDSSEParser *const parser,
    ld_sse_dispatch           dispatch,
    void *const               context);

void
LDSSEParserDestroy(struct LDSSEParser *const parser);

LDBoolean
LDSSEParserProcess(
    struct LDSSEParser *const parser,
    const void *const         buffer,
    const size_t              bufferSize);
