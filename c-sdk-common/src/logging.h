#pragma once

#include <launchdarkly/logging.h>

#define LD_LOG_1(level, format, x) \
    LDi_log(level, "[%s, %d] " format, __FILE__, __LINE__, x)

#define LD_LOG_2(level, format, x, y) \
    LDi_log(level, "[%s, %d] " format, __FILE__, __LINE__, x, y)

#define LD_LOG_3(level, format, x, y, z) \
    LDi_log(level, "[%s, %d] " format, __FILE__, __LINE__, x, y, z)
