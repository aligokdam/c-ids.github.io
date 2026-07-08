# C-IDS — Cyber-security Intrusion Detection & Analysis Engine

A local-first, cross-platform security analysis engine. No server, no
telemetry off-device: capture and analysis happen entirely on the user's
Windows machine or iOS device.

- **Core** — C++20 engine, Clean-Architecture-layered, exposed through a
  deliberately narrow, stable C ABI (see [`docs/ABI_CONTRACT.md`](docs/ABI_CONTRACT.md)).
- **Desktop** — a C#/.NET host consuming Core via type-safe P/Invoke.
- **Mobile** — a Swift package consuming Core via a C module map.
- **Plugins** — optional, sandboxed extensions (Python today; see
  [`docs/PLUGIN_GUIDE.md`](docs/PLUGIN_GUIDE.md)) that react to detections
  without Core ever depending on the plugin runtime itself.

Read [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) first — it explains
*why* the repo is laid out this way, not just what's where.

## Building Core

```bash
cmake -B build -S . \
    -DCIDS_BUILD_TESTS=ON \
    -DCIDS_ENABLE_SANITIZERS=ON        # Debug builds only
cmake --build build
ctest --test-dir build
```

For iOS:

```bash
cmake -B build-ios -S . \
    -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake \
    -DPLATFORM=OS64 \
    -DCIDS_BUILD_FOR_IOS=ON
cmake --build build-ios
```

(`cmake/ios.toolchain.cmake` — use the well-known
[leetal/ios-cmake](https://github.com/leetal/ios-cmake) toolchain file,
vendored or fetched via `FetchContent`.)

For the Python plugin host:

```bash
cmake -B build -S . -DCIDS_BUILD_PYTHON_PLUGIN=ON
```

## Repository map

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md#directory-layout) for
the annotated directory tree.

## Design principles

1. **ABI stability over convenience** — the C++ implementation is free to
   change; the `extern "C"` surface in `Core/include/cids/*_abi.h` is not,
   without a version bump (`docs/ABI_CONTRACT.md`).
2. **Zero-Trust memory policy** — no raw owning pointers, no pointer
   arithmetic, no exception ever crosses a language boundary
   (`docs/SECURITY_POLICY.md`).
3. **Plugins are guests, not family** — Core has no compile-time knowledge
   of Python or any other plugin runtime (`docs/PLUGIN_GUIDE.md`).

## License

TBD — add a `LICENSE` file appropriate to your intended distribution
before making the repository public.
