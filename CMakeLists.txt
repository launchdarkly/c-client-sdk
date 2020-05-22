cmake_minimum_required(VERSION 2.8.8)

project(ldclientapi)

include(CTest)

find_package(CURL REQUIRED)

if(NOT DEFINED MSVC)
    set(LD_LIBRARIES pthread m)
endif()

set(LD_INCLUDE_PATHS "src" "include" ${CURL_INCLUDE_DIR})

if(APPLE)
    set(LD_INCLUDE_PATHS ${LD_INCLUDE_PATHS} "/usr/local/include")
    set(LD_PUBLIC_LIBRARIES "-framework CoreFoundation" "-framework IOKit")
endif(APPLE)

file(GLOB SOURCES "src/*")

set(LD_LIBRARIES ${LD_LIBRARIES} ${CURL_LIBRARIES})

# ldclientapi target -----------------------------------------------------------

add_library(ldclientapi ${SOURCES})

target_link_libraries(ldclientapi
    PUBLIC  ${LD_PUBLIC_LIBRARIES}
    PRIVATE ${LD_LIBRARIES}
)

target_include_directories(ldclientapi
    PUBLIC "include"
    PRIVATE ${LD_INCLUDE_PATHS}
)

if(MSVC)
    target_compile_definitions(ldclientapi
        PRIVATE -D CURL_STATICLIB
                -D PCRE_STATIC
                -D _CRT_SECURE_NO_WARNINGS
    )
else()
    target_compile_definitions(ldclientapi
        PRIVATE -D __USE_XOPEN
                -D _GNU_SOURCE
    )

    target_compile_options(ldclientapi
        PRIVATE -fno-omit-frame-pointer
                -pedantic
                -Wall
                -Wextra
    )
endif(MSVC)

if(BUILD_SHARED_LIBS)
    set_property(TARGET ldclientapi PROPERTY C_VISIBILITY_PRESET hidden)
endif(BUILD_SHARED_LIBS)

if(NOT SKIP_BASE_INSTALL)
    INSTALL(
        TARGETS     ldclientapi
        DESTINATION lib
    )

    INSTALL(
        DIRECTORY              ${PROJECT_SOURCE_DIR}/include/
        DESTINATION            include
        FILES_MATCHING PATTERN "*.h*"
    )
endif()

# test targets ----------------------------------------------------------------

if(BUILD_TESTING)
    file(GLOB TESTS "tests/test-*")
    foreach(testsource ${TESTS})
        get_filename_component(testsourceleaf ${testsource} NAME)
        string(REPLACE ".c" "" testexe ${testsourceleaf})
        add_executable(${testexe} ${testsource})
        target_link_libraries(${testexe} ldclientapi)
        target_include_directories(${testexe} PRIVATE ${LD_INCLUDE_PATHS})
        add_test(NAME ${testexe} COMMAND ${CMAKE_BINARY_DIR}/${testexe})
    endforeach(testsource)
endif()
