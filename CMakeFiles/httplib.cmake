set(HTTPLIB_COMPILE ON)

FetchContent_Declare(httplib
        GIT_REPOSITORY    https://github.com/yhirose/cpp-httplib.git
        GIT_TAG           v0.10.2
        SOURCE_DIR        "${CMAKE_BINARY_DIR}/httplib-src"
        BINARY_DIR        "${CMAKE_BINARY_DIR}/httplib-build"
        )

FetchContent_MakeAvailable(httplib)
