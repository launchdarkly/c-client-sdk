#ifndef LDSHAREDAPI_COMMONFIXTURE_H
#define LDSHAREDAPI_COMMONFIXTURE_H
#include "gtest/gtest.h"

class CommonFixture : public ::testing::Test {
protected:
    void SetUp() override;

    void TearDown() override;
};

#endif // LDSHAREDAPI_COMMONFIXTURE_H
