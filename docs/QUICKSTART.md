# 🎮 NES Emulator - Quick Start

## 🚀 Quick Setup

### 1. Cài Đặt Compiler (Chỉ lần đầu)

**Khuyến nghị: MSYS2 + MinGW**
```powershell
# Download: https://www.msys2.org/
# Sau khi cài, mở MSYS2 terminal:
pacman -Syu
pacman -S --needed base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake

# Thêm vào PATH: C:\msys64\mingw64\bin
```

Chi tiết xem: **[SETUP_COMPILER.md](SETUP_COMPILER.md)**

### 2. Build & Test

```powershell
# Build debug version
.\build.ps1 debug

# Hoặc build và chạy test luôn
.\build.ps1 test
```

**Output:**
- Executable: `build/desktop/Debug/NESEmulator.exe`
- CPU trace log: `cpu_trace.log`

### 3. So Sánh Kết Quả

```powershell
# So sánh output vs expected
fc cpu_trace.log tests\test_roms\nestest.log
```

---

## 📁 Cấu Trúc Project

```
GameEnter/
├── core/              # C++ emulator core
│   ├── cpu/          # CPU 6502 (✅ 95% complete)
│   ├── ppu/          # Graphics (🚧 stub)
│   ├── apu/          # Audio (🚧 stub)
│   ├── memory/       # Memory bus (✅ complete)
│   ├── cartridge/    # ROM loader (✅ complete)
│   └── mappers/      # Mapper 0 (✅ complete)
├── desktop/          # PC test application
├── tests/            # Test ROMs
│   └── test_roms/    # nestest.nes, nestest.log
└── docs/             # Documentation
```

---

## 🎯 Status

**Tuần 1: ✅ 95% Complete**
- ✅ CPU: 256 opcodes implemented
- ✅ Memory system
- ✅ ROM loader (iNES format)
- ✅ Mapper 0 (NROM)
- 🎯 Next: CPU testing với nestest.nes

Xem chi tiết: **[docs/TIEN_DO_TUAN_1.md](docs/TIEN_DO_TUAN_1.md)**

---

## 📚 Tài Liệu

- [Kế Hoạch Dự Án](KE_HOACH_DU_AN_NES_EMULATOR.md) - Roadmap đầy đủ
- [Tiến Độ Tuần 1](docs/TIEN_DO_TUAN_1.md) - Chi tiết implementation
- [Setup Compiler](SETUP_COMPILER.md) - Hướng dẫn cài đặt
- [Build Instructions](BUILD_INSTRUCTIONS.md) - Chi tiết build

---

## 🔧 Build Commands

```powershell
.\build.ps1 clean    # Xóa build directory
.\build.ps1 debug    # Build debug (với symbols)
.\build.ps1 release  # Build release (optimized)
.\build.ps1 test     # Build + run nestest
```

---

## 🐛 Troubleshooting

**❌ Compiler not found**
→ Xem [SETUP_COMPILER.md](SETUP_COMPILER.md)

**❌ Build failed**
```powershell
.\build.ps1 clean
.\build.ps1 debug
```

**❌ nestest.nes not found**
```powershell
# Tự động download khi chạy:
.\build.ps1 test
```

---

**Project:** NES Emulator  
**Language:** C++ (core), PowerShell (scripts)  
**Platform:** Windows (hiện tại), Android (tương lai)  
**License:** MIT
