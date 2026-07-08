#pragma once

#include <filesystem>
#include <memory>
#include <mutex>
#include <vector>

#include "plugin_dispatcher.hpp"   // cids::core::IPluginSink

namespace cids::plugins::python {

// Bridges a detection event from the C++ core into a sandboxed Python
// script. Implements cids::core::IPluginSink so Engine/PluginDispatcher
// never need to know Python exists — this class is the entire coupling
// point (see docs/PLUGIN_GUIDE.md).
class PythonPluginSink final : public cids::core::IPluginSink {
public:
    // Loads every *.py file under plugin_dir that defines an
    // `on_detection(event: dict) -> None` top-level function.
    explicit PythonPluginSink(std::filesystem::path plugin_dir);
    ~PythonPluginSink() override;

    PythonPluginSink(const PythonPluginSink&)            = delete;
    PythonPluginSink& operator=(const PythonPluginSink&) = delete;

    void on_detection(const cids::core::DetectionEvent& event) override;

private:
    struct Impl;                       // pimpl: keeps <pybind11/embed.h> out
    std::unique_ptr<Impl> impl_;       // of this public header entirely
    std::mutex gil_dispatch_mutex_;    // serializes calls into the interpreter
};

}  // namespace cids::plugins::python
