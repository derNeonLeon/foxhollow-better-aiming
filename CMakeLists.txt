cmake_minimum_required(VERSION 3.25)
project(BetterAiming LANGUAGES C VERSION 1.2.0)

set(FOXHOLLOW_SOURCE_DIR "" CACHE PATH "Path to a Foxhollow source checkout")
set(BETTER_AIMING_PLATFORM "" CACHE STRING "Foxhollow platform directory override")

if(NOT FOXHOLLOW_SOURCE_DIR AND DEFINED ENV{FOXHOLLOW_SOURCE_DIR})
    set(FOXHOLLOW_SOURCE_DIR "$ENV{FOXHOLLOW_SOURCE_DIR}")
endif()

if(NOT FOXHOLLOW_SOURCE_DIR AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.deps/foxhollow/port/include/foxhollow_mod_api.h")
    set(FOXHOLLOW_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.deps/foxhollow")
endif()

if(NOT EXISTS "${FOXHOLLOW_SOURCE_DIR}/port/include/foxhollow_mod_api.h")
    message(FATAL_ERROR "A Foxhollow source checkout is required. Set FOXHOLLOW_SOURCE_DIR or run BUILD-WINDOWS.cmd.")
endif()

if(NOT EXISTS "${FOXHOLLOW_SOURCE_DIR}/extern/aurora/include/dolphin/pad.h")
    message(FATAL_ERROR "Foxhollow's Aurora submodule is missing. Clone Foxhollow recursively.")
endif()

if(NOT BETTER_AIMING_PLATFORM)
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" BETTER_AIMING_PROCESSOR)

    if(WIN32)
        if(BETTER_AIMING_PROCESSOR MATCHES "^(arm64|aarch64)$")
            set(BETTER_AIMING_PLATFORM "windows-arm64")
        else()
            set(BETTER_AIMING_PLATFORM "windows-amd64")
        endif()
    elseif(APPLE)
        if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64" OR BETTER_AIMING_PROCESSOR MATCHES "^(arm64|aarch64)$")
            set(BETTER_AIMING_PLATFORM "macos-arm64")
        elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64" OR BETTER_AIMING_PROCESSOR MATCHES "^(x86_64|amd64)$")
            set(BETTER_AIMING_PLATFORM "macos-x86_64")
        else()
            message(FATAL_ERROR "Unsupported macOS architecture")
        endif()
    elseif(UNIX)
        if(BETTER_AIMING_PROCESSOR MATCHES "^(arm64|aarch64)$")
            set(BETTER_AIMING_PLATFORM "linux-arm64")
        elseif(BETTER_AIMING_PROCESSOR MATCHES "^(x86_64|amd64)$")
            set(BETTER_AIMING_PLATFORM "linux-amd64")
        else()
            message(FATAL_ERROR "Unsupported Linux architecture")
        endif()
    else()
        message(FATAL_ERROR "Unsupported Better Aiming build platform")
    endif()
endif()

set(BETTER_AIMING_SUPPORTED_PLATFORMS
    windows-amd64
    windows-arm64
    linux-amd64
    linux-arm64
    macos-x86_64
    macos-arm64
)

if(NOT BETTER_AIMING_PLATFORM IN_LIST BETTER_AIMING_SUPPORTED_PLATFORMS)
    message(FATAL_ERROR "Unsupported Foxhollow platform directory: ${BETTER_AIMING_PLATFORM}")
endif()

add_library(BetterAiming SHARED
    include/better_aiming.h
    src/better_aiming.c
)

set_target_properties(BetterAiming PROPERTIES
    PREFIX ""
    OUTPUT_NAME "mod"
)

if(APPLE)
    set_target_properties(BetterAiming PROPERTIES SUFFIX ".so")
endif()

target_include_directories(BetterAiming PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${FOXHOLLOW_SOURCE_DIR}/port/include"
    "${FOXHOLLOW_SOURCE_DIR}/extern/aurora/include"
    "${FOXHOLLOW_SOURCE_DIR}/game/include"
    "${FOXHOLLOW_SOURCE_DIR}/game"
)

target_compile_features(BetterAiming PRIVATE c_std_11)
target_compile_definitions(BetterAiming PRIVATE TARGET_PC)

if(MSVC)
    target_compile_options(BetterAiming PRIVATE /clang:-fsigned-char)
else()
    target_compile_options(BetterAiming PRIVATE -fsigned-char)
endif()

install(TARGETS BetterAiming
    RUNTIME DESTINATION "lib/${BETTER_AIMING_PLATFORM}"
    LIBRARY DESTINATION "lib/${BETTER_AIMING_PLATFORM}"
)
install(FILES mod.json DESTINATION ".")
