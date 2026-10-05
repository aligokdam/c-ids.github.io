# ---------------------------------------------------------------------------
# Core :: cids_abi
#
# Two-target split (important for the ABI-stability requirement):
#
#   cids_core_impl  (STATIC, internal)
#       -> all "real" C++: STL types, templates, exceptions allowed freely.
#          Never linked directly by Desktop/Mobile.
#
#   cids_abi        (SHARED, public)
#       -> thin extern "C" shim only. This is the ONLY thing that crosses
#          the process/language boundary, and the ONLY thing whose
#          binary interface we promise to keep stable (see docs/ABI_CONTRACT.md).
# ---------------------------------------------------------------------------

# ---- internal implementation library --------------------------------------
add_library(cids_core_impl STATIC
    src/engine.cpp
    src/packet_parser.cpp
    src/signature_registry.cpp
    src/rule_matcher.cpp
    src/plugin_dispatcher.cpp
)

target_include_directories(cids_core_impl
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(cids_core_impl PUBLIC cxx_std_20)
cids_apply_hardening(cids_core_impl)

# ---- public ABI shim (what actually ships as .dll / .dylib / .framework) ---
if(CIDS_BUILD_FOR_IOS)
    # iOS forbids arbitrary dylib loading outside the app bundle in most
    # distribution channels -> ship as a static lib wrapped in a .framework.
    add_library(cids_abi STATIC src/abi_shim.cpp)
    set_target_properties(cids_abi PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION A
        MACOSX_FRAMEWORK_IDENTIFIER com.yourorg.cids
        MACOSX_FRAMEWORK_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/platform/ios/Info.plist
        PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/cids/abi.h;${CMAKE_CURRENT_SOURCE_DIR}/include/cids/packet_abi.h"
    )
else()
    add_library(cids_abi SHARED src/abi_shim.cpp)
    set_target_properties(cids_abi PROPERTIES
        OUTPUT_NAME "cids_abi"
        VERSION   ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        WINDOWS_EXPORT_ALL_SYMBOLS OFF   # explicit dllexport only -> see abi.h
    )
endif()

target_link_libraries(cids_abi PRIVATE cids_core_impl)
target_include_directories(cids_abi PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_definitions(cids_abi PRIVATE CIDS_ABI_BUILDING_DLL)
cids_apply_hardening(cids_abi)

# Symbol visibility: default-hidden everywhere except what abi.h explicitly
# marks with CIDS_API. This keeps the exported surface identical to the
# documented contract in docs/ABI_CONTRACT.md — nothing "leaks" by accident.
set_target_properties(cids_core_impl cids_abi PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

# ---- platform-specific packaging -------------------------------------------
if(WIN32)
    # .dll + matching .lib import library land in bin/ and lib/ respectively;
    # Desktop/CIDS.Desktop.csproj copies cids_abi.dll next to the managed exe.
    install(TARGETS cids_abi RUNTIME DESTINATION bin ARCHIVE DESTINATION lib)
elseif(APPLE AND CIDS_BUILD_FOR_IOS)
    install(TARGETS cids_abi FRAMEWORK DESTINATION Frameworks)
else()
    install(TARGETS cids_abi LIBRARY DESTINATION lib)
endif()

install(DIRECTORY include/cids DESTINATION include)

# ---- tests ------------------------------------------------------------------
if(CIDS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
