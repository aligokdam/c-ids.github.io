#include "python_plugin_sink.hpp"

#include <pybind11/embed.h>
#include <pybind11/stl.h>

#include <iostream>

namespace py = pybind11;

namespace cids::plugins::python {

struct PythonPluginSink::Impl {
    // guard is a scoped interpreter: constructed once, alive for the
    // lifetime of this Impl, torn down automatically (RAII) — no manual
    // Py_Initialize/Py_Finalize pairing to get wrong.
    py::scoped_interpreter guard;
    std::vector<py::object> handlers;   // one per loaded plugin script
};

PythonPluginSink::PythonPluginSink(std::filesystem::path plugin_dir)
    : impl_(std::make_unique<Impl>())
{
    py::gil_scoped_acquire acquire;

    for (const auto& entry : std::filesystem::directory_iterator(plugin_dir)) {
        if (entry.path().extension() != ".py") continue;

        try {
            // exec_file-style load into a fresh, isolated module namespace
            // per script — one misbehaving plugin cannot corrupt another's
            // globals. This is the "sandbox" boundary at the Python level;
            // OS-level sandboxing (seccomp/AppContainer) is a deployment
            // concern documented separately in docs/PLUGIN_GUIDE.md.
            py::dict globals = py::globals().attr("copy")();
            py::eval_file(entry.path().string(), globals);

            if (globals.contains("on_detection")) {
                impl_->handlers.push_back(globals["on_detection"]);
            }
        } catch (const py::error_already_set& e) {
            // A broken plugin script must never take down the security
            // engine — log and skip, exactly like a C++ exception at the
            // ABI boundary is contained rather than propagated.
            std::cerr << "[PythonHost] failed to load " << entry.path()
                      << ": " << e.what() << '\n';
        }
    }
}

PythonPluginSink::~PythonPluginSink() = default;

void PythonPluginSink::on_detection(const cids::core::DetectionEvent& event) {
    std::lock_guard lock(gil_dispatch_mutex_);
    py::gil_scoped_acquire acquire;

    py::dict event_dict;
    event_dict["signature_id"] = event.signature_id;
    event_dict["severity"]     = event.severity;
    event_dict["timestamp_ns"] = event.timestamp_ns;
    event_dict["description"]  = event.description;

    for (auto& handler : impl_->handlers) {
        try {
            handler(event_dict);
        } catch (const py::error_already_set& e) {
            std::cerr << "[PythonHost] plugin raised: " << e.what() << '\n';
        }
    }
}

}  // namespace cids::plugins::python
