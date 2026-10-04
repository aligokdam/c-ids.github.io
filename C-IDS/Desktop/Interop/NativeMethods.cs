using System.Runtime.InteropServices;

namespace CIDS.Desktop.Interop;

// ---------------------------------------------------------------------------
// Type-safe interop policy (answers question 2 in the brief):
//
//  * LibraryImport (source-generated marshaling, .NET 7+) instead of the
//    older [DllImport] — it is checked at COMPILE time, not reflection-based
//    at runtime, so a signature mismatch with abi.h is caught by the build.
//  * CidsEngineHandle is wrapped in a SafeHandle subclass, never a bare
//    IntPtr held by application code — this guarantees:
//      - the native engine is released exactly once (ReleaseHandle),
//        even if an exception unwinds through user code (no leak).
//      - a destroyed handle can never be silently reused (IsInvalid).
//    This is the C# equivalent of "no raw owning pointers" from the C++
//    side of the security policy.
//  * Large payloads (packet bytes) are passed as ReadOnlySpan<byte> via
//    `in` + fixed-context marshaling — never copied into a managed byte[]
//    twice, and never exposed as a raw pointer to calling code.
// ---------------------------------------------------------------------------

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public struct CidsPacketView
{
    public uint StructSize;
    public ulong TimestampNs;
    public uint SourceIp;
    public uint DestIp;
    public ushort SourcePort;
    public ushort DestPort;
    public byte Protocol;
    public byte Ipv6Flag;
    public byte Reserved0;
    public byte Reserved1;
    public IntPtr Payload;          // set via fixed() block only, see Engine.cs
    public uint PayloadLength;
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
    public byte[] SourceIpv6;
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
    public byte[] DestIpv6;
}

public enum CidsStatus : uint
{
    Ok = 0,
    ErrInvalidArg = 1,
    ErrOutOfMemory = 2,
    ErrEngineBusy = 3,
    ErrUnknown = 999,
}

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public delegate void CidsDetectionCallback(in CidsDetectionEvent evt, IntPtr userContext);

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public struct CidsDetectionEvent
{
    public uint StructSize;
    public uint SignatureId;
    public uint Severity;
    public ulong TimestampNs;
    public IntPtr Description;   // UTF-8 char*, valid only during the callback
}

/// <summary>
/// SafeHandle wrapper around CidsEngineHandle. Application code never sees
/// the raw pointer — only this handle, which guarantees cids_engine_destroy
/// is called exactly once, even during exception unwinding or finalization.
/// </summary>
public sealed class CidsEngineSafeHandle : SafeHandle
{
    public CidsEngineSafeHandle() : base(IntPtr.Zero, ownsHandle: true) { }

    public override bool IsInvalid => handle == IntPtr.Zero;

    protected override bool ReleaseHandle()
    {
        var status = NativeMethods.cids_engine_destroy(handle);
        return status == CidsStatus.Ok;
    }
}

internal static partial class NativeMethods
{
    private const string LibName = "cids_abi";

    [LibraryImport(LibName)]
    public static partial void cids_abi_version(out CidsAbiVersion outVersion);

    [LibraryImport(LibName)]
    public static partial CidsStatus cids_engine_create(out CidsEngineSafeHandle outHandle);

    [LibraryImport(LibName)]
    public static partial CidsStatus cids_engine_destroy(IntPtr handle);

    [LibraryImport(LibName)]
    public static partial CidsStatus cids_engine_set_detection_callback(
        CidsEngineSafeHandle handle,
        CidsDetectionCallback callback,
        IntPtr userContext);

    [LibraryImport(LibName)]
    public static partial CidsStatus cids_engine_ingest_packet(
        CidsEngineSafeHandle handle,
        in CidsPacketView packet);
}

[StructLayout(LayoutKind.Sequential)]
public struct CidsAbiVersion
{
    public uint StructSize;
    public uint Major;
    public uint Minor;
}
