cmake_minimum_required(VERSION 3.25)

project(
    DialogueMovementEnabler
    VERSION 2.4.0
    LANGUAGES CXX
)

if(NOT WIN32)
    message(FATAL_ERROR "DialogueMovementEnabler-LE targets Windows only")
endif()
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "DialogueMovementEnabler-LE targets Win32/x86 (Skyrim LE 1.9.32.0) only")
endif()

set(FORMATTED_NAME "Dialogue Movement Enabler LE")
set(LICENSE "GPL-3.0-or-later WITH Modding Exception")
set(AUTHOR "Vermunds (LE Backport)")

# Locate CommonLibLE SDK_SOURCE
set(COMMONLIBLE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../CommonLibLE_MASTER_FULL_PROJECT_AUDITED_V0_9_5_2026-08-04/SDK_SOURCE"
    CACHE PATH "Path to CommonLibLE SDK_SOURCE directory")

if(NOT EXISTS "${COMMONLIBLE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "CommonLibLE SDK_SOURCE not found at ${COMMONLIBLE_DIR}. Set -DCOMMONLIBLE_DIR=<path>.")
endif()

set(COMMONLIBLE_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
add_subdirectory("${COMMONLIBLE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/CommonLibLE")

# Configure version header and resource file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/Version.h
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.rc
    @ONLY
)

set(SOURCES
    src/AutoCloseManager.cpp
    src/Hooks.cpp
    src/Settings.cpp
    src/main.cpp
)

set(HEADERS
    include/AutoCloseManager.h
    include/Hooks.h
    include/LECompat.h
    include/PCH.h
    include/Settings.h
)

add_library(
    ${PROJECT_NAME}
    SHARED
    ${SOURCES}
    ${HEADERS}
    exports.def
    ${CMAKE_CURRENT_BINARY_DIR}/include/Version.h
    ${CMAKE_CURRENT_BINARY_DIR}/version.rc
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    SUFFIX ".dll"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(
    ${PROJECT_NAME}
    PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
)

target_precompile_headers(
    ${PROJECT_NAME}
    PRIVATE
        "include/PCH.h"
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
        CommonLibLE::CommonLibLE
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive- /Zc:__cplusplus)
    target_link_options(${PROJECT_NAME} PRIVATE "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/exports.def")
endif()
