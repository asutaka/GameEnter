# 🎮 NES Emulator

Emulator NES được viết bằng C++ với mục tiêu chạy trên Android.

## 🎯 Mục Tiêu

Xây dựng emulator NES thương mại với:
- ✅ Hỗ trợ game Contra (Mapper MMC3)
- ✅ Multiplayer qua Bluetooth/WiFi
- ✅ Monetization (AdMob, Premium version)
- ✅ Cloud save
- ✅ UI/UX hiện đại

## 📂 Cấu Trúc Project

```
NESEmulator/
├── core/              # C++ emulator core
│   ├── cpu/          # CPU 6502
│   ├── ppu/          # Picture Processing Unit
│   ├── apu/          # Audio Processing Unit
│   ├── memory/       # Memory management
│   ├── cartridge/    # ROM loader
│   └── mappers/      # Cartridge mappers
├── desktop/          # Desktop version (SDL2)
├── android/          # Android app (sẽ tạo sau)
├── tests/            # Unit tests
└── docs/             # Tài liệu
```

## 🚀 Bắt Đầu

### Yêu Cầu

- **CMake** 3.15+
- **C++ Compiler** hỗ trợ C++17 (MSVC, GCC, Clang)
- **SDL2** (optional, cho desktop version)
- **Google Test** (optional, cho tests)

### Build Trên Windows

```powershell
# Tạo build directory
mkdir build
cd build

# Configure với CMake
cmake ..

# Build
cmake --build . --config Release

# Chạy tests (nếu có)
ctest -C Release
```

### Build Desktop Version (Cần SDL2)

Cài SDL2:
```powershell
# Dùng vcpkg
vcpkg install sdl2:x64-windows

# Hoặc tải từ https://www.libsdl.org/download-2.0.php
```

Sau đó build lại:
```powershell
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

## 📚 Tài Liệu

- [Kế Hoạch Dự Án](KE_HOACH_DU_AN_NES_EMULATOR.md) - Lộ trình chi tiết
- [NESDev Wiki](https://www.nesdev.org/wiki/) - Tài liệu kỹ thuật NES

## 🎮 Roadmap

### Phase 1: Core Emulator (Hiện tại)
- [x] Project structure
- [x] CPU skeleton
- [x] Memory system
- [ ] CPU opcodes (256 opcodes)
- [ ] PPU implementation
- [ ] APU implementation
- [ ] Mapper 0 (NROM)

### Phase 2: Testing
- [ ] nestest.nes CPU test
- [ ] Donkey Kong playable
- [ ] Super Mario Bros playable
- [ ] Mapper 4 (MMC3)
- [ ] Contra playable

### Phase 3: Android Port
- [ ] Android project setup
- [ ] NDK integration
- [ ] OpenGL ES renderer
- [ ] Touch controls

### Phase 4: Features
- [ ] Multiplayer (Bluetooth)
- [ ] Account system
- [ ] AdMob
- [ ] Cloud save

## 🔧 Trạng Thái Hiện Tại

**Đã hoàn thành:**
- ✅ Cấu trúc project
- ✅ CPU 6502 skeleton với addressing modes
- ✅ Memory system với CPU memory map
- ✅ Stub cho PPU, APU, Cartridge
- ✅ Mapper base class
- ✅ Emulator orchestration

**Đang làm:**
- ⏳ Implement đầy đủ 256 opcodes cho CPU
- ⏳ CPU testing với nestest.nes

**Tiếp theo:**
- 📋 PPU implementation
- 📋 ROM loader (iNES format)

## 📝 License

TBD

## 👥 Contributors

- Bạn - Developer

## 🙏 Credits

- NESDev Community
- FCEUX, Nestopia (reference implementations)
