# Architecture

C-IDS is a local-first security analysis engine built as three concentric
layers, each with a single responsibility:

```
┌─────────────────────────────────────────────────────────────┐
│  Desktop (C#/.NET)         Mobile (Swift)                    │
│  - UI, persistence, OS-level packet capture integration      │
└───────────────┬───────────────────────┬─────────────────────┘
                │ P/Invoke              │ C module map
┌───────────────▼───────────────────────▼─────────────────────┐
│  cids_abi  (extern "C" shim, Core/src/abi_shim.cpp)          │
│  - the ONLY code with a stable binary interface promise       │
└───────────────┬───────────────────────────────────────────────┘
                │ internal C++ calls only
┌───────────────▼───────────────────────────────────────────────┐
│  cids_core_impl (real C++20: Engine, RuleMatcher,             │
│                  SignatureRegistry, PluginDispatcher)          │
└───────────────┬───────────────────────────────────────────────┘
                │ IPluginSink interface
┌───────────────▼───────────────────────────────────────────────┐
│  Plugins/PythonHost (optional, CIDS_BUILD_PYTHON_PLUGIN=ON)    │
└─────────────────────────────────────────────────────────────┘
```

## Why the split into `cids_core_impl` + `cids_abi`

This is the single most important structural decision in the repo, and it
answers the brief's ABI-stability question directly:

- **`cids_core_impl`** is a static library. It uses exceptions, STL
  containers, templates, RAII — everything modern C++ offers, with no
  constraints from any consumer's language or compiler.
- **`cids_abi`** is a thin shared library (`.dll` / `.dylib` / static
  `.framework` on iOS). It exposes *only* `extern "C"` functions operating
  on POD structs and opaque handles. It is the only artifact Desktop and
  Mobile ever link against.

Because Desktop and Mobile never see `cids_core_impl`'s types, that library
is free to change *completely* between releases — new classes, different
inheritance, different STL usage, even a different C++ standard version —
without forcing a rebuild of the C# or Swift layers. Only changes to the
handful of functions declared in `Core/include/cids/*_abi.h` require
coordinated updates across all three languages. See `ABI_CONTRACT.md`.

## Directory layout

```
C-IDS/
├── CMakeLists.txt              # top-level orchestrator, global options
├── cmake/
│   └── CidsCompilerHardening.cmake
├── .clang-tidy                 # static-analysis policy (see SECURITY_POLICY.md)
├── Core/                       # C++20, platform-independent
│   ├── CMakeLists.txt
│   ├── include/cids/           # PUBLIC, ABI-stable headers only
│   │   ├── abi.h
│   │   └── packet_abi.h
│   ├── src/                    # PRIVATE C++ implementation
│   │   ├── engine.hpp/.cpp
│   │   ├── signature_registry.hpp/.cpp
│   │   ├── rule_matcher.hpp/.cpp
│   │   ├── packet_parser.hpp/.cpp
│   │   ├── plugin_dispatcher.hpp/.cpp
│   │   └── abi_shim.cpp        # the ONLY file touching both worlds
│   └── tests/                  # GoogleTest: engine logic + ABI boundary
├── Desktop/                     # C#/.NET host
│   ├── CIDS.Desktop/CIDS.Desktop.csproj
│   └── Interop/
│       ├── NativeMethods.cs     # raw P/Invoke surface
│       └── CidsEngineClient.cs  # safe public wrapper
├── Mobile/                       # Swift package
│   ├── Package.swift
│   └── Sources/
│       ├── CIDSCBridge/          # raw C import via module map
│       └── CIDSMobile/           # safe public Swift API
├── Plugins/
│   ├── PythonHost/               # optional embedded-Python plugin runtime
│   └── examples/
└── docs/
    ├── ARCHITECTURE.md           # this file
    ├── ABI_CONTRACT.md
    ├── SECURITY_POLICY.md
    └── PLUGIN_GUIDE.md
```

## Data flow for one packet

1. OS-level capture (Desktop: e.g. a WinDivert/npcap integration; Mobile:
   NEPacketTunnelProvider) hands raw bytes to the host language.
2. Host language builds a `CidsPacketView` (C#) / fills a Swift struct,
   pointing at its own buffer — no copy into Core yet.
3. `cids_engine_ingest_packet` crosses the ABI boundary. `abi_shim.cpp`
   validates the pointer/length pair, then constructs a `std::span` view
   (still zero-copy) and calls `Engine::ingest_packet`.
4. `Engine` locks its mutex, runs `RuleMatcher::evaluate` against the
   `SignatureRegistry`, and for every match: invokes the host's detection
   callback *and* fans the event out to `PluginDispatcher` (native or
   Python plugins), synchronously, on the calling thread.
5. Any exception inside step 4 is caught inside `Engine::ingest_packet`
   and never crosses back into the ABI shim as a C++ exception.
