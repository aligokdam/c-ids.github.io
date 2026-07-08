#pragma once

#include <span>
#include <vector>

#include "signature_registry.hpp"

namespace cids::core {

struct Packet;  // fwd-declared in engine.hpp; full def not needed here

struct Match {
    uint32_t signature_id{};
    uint32_t severity{};
    std::string description;
};

// Stateless evaluator over a SignatureRegistry it does not own — holds
// only a non-owning reference, since the registry's lifetime is always
// bound to the owning Engine (see engine.hpp: matcher_ constructed from
// *signatures_, both owned by the same Engine instance).
class RuleMatcher {
public:
    explicit RuleMatcher(const SignatureRegistry& registry) : registry_(registry) {}

    [[nodiscard]] std::vector<Match> evaluate(const Packet& packet) const;

private:
    const SignatureRegistry& registry_;
};

}  // namespace cids::core
