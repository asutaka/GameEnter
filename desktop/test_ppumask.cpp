#include <iostream>
#include <cstdint>
#include <iomanip>

// Replicate PPU PPUMASK structure
struct PPUMASK {
    uint8_t grayscale : 1;           // Bit 0
    uint8_t show_bg_left : 1;        // Bit 1
    uint8_t show_sprites_left : 1;   // Bit 2
    uint8_t show_bg : 1;             // Bit 3
    uint8_t show_sprites : 1;        // Bit 4
    uint8_t emphasize_red : 1;       // Bit 5
    uint8_t emphasize_green : 1;     // Bit 6
    uint8_t emphasize_blue : 1;      // Bit 7
};

int main() {
    std::cout << "=== PPUMASK Bitfield Test ===" << std::endl;
    std::cout << "Size of PPUMASK struct: " << sizeof(PPUMASK) << " bytes" << std::endl;
    std::cout << std::endl;
    
    PPUMASK mask;
    
    // Test 1: Write $0E (nestest value)
    std::cout << "Test 1: Writing $0E (0b00001110)" << std::endl;
    *reinterpret_cast<uint8_t*>(&mask) = 0x0E;
    
    std::cout << "  grayscale: " << (int)mask.grayscale << std::endl;
    std::cout << "  show_bg_left: " << (int)mask.show_bg_left << std::endl;
    std::cout << "  show_sprites_left: " << (int)mask.show_sprites_left << std::endl;
    std::cout << "  show_bg: " << (int)mask.show_bg << " (should be 1) <<<" << std::endl;
    std::cout << "  show_sprites: " << (int)mask.show_sprites << std::endl;
    std::cout << std::endl;
    
    // Test 2: Write $1E (all rendering on)
    std::cout << "Test 2: Writing $1E (0b00011110)" << std::endl;
    *reinterpret_cast<uint8_t*>(&mask) = 0x1E;
    
    std::cout << "  grayscale: " << (int)mask.grayscale << std::endl;
    std::cout << "  show_bg_left: " << (int)mask.show_bg_left << std::endl;
    std::cout << "  show_sprites_left: " << (int)mask.show_sprites_left << std::endl;
    std::cout << "  show_bg: " << (int)mask.show_bg << " (should be 1) <<<" << std::endl;
    std::cout << "  show_sprites: " << (int)mask.show_sprites << " (should be 1) <<<" << std::endl;
    std::cout << std::endl;
    
    // Test 3: Read back
    std::cout << "Test 3: Reading back value" << std::endl;
    uint8_t readback = *reinterpret_cast<uint8_t*>(&mask);
    std::cout << "  Read value: $" << std::hex << std::setw(2) << std::setfill('0') << (int)readback << std::dec;
    std::cout << " (should be $1E)" << std::endl;
    std::cout << std::endl;
    
    // Verdict
    if (mask.show_bg == 1 && mask.show_sprites == 1 && readback == 0x1E) {
        std::cout << "✅ SUCCESS: Bitfield layout is correct!" << std::endl;
    } else {
        std::cout << "❌ FAIL: Bitfield layout is WRONG!" << std::endl;
        std::cout << "   This means reinterpret_cast is not safe." << std::endl;
        std::cout << "   Need to parse bits manually instead." << std::endl;
    }
    
    return 0;
}
