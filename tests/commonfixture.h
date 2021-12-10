#ifndef LDSERVERAPI_COMMONFIXTURE_H
#define LDSERVERAPI_COMMONFIXTURE_H

class CommonFixture : public ::testing::Test {
protected:
    void SetUp() override;

    void TearDown() override;
};

#endif //LDSERVERAPI_COMMONFIXTURE_H
