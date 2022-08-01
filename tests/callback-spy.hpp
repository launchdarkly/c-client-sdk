#pragma once

#include <string>
#include <vector>
#include <unordered_map>

// Represents a status callback invocation.
struct statusCall {
    int status;
    void *userData;
    statusCall(int status, void *userData);
};

// Represents a flag change callback invocation.
struct flagCall {
    std::string flag;
    int status;
    flagCall(const char* flagKey, int status);
};

// Simple container for callback invocations, parameterized
// on the call type.
template<class T>
struct callbackSpy {
    std::unordered_map<const char*, std::vector<T>> calls;

    // Record a callback for a particular unit test name.
    template<class ...Args>
    void record(const char* test, Args &&... args) {
        calls[test].emplace_back(std::forward<Args>(args)...);
    }

    // Retrieve callback data associated with a particular unit test name.
    // If no callbacks invoked for the test, an empty vector will be returned.
    const std::vector<T>& test(const char* test) {
        return calls[test];
    }
};

// Records a status callback to the global status spy.
void recordStatus(const char* test, int status, void *userData);

// Records a flag callback to the global flag spy.
void recordFlag(const char* test, const char *flagKey, int status);

// Retrieve calls associated with global flag spy.
// Use macro FLAG_CALLS(testName) for convenience.
const std::vector<flagCall>& flagCalls(const char* test);

// Retrieve calls associated with global status spy.
// Use macro STATUS_CALLS(testName) for convenience.
const std::vector<statusCall>& statusCalls(const char* test);


// Define deprecated status callback (not taking userdata)
#define DEFINE_STATUS_CALLBACK_2_ARG(name) \
static void name(const int status) { recordStatus(#name, status, nullptr); }

// Define status callback taking userdata
#define DEFINE_STATUS_CALLBACK_3_ARG(name) \
static void name(LDStatus status, void *userData) { recordStatus(#name, status, userData); }

// Define flag change callback
#define DEFINE_FLAG_CALLBACK(name) \
static void name(const char* const flagKey, const int status) { recordFlag(#name, flagKey, status); }

// Retrieve all flag calls
#define FLAG_CALLS(name) flagCalls(#name)

// Retrieve all status calls
#define STATUS_CALLS(name) statusCalls(#name)
