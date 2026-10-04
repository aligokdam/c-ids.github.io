# Security Policy for the Core codebase

C-IDS's Core is a memory-safety-critical component: it parses untrusted,
attacker-controlled network data. The following rules are enforced by
tooling, not just convention — see the "Enforcement" column.

| Rule | Rationale | Enforcement |
|---|---|---|
| No raw owning pointers. Every heap allocation is wrapped in `std::unique_ptr`/`std::shared_ptr`, or lives inside a standard container. | Eliminates leaks and double-frees by construction — ownership is always encoded in the type. | `.clang-tidy`: `cppcoreguidelines-owning-memory`, `cppcoreguidelines-no-malloc` as build-breaking errors. |
| No manual `new`/`delete` outside of `abi_shim.cpp`'s single create/destroy pair. | Confines "raw lifetime management" to one auditable file instead of scattering it. | Code review + `cppcoreguidelines-owning-memory`. |
| No pointer arithmetic. Use `std::span`, iterators, or range-based loops. | Removes the single most common cause of buffer overflows. | `.clang-tidy`: `cppcoreguidelines-pro-bounds-pointer-arithmetic`. |
| Every buffer access is bounds-checked *before* the read, using the container's own size — never a caller-supplied length taken on faith. | Defends against a malicious/malformed length field, e.g. `payload_length` in `CidsPacketView`. | Pattern demonstrated in `packet_parser.cpp` ("parse, don't assume"); reviewed per-PR. |
| No C++ exception ever crosses the ABI boundary. | Exception unwinding across a C boundary (or between different compilers/runtimes) is undefined behavior. | `abi_shim.cpp` wraps every call in try/catch; `Engine::ingest_packet` is `noexcept` and catches internally. |
| All shared mutable state is guarded by `std::mutex`; no bare data races. | Multiple ingest calls may originate from different capture threads. | `std::lock_guard` in every method touching shared state (see `engine.cpp`); ThreadSanitizer run in CI. |
| Compiler warnings are errors (`-Werror` / `/WX`), with an aggressive warning set (`-Wconversion`, `-Wshadow`, `-Wold-style-cast`, ...). | Turns entire classes of latent bugs into build failures instead of runtime surprises. | `cmake/CidsCompilerHardening.cmake`, applied to every target via `cids_apply_hardening()`. |
| Debug builds run with AddressSanitizer + UndefinedBehaviorSanitizer. | Catches use-after-free, buffer overflows, and UB at test time, not in production. | `CIDS_ENABLE_SANITIZERS` option, wired into `CidsCompilerHardening.cmake`. |
| Release binaries are hardened: stack protector, `/GS` + Control Flow Guard on MSVC, `-D_FORTIFY_SOURCE=2`, PIE. | Defense-in-depth against the exploitation primitives that *do* slip through review. | `CIDS_ENABLE_HARDENING` option in `CidsCompilerHardening.cmake`. |
| `.clang-tidy` runs as part of every build (`CXX_CLANG_TIDY` target property), not just in CI. | A developer sees a violation the moment they compile locally, not days later in a PR check. | `cmake/CidsCompilerHardening.cmake`. |

## Threat model notes

- **Untrusted input boundary:** `CidsPacketView.payload` /
  `payload_length` are the primary attacker-controlled surface. Every
  parser downstream (`packet_parser.cpp`, `rule_matcher.cpp`) treats
  `payload_length` as a claim to verify against the actual `std::span`
  size, never as ground truth on its own.
- **Plugin surface:** Python plugins run with the same process privileges
  as Core. This is a deliberate v1 trade-off for local-first simplicity —
  see `docs/PLUGIN_GUIDE.md` for the planned out-of-process isolation.
- **Cross-language boundary:** the ABI contract in `docs/ABI_CONTRACT.md`
  is itself a security control — an inconsistent struct layout between
  Core and a host is a memory-safety bug, not just an interop bug.
