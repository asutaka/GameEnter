# 📚 Documentation Index - NES Emulator

## 🚀 Getting Started (BẮT ĐẦU TẠI ĐÂY!)

1. **[QUICKSTART.md](QUICKSTART.md)** ⭐ START HERE
   - Quick setup guide
   - Essential commands
   - 5-minute intro

2. **[SETUP_COMPILER.md](SETUP_COMPILER.md)**
   - Compiler installation guide
   - Multiple options (MinGW, Visual Studio)
   - Troubleshooting

3. **[README.md](README.md)**
   - Project overview
   - Current status
   - Quick reference

---

## 🔧 Build & Development

### Build System
- **`build.ps1`** - Main build script
  ```powershell
  .\build.ps1 [clean|debug|release|test]
  ```

- **`compare_logs.ps1`** - Log comparison tool
  ```powershell
  .\compare_logs.ps1
  ```

- **[BUILD_INSTRUCTIONS.md](BUILD_INSTRUCTIONS.md)**
  - Detailed build instructions
  - Advanced configurations
  - CMake options

---

## 📋 Planning & Progress

### Project Planning
- **[KE_HOACH_DU_AN_NES_EMULATOR.md](KE_HOACH_DU_AN_NES_EMULATOR.md)**
  - Full 5-7 month roadmap
  - Monetization strategy
  - Technical architecture
  - Timeline (Gantt chart style)

- **[NES_EMULATOR_PROJECT_PLAN.md](NES_EMULATOR_PROJECT_PLAN.md)**
  - English version planning doc
  - Detailed milestones

### Progress Tracking
- **[docs/TIEN_DO_TUAN_1.md](docs/TIEN_DO_TUAN_1.md)**
  - Week 1 progress report
  - Component breakdown
  - Statistics
  - Next steps

- **[docs/COMPLETION_SUMMARY.md](docs/COMPLETION_SUMMARY.md)** ⭐ LATEST
  - Current completion status
  - Quick checklist
  - Immediate next steps

---

## 📝 Session Reports

### Latest Session
- **[docs/SESSION_2025_12_25.md](docs/SESSION_2025_12_25.md)** ⭐ LATEST
  - Testing infrastructure setup
  - Files created/modified
  - Key decisions
  - Next milestones

### Historical Sessions
- **[SESSION_SUMMARY.md](SESSION_SUMMARY.md)**
  - Previous session summary
  
- **[FINAL_SESSION_SUMMARY.md](FINAL_SESSION_SUMMARY.md)**
  - Earlier completion summary

- **[TONG_KET_IMPLEMENTATION.md](TONG_KET_IMPLEMENTATION.md)**
  - Implementation summary (Vietnamese)

---

## 🎮 Technical Documentation

### Architecture
- **CPU (6502)**: `core/cpu/`
  - cpu.h - CPU header with all registers
  - cpu.cpp - CPU implementation
  - opcodes.cpp - 256 opcode table

- **Memory**: `core/memory/`
  - Full NES memory map
  - Memory bus implementation

- **Cartridge**: `core/cartridge/`
  - iNES ROM loader
  - Mapper system

- **Mappers**: `core/mappers/`
  - Mapper 0 (NROM) - Complete
  - Mapper 1-4 - Planned

### Testing
- **Test ROMs**: `tests/test_roms/`
  - nestest.nes - CPU validation
  - nestest.log - Expected output

- **Desktop App**: `desktop/main.cpp`
  - Test harness
  - CPU trace logging

---

## 📊 Status Overview

### Week 1 Status (Current)
- ✅ CPU: 95% (256 opcodes)
- ✅ Memory: 100%
- ✅ Cartridge: 90%
- ✅ Mapper 0: 100%
- 🚧 PPU: 5% (stub)
- 🚧 APU: 5% (stub)
- ✅ Build System: 100%
- ✅ Documentation: 100%

**Overall: ~50% complete**

### Next Milestone
🎯 **Pass nestest.nes** = CPU foundation complete

---

## 🔗 External Resources

### NES Development
- [NESDev Wiki](https://www.nesdev.org/wiki/) - THE reference
- [6502 Opcodes](http://www.6502.org/tutorials/6502opcodes.html)
- [nestest ROM](https://github.com/christopherpow/nes-test-roms)

### Reference Emulators
- [FCEUX](https://github.com/TASEmulators/fceux)
- [Nestopia](https://github.com/rdanbrook/nestopia)

---

## 🗂️ File Organization

```
GameEnter/
├── 📖 Documentation (Root)
│   ├── README.md ⭐ Main readme
│   ├── QUICKSTART.md ⭐ Start here
│   ├── SETUP_COMPILER.md
│   ├── BUILD_INSTRUCTIONS.md
│   ├── KE_HOACH_DU_AN_NES_EMULATOR.md
│   └── ...
│
├── 📁 docs/ (Detailed docs)
│   ├── COMPLETION_SUMMARY.md ⭐ Current status
│   ├── SESSION_2025_12_25.md ⭐ Latest session
│   ├── TIEN_DO_TUAN_1.md
│   └── ...
│
├── 🔧 Scripts (Root)
│   ├── build.ps1
│   └── compare_logs.ps1
│
├── 💻 Source Code
│   ├── core/ (C++ emulator)
│   ├── desktop/ (Test app)
│   └── tests/ (Test ROMs)
│
└── 🏗️ Build Output
    └── build/ (Generated)
```

---

## 📝 Quick Commands Reference

### Build
```powershell
.\build.ps1 clean     # Clean build directory
.\build.ps1 debug     # Build debug version
.\build.ps1 release   # Build release version
.\build.ps1 test      # Build + run nestest
```

### Compare
```powershell
.\compare_logs.ps1    # Compare CPU trace with expected
```

### Manual Build
```powershell
cmake -B build -S . -G "MinGW Makefiles"
cmake --build build --config Debug
.\build\desktop\Debug\NESEmulator.exe .\tests\test_roms\nestest.nes
```

---

## ✅ Document Status

| Document | Status | Last Update |
|----------|--------|-------------|
| README.md | ✅ Current | 2025-12-25 |
| QUICKSTART.md | ✅ Current | 2025-12-25 |
| SETUP_COMPILER.md | ✅ Current | 2025-12-25 |
| docs/COMPLETION_SUMMARY.md | ✅ Current | 2025-12-25 |
| docs/SESSION_2025_12_25.md | ✅ Current | 2025-12-25 |
| docs/TIEN_DO_TUAN_1.md | ✅ Current | 2025-12-25 |
| KE_HOACH_DU_AN_NES_EMULATOR.md | ✅ Valid | 2025-12-25 |

---

## 🎯 Recommended Reading Order

### For New Users
1. **QUICKSTART.md** - Understand project quickly
2. **SETUP_COMPILER.md** - Install compiler
3. **docs/COMPLETION_SUMMARY.md** - Current status
4. Run: `.\build.ps1 test`

### For Development
1. **KE_HOACH_DU_AN_NES_EMULATOR.md** - Full roadmap
2. **docs/TIEN_DO_TUAN_1.md** - Week 1 details
3. **docs/SESSION_2025_12_25.md** - Latest changes
4. Check: `core/cpu/opcodes.cpp` - Implementation

### For Testing
1. **BUILD_INSTRUCTIONS.md** - Build details
2. Run: `.\build.ps1 test`
3. Run: `.\compare_logs.ps1`
4. Fix bugs, rebuild, repeat

---

**Last Updated:** 2025-12-25  
**Maintainer:** NES Emulator Project  
**Status:** ✅ Up to date
