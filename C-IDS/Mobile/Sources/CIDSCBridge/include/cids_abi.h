// NOTE: this is a synced copy of Core/include/cids/abi.h for Swift's module
// map to consume. In CI, a `scripts/sync_abi_headers.sh` step keeps this in
// lock-step with Core so the two never drift silently — see docs/ABI_CONTRACT.md.
#ifndef CIDS_ABI_H
#define CIDS_ABI_H

/* ---------------------------------------------------------------------------
 * abi.h — the ONE header that defines what "stable ABI" means for C-IDS.
 *
 * Hard rules enforced across every file under Core/include/cids/*_abi.h:
 *
 *   1. extern "C" only. No C++ name mangling, no exceptions crossing the
 *      boundary, no virtual functions, no templates, no STL types
 *      (std::string, std::vector, std::function...) in any signature.
 *   2. Only POD ("Plain Old Data") structs with fixed-width integer types
 *      (uint32_t, int64_t, ...) and explicit padding. No bool (impl-defined
 *      size), no enum class in structs (use uint32_t + named constants).
 *   3. Every struct starts with a `uint32_t struct_size` field so a future
 *      version can grow the struct without breaking callers built against
 *      an older header (they just don't see the new tail fields).
 *   4. Opaque handles (CidsEngineHandle, etc.) are void* — callers never
 *      touch internal layout, so the real C++ class behind it is free to
 *      change completely between releases without breaking Desktop/Mobile.
 *   5. Ownership crossing the boundary is always explicit: cids_*_create()
 *      is paired with exactly one cids_*_destroy(); never expect the callee
 *      to free memory it didn't allocate, and vice versa.
 * ------------------------------------------------------------------------- */

#include <stdint.h>

#if defined(_WIN32)
  #ifdef CIDS_ABI_BUILDING_DLL
    #define CIDS_API __declspec(dllexport)
  #else
    #define CIDS_API __declspec(dllimport)
  #endif
  #define CIDS_CALL __cdecl
#else
  #define CIDS_API __attribute__((visibility("default")))
  #define CIDS_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Semantic version of the ABI itself (NOT the product version).
 * Bump MAJOR on any breaking layout change. Desktop/Mobile call
 * cids_abi_version() at startup and refuse to load on MAJOR mismatch. */
#define CIDS_ABI_VERSION_MAJOR 1
#define CIDS_ABI_VERSION_MINOR 0

typedef struct CidsAbiVersion {
    uint32_t struct_size;
    uint32_t major;
    uint32_t minor;
} CidsAbiVersion;

CIDS_API void CIDS_CALL cids_abi_version(CidsAbiVersion* out_version);

/* Opaque engine handle. Real type is cids::core::Engine*, but no caller
 * outside Core/src is ever allowed to know that. */
typedef struct CidsEngine* CidsEngineHandle;

/* Uniform status codes instead of C++ exceptions at the boundary. */
typedef enum CidsStatus {
    CIDS_OK                  = 0,
    CIDS_ERR_INVALID_ARG     = 1,
    CIDS_ERR_OUT_OF_MEMORY   = 2,
    CIDS_ERR_ENGINE_BUSY     = 3,
    CIDS_ERR_UNKNOWN         = 999
} CidsStatus;

CIDS_API CidsStatus CIDS_CALL cids_engine_create(CidsEngineHandle* out_handle);
CIDS_API CidsStatus CIDS_CALL cids_engine_destroy(CidsEngineHandle handle);

#ifdef __cplusplus
}
#endif

#endif /* CIDS_ABI_H */
