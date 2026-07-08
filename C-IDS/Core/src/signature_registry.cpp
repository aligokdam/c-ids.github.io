#include "signature_registry.hpp"

// Intentionally minimal for this scaffold: SignatureRegistry is header-only
// today. This translation unit exists so the CMake target has a stable
// place to grow into (e.g. loading signature sets from disk/DB) without
// callers ever needing to know — only the .hpp's public surface matters.
