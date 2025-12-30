# 🔧 Hướng Dẫn Setup C++ Compiler trên Windows

## ⚡ Quick Start (Khuyến Nghị)

### **Option 1: MSYS2 + MinGW (Nhanh nhất)**

1. **Download MSYS2**
   - Link: https://www.msys2.org/
   - Download installer (msys2-x86_64-*.exe)
   - Chạy installer và cài vào `C:\msys64`

2. **Cài MinGW toolchain**
   ```bash
   # Mở MSYS2 terminal và chạy:
   pacman -Syu
   pacman -S --needed base-devel mingw-w64-x86_64-toolchain
   pacman -S mingw-w64-x86_64-cmake
   ```

3. **Thêm vào PATH**
   - Mở **Environment Variables**
   - Thêm vào PATH: `C:\msys64\mingw64\bin`
   - Restart PowerShell

4. **Verify installation**
   ```powershell
   g++ --version
   cmake --version
   ```

---

### **Option 2: Visual Studio Build Tools**

1. **Download Visual Studio Build Tools 2022**
   - Link: https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022

2. **Chọn Workload**
   - ✅ Desktop development with C++
   - ✅ CMake tools for Windows

3. **Build qua Developer Command Prompt**
   - Tìm "Developer Command Prompt for VS 2022" trong Start Menu
   - Navigate đến project folder
   - Chạy build commands

---

### **Option 3: Visual Studio Community (Full IDE)**

1. **Download VS Community**
   - Link: https://visualstudio.microsoft.com/vs/community/

2. **Chọn Workload**
   - ✅ Desktop development with C++
   - ✅ Game development with C++ (optional)

3. **Build qua IDE hoặc command line**

---

## 🚀 Sau Khi Cài Compiler

### **1. Configure CMake**
```powershell
# Xóa build folder cũ (nếu có)
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue

# Configure với MinGW
cmake -B build -S . -G "MinGW Makefiles"

# Hoặc với Visual Studio
cmake -B build -S . -G "Visual Studio 17 2022"
```

### **2. Build Project**
```powershell
# Build Debug
cmake --build build --config Debug

# Build Release (nhanh hơn)
cmake --build build --config Release
```

### **3. Run Tests**
```powershell
# Chạy nestest
.\build\desktop\NESEmulator.exe .\tests\test_roms\nestest.nes

# So sánh kết quả
# File output: cpu_trace.log
# File chuẩn: tests\test_roms\nestest.log
```

---

## 📊 So Sánh Options

| Feature | MSYS2/MinGW | VS Build Tools | VS Community |
|---------|-------------|----------------|--------------|
| **Kích thước** | ~1 GB | ~3 GB | ~6 GB |
| **Tốc độ build** | Nhanh | Trung bình | Trung bình |
| **IDE** | ❌ | ❌ | ✅ |
| **Cross-platform** | ✅ | ❌ | ❌ |
| **Debugging** | GDB | VS Debugger | VS Debugger |
| **Khuyến nghị** | ⭐ Tốt nhất | ⭐ Cho production | ⭐ Nếu cần IDE |

---

## 🐛 Troubleshooting

### Problem: `g++: command not found`
**Solution**: Chưa thêm vào PATH. Restart PowerShell sau khi thêm PATH.

### Problem: `CMake Error: CMAKE_CXX_COMPILER not set`
**Solution**: CMake không tìm thấy compiler. Check PATH và reinstall.

### Problem: Build failed với MinGW
**Solution**: 
```powershell
# Thử clean build
Remove-Item -Recurse -Force build
cmake -B build -S . -G "MinGW Makefiles"
cmake --build build
```

---

## ✅ Verify Setup

Sau khi setup xong, chạy các lệnh này để verify:

```powershell
# Check compiler
g++ --version
# Hoặc (nếu dùng VS)
cl

# Check CMake
cmake --version

# Check make
mingw32-make --version
# Hoặc
nmake /?
```

Nếu tất cả đều pass → ✅ Ready to build!

---

**Cập nhật**: 2025-12-25
**Tác giả**: NES Emulator Project
