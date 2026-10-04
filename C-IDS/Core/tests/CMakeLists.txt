include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(cids_core_tests
    test_engine.cpp
    test_abi_boundary.cpp
)

target_link_libraries(cids_core_tests PRIVATE
    cids_core_impl
    cids_abi
    GTest::gtest_main
)

target_include_directories(cids_core_tests PRIVATE ${CMAKE_SOURCE_DIR}/Core/src)
cids_apply_hardening(cids_core_tests)

include(GoogleTest)
gtest_discover_tests(cids_core_tests)
