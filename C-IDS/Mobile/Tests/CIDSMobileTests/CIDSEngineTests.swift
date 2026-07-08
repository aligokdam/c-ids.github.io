import XCTest
@testable import CIDSMobile

final class CIDSEngineTests: XCTestCase {
    func testEngineCreationAndDestruction() throws {
        let engine = try CIDSEngine()
        // deinit runs at end of scope — asserts no crash / no leak under ASan.
        _ = engine
    }

    func testIngestEmptyPayloadSucceeds() throws {
        let engine = try CIDSEngine()
        try engine.ingest(payload: [], sourceIp: 0, destIp: 0,
                           sourcePort: 0, destPort: 0, protocolId: 0)
    }
}
