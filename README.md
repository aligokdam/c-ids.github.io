# C-IDS (Cross-Platform Intrusion Detection System)

C-IDS is a high-performance, modular, and cross-platform Intrusion Detection System (IDS) core. Developed in **C++** for optimal performance, the project provides native bindings for **.NET** and **Swift** to ensure seamless integration across different development ecosystems.

## 🚀 About the Project

C-IDS is a flexible security engine designed to analyze network traffic and detect suspicious activities with minimal latency.

- **High Performance:** The core engine is built with C++ for efficient packet analysis.
- **Modular Architecture:** Easily extendable with new detection algorithms and custom filters.
- **Broad Compatibility:** Includes native bindings for .NET and Swift.
- **Cross-Platform:** Optimized for deployment on Windows, macOS, and Linux.
- **Local-first:** There is no server and no telemetry — every packet is inspected on the device it runs on, and nothing leaves that device.

## 🛠 Technical Stack

- **Core:** C++20 (packet analysis and engine management), built behind a stable `extern "C"` ABI boundary.
- **Build System:** CMake.
- **Bindings:**
  * **.NET / C#** — a type-safe P/Invoke layer (`LibraryImport` source-gen) using `SafeHandle` for lifetime management. Targets .NET 8.
  * **Swift** — a Swift Package that connects to the core through a C module map and ships as an `.xcframework`. Targets iOS 15+.

---

## 📦 İndirme ve Kurulum

Aşağıdaki adımlar, projeyi bilgisayarınıza indirip her bir bileşeni (C++ çekirdek, .NET binding'i, Swift binding'i) derlemeniz için gereken tüm bilgileri içerir.

### 1. Depoyu İndirme

**Git ile klonlama (önerilen):**

```bash
git clone https://github.com/aligokdam/c-ids.github.io.git
cd c-ids.github.io/C-IDS
```

**ZIP olarak indirme:**

1. [Depo sayfasına](https://github.com/aligokdam/c-ids.github.io) gidin.
2. Yeşil **Code** butonuna tıklayın.
3. **Download ZIP** seçeneğini seçin ve indirilen dosyayı istediğiniz konuma çıkarın.

Ayrıca [Releases](https://github.com/aligokdam/c-ids.github.io/releases) sayfasından etiketlenmiş sürümleri (örn. `v1.0.0`) doğrudan indirebilirsiniz.

### 2. Ön Gereksinimler

| Platform | Bileşen | Gereksinim |
| --- | --- | --- |
| 🪟 Windows | C++ Çekirdek | CMake ≥ 3.20, Visual Studio 2022 / MSVC v143 build araçları |
| 🪟 Windows | .NET Bağlayıcısı | .NET 8 SDK |
| 🐧 Linux | C++ Çekirdek | CMake ≥ 3.20, GCC ≥ 11 veya Clang ≥ 14 (C++20 desteği), Ninja/Make önerilir |
| 🍎 macOS | C++ Çekirdek | CMake ≥ 3.20, Xcode Command Line Tools veya Clang ≥ 14 |
| 📱 iOS | Swift Bağlayıcısı | macOS + Xcode 15+, Swift Package Manager, iOS 15+ dağıtım hedefi |
| Genel | — | Git |

> Not: Swift/iOS bağlayıcısı yalnızca macOS üzerindeki Xcode araç zinciriyle derlenebilir; Linux veya Windows üzerinde iOS hedefi derlemesi desteklenmez.

### 3. C++ Çekirdeğini Derleme

Proje CMake tabanlıdır ve Windows, Linux ile macOS'ta derlenebilir. Aşağıda her platform için ayrı talimatlar bulunur.

#### 🪟 Windows

Visual Studio 2022 (veya en az MSVC v143 build araçları) ve CMake kurulu olmalıdır.

```powershell
git clone https://github.com/aligokdam/c-ids.github.io.git
cd c-ids.github.io\C-IDS
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Alternatif olarak Visual Studio'nun **"Open a local folder"** özelliğiyle `C-IDS` klasörünü doğrudan açıp CMake entegrasyonunu kullanabilirsiniz. Derleme sonunda `build\Release\` altında bir `.dll` (ve eşlik eden `.lib`) dosyası oluşur.

> Visual Studio Build Tools yoksa `winget install Microsoft.VisualStudio.2022.BuildTools` veya `winget install Kitware.CMake` ile hızlıca kurabilirsiniz.

#### 🐧 Linux

GCC ≥ 11 veya Clang ≥ 14 (C++20 desteği için), CMake ve Ninja/Make önerilir.

```bash
# Debian/Ubuntu için gerekli paketler
sudo apt update
sudo apt install build-essential cmake ninja-build git

git clone https://github.com/aligokdam/c-ids.github.io.git
cd c-ids.github.io/C-IDS
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

Derleme sonunda `build/` altında bir `.so` paylaşımlı kütüphane dosyası oluşur. Sisteminizde kullanılabilir hale getirmek için isterseniz `sudo cmake --install .` çalıştırabilir veya kütüphaneyi doğrudan uygulamanızın çalıştırılabilir dosyasıyla aynı dizine kopyalayabilirsiniz.

#### 🍎 macOS

```bash
brew install cmake ninja
git clone https://github.com/aligokdam/c-ids.github.io.git
cd c-ids.github.io/C-IDS
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

Bu, `build/` altında bir `.dylib` üretir; bu dosya doğrudan macOS uygulamalarında, iOS için ise aşağıdaki Swift Package akışı üzerinden kullanılır.

> Bellek güvenliği doğrulamaları için ASan/UBSan etkin bir derleme yapmak isterseniz (Linux/macOS), CMake yapılandırmasına ilgili sanitizer bayraklarını (örn. `-DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"`) ekleyebilirsiniz.

### 4. .NET (Windows) Bağlayıcısını Kurma

.NET tarafı, `SafeHandle` ile yaşam döngüsü yönetimi yapan tip güvenli bir P/Invoke katmanıdır.

```bash
cd C-IDS  # .NET proje/çözüm dosyasının bulunduğu klasöre gidin
dotnet restore
dotnet build -c Release
```

Kendi .NET projenize eklemek için derlenmiş kütüphaneye proje referansı verin veya oluşan NuGet paketini/DLL'i projenize kopyalayın; ardından native C++ çekirdek kütüphanesinin (adım 3'te derlenen `.dll`) çalıştırılabilir dosyanızla aynı dizinde bulunduğundan emin olun.

### 5. 📱 iOS (Swift) Bağlayıcısını Kurma

Swift bağlayıcısı bir Swift Package olarak dağıtılır ve C++ çekirdeğini bir `.xcframework` içinde sarmalayarak Objective-C köprüsü olmadan doğrudan Swift'ten kullanılabilir hale getirir. Gereksinimler: **macOS + Xcode 15+** ve **iOS 15.0+** dağıtım hedefi (iOS derlemesi yalnızca Xcode araç zinciri üzerinden yapılabilir; Linux veya Windows'ta doğrudan derlenemez).

**A) Xcode üzerinden (önerilen):**

1. Xcode'da iOS projenizi açın.
2. **File → Add Package Dependencies…** seçin.
3. Depo URL'sini girin: `https://github.com/aligokdam/c-ids.github.io.git`
4. Sürüm kuralını seçin (örn. "Branch: main" veya belirli bir etiket) ve Swift binding hedefini (target) seçip **Add Package** ile ekleyin.
5. Kütüphaneyi kullanacağınız uygulama hedefinde (App Target) **Frameworks, Libraries, and Embedded Content** altında paketin eklendiğini doğrulayın.

**B) `Package.swift` üzerinden (kendi Swift paketinize bağımlılık olarak):**

```swift
// swift-tools-version:5.9
import PackageDescription

let package = Package(
    name: "SizinUygulamaniz",
    platforms: [.iOS(.v15)],
    dependencies: [
        .package(url: "https://github.com/aligokdam/c-ids.github.io.git", branch: "main")
    ],
    targets: [
        .target(
            name: "SizinHedefiniz",
            dependencies: [
                .product(name: "CIDS", package: "c-ids.github.io") // paket içindeki gerçek ürün adını depodan doğrulayın
            ]
        )
    ]
)
```

**C) Komut satırından derleme/test (simülatör veya cihaz için):**

```bash
git clone https://github.com/aligokdam/c-ids.github.io.git
cd c-ids.github.io/C-IDS
swift build
# veya bir iOS simülatörü hedefleyerek:
xcodebuild -scheme CIDS -destination "generic/platform=iOS Simulator"
```

> `.xcframework` hem cihaz (`arm64`) hem simülatör (`arm64`/`x86_64`) mimarilerini içerdiğinden ek bir "lipo" birleştirme adımına gerek yoktur.

### 6. Kurulumu Doğrulama

Derleme başarılı olduysa, C++ çekirdeğinin ürettiği kütüphaneyi bir örnek uygulamaya bağlayıp temel bir paket/kural eşleştirme çağrısı yaparak (örn. bir örnek trafik günlüğü besleyerek) motorun beklenen `rule=...` çıktısını ürettiğini kontrol edebilirsiniz. Canlı demoda örnek çıktı formatını görebilirsiniz: <https://aligokdam.github.io/c-ids.github.io/>

---

## 📖 Documentation and Live Demo

For technical specifications, architecture notes, the ABI contract, and a live look at the project, please visit the official page: **<https://aligokdam.github.io/c-ids.github.io/>**

Detailed architecture decisions, the ABI contract, and the security policy live in the `docs/` folder of the repository.

## 🤝 Contributing

Contributions are welcome! Please review the repository guidelines for instructions on how to submit improvements.

## ⚖️ License

This project is licensed under the **MIT License**.
