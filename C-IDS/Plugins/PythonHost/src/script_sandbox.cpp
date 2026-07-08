// script_sandbox.cpp
//
// Placeholder for hardening the embedded interpreter beyond module-level
// isolation, e.g.:
//   - wall-clock timeout per on_detection() call (watchdog thread that
//     calls PyErr_SetInterrupt on the embedded interpreter)
//   - restricting importable modules (no `os`, `subprocess`, `socket`
//     unless explicitly whitelisted per plugin in a manifest)
//   - resource limits via OS primitives (Job Objects on Windows,
///    rlimits/seccomp on Linux/macOS) around the process if plugins are
//     ever moved out-of-process (see docs/PLUGIN_GUIDE.md, "Future: out-
//     of-process plugin isolation").
