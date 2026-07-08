# C-IDS — Installation Guide

This guide walks you through downloading the repository and building each component (C++ core, .NET bindings, Swift bindings) from scratch. Every section below is **self-contained** — you can jump straight to the one you need without having run the others first.

> **Folder convention used throughout this guide**
> After cloning, you will have a folder named `c-ids.github.io`, and inside it a subfolder named `C-IDS` (this is where all the source code lives). Every command block below starts from a clearly stated location — follow the `cd` lines exactly and you won't hit "no such file or directory" errors, even if you're jumping between sections.

---

## 1. Get the Repository

**Option A — Clone with Git (recommended):**

```bash
git clone https://github.com/aligokdam/c-ids.github.io.git
```

This creates a `c-ids.github.io` folder in your current directory. Don't `cd` into it yet — each section below tells you exactly when and where to `cd`.

**Option B — Download as ZIP:**

1. Go to the [repository page](https://github.com/aligokdam/c-ids.github.io).
2. Click the green **Code** button.
3. Choose **Download ZIP** and extract it wherever you like.

You can also grab a tagged release (e.g. `v1.0.0`) directly from the [Releases page](https://github.com/aligokdam/c-ids.github.io/releases).

---

## 2. Prerequisites

| Platform | Component | Requirement |
|---|---|---|
| 🪟 Windows | C++ core | CMake ≥ 3.20, Visual Studio 2022 / MSVC v143 build tools |
| 🪟 Windows | .NET bindings | .NET 8 SDK |
| 🐧 Linux | C++ core | CMake ≥ 3.20, GCC ≥ 11 or Clang ≥ 14 (for C++20), Ninja or Make recommended |
| 🍎 macOS | C++ core | CMake ≥ 3.20, Xcode Command Line Tools or Clang ≥ 14 |
| 📱 iOS | Swift bindings | macOS + Xcode 15+, Swift Package Manager, iOS 15+ deployment target |
| All | — | Git |

> **Note:** The Swift/iOS bindings can only be built with the Xcode toolchain on macOS. Building the iOS target on Linux or Windows is not supported.

---

## 3. Build the C++ Core

The core is CMake-based and builds the same way on all three desktop platforms — only the generator and package manager differ. Pick your platform below.

### 🪟 Windows

Requires Visual Studio 2022 (or at least the MSVC v143 build tools) and CMake.

```powershell
git clone https://github.com/aligokdam/c-ids.github.io.git
cd c-ids.github.io\C-IDS
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Result: a `.dll` (with a matching `.lib`) is produced under `build\Release\`.

Alternatively, use Visual Studio's **"Open a local folder"** feature and point it at the `C-IDS` folder — Visual Studio will pick up the CMake project automatically.

> Missing Visual Studio Build Tools or CMake? Install them quickly with:
> `winget install Microsoft.VisualStudio.2022.BuildTools`
> `winget install Kitware.CMake`

### 🐧 Linux

Requires GCC ≥ 11 or Clang ≥ 14 (for C++20 support), CMake, and preferably Ninja.

```bash
# Debian/Ubuntu example
sudo apt update
sudo apt install build-essential cmake ninja-build git

git clone https://github.com/aligokdam/c-ids.github.io.git
cd c-ids.github.io/C-IDS
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

Result: a shared library (`.so`) is produced under `build/`. Optionally install it system-wide with `sudo cmake --install .`, or simply copy it next to your application's executable.

### 🍎 macOS

```bash
brew install cmake ninja
git clone https://github.com/aligokdam/c-ids.github.io.git
cd c-ids.github.io/C-IDS
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

Result: a `.dylib` is produced under `build/`. This library is used directly by macOS apps, and indirectly by iOS apps via the Swift Package (see Step 5).

> **Sanitizer builds (Linux/macOS):** to verify memory safety, add sanitizer flags to the CMake configure step, e.g.:
> `cmake .. -G Ninja -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"`

---

## 4. Build the .NET Bindings (Windows)

The .NET layer is a type-safe P/Invoke wrapper around the native core, using `SafeHandle` for lifetime management.

**Where to start from:**

- If you just finished **Step 3 (Windows)**, you are currently inside `c-ids.github.io\C-IDS\build`. Go back two levels first:
  ```powershell
  cd ..\..
  ```
  You should now be in `c-ids.github.io\`.

- If you're starting fresh in a new terminal instead, clone the repo and move into it:
  ```powershell
  git clone https://github.com/aligokdam/c-ids.github.io.git
  cd c-ids.github.io
  ```

Either way, once you're in the `c-ids.github.io` folder, run:

```powershell
cd C-IDS
dotnet restore
dotnet build -c Release
```

**Using it in your own app:** add a project reference to the built library (or copy the resulting package/DLL into your project), and make sure the native core library you built in Step 3 (the `.dll` from `build\Release\`) sits in the same directory as your app's executable.

---

## 5. Build the Swift (iOS) Bindings

The Swift bindings are distributed as a Swift Package. They wrap the C++ core inside an `.xcframework`, so application code calls into it directly from Swift with no Objective-C bridge required. Requires **macOS + Xcode 15+** and an **iOS 15.0+** deployment target.

### A) Via Xcode (recommended)

1. Open your iOS project in Xcode.
2. Go to **File → Add Package Dependencies…**
3. Enter the repository URL: `https://github.com/aligokdam/c-ids.github.io.git`
4. Choose a version rule (e.g. "Branch: main" or a specific tag), select the Swift binding target, and click **Add Package**.
5. In your app target, confirm the package appears under **Frameworks, Libraries, and Embedded Content**.

### B) Via `Package.swift` (as a dependency of your own Swift package)

```swift
// swift-tools-version:5.9
import PackageDescription

let package = Package(
    name: "YourApp",
    platforms: [.iOS(.v15)],
    dependencies: [
        .package(url: "https://github.com/aligokdam/c-ids.github.io.git", branch: "main")
    ],
    targets: [
        .target(
            name: "YourTarget",
            dependencies: [
                .product(name: "CIDS", package: "c-ids.github.io") // verify the exact product name in the repo's Package.swift
            ]
        )
    ]
)
```

### C) Via the command line (simulator or device)

**Where to start from:**

- If you just finished **Step 3 (macOS)**, you are currently inside `c-ids.github.io/C-IDS/build`. Go back two levels first:
  ```bash
  cd ../..
  ```
  You should now be in `c-ids.github.io/`.

- If you're starting fresh in a new terminal instead, clone the repo and move into it:
  ```bash
  git clone https://github.com/aligokdam/c-ids.github.io.git
  cd c-ids.github.io
  ```

Either way, once you're in the `c-ids.github.io` folder, run:

```bash
cd C-IDS
swift build
# or targeting an iOS simulator:
xcodebuild -scheme CIDS -destination "generic/platform=iOS Simulator"
```

> The `.xcframework` bundles both device (`arm64`) and simulator (`arm64`/`x86_64`) architectures, so no manual `lipo` merge step is needed.

---

## 6. Verify Your Installation

Once a build succeeds, link the resulting core library into a small test app and feed it a sample traffic log or packet. If the engine prints the expected `rule=...` output for matched signatures, your build is working correctly. You can compare against the live demo's output format here: <https://aligokdam.github.io/c-ids.github.io/>

---

## 7. Try It in the Browser

**// TRY IT IN THE BROWSER**
### Run the detector, no terminal required

A client-side simulation of the same rule-matching logic shared by the C++ core and its .NET and Swift bindings — paste or generate a traffic sample and see which lines a detection pass would flag, right here in the page.

👉 <https://aligokdam.github.io/c-ids.github.io/#demo>

---

## More Information

- **Docs & live demo:** <https://aligokdam.github.io/c-ids.github.io/>
- Architecture decisions, the ABI contract, and the security policy live in the `docs/` folder of the repository.
- **License:** MIT
