## SDK contract test service

This directory contains an implementation of the cross-platform SDK testing protocol defined by
https://github.com/launchdarkly/sdk-test-harness. See that project's README for details of this
protocol, and the kinds of SDK capabilities that are relevant to the contract tests. This code should
not need to be updated unless the SDK has added or removed such capabilities.

To run these tests locally, run `make contract-tests` from the SDK project root directory.
This downloads the correct version of the test harness tool automatically.

Or, to test against an in-progress local version of the test harness, run
`make start-contract-test-service` from the SDK project root directory; then, in the root directory of the sdk-test-harness project, build the test harness and run it from the command line.


## Architecture

The source code consists of two main components.

1. A binary named `testservice`, defined [here](main.cpp).
2. A library named `service`, defined [here](include/service.hpp).

The binary sets up the required routes, and then calls into the `service` library as necessary.

## 3rd Party Dependencies
1. JSON handling: [nlohmann/json](https://github.com/nlohmann/json)
2. HTTP Server: [yhirose/cpp-httplib](https://github.com/yhirose/cpp-httplib)
