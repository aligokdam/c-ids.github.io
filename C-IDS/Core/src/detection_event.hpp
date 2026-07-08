#pragma once

#include <cstdint>
#include <string>

namespace cids::core {

// Split into its own header so both engine.hpp and plugin_dispatcher.hpp
// can depend on it without depending on each other (plugin_dispatcher
// only needs the event type, not the full Engine declaration).
struct DetectionEvent {
    uint32_t signature_id{};
    uint32_t severity{};
    uint64_t timestamp_ns{};
    std::string description;
};

}  // namespace cids::core
