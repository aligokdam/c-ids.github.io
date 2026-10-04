# ABI Contract

This document is the actual promise made to Desktop (C#) and Mobile
(Swift) consumers. If a change violates a rule here, it is a **breaking
change** and must bump `CIDS_ABI_VERSION_MAJOR` in `Core/include/cids/abi.h`.

## What is covered

Only symbols declared in `Core/include/cids/abi.h` and
`Core/include/cids/packet_abi.h`, exported via `CIDS_API`. Nothing inside
`Core/src/` is covered — those files can change freely between releases.

## Rules

1. **C linkage only.** Every exported function is `extern "C"`. No name
   mangling dependency, no calling-convention ambiguity (`__cdecl`
   explicitly pinned via `CIDS_CALL`).
2. **No STL types cross the boundary.** No `std::string`, `std::vector`,
   `std::function`, no C++ exceptions. Only fixed-width integers, POD
   structs, and opaque `void*` handles.
3. **Structs are append-only.** Every struct begins with `struct_size`.
   New fields may only be added at the *end* of a struct, and old binaries
   built against a smaller `struct_size` must remain valid — new fields are
   simply ignored by them. Removing, reordering, or resizing an existing
   field is a MAJOR-version-breaking change.
4. **Explicit ownership at every call.** Every `cids_*_create` has exactly
   one paired `cids_*_destroy`. Every pointer passed *into* a function
   (e.g. `payload` in `CidsPacketView`) is caller-owned and valid only for
   the duration of that call — Core never frees it, never stores it past
   the call, and never assumes it remains valid afterward.
5. **No callback re-entrancy assumptions.** `CidsDetectionCallback` is
   invoked synchronously on the calling thread. Any pointer inside the
   passed `CidsDetectionEvent` (e.g. `description`) is valid *only* for the
   duration of the callback — hosts must copy what they need before
   returning (see `CidsEngineClient.cs::OnNativeDetection` and
   `CIDSEngine.swift`'s callback closure for the reference pattern).
6. **Status codes, not exceptions.** Every fallible function returns
   `CidsStatus`. `abi_shim.cpp` catches every C++ exception before it can
   unwind across the boundary (undefined behavior in mixed-runtime builds,
   e.g. MSVC exception tables vs. a C# host).
7. **Version check at load time.** Hosts call `cids_abi_version()` at
   startup and refuse to proceed if `major` doesn't match the version they
   were built against — see `docs/PLUGIN_GUIDE.md` for the same rule
   applied to plugins.

## Process for changing the ABI

1. Add new fields at the end of the relevant struct, or add a brand-new
   function — bump `CIDS_ABI_VERSION_MINOR`.
2. If an existing field must change meaning, size, or be removed — bump
   `CIDS_ABI_VERSION_MAJOR`, and update `NativeMethods.cs` and
   `CIDSCBridge`'s copied headers in the same pull request (a CI check,
   `scripts/sync_abi_headers.sh`, diffs `Core/include/cids/*_abi.h`
   against the copies under `Mobile/Sources/CIDSCBridge/include/` and
   fails the build on drift).
3. Update this document's rule list if the change introduces a new pattern.
