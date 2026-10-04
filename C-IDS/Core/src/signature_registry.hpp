#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace cids::core {

struct Signature {
    uint32_t id{};
    uint32_t severity{};
    std::string description;
    std::vector<std::byte> pattern;   // owned by value, no raw new[]
};

// Owns its Signature collection by value inside a std::vector — no
// container-of-raw-pointers anywhere, so lifetime is automatic (RAII)
// and there is nothing to leak or double-free.
class SignatureRegistry {
public:
    void add(Signature signature) { signatures_.push_back(std::move(signature)); }
    [[nodiscard]] const std::vector<Signature>& all() const noexcept { return signatures_; }

private:
    std::vector<Signature> signatures_;
};

}  // namespace cids::core
