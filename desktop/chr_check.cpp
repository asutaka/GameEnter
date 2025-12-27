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

    std::cout << "=== CHR ROM Direct Check ===" << std::endl;
    std::cout << std::endl;

    // Read CHR ROM around 0x200 (Tile $20)
    std::cout << "Reading CHR $0200-$021F (Tile $20 and $21):" << std::endl;
    
    for (int i = 0x200; i < 0x220; i++) {
        uint8_t byte = cart.read(i);
        
        if (i % 16 == 0) {
            std::cout << std::endl << "$" << std::hex << std::setw(4) << std::setfill('0') << i << ": ";
        }
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
    }
    
    std::cout << std::dec << std::endl << std::endl;
    std::cout << "Check complete." << std::endl;

    return 0;
}
