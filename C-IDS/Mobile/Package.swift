// swift-tools-version:5.9
import PackageDescription

let package = Package(
    name: "CIDSMobile",
    platforms: [.iOS(.v15)],
    products: [
        .library(name: "CIDSMobile", targets: ["CIDSMobile"]),
    ],
    targets: [
        // Binary artifact produced by Core's CMake build with
        // -DCIDS_BUILD_FOR_IOS=ON (see Core/CMakeLists.txt). Distributed
        // as an .xcframework so a single package works for device + simulator.
        .binaryTarget(
            name: "CidsAbiFramework",
            path: "Vendor/CidsAbi.xcframework"
        ),

        // Thin C target exposing the extern "C" ABI headers to Swift via
        // a module map — this is the ONLY target that imports the raw
        // C structs/handles directly.
        .target(
            name: "CIDSCBridge",
            dependencies: ["CidsAbiFramework"],
            path: "Sources/CIDSCBridge"
        ),

        // The actual public Swift API. Wraps CIDSCBridge so consuming iOS
        // app code never touches UnsafeMutablePointer or #imported C
        // structs directly — mirrors the Desktop/CidsEngineClient.cs split.
        .target(
            name: "CIDSMobile",
            dependencies: ["CIDSCBridge"],
            path: "Sources/CIDSMobile"
        ),

        .testTarget(
            name: "CIDSMobileTests",
            dependencies: ["CIDSMobile"],
            path: "Tests/CIDSMobileTests"
        ),
    ]
)
