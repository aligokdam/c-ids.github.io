using System.Runtime.InteropServices;

namespace CIDS.Desktop.Interop;

/// <summary>
/// The only class application code should touch. Wraps the raw P/Invoke
/// surface in NativeMethods.cs so nothing above this layer ever sees an
/// IntPtr, a delegate marshaled as a function pointer, or a fixed buffer.
/// </summary>
public sealed class CidsEngineClient : IDisposable
{
    private readonly CidsEngineSafeHandle _handle;
    private readonly CidsDetectionCallback _callbackKeepAlive; // prevents GC of the delegate

    public event Action<DetectionEvent>? DetectionRaised;

    public CidsEngineClient()
    {
        var status = NativeMethods.cids_engine_create(out _handle);
        if (status != CidsStatus.Ok || _handle.IsInvalid)
        {
            throw new InvalidOperationException($"cids_engine_create failed: {status}");
        }

        // Keep a reference to the delegate for the client's lifetime — the
        // native side only stores a function pointer, so if this delegate
        // were garbage-collected the native callback would jump into freed
        // memory. This is the C# analogue of a dangling pointer.
        _callbackKeepAlive = OnNativeDetection;
        NativeMethods.cids_engine_set_detection_callback(_handle, _callbackKeepAlive, IntPtr.Zero);
    }

    public void IngestPacket(ReadOnlySpan<byte> payload, uint sourceIp, uint destIp,
                              ushort sourcePort, ushort destPort, byte protocol)
    {
        unsafe
        {
            fixed (byte* payloadPtr = payload)
            {
                var view = new CidsPacketView
                {
                    StructSize = (uint)Marshal.SizeOf<CidsPacketView>(),
                    TimestampNs = (ulong)DateTimeOffset.UtcNow.ToUnixTimeMilliseconds() * 1_000_000,
                    SourceIp = sourceIp,
                    DestIp = destIp,
                    SourcePort = sourcePort,
                    DestPort = destPort,
                    Protocol = protocol,
                    Payload = (IntPtr)payloadPtr,
                    PayloadLength = (uint)payload.Length,
                    SourceIpv6 = new byte[16],
                    DestIpv6 = new byte[16],
                };

                var status = NativeMethods.cids_engine_ingest_packet(_handle, in view);
                if (status != CidsStatus.Ok)
                {
                    throw new InvalidOperationException($"ingest_packet failed: {status}");
                }
            } // payloadPtr is only valid inside this fixed block, matching
              // the "valid only for call duration" contract in packet_abi.h
        }
    }

    private void OnNativeDetection(in CidsDetectionEvent evt, IntPtr userContext)
    {
        // Copy every field out of the native struct immediately — evt.Description
        // points into native memory that is invalid the instant this method
        // returns, so it MUST be marshaled to a managed string here and now.
        var description = Marshal.PtrToStringUTF8(evt.Description) ?? string.Empty;

        DetectionRaised?.Invoke(new DetectionEvent(
            evt.SignatureId, evt.Severity, evt.TimestampNs, description));
    }

    public void Dispose()
    {
        _handle.Dispose();  // triggers SafeHandle.ReleaseHandle exactly once
    }
}

public readonly record struct DetectionEvent(
    uint SignatureId, uint Severity, ulong TimestampNs, string Description);
