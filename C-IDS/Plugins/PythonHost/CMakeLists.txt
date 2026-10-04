# PythonHost — only built when -DCIDS_BUILD_PYTHON_PLUGIN=ON.
# Core has zero knowledge of this target existing; it only knows the
# IPluginSink interface (Core/src/plugin_dispatcher.hpp). This is what
# keeps "Python as an optional plugin runtime" from becoming "Python as
# a hard dependency of the security engine".

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 CONFIG REQUIRED)   # vcpkg/conan-provided

add_library(cids_python_host STATIC
    src/python_plugin_sink.cpp
    src/script_sandbox.cpp
)

target_include_directories(cids_python_host
    PUBLIC include
    PRIVATE ${CMAKE_SOURCE_DIR}/Core/src   # to see IPluginSink
)

target_link_libraries(cids_python_host
    PUBLIC pybind11::embed
    PRIVATE cids_core_impl
)

cids_apply_hardening(cids_python_host)
