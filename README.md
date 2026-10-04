# C-IDS

C-IDS is a cross-platform, modular intrusion detection core. The detection logic is written once in C++20 and exposed through a small, stable C ABI, so desktop (.NET) and mobile (Swift) hosts can use the same engine without keeping their own copy of the rules.

The repository also contains the project website, including a client-side browser demo of the rule-matching logic:
<https://aligokdam.github.io/c-ids.github.io/>

> **Project status:** the C++ core builds and its unit tests pass. The .NET and Swift layers are interop scaffolds: the binding code is present, but each still needs the setup steps described below before it can be used in an application. See [Known issues](#known-issues).

---

## Contents

- [Architecture](#architecture)
- [Requirements](#requirements)
- [Get the source](#get-the-source)
- [C++ core](#c-core)
  - [Linux](#linux)
  - [macOS](#macos)
  - [Windows](#windows)
  - [Build options](#build-options)
- [.NET bindings](#net-bindings)
- [Swift / iOS bindings](#swift--ios-bindings)
- [Browser demo](#browser-demo)
- [Repository structure](#repository-structure)
- [Known issues](#known-issues)
- [Security and privacy](#security-and-privacy)
- [License](#license)

---

## Architecture

```
┌──────────────────────────────┬──────────────────────────────┐
│  Desktop host (C# / .NET 8)  │  Mobile host (Swift, iOS 15+) │
└──────────────┬───────────────┴───────────────┬──────────────┘
               │ P/Invoke (LibraryImport)      │ C module map
┌──────────────▼───────────────────────────────▼──────────────┐
│  cids_abi        extern "C" shim — the only stable interface │
└──────────────┬──────────────────────────────────────────────┘
               │ internal C++ calls
┌──────────────▼──────────────────────────────────────────────┐
│  cids_core_impl  C++20 engine: packet parser, rule matcher,  │
│                  signature registry, plugin dispatcher       │
└──────────────┬──────────────────────────────────────────────┘
               │ IPluginSink (optional)
┌──────────────▼──────────────────────────────────────────────┐
│  Plugins/PythonHost   embedded Python plugin host (opt-in)   │
└─────────────────────────────────────────────────────────────┘
```

### C++ core

- `cids_core_impl` (static library) holds the real implementation and is free to use modern C++: STL, templates, exceptions, RAII.
- `cids_abi` (shared library: `.so` / `.dylib` / `.dll`; a static `.framework` on iOS) exposes only `extern "C"` functions, POD structs and opaque handles. It is the only artifact the .NET and Swift layers link against.
- Exceptions never cross the ABI boundary; symbol visibility is hidden by default so only the documented functions are exported.
- Public headers: `C-IDS/Core/include/cids/abi.h` and `packet_abi.h`.

Details: [`C-IDS/docs/ARCHITECTURE.md`](C-IDS/docs/ARCHITECTURE.md) and [`C-IDS/docs/ABI_CONTRACT.md`](C-IDS/docs/ABI_CONTRACT.md).

### .NET bindings

`C-IDS/Desktop/Interop/` contains a type-safe P/Invoke layer:

- `NativeMethods.cs` — `LibraryImport` declarations for the C ABI and a `SafeHandle` subclass that guarantees the native engine is destroyed exactly once.
- `CidsEngineClient.cs` — the managed wrapper intended for application code.

The project file `C-IDS/Desktop/CIDS.Desktop/CIDS.Desktop.csproj` targets `net8.0-windows` (x64 and ARM64).

### Swift / iOS bindings

`C-IDS/Mobile/` is a Swift package (`CIDSMobile`, iOS 15+):

- `CIDSCBridge` — exposes the C headers to Swift through a module map.
- `CIDSMobile` — the public Swift API (`CIDSEngine`), so app code never handles raw pointers.
- `CidsAbiFramework` — a binary target that expects the core as `Vendor/CidsAbi.xcframework`.

---

## Requirements

| Component | Requirement |
|---|---|
| All | Git |
| C++ core | CMake **3.24+**, a C++20 compiler (GCC 11+, Clang 14+, or MSVC from Visual Studio 2022), Ninja (recommended) |
| C++ tests | Network access during configure — GoogleTest v1.15.2 is downloaded from GitHub via `FetchContent` |
| Python plugin host (optional) | Python 3 development headers and `pybind11` (CMake config package) |
| .NET bindings | Windows, .NET 8 SDK |
| Swift / iOS bindings | macOS, Xcode 15+ (Swift 5.9), iOS 15+ deployment target, the [ios-cmake](https://github.com/leetal/ios-cmake) toolchain file |

---

## Get the source

```bash
git clone https://github.com/aligokdam/c-ids.github.io.git
cd c-ids.github.io/C-IDS
```

All build commands below are run from the `C-IDS` folder (the one containing the top-level `CMakeLists.txt`). You can also download a ZIP from the [repository page](https://github.com/aligokdam/c-ids.github.io) (**Code → Download ZIP**).

---

## C++ core

The commands below pass two options that the current build needs on most machines:

- `-DCIDS_ENABLE_HARDENING=OFF` — with GCC/Clang the hardening flags add `-pie` to every target, including the shared library, and the link fails.
- `-DCLANG_TIDY_EXE=` — if `clang-tidy` is installed, the build runs it with `C-IDS/.clang-tidy`, which is not in the repository yet, and every compile step fails.

Both are explained in [Known issues](#known-issues).

### Linux

```bash
# Debian / Ubuntu
sudo apt update
sudo apt install build-essential cmake ninja-build git

# from c-ids.github.io/C-IDS
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCIDS_ENABLE_HARDENING=OFF -DCLANG_TIDY_EXE=
cmake --build build
ctest --test-dir build/Core
```

Output: `build/bin/libcids_abi.so` (plus the `libcids_abi.so.0` symlink chain). The test executable is `build/bin/cids_core_tests`.

Debug build with AddressSanitizer + UndefinedBehaviorSanitizer (enabled automatically for Debug):

```bash
cmake -B build-debug -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug \
      -DCIDS_ENABLE_HARDENING=OFF -DCLANG_TIDY_EXE=
cmake --build build-debug
ctest --test-dir build-debug/Core
```

Install headers and library (optional):

```bash
sudo cmake --install build
```

> Verified on Ubuntu 24.04 with GCC 13.3 and CMake 3.28: Release and Debug builds succeed and all 7 unit tests pass.

### macOS

```bash
xcode-select --install          # Command Line Tools, if not already installed
brew install cmake ninja

# from c-ids.github.io/C-IDS
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCIDS_ENABLE_HARDENING=OFF -DCLANG_TIDY_EXE=
cmake --build build
ctest --test-dir build/Core
```

Output: `build/bin/libcids_abi.dylib`.

### Windows

Install the build tools (or use an existing Visual Studio 2022 with the **Desktop development with C++** workload):

```powershell
winget install Microsoft.VisualStudio.2022.BuildTools
winget install Kitware.CMake
winget install Ninja-build.Ninja
```

Open **Developer PowerShell for VS 2022** (x64) so the MSVC compiler is on `PATH`, then:

```powershell
# from c-ids.github.io\C-IDS
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release -DCLANG_TIDY_EXE=
cmake --build build
ctest --test-dir build\Core
```

Output: `build\bin\cids_abi.dll` (import library in `build\lib\`).

The Ninja generator is recommended because it places the DLL exactly at `build\bin\cids_abi.dll`, which is the path the .NET project copies from. A Visual Studio generator (`-G "Visual Studio 17 2022" -A x64`) also works, but multi-config generators put the DLL in `build\bin\Release\`, so you would copy it yourself. With MSVC the hardening flags (`/GS /guard:cf /Qspectre`, `/CETCOMPAT`) are left on; if your toolchain rejects one of them, add `-DCIDS_ENABLE_HARDENING=OFF`.

### Build options

| Option | Default | Purpose |
|---|---|---|
| `CIDS_BUILD_SHARED` | `ON` | Build the core as a shared library |
| `CIDS_BUILD_TESTS` | `ON` | Build the GoogleTest suite (downloads GoogleTest) |
| `CIDS_ENABLE_SANITIZERS` | `ON` | ASan/UBSan in Debug builds (GCC/Clang) |
| `CIDS_ENABLE_HARDENING` | `ON` | Stack protector, FORTIFY, PIE / CFG flags |
| `CIDS_TREAT_WARNINGS_AS_ERRORS` | `ON` | `-Werror` / `/WX` |
| `CIDS_BUILD_PYTHON_PLUGIN` | `OFF` | Build the embedded Python plugin host |
| `CIDS_BUILD_FOR_IOS` | `OFF` | Build the ABI as a static `.framework` for iOS |

To skip the tests (and the GoogleTest download), add `-DCIDS_BUILD_TESTS=OFF`.
The Python plugin host is described in [`C-IDS/docs/PLUGIN_GUIDE.md`](C-IDS/docs/PLUGIN_GUIDE.md).

---

## .NET bindings

The .NET layer runs on Windows and needs the native DLL from the [Windows](#windows) build.

```powershell
winget install Microsoft.DotNet.SDK.8

# from c-ids.github.io\C-IDS, after building the core
dotnet build Desktop\CIDS.Desktop\CIDS.Desktop.csproj -c Release
```

The project copies `C-IDS\build\bin\cids_abi.dll` next to the managed output when that file exists, so P/Invoke finds `cids_abi` without an absolute path.

> **Current state:** `CIDS.Desktop.csproj` is declared as a `WinExe` but the repository does not contain an application entry point, and the project compiles only `Interop/NativeMethods.cs`. Building it as-is is therefore not expected to succeed. To use the bindings today, add `Interop/NativeMethods.cs` and `Interop/CidsEngineClient.cs` to your own .NET 8 project (with `AllowUnsafeBlocks` enabled, which `CidsEngineClient.cs` and the `LibraryImport` source generator require) and ship `cids_abi.dll` next to your executable. These steps were not compiled while writing this guide; treat them as the integration path, not a tested recipe.

---

## Swift / iOS bindings

The Swift package lives in `C-IDS/Mobile`, not at the repository root, so it cannot be added to Xcode by repository URL. Use it as a **local package** after providing the binary it depends on.

**1. Build the core for iOS.** This needs the [leetal/ios-cmake](https://github.com/leetal/ios-cmake) toolchain file saved as `C-IDS/cmake/ios.toolchain.cmake`, and an `Info.plist` for the framework at `C-IDS/Core/platform/ios/Info.plist` (referenced by `Core/CMakeLists.txt`, not yet in the repository).

```bash
# from c-ids.github.io/C-IDS — device and simulator slices
cmake -B build-ios -S . -G Xcode \
      -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake -DPLATFORM=OS64 \
      -DCIDS_BUILD_FOR_IOS=ON -DCIDS_BUILD_TESTS=OFF -DCLANG_TIDY_EXE=
cmake --build build-ios --config Release

cmake -B build-sim -S . -G Xcode \
      -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake -DPLATFORM=SIMULATORARM64 \
      -DCIDS_BUILD_FOR_IOS=ON -DCIDS_BUILD_TESTS=OFF -DCLANG_TIDY_EXE=
cmake --build build-sim --config Release
```

**2. Package an `.xcframework`** at the path `Package.swift` expects:

```bash
find build-ios build-sim -name "cids_abi.framework" -type d    # locate both slices
xcodebuild -create-xcframework \
  -framework <path-from-build-ios>/cids_abi.framework \
  -framework <path-from-build-sim>/cids_abi.framework \
  -output Mobile/Vendor/CidsAbi.xcframework
```

**3. Add the package to your app.** In Xcode choose **File → Add Package Dependencies… → Add Local…**, select the `C-IDS/Mobile` folder, and link the `CIDSMobile` product to your target. Or, from another Swift package:

```swift
// swift-tools-version:5.9
.package(path: "../c-ids.github.io/C-IDS/Mobile")
// ...
.product(name: "CIDSMobile", package: "Mobile")
```

**4. Run the package tests** on a simulator:

```bash
cd Mobile
xcodebuild test -scheme CIDSMobile -destination "platform=iOS Simulator,name=iPhone 15"
```

> The iOS steps depend on files that are not committed (toolchain file, `Info.plist`, `Vendor/CidsAbi.xcframework`) and were not run while writing this guide.

---

## Browser demo

The website includes a demo that simulates the core's rule matching in JavaScript.

- Online: <https://aligokdam.github.io/c-ids.github.io/#demo>
- Locally: open `index.html` in a browser, or serve the repository root:

  ```bash
  python3 -m http.server 8000     # then open http://localhost:8000/#demo
  ```

How to use it:

1. Click **Load sample traffic**, or paste your own lines.
2. Use one packet per line: `PROTO SRC_IP:PORT DST_IP:PORT` (ICMP lines omit ports), e.g. `TCP 10.0.0.22:44110 45.83.12.9:22`.
3. Click **Run detection**. Matching lines are tagged with a rule (SSH brute force, port scan, ping sweep, UDP flood) and a severity: low (1–29), medium (30–69), high (70+).

The demo is a simplified simulation for illustration; it is not the C++ engine compiled to the web and not a production IDS.

---

## Repository structure

```
c-ids.github.io/
├── index.html                  # project website + browser demo (static, no build step)
├── README.md
├── LICENSE                     # MIT
└── C-IDS/
    ├── CMakeLists.txt          # top-level build and options
    ├── README.md               # short developer notes
    ├── cmake/
    │   └── CidsCompilerHardening.cmake
    ├── Core/
    │   ├── CMakeLists.txt
    │   ├── include/cids/       # public C ABI headers (abi.h, packet_abi.h)
    │   ├── src/                # C++20 implementation + abi_shim.cpp
    │   └── tests/              # GoogleTest suite
    ├── Desktop/
    │   ├── CIDS.Desktop/CIDS.Desktop.csproj
    │   └── Interop/            # NativeMethods.cs, CidsEngineClient.cs
    ├── Mobile/
    │   ├── Package.swift
    │   ├── Sources/CIDSCBridge/  # module map + C headers
    │   ├── Sources/CIDSMobile/   # CIDSEngine.swift
    │   └── Tests/CIDSMobileTests/
    ├── Plugins/
    │   ├── PythonHost/         # optional embedded Python host
    │   └── examples/
    └── docs/
        ├── ARCHITECTURE.md
        ├── ABI_CONTRACT.md
        ├── PLUGIN_GUIDE.md
        └── SECURITY_POLICY.md
```

---

## Known issues

| Issue | Effect | Workaround |
|---|---|---|
| `C-IDS/.clang-tidy` is referenced by `CidsCompilerHardening.cmake` but not committed | If `clang-tidy` is on `PATH`, every compile fails with "can't read config-file" | Configure with `-DCLANG_TIDY_EXE=` (or add a `.clang-tidy` file) |
| Hardening adds `-fPIE` / `-pie` to all targets on GCC/Clang | Linking `libcids_abi` as a shared library fails | Configure with `-DCIDS_ENABLE_HARDENING=OFF` |
| `ctest --test-dir build` finds no tests | `enable_testing()` is called in `Core/` | Run `ctest --test-dir build/Core` |
| .NET project has no entry point | `dotnet build` of `CIDS.Desktop.csproj` does not produce an app | Use the `Interop/` sources in your own project (see [.NET bindings](#net-bindings)) |
| iOS build inputs not committed | `Core/platform/ios/Info.plist`, `cmake/ios.toolchain.cmake` and `Mobile/Vendor/CidsAbi.xcframework` are missing | Provide them as described in [Swift / iOS bindings](#swift--ios-bindings) |

---

## Security and privacy

- **Local-first.** The core analyzes data inside the host process. The source code in this repository contains no telemetry and no network reporting.
- **Memory-safety policy.** No raw owning pointers, RAII throughout, exceptions contained at the ABI boundary, sanitizers in Debug builds. See [`C-IDS/docs/SECURITY_POLICY.md`](C-IDS/docs/SECURITY_POLICY.md).
- **Browser demo.** Runs entirely in your browser. Traffic you enter is processed in memory on your device and is never sent to a server. The site stores only two preferences in `localStorage` (`cids-lang`, `cids-theme`), uses no cookies and no analytics. Fonts are loaded from Google Fonts, and the site is hosted on GitHub Pages. The full notice is on the site's **Privacy** page (`#privacy`); usage terms are on the **Terms** page (`#terms`).
- **Responsible use.** C-IDS is intended for security research, education and development. Only analyze traffic and systems you own or are authorized to examine.

---

## License

Released under the [MIT License](LICENSE).
