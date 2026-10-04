#pragma once

#include <cstdint>
#include <optional>
#include <span>

namespace cids::core {

// Placeholder for future protocol-specific parsing (e.g. reassembling
// TCP streams, decoding TLS ClientHello for JA3 fingerprinting, etc).
// Kept isolated from engine.cpp so parsing bugs are unit-testable without
// spinning up a full Engine.
struct ParsedHeader {
    uint8_t protocol{};
    uint16_t header_length{};
};

[[nodiscard]] std::optional<ParsedHeader> parse_header(std::span<const std::byte> data) noexcept;

}  // namespace cids::core
