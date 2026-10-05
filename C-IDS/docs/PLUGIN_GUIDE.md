# Plugin Guide

C-IDS is extensible through `cids::core::IPluginSink`
(`Core/src/plugin_dispatcher.hpp`). Core has zero compile-time or runtime
dependency on any specific plugin runtime — Python is the first
implementation, not a special case baked into `Engine`.

## Adding the Python plugin host

1. Build with `-DCIDS_BUILD_PYTHON_PLUGIN=ON` (requires Python 3 dev
   headers and `pybind11`, resolved via vcpkg/conan — see
   `Plugins/PythonHost/CMakeLists.txt`).
2. At startup, register the sink once:

   ```cpp
   auto python_sink = std::make_shared<cids::plugins::python::PythonPluginSink>(
       "Plugins/examples"
   );
   engine_internal_plugin_dispatcher->register_sink(python_sink);
   ```

   (Wiring this into `cids_engine_create`'s ABI surface — e.g. a
   `cids_engine_load_python_plugins(handle, path)` function — is the next
   increment; the dispatcher API already supports it today.)

3. Drop a `.py` file into that directory defining:

   ```python
   def on_detection(event: dict) -> None:
       ...
   ```

   See `Plugins/examples/auto_block_high_severity.py` for the full
   contract (allowed keys, threading/GIL notes, exception handling).

## Design constraints plugins must respect

- `on_detection` runs **synchronously** on the engine's calling thread
  while the process holds the Python GIL. Long-running work should be
  handed off to a queue/worker inside the plugin, not done inline —
  otherwise it blocks packet ingestion.
- Exceptions raised inside a plugin are caught and logged by
  `PythonPluginSink`; they never propagate into `Engine` and never crash
  the process. A broken plugin degrades to "silently does nothing," which
  is intentional (fail-safe, not fail-open on detections already computed
  by the C++ matcher).
- Plugins currently run **in-process**, with the same privileges as Core.
  For v1 / local-first single-user deployments this is an accepted
  trade-off. Planned hardening (tracked, not yet implemented):
  - out-of-process plugin execution (separate process per plugin,
    communicating over a local pipe/socket, so a plugin crash or malicious
    plugin cannot touch Core's memory at all);
  - a per-plugin manifest declaring required capabilities (network access,
    filesystem paths) enforced via OS sandboxing primitives
    (AppContainer on Windows, seccomp/Landlock on Linux);
  - a wall-clock timeout per `on_detection` invocation.

## Adding a native (non-Python) plugin instead

Implement `cids::core::IPluginSink` directly in C++ and register a
`std::shared_ptr` to it the same way — no Python involved at all. This is
the right choice for latency-sensitive plugins (e.g. an in-process
firewall rule updater) where crossing into an interpreter isn't
acceptable.
