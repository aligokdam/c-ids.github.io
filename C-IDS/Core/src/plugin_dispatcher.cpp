#include "plugin_dispatcher.hpp"

// Header-only for now; kept as its own translation unit so future
// cross-cutting concerns (plugin load-order, error isolation per plugin,
// timeout/kill-switch for misbehaving plugins) have a natural home.
