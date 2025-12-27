#include "../core/cartridge/cartridge.h"
#include <iostream>
#include <iomanip>

using namespace nes;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <rom_file>" << std::endl;
        return 1;
    }

    Cartridge cart;
    if (!cart.load_from_file(argv[1])) {
        std::cerr << "Failed to load ROM" << std::endl;
        return 1;
    }

    std::cout << "=== CHR Tile Check ===" << std::endl;
    
    // Check tile $24 (which Donkey Kong uses)
    uint16_t tile_addr = 0x24 * 16; // Each tile is 16 bytes
    
    std::cout << "\nTile $24 at CHR address $" << std::hex << std::setw(4) << std::setfill('0') 
             << tile_addr << ":" << std::endl;
    
    bool has_data = false;
    
    std::cout << "Pattern low:" << std::endl;
    for (int row = 0; row < 8; row++) {
        uint8_t byte = cart.read(tile_addr + row);
        std::cout << "  $" << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
        std::cout << " = ";
        for (int bit = 7; bit >= 0; bit--) {
            std::cout << ((byte & (1 << bit)) ? "█" : "·");
        }
        std::cout << std::endl;
        if (byte != 0) has_data = true;
    }
    
    std::cout << "Pattern high:" << std::endl;
    for (int row = 0; row < 8; row++) {
        uint8_t byte = cart.read(tile_addr + 8 + row);
        std::cout << "  $" << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
        std::cout << " = ";
        for (int bit = 7; bit >= 0; bit--) {
            std::cout << ((byte & (1 << bit)) ? "█" : "·");
        }
        std::cout << std::endl;
        if (byte != 0) has_data = true;
    }
    
    if (has_data) {
        std::cout << "\n✓ Tile $24 HAS pattern data!" << std::endl;
    } else {
        std::cout << "\n✗ Tile $24 is EMPTY (all zeros)!" << std::endl;
    }
    
    // Also check tile $00
    std::cout << "\n\nTile $00 at CHR address $0000:" << std::endl;
    bool tile00_has_data = false;
    for (int i = 0; i < 16; i++) {
        uint8_t byte = cart.read(i);
        if (byte != 0) tile00_has_data = true;
    }
    std::cout << (tile00_has_data ? "✓ Has data" : "✗ Empty") << std::endl;
    
    return 0;
}
