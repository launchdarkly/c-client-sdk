#include "commonfixture.h"

extern "C" {
#include <launchdarkly/logging.h>
#include <launchdarkly/memory.h>
}

void
CommonFixture::SetUp()
{
    LDBasicLoggerThreadSafeInitialize();
    LDConfigureGlobalLogger(LD_LOG_TRACE, LDBasicLoggerThreadSafe);
    LDGlobalInit();
}

void
CommonFixture::TearDown()
{
    LDBasicLoggerThreadSafeShutdown();
}
