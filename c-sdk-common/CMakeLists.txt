cmake_minimum_required(VERSION 3.10)

project(ldsharedapi)

include(CTest)

if(BUILD_TESTING)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    configure_file(CMakeLists.txt.in
            ${CMAKE_BINARY_DIR}/googletest-download/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
    execute_process(COMMAND ${CMAKE_COMMAND} --build .
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )


    add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
            ${CMAKE_BINARY_DIR}/googletest-build)

    enable_testing()
    add_subdirectory(tests)
endif()


find_package(CURL REQUIRED)

if (COVERAGE)
    include(CMakeFiles/CodeCoverage.cmake)
    append_coverage_compiler_flags()
    setup_target_for_coverage_gcovr_html(NAME coverage)
endif (COVERAGE)

if(NOT DEFINED MSVC)
    set(LD_LIBRARIES pthread m)
endif()

set(LD_INCLUDE_PATHS
    "src"
    "include"
    ${CURL_INCLUDE_DIR}
    "test-utils/include"
    "test-utils/src"
)

if(APPLE)
    set(LD_INCLUDE_PATHS ${LD_INCLUDE_PATHS} "/usr/local/include")
    set(LD_PUBLIC_LIBRARIES "-framework CoreFoundation" "-framework IOKit")
endif(APPLE)

file(GLOB SOURCES "src/*")

set(LD_LIBRARIES ${LD_LIBRARIES} ${CURL_LIBRARIES})

# ldsharedapi target -----------------------------------------------------------

add_library(ldsharedapi ${SOURCES})

target_link_libraries(ldsharedapi
    PUBLIC  ${LD_PUBLIC_LIBRARIES}
    PRIVATE ${LD_LIBRARIES}
)

target_include_directories(ldsharedapi
    PUBLIC  "include"
    PRIVATE ${LD_INCLUDE_PATHS}
)

if(MSVC)
    target_compile_definitions(ldsharedapi
        PRIVATE -D CURL_STATICLIB
                -D _CRT_SECURE_NO_WARNINGS
                -D LAUNCHDARKLY_USE_ASSERT
    )
    
    target_compile_options(ldsharedapi
        PRIVATE /WX
    )
else()
    target_compile_definitions(ldsharedapi
        PRIVATE -D __USE_XOPEN
                -D _GNU_SOURCE
                -D LAUNCHDARKLY_USE_ASSERT
    )

    target_compile_options(ldsharedapi
        PRIVATE -fno-omit-frame-pointer
                -pedantic
                -Wall
                -Wextra
                -Werror
                -Wstrict-prototypes
                -Wmissing-prototypes
                -Wmissing-declarations
                -std=c89
    )
endif(MSVC)

# test-utils target -----------------------------------------------------------

file(GLOB TEST_UTILS_SOURCES "test-utils/src/*")

add_library(test-utils ${SOURCES})

target_include_directories(test-utils
    PUBLIC "test-utils/src"
           "test-utils/include/test-utils"
           ${LD_INCLUDE_PATHS}
)

if(MSVC)
    target_compile_definitions(test-utils
        PRIVATE -D LAUNCHDARKLY_USE_ASSERT
    )
else()
    target_compile_definitions(test-utils
        PRIVATE -D __USE_XOPEN
                -D _GNU_SOURCE
                -D LAUNCHDARKLY_USE_ASSERT
    )

    # the 3rd party http parser does not pass -pedantic
    target_compile_options(test-utils
        PRIVATE -fno-omit-frame-pointer
                -Wall
                -Wextra
                -Werror
                -Wstrict-prototypes
                -Wmissing-prototypes
                -Wmissing-declarations
                -std=c89
    )
endif(MSVC)