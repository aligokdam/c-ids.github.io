import Foundation
import CIDSCBridge

/// Public Swift API. Application code using this package never imports
/// CIDSCBridge directly and never sees an UnsafeMutableRawPointer — the
/// same "one safe wrapper class" pattern as CidsEngineClient.cs on Desktop.
public final class CIDSEngine {

    /// Wraps the opaque CidsEngineHandle. Deinit guarantees exactly-once
    /// destruction, mirroring SafeHandle on the C# side and unique_ptr on
    /// the C++ side — the same ownership discipline enforced at all three
    /// layers of the stack.
    private var handle: CidsEngineHandle?

    public struct DetectionEvent {
        public let signatureId: UInt32
        public let severity: UInt32
        public let timestampNs: UInt64
        public let description: String
    }

    public var onDetection: ((DetectionEvent) -> Void)?

    // Bridges the C callback into `onDetection` via a Swift Unmanaged
    // reference to `self` passed as user_context — required because C
    // function pointers cannot capture Swift closures directly.
    private var callbackBox: CallbackBox?

    public init() throws {
        var newHandle: CidsEngineHandle?
        let status = cids_engine_create(&newHandle)
        guard status == CIDS_OK, let created = newHandle else {
            throw CIDSError.creationFailed(status)
        }
        self.handle = created

        let box = CallbackBox(owner: self)
        self.callbackBox = box
        let ctx = Unmanaged.passUnretained(box).toOpaque()

        cids_engine_set_detection_callback(created, { evtPtr, ctxPtr in
            guard let evtPtr, let ctxPtr else { return }
            let box = Unmanaged<CallbackBox>.fromOpaque(ctxPtr).takeUnretainedValue()
            // Copy every field out of evtPtr NOW — the pointee is only
            // valid for the duration of this C call, same contract as
            // documented in packet_abi.h and honored on the C# side.
            let evt = evtPtr.pointee
            let description = evt.description.map { String(cString: $0) } ?? ""
            let event = DetectionEvent(
                signatureId: evt.signature_id,
                severity: evt.severity,
                timestampNs: evt.timestamp_ns,
                description: description
            )
            box.owner?.onDetection?(event)
        }, ctx)
    }

    public func ingest(payload: [UInt8], sourceIp: UInt32, destIp: UInt32,
                        sourcePort: UInt16, destPort: UInt16, protocolId: UInt8) throws {
        guard let handle else { throw CIDSError.engineReleased }

        var status: CidsStatus = CIDS_OK
        payload.withUnsafeBufferPointer { buffer in
            var view = CidsPacketView()
            view.struct_size = UInt32(MemoryLayout<CidsPacketView>.size)
            view.timestamp_ns = UInt64(Date().timeIntervalSince1970 * 1_000_000_000)
            view.source_ip = sourceIp
            view.dest_ip = destIp
            view.source_port = sourcePort
            view.dest_port = destPort
            view.protocol = protocolId
            view.payload = buffer.baseAddress
            view.payload_length = UInt32(buffer.count)
            // view.payload is only valid inside this closure — matches the
            // "non-owning, call-duration-only" contract at the ABI boundary.
            status = cids_engine_ingest_packet(handle, &view)
        }

        if status != CIDS_OK {
            throw CIDSError.ingestFailed(status)
        }
    }

    deinit {
        if let handle {
            cids_engine_destroy(handle)
        }
    }
}

public enum CIDSError: Error {
    case creationFailed(CidsStatus)
    case ingestFailed(CidsStatus)
    case engineReleased
}

/// Small helper so the C callback's `void* user_context` can carry a weak
/// reference back to the owning CIDSEngine without creating a retain cycle
/// (the engine owns callbackBox, callbackBox only weakly refs the engine).
private final class CallbackBox {
    weak var owner: CIDSEngine?
    init(owner: CIDSEngine) { self.owner = owner }
}
