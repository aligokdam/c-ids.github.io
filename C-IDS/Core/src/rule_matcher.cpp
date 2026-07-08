#include "rule_matcher.hpp"
#include "engine.hpp"   // for the full Packet definition

#include <algorithm>

namespace cids::core {

std::vector<Match> RuleMatcher::evaluate(const Packet& packet) const {
    std::vector<Match> matches;

    for (const auto& sig : registry_.all()) {
        if (sig.pattern.empty() || sig.pattern.size() > packet.payload.size()) {
            continue;
        }

        // Bounds-safe substring search over std::span — no pointer
        // arithmetic, no manual index tracking past the buffer's actual size.
        auto it = std::search(
            packet.payload.begin(), packet.payload.end(),
            sig.pattern.begin(), sig.pattern.end()
        );

        if (it != packet.payload.end()) {
            matches.push_back(Match{
                .signature_id = sig.id,
                .severity     = sig.severity,
                .description  = sig.description,
            });
        }
    }

    return matches;
}

}  // namespace cids::core
