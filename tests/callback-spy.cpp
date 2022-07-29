#include "callback-spy.hpp"

statusCall::statusCall(int status, void *userData)
        : status{status}, userData{userData} {}

flagCall::flagCall(const char* flagKey, int status)
        : flag{flagKey}, status{status} {}


static callbackSpy<flagCall> flagSpy;
static callbackSpy<statusCall> statusSpy;

void recordStatus(const char* test, int status, void *userData) {
    statusSpy.record(test, status, userData);
}
void recordFlag(const char* test, const char *flagKey, const int status) {
    flagSpy.record(test, flagKey, status);
}

const std::vector<flagCall>& flagCalls(const char* test) {
    return flagSpy.test(test);
}
const std::vector<statusCall>& statusCalls(const char* test) {
    return statusSpy.test(test);
}
