cmake_minimum_required(VERSION 3.24)

# ---------------------------------------------------------------------------
# C-IDS :: Cyber-security Intrusion Detection & analysis System
# Top-level build orchestrator.
#
#   Core     -> platform-independent C++20 engine, exposed via a stable
#               C ABI ("cids_abi"). Builds to .dll (Windows) / .dylib
#               or static .framework (iOS) / .so (Linux).
#   Desktop  -> C#/.NET host consuming Core via P/Invoke.
#   Mobile   -> Swift package consuming Core via a C module map.
#   Plugins  -> optional native plugin hosts (e.g. embedded Python).
# ---------------------------------------------------------------------------

project(CIDS
    VERSION 0.1.0
    DESCRIPTION "Cyber-security Intrusion Detection & Analysis Engine"
    LANGUAGES CXX C
)

# --- Global toggles ---------------------------------------------------------
option(CIDS_BUILD_SHARED        "Build Core as a shared library (.dll/.dylib/.so)" ON)
option(CIDS_BUILD_TESTS         "Build Core unit tests" ON)
option(CIDS_BUILD_PYTHON_PLUGIN "Build the embedded-Python plugin host" OFF)
option(CIDS_ENABLE_SANITIZERS   "Enable ASan/UBSan in Debug builds" ON)
option(CIDS_ENABLE_HARDENING    "Enable compiler hardening flags (stack protector, FORTIFY, CFI)" ON)
option(CIDS_TREAT_WARNINGS_AS_ERRORS "Treat all warnings as errors" ON)
option(CIDS_BUILD_FOR_IOS       "Configure output as an iOS .framework / static lib" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)          # no compiler-specific dialects -> portable ABI-adjacent code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Single output directory across all sub-targets: simplifies packaging
# for the .NET / Swift hosts that need to find the produced binary.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(CidsCompilerHardening)   # shared warning / sanitizer / hardening flags

message(STATUS "C-IDS :: building for ${CMAKE_SYSTEM_NAME} (${CMAKE_CXX_COMPILER_ID})")

# --- Sub-projects ------------------------------------------------------------
add_subdirectory(Core)

if(CIDS_BUILD_PYTHON_PLUGIN)
    add_subdirectory(Plugins/PythonHost)
endif()

# Desktop (C#) and Mobile (Swift) are *not* driven by this CMake build:
# they consume the artifact produced by Core (cids_abi.dll / .dylib / .framework)
# as a prebuilt native dependency. See Desktop/CIDS.Desktop.csproj and
# Mobile/Package.swift respectively. They are listed here only so the
# structure is discoverable from the root CMakeLists.
message(STATUS "C-IDS :: Desktop (.NET) and Mobile (Swift) projects build independently; see docs/ARCHITECTURE.md")
