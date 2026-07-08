#ifndef CIDS_PACKET_ABI_H
#define CIDS_PACKET_ABI_H

#include "cids/abi.h"

/* ---------------------------------------------------------------------------
 * Network-packet / event data crossing the C++ <-> C# <-> Swift boundary.
 *
 * Big structs are passed as (pointer + explicit length), NEVER by value,
 * and NEVER own their own memory across the boundary:
 *   - The caller (C# / Swift) allocates and pins the raw byte buffer.
 * -   Core only reads it for the duration of the call (or copies internally
 *     into its own std::vector immediately — see engine.cpp).
 * This avoids: marshaling overhead of copying huge structs by value,
 * lifetime ambiguity (who frees a struct returned by value?), and
 * alignment mismatches between MSVC / Clang / Swift's C importer.
 * ------------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)   /* explicit, compiler-independent layout */

typedef struct CidsPacketView {
    uint32_t struct_size;     /* = sizeof(CidsPacketView), for forward-compat */
    uint64_t timestamp_ns;    /* capture time, ns since epoch (monotonic-safe) */
    uint32_t source_ip;       /* IPv4 in host byte order; see ipv6 fields below for v6 */
    uint32_t dest_ip;
    uint16_t source_port;
    uint16_t dest_port;
    uint8_t  protocol;        /* CidsProtocol enum value, see below */
    uint8_t  ipv6_flag;       /* 1 if source_ipv6/dest_ipv6 below are valid */
    uint8_t  reserved[2];     /* explicit padding, keeps size stable & aligned */
    const uint8_t* payload;   /* NON-owning pointer, valid only for call duration */
    uint32_t payload_length;  /* bytes in payload — bounds-checked on the C++ side */
    uint8_t  source_ipv6[16];
    uint8_t  dest_ipv6[16];
} CidsPacketView;

typedef enum CidsProtocol {
    CIDS_PROTO_UNKNOWN = 0,
    CIDS_PROTO_TCP     = 1,
    CIDS_PROTO_UDP     = 2,
    CIDS_PROTO_ICMP    = 3
} CidsProtocol;

/* Result of a signature match, delivered to the host via callback. */
typedef struct CidsDetectionEvent {
    uint32_t struct_size;
    uint32_t signature_id;
    uint32_t severity;         /* 0-100 */
    uint64_t timestamp_ns;
    const char* description;   /* UTF-8, NUL-terminated, valid only during callback */
} CidsDetectionEvent;

/* Host-supplied callback invoked synchronously on the calling thread when a
 * signature fires. The host (C#/Swift) must copy any data it needs out of
 * `event` before returning — no pointers inside it remain valid afterward. */
typedef void (CIDS_CALL *CidsDetectionCallback)(const CidsDetectionEvent* event, void* user_context);

CIDS_API CidsStatus CIDS_CALL cids_engine_set_detection_callback(
    CidsEngineHandle handle,
    CidsDetectionCallback callback,
    void* user_context
);

/* Ingest one packet. Bounds-checked against payload_length internally;
 * never trusts the caller's declared length beyond what was actually
 * allocated — see docs/SECURITY_POLICY.md, "boundary validation". */
CIDS_API CidsStatus CIDS_CALL cids_engine_ingest_packet(
    CidsEngineHandle handle,
    const CidsPacketView* packet
);

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif /* CIDS_PACKET_ABI_H */
