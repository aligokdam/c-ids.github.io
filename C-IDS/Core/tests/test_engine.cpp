#include <gtest/gtest.h>

#include "engine.hpp"

using cids::core::Engine;
using cids::core::Packet;

TEST(Engine, RejectsInconsistentPayload) {
    Engine engine;

    Packet bad{};
    bad.payload = std::span<const std::byte>(
        static_cast<const std::byte*>(nullptr), 10  // null ptr, nonzero length
    );

    EXPECT_FALSE(engine.ingest_packet(bad));
}

TEST(Engine, AcceptsEmptyPacket) {
    Engine engine;
    Packet empty{};
    EXPECT_TRUE(engine.ingest_packet(empty));
}

TEST(Engine, DetectionSinkReceivesNoEventsWithoutSignatures) {
    Engine engine;
    bool called = false;
    engine.set_detection_sink([&](const auto&) { called = true; });

    std::vector<std::byte> payload(16, std::byte{0x41});
    Packet p{};
    p.payload = payload;

    EXPECT_TRUE(engine.ingest_packet(p));
    EXPECT_FALSE(called);  // no signatures registered -> no matches possible
}
