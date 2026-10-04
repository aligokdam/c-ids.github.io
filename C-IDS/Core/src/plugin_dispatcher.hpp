#pragma once

#include <memory>
#include <vector>

#include "detection_event.hpp"

namespace cids::core {

// Abstract sink any plugin implements. Core NEVER depends on Python (or
// any other embedded runtime) directly — it only knows this interface.
// Plugins/PythonHost implements IPluginSink and registers itself at
// startup via PluginDispatcher::register_sink(). This is what lets
// CIDS_BUILD_PYTHON_PLUGIN=OFF produce a Core binary with zero Python
// dependency at all, per docs/PLUGIN_GUIDE.md.
class IPluginSink {
public:
    virtual ~IPluginSink() = default;
    virtual void on_detection(const DetectionEvent& event) = 0;
};

// Owns its plugins as shared_ptr (not raw pointers) because plugin
// callbacks may be invoked from a worker thread (e.g. the Python host
// dispatches into a GIL-owning thread) whose lifetime can outlive a
// single ingest_packet() call.
class PluginDispatcher {
public:
    void register_sink(std::shared_ptr<IPluginSink> sink) {
        sinks_.push_back(std::move(sink));
    }

    void notify(const DetectionEvent& event) const {
        for (const auto& sink : sinks_) {
            sink->on_detection(event);
        }
    }

private:
    std::vector<std::shared_ptr<IPluginSink>> sinks_;
};

}  // namespace cids::core
