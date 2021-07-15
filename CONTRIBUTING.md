# Contributing to the LaunchDarkly Client-Side SDK for C/C++

LaunchDarkly has published an [SDK contributor's guide](https://docs.launchdarkly.com/docs/sdk-contributors-guide) that provides a detailed explanation of how our SDKs work. See below for additional information on how to contribute to this SDK.

## Submitting bug reports and feature requests

The LaunchDarkly SDK team monitors the [issue tracker](https://github.com/launchdarkly/c-client-sdk/issues) in the SDK repository. Bug reports and feature requests specific to this SDK should be filed in this issue tracker. The SDK team will respond to all newly filed issues within two business days.

## Submitting pull requests

We encourage pull requests and other contributions from the community. Before submitting pull requests, ensure that all temporary or unintended code is removed. Don't worry about adding reviewers to the pull request; the LaunchDarkly SDK team will add themselves. The SDK team will acknowledge all pull requests within two business days.

## Build instructions

This SDK is built with [CMake](https://cmake.org/), and depends on `libcurl`. Install `libcurl` and `cmake` with your systems package manager.

To build the SDK run:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Compiling curl on Windows

You can obtain the libcurl dependency at [curl.haxx.se](https://curl.haxx.se/download/curl-7.59.0.zip). You will need to extract curl and then update `Makefile.win` with the path you saved it to. The Makefile will automatically build curl for you. Ensure you edit `Makefile.win` to indicate if you want an `x64`, or `x86` build.

```bash
cd curl-7.59.0/winbuild
nmake /f Makefile.vc mode=static
```

The Visual Studio command prompt can be configured for multiple environments. To ensure that you are using your intended tool chain you can launch an environment with: `call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\Tools\VsDevCmd.bat" -host_arch=amd64 -arch=amd64`, where `arch` is your target, `host_arch` is the platform you are building on, and the path is your path to `VsDevCmd.bat`. You will need to modify the above command for your specific setup.
