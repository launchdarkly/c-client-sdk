#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include <launchdarkly/memory.h>

#include "assertion.h"

#include "sse.h"

void
LDSSEParserInitialize(struct LDSSEParser *const parser,
    ld_sse_dispatch dispatch, void *const context)
{
    LD_ASSERT(parser);
    LD_ASSERT(dispatch);

    parser->buffer     = NULL;
    parser->bufferSize = 0;
    parser->eventName  = NULL;
    parser->eventBody  = NULL;
    parser->dispatch   = dispatch;
    parser->context    = context;
}

void
LDSSEParserDestroy(struct LDSSEParser *const parser)
{
    if (parser) {
        LDFree(parser->buffer);
        LDFree(parser->eventName);
        LDFree(parser->eventBody);

        parser->buffer    = NULL;
        parser->eventName = NULL;
        parser->eventBody = NULL;
    }
}

static bool
LDi_processLine(struct LDSSEParser *const parser, const char *line)
{
    LD_ASSERT(parser);
    LD_ASSERT(line);

    if (line[0] == ':') {
        /* skip comment */
    } else if (line[0] == '\0') {
        bool status;
        /* dispatch */
        if (parser->eventName == NULL) {
            LD_LOG(LD_LOG_WARNING, "SSE dispatch with NULL event name");

            status = true;
        } else if (parser->eventBody == NULL) {
            LD_LOG(LD_LOG_WARNING, "SSE dispatch with NULL event body");

            status = true;
        } else {
            LD_ASSERT(parser->dispatch);

            status = parser->dispatch(parser->eventName, parser->eventBody,
                parser->context);
        }

        LDFree(parser->eventName);
        LDFree(parser->eventBody);

        parser->eventName = NULL;
        parser->eventBody = NULL;

        if (status == false) {
            return false;
        }
    } else if (strncmp(line, "data:", 5) == 0) {
        char *eventBodyTmp;
        size_t lineSize, currentBodySize;
        bool notEmpty;

        line += 5;
        line += line[0] == ' ';

        lineSize = strlen(line);
        notEmpty = parser->eventBody != NULL;

        if (notEmpty) {
            currentBodySize = strlen(parser->eventBody);
        } else {
            currentBodySize = 0;
        }

        eventBodyTmp = (char *)LDRealloc(
            parser->eventBody, lineSize + currentBodySize + notEmpty + 1);

        if (eventBodyTmp == NULL) {
            return false;
        }

        parser->eventBody = eventBodyTmp;

        if (notEmpty) {
            parser->eventBody[currentBodySize] = '\n';
        }

        memcpy(parser->eventBody + currentBodySize + notEmpty, line, lineSize);

        parser->eventBody[currentBodySize + notEmpty + lineSize] = '\0';
    } else if (strncmp(line, "event:", 6) == 0) {
        /* skip prefix and optional space*/
        line += 6;
        line += line[0] == ' ';

        if (parser->eventName) {
            LDFree(parser->eventName);
        }

        parser->eventName = LDStrDup(line);

        if (parser->eventName == NULL) {
            return false;
        }
    }

    return true;
}

bool
LDSSEParserProcess(struct LDSSEParser *const parser,
    const void *const buffer, const size_t bufferSize)
{
    void *bufferTmp;
    char *newLineLocation;
    size_t consumed;

    LD_ASSERT(parser);

    if (bufferSize == 0) {
        return true;
    }

    LD_ASSERT(buffer);

    bufferTmp = LDRealloc(parser->buffer, parser->bufferSize + bufferSize + 1);

    if (bufferTmp == NULL) {
        return false;
    }

    parser->buffer = bufferTmp;
    consumed       =  0;

    memcpy(&(parser->buffer[parser->bufferSize]), buffer, bufferSize);

    parser->bufferSize                 += bufferSize;
    parser->buffer[parser->bufferSize]  = '\0';

    while ((newLineLocation = (char *)memchr(parser->buffer + consumed, '\n',
        parser->bufferSize - consumed)))
    {
        *newLineLocation = '\0';

        if (!LDi_processLine(parser, parser->buffer + consumed)) {
            return false;
        }

        consumed = newLineLocation - parser->buffer + 1;
    }

    if (consumed) {
        parser->bufferSize -= consumed;
        memmove(parser->buffer, parser->buffer + consumed, parser->bufferSize);
    }

    return true;
}
