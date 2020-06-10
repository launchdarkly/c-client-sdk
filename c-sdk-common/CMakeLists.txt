cmake_minimum_required(VERSION 2.8.8)

project(ldsharedapi)

find_package(CURL REQUIRED)

if(NOT DEFINED MSVC)
    set(LD_LIBRARIES pthread m)
endif()

set(LD_INCLUDE_PATHS
    "src"
    "include"
    ${CURL_INCLUDE_DIR}
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
    )
else()
    target_compile_definitions(ldsharedapi
        PRIVATE -D __USE_XOPEN
                -D _GNU_SOURCE
    )

    target_compile_options(ldsharedapi
        PRIVATE -fno-omit-frame-pointer
                -pedantic
                -Wall
                -Wextra
    )
endif(MSVC)
