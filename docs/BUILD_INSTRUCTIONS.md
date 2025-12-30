# 🔨 Hướng Dẫn Build NES Emulator

## ⚠️ CMake Không Cài

CMake chưa được cài trên hệ thống. Có 2 options:

---

## 🎯 OPTION 1: Cài CMake (Khuyến nghị)

### Download:
https://cmake.org/download/

### Hoặc dùng Chocolatey:
```powershell
choco install cmake
```

### Sau đó build:
```powershell
cd e:\Data\GameEnter
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### Chạy test:
```powershell
.\Release\nes_test.exe nestest.nes
```

---

## 🎯 OPTION 2: Build Thủ Công với MSVC (Không cần CMake)

### Bước 1: Mở Developer Command Prompt for VS

Tìm "Developer Command Prompt" trong Start Menu

### Bước 2: Compile tất cả files

```batch
cd e:\Data\GameEnter

REM Tạo build directory
mkdir build_manual
cd build_manual

REM Compile core files
cl /c /EHsc /std:c++17 /O2 /I.. ^
    ..\core\cpu\cpu.cpp ^
    ..\core\cpu\opcodes.cpp ^
    ..\core\ppu\ppu.cpp ^
    ..\core\apu\apu.cpp ^
    ..\core\memory\memory.cpp ^
    ..\core\cartridge\cartridge.cpp ^
    ..\core\mappers\mapper0.cpp ^
    ..\core\mappers\mapper1.cpp ^
    ..\core\mappers\mapper4.cpp ^
    ..\core\emulator.cpp

REM Tạo static library
lib /OUT:nes_core.lib ^
    cpu.obj opcodes.obj ppu.obj apu.obj memory.obj ^
    cartridge.obj mapper0.obj mapper1.obj mapper4.obj emulator.obj

REM Compile main.cpp
cl /c /EHsc /std:c++17 /O2 /I.. ..\desktop\main.cpp

REM Link executable
link /OUT:nes_test.exe main.obj nes_core.lib

echo.
echo Build complete!
echo Run: nes_test.exe nestest.nes
```

---

## 🎯 OPTION 3: Visual Studio Project (Dễ nhất)

### Bước 1: Tạo Visual Studio Solution

1. Mở Visual Studio
2. File → New → Project
3. Chọn "Empty Project" (C++)
4. Tên: NESEmulator
5. Location: `e:\Data\GameEnter`

### Bước 2: Add Files

**Add Core Files:**
- Right click Solution → Add → Existing Items
- Chọn tất cả `.cpp` và `.h` trong `core/`

**Add Desktop Main:**
- Add `desktop/main.cpp`

### Bước 3: Configure Project

**Project Properties:**
- Configuration: Release
- C++ Language Standard: C++17
- Additional Include Directories: `$(ProjectDir)`

### Bước 4: Build

- Build → Build Solution (Ctrl+Shift+B)
- Output: `x64\Release\NESEmulator.exe`

### Bước 5: Run

```batch
cd e:\Data\GameEnter\x64\Release
NESEmulator.exe nestest.nes
```

---

## 📥 Download nestest.nes

### Link:
https://github.com/christopherpow/nes-test-roms/raw/master/other/nestest.nes

### Download cả expected log:
https://www.qmtpro.com/~nes/misc/nestest.log

### Đặt vào:
```
e:\Data\GameEnter\tests\test_roms\nestest.nes
e:\Data\GameEnter\tests\test_roms\nestest.log
```

---

## 🧪 Chạy Test

### Sau khi build:

```powershell
# Run with nestest.nes
.\nes_test.exe tests\test_roms\nestest.nes

# Compare output
fc cpu_trace.log tests\test_roms\nestest.log
```

### Nếu khớp 100%:
✅ **CPU HOÀN HẢO!**

### Nếu có sai:
- Kiểm tra dòng đầu tiên khác nhau
- Debug opcode đó
- Fix và build lại

---

## ⚡ Quick Start (Nếu đã có MSVC)

### Build một lệnh:

```batch
cd e:\Data\GameEnter
cl /EHsc /std:c++17 /O2 /Fe:nes_test.exe ^
    /I. ^
    desktop\main.cpp ^
    core\cpu\*.cpp ^
    core\ppu\*.cpp ^
    core\apu\*.cpp ^
    core\memory\*.cpp ^
    core\cartridge\*.cpp ^
    core\mappers\*.cpp ^
    core\emulator.cpp

nes_test.exe nestest.nes
```

**Note**: Có thể có warnings, ignore them nếu build thành công

---

## 🐛 Troubleshooting

### Lỗi: "cannot open file 'nes_core.lib'"
→ Build lại core library

### Lỗi: "unresolved external symbol"
→ Thiếu .cpp file, check lại danh sách

### Lỗi: "C++ standard not supported"
→ Dùng Visual Studio 2017 trở lên

### Lỗi compile trong opcodes.cpp
→ Có thể do syntax, check line number

---

## 📝 Expected Output

### Khi chạy thành công:

```
=== NES Emulator - CPU Test ===
Loading ROM: nestest.nes
=== iNES ROM Info ===
PRG ROM: 1 x 16KB
CHR ROM: 1 x 8KB
Mapper: 0
ROM loaded successfully!

Starting CPU test...
C000  A:00 X:00 Y:00 P:24 SP:FD CYC:7
C003  A:00 X:00 Y:00 P:24 SP:FD CYC:13
...
```

### Output sẽ được save vào:
`cpu_trace.log`

---

## ✅ Success Criteria

### CPU Test Pass khi:
1. ✅ ROM load thành công
2. ✅ CPU execute instructions
3. ✅ Log file created
4. ✅ Log khớp với nestest.log
5. ✅ Kết thúc tại PC=$C66E

---

## 🚀 Sau Khi Pass

### Tiếp theo:
1. ✅ CPU validated & production-ready
2. 🎨 Implement PPU rendering
3. 🖼️ Show first pixels
4. 🎮 Run Donkey Kong
5. 🔊 Add APU (audio)
6. 📱 Port to Android

---

**Good luck with building!** 🔨

**Nếu gặp issues, share error message để tôi giúp!**
