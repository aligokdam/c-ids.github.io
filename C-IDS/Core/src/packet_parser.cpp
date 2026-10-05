#include "packet_parser.hpp"

namespace cids::core {

std::optional<ParsedHeader> parse_header(std::span<const std::byte> data) noexcept {
    // Bounds check FIRST, before touching any byte — this is the pattern
    // every parser in Core must follow (see docs/SECURITY_POLICY.md,
    // "parse, don't assume").
    constexpr size_t kMinHeaderSize = 2;
    if (data.size() < kMinHeaderSize) {
        return std::nullopt;
    }

    ParsedHeader header{};
    header.protocol      = static_cast<uint8_t>(data[0]);
    header.header_length = static_cast<uint16_t>(data[1]);
    return header;
}

}  // namespace cids::core
