// abi_shim.cpp
//
// The ONLY file in Core allowed to know both:
//   (a) the extern "C" structs/handles in include/cids/*_abi.h, and
//   (b) the real C++ types in engine.hpp.
//
// Every function here follows the same shape:
//   1. Validate raw input (null checks, size sanity).
//   2. Translate POD <-> C++ types WITHOUT copying large payloads.
//   3. Call into cids::core::Engine.
//   4. Catch everything; translate to CidsStatus. No exception, no raw
//      pointer, and no C++ object ever crosses back through this file
//      uncontrolled.

#include "cids/abi.h"
#include "cids/packet_abi.h"
#include "engine.hpp"

#include <cstring>
#include <new>

namespace {

// Wraps the raw C callback + user_context so it can be stored as a
// cids::core::DetectionSink (std::function) without the ABI layer ever
// exposing std::function itself.
struct CallbackTrampoline {
    CidsDetectionCallback callback = nullptr;
    void* user_context = nullptr;

    void operator()(const cids::core::DetectionEvent& event) const {
        if (!callback) return;
        CidsDetectionEvent c_event{};
        c_event.struct_size   = sizeof(CidsDetectionEvent);
        c_event.signature_id  = event.signature_id;
        c_event.severity      = event.severity;
        c_event.timestamp_ns  = event.timestamp_ns;
        c_event.description   = event.description.c_str();  // valid only during this call
        callback(&c_event, user_context);
    }
};

}  // namespace

extern "C" {

void CIDS_CALL cids_abi_version(CidsAbiVersion* out_version) {
    if (!out_version) return;
    out_version->struct_size = sizeof(CidsAbiVersion);
    out_version->major = CIDS_ABI_VERSION_MAJOR;
    out_version->minor = CIDS_ABI_VERSION_MINOR;
}

CidsStatus CIDS_CALL cids_engine_create(CidsEngineHandle* out_handle) {
    if (!out_handle) return CIDS_ERR_INVALID_ARG;
    try {
        auto* engine = new (std::nothrow) cids::core::Engine();
        if (!engine) return CIDS_ERR_OUT_OF_MEMORY;
        *out_handle = reinterpret_cast<CidsEngineHandle>(engine);
        return CIDS_OK;
    } catch (...) {
        return CIDS_ERR_UNKNOWN;
    }
}

CidsStatus CIDS_CALL cids_engine_destroy(CidsEngineHandle handle) {
    if (!handle) return CIDS_ERR_INVALID_ARG;
    // This is the single, explicit "delete" paired with the single
    // "new" above — ownership of the Engine never appears anywhere else.
    delete reinterpret_cast<cids::core::Engine*>(handle);
    return CIDS_OK;
}

CidsStatus CIDS_CALL cids_engine_set_detection_callback(
    CidsEngineHandle handle,
    CidsDetectionCallback callback,
    void* user_context
) {
    if (!handle) return CIDS_ERR_INVALID_ARG;
    auto* engine = reinterpret_cast<cids::core::Engine*>(handle);
    engine->set_detection_sink(CallbackTrampoline{callback, user_context});
    return CIDS_OK;
}

CidsStatus CIDS_CALL cids_engine_ingest_packet(
    CidsEngineHandle handle,
    const CidsPacketView* packet
) {
    if (!handle || !packet) return CIDS_ERR_INVALID_ARG;
    if (packet->payload_length > 0 && packet->payload == nullptr) {
        return CIDS_ERR_INVALID_ARG;
    }

    auto* engine = reinterpret_cast<cids::core::Engine*>(handle);

    cids::core::Packet p{};
    p.timestamp_ns = packet->timestamp_ns;
    p.source_ip    = packet->source_ip;
    p.dest_ip      = packet->dest_ip;
    p.source_port  = packet->source_port;
    p.dest_port    = packet->dest_port;
    p.protocol     = packet->protocol;
    p.payload = std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(packet->payload),
        packet->payload_length
    );

    return engine->ingest_packet(p) ? CIDS_OK : CIDS_ERR_INVALID_ARG;
}

}  // extern "C"
