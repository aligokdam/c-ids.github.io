#include "engine.hpp"

#include <utility>

namespace cids::core {

Engine::Engine()
    : signatures_(std::make_unique<SignatureRegistry>())
    , matcher_(std::make_unique<RuleMatcher>(*signatures_))
    , plugin_dispatcher_(std::make_shared<PluginDispatcher>())
{
}

// Defined out-of-line (even though it's empty) because SignatureRegistry,
// RuleMatcher and PluginDispatcher are only forward-visible where unique_ptr
// is *declared* in the header — the destructor needs their complete types,
// which live in this translation unit only. Prevents "incomplete type"
// compile errors and keeps Core/src/*.hpp out of the public include path.
Engine::~Engine() = default;

void Engine::set_detection_sink(DetectionSink sink) {
    std::lock_guard lock(mutex_);
    detection_sink_ = std::move(sink);
}

bool Engine::ingest_packet(const Packet& packet) noexcept {
    // Boundary validation: reject structurally impossible input instead of
    // trusting whatever crossed the ABI. This is the C++-side half of the
    // "never re-trust a raw length" rule described in packet_abi.h.
    if (packet.payload.data() == nullptr && !packet.payload.empty()) {
        return false;
    }

    try {
        std::lock_guard lock(mutex_);
        auto matches = matcher_->evaluate(packet);
        for (const auto& match : matches) {
            DetectionEvent event{
                .signature_id = match.signature_id,
                .severity     = match.severity,
                .timestamp_ns = packet.timestamp_ns,
                .description  = match.description,
            };
            if (detection_sink_) {
                detection_sink_(event);
            }
            plugin_dispatcher_->notify(event);  // fan-out to Python/native plugins
        }
        return true;
    } catch (...) {
        // Zero-Trust rule: exceptions never cross out of Engine's public
        // API. Anything unexpected is treated as "packet rejected", not
        // as a crash or an undefined-behavior propagation into the ABI shim.
        return false;
    }
}

}  // namespace cids::core
