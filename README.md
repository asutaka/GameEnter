# NES Emulator (C++ From Scratch)

A Cycle-Accurate Nintendo Entertainment System (NES) Emulator written in C++17.

![NES Controller](https://upload.wikimedia.org/wikipedia/commons/thumb/8/83/Nes-controller.jpg/320px-Nes-controller.jpg)

## 🌟 Features

*   **CPU**: Ricoh 2A03 (MOS 6502 variant) - **100% Cycle Accurate**.
    *   Full instruction set implementation (including unofficial opcodes).
    *   Cycle-accurate timing and addressing modes.
*   **PPU**: Ricoh 2C02 (Picture Processing Unit) - **Pixel Perfect**.
    *   Background rendering (Nametables, Attribute tables).
    *   Sprite rendering (8x8 and 8x16 mode).
    *   Scrolling (Horizontal, Vertical).
    *   Full palette support.
*   **APU**: Ricoh 2A03 (Audio Processing Unit) - **Full Audio Support**.
    *   Pulse Channels 1 & 2 (Square waves with sweep/envelope).
    *   Triangle Channel (Linear counter).
    *   Noise Channel (LFSR).
    *   DMC (Delta Modulation Channel) for samples.
*   **Mappers**: Support for common cartridges.
    *   Mapper 0 (NROM) - e.g., Super Mario Bros, Donkey Kong.
    *   Mapper 1 (MMC1) - e.g., Metroid, Zelda.
    *   Mapper 4 (MMC3) - e.g., Super Mario Bros 3.
*   **Input**: Keyboard support mapped to NES controller.
*   **Platform**: Cross-platform support via SDL2 (Windows, Linux, macOS).

## 🎮 Controls

| NES Button | Keyboard Key |
| :--- | :--- |
| **D-Pad** | Arrow Keys |
| **A** | `Z` |
| **B** | `X` |
| **Select** | `A` |
| **Start** | `S` |
| **Reset** | `R` |
| **Quit** | `ESC` |

## 🛠️ Build Instructions

### Prerequisites
*   **CMake** (3.15 or higher)
*   **C++ Compiler** (GCC, Clang, or MSVC) supporting C++17.
*   **SDL2** (Automatically downloaded via CMake FetchContent, or install system-wide).

### Windows (PowerShell)
```powershell
# 1. Clone the repository
git clone https://github.com/yourusername/nes-emulator.git
cd nes-emulator

# 2. Build the project
.\build.ps1
# OR manually:
# cmake -S . -B build
# cmake --build build --config Release

# 3. Run the emulator
.\build\Release\nes_app.exe path\to\game.nes
```

### Linux / macOS
```bash
# 1. Clone the repository
git clone https://github.com/yourusername/nes-emulator.git
cd nes-emulator

# 2. Build the project
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# 3. Run the emulator
./nes_app path/to/game.nes
```

## 📂 Project Structure

```
nes-emulator/
├── core/               # Emulator Core Logic
│   ├── cpu/            # CPU Implementation (6502)
│   ├── ppu/            # PPU Implementation (Graphics)
│   ├── apu/            # APU Implementation (Audio)
│   ├── memory/         # Memory Bus & Mapping
│   ├── cartridge/      # Cartridge & Mapper Loading
│   ├── mappers/        # Mapper Implementations (0, 1, 4...)
│   ├── input/          # Controller Input Handling
│   └── emulator.h      # Main Emulator Class
├── desktop/            # Platform Layer (SDL2)
│   ├── main_sdl.cpp    # Main Entry Point & Loop
│   └── ...
├── tests/              # Unit Tests & Test ROMs
├── CMakeLists.txt      # Build Configuration
└── README.md           # This file
```

## 🧩 Compatibility

Tested and working with:
*   *Donkey Kong* (NROM)
*   *Super Mario Bros* (NROM)
*   *Legend of Zelda* (MMC1)
*   *Metroid* (MMC1)
*   *Super Mario Bros 3* (MMC3)
*   *Nestest* (CPU Test Suite)

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## 📜 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---
*Created with ❤️ by Antigravity & User*
