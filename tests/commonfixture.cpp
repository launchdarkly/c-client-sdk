#include "gtest/gtest.h"
#include "commonfixture.h"

extern "C" {
#include <launchdarkly/api.h>
}

void CommonFixture::SetUp() {
    LDBasicLoggerThreadSafeInitialize();
    LDConfigureGlobalLogger(LD_LOG_TRACE, LDBasicLoggerThreadSafe);
}

void CommonFixture::TearDown() {
    LDBasicLoggerThreadSafeShutdown();
}
