#include <gtest/gtest.h>

#include "cids/abi.h"
#include "cids/packet_abi.h"

TEST(AbiBoundary, VersionIsReported) {
    CidsAbiVersion version{};
    cids_abi_version(&version);
    EXPECT_EQ(version.major, static_cast<uint32_t>(CIDS_ABI_VERSION_MAJOR));
}

TEST(AbiBoundary, CreateDestroyRoundtrip) {
    CidsEngineHandle handle = nullptr;
    ASSERT_EQ(cids_engine_create(&handle), CIDS_OK);
    ASSERT_NE(handle, nullptr);
    EXPECT_EQ(cids_engine_destroy(handle), CIDS_OK);
}

TEST(AbiBoundary, NullHandleIsRejectedNotCrashed) {
    EXPECT_EQ(cids_engine_destroy(nullptr), CIDS_ERR_INVALID_ARG);
}

TEST(AbiBoundary, RejectsInconsistentPacketView) {
    CidsEngineHandle handle = nullptr;
    ASSERT_EQ(cids_engine_create(&handle), CIDS_OK);

    CidsPacketView view{};
    view.struct_size = sizeof(CidsPacketView);
    view.payload = nullptr;
    view.payload_length = 42;  // inconsistent: nonzero length, null pointer

    EXPECT_EQ(cids_engine_ingest_packet(handle, &view), CIDS_ERR_INVALID_ARG);

    cids_engine_destroy(handle);
}
