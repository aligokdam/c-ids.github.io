#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

#include "detection_event.hpp"
#include "signature_registry.hpp"
#include "rule_matcher.hpp"
#include "plugin_dispatcher.hpp"

namespace cids::core {

/* Immutable, bounds-safe view of a single packet inside the C++ core.
 * Built once at the ABI boundary (abi_shim.cpp) from the raw CidsPacketView,
 * after validating payload_length against the actual buffer — nothing
 * downstream ever re-trusts a raw length again. */
struct Packet {
    uint64_t timestamp_ns{};
    uint32_t source_ip{};
    uint32_t dest_ip{};
    uint16_t source_port{};
    uint16_t dest_port{};
    uint8_t  protocol{};
    std::span<const std::byte> payload;   // non-owning, lifetime = call scope only
};

using DetectionSink = std::function<void(const DetectionEvent&)>;

// -----------------------------------------------------------------------
// Zero-Trust memory policy applied throughout this class (see
// docs/SECURITY_POLICY.md):
//   * No raw owning pointers anywhere — every owned resource lives in a
//     std::unique_ptr / std::shared_ptr / standard container.
//   * No manual new/delete, no malloc/free.
//   * Non-copyable (engine holds unique state); movable is intentionally
//     also disabled since it's referenced by an opaque handle across
//     the ABI boundary — moving it would invalidate that handle.
//   * All shared mutable state guarded by std::mutex; no bare
//     data races across ingest_packet() calls from multiple threads.
// -----------------------------------------------------------------------
class Engine final {
public:
    Engine();
    ~Engine();

    Engine(const Engine&)            = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&)                 = delete;
    Engine& operator=(Engine&&)      = delete;

    void set_detection_sink(DetectionSink sink);

    // Returns false only for structurally invalid input (e.g. payload
    // pointer null with nonzero length); never throws across this API —
    // callers at the ABI shim translate this into a CidsStatus.
    [[nodiscard]] bool ingest_packet(const Packet& packet) noexcept;

private:
    std::mutex mutex_;                                  // guards everything below
    std::unique_ptr<SignatureRegistry> signatures_;      // owned, RAII
    std::unique_ptr<RuleMatcher> matcher_;                // owned, RAII
    std::shared_ptr<PluginDispatcher> plugin_dispatcher_; // shared with async plugin workers
    DetectionSink detection_sink_;
};

}  // namespace cids::core
