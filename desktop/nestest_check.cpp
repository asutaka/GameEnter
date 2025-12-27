#include "../core/emulator.h"
#include <iostream>
#include <iomanip>

using namespace nes;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <nestest.nes>" << std::endl;
        return 1;
    }

    Emulator emu;
    if (!emu.load_rom(argv[1])) {
        std::cerr << "Failed to load ROM" << std::endl;
        return 1;
    }

    // Set PC to 0xC000 for automation mode
    emu.reset();
    emu.cpu_.PC = 0xC000;
    emu.cpu_.P = 0x24;
    emu.cpu_.SP = 0xFD;
    emu.cpu_.A = 0;
    emu.cpu_.X = 0;
    emu.cpu_.Y = 0;
    
    std::cout << "Running nestest in automation mode..." << std::endl;
    
    // Run for 5 million cycles (enough for all tests)
    uint64_t total_cycles = 0;
    while (total_cycles < 5000000) {
        total_cycles += emu.cpu_.step();
    }
    
    uint8_t result1 = emu.memory_.read(0x0002);
    uint8_t result2 = emu.memory_.read(0x0003);
    
    std::cout << "---------------------------------------" << std::endl;
    std::cout << "Nestest Result (Memory $02, $03):" << std::endl;
    std::cout << "  $02 = 0x" << std::hex << std::setw(2) << std::setfill('0') << (int)result1 << std::endl;
    std::cout << "  $03 = 0x" << std::hex << std::setw(2) << std::setfill('0') << (int)result2 << std::endl;
    
    if (result1 == 0) {
        std::cout << "SUCCESS: All CPU instructions passed!" << std::endl;
    } else {
        std::cout << "FAILURE: CPU test failed with error code 0x" << std::hex << (int)result1 << std::endl;
        std::cout << "Check nestest documentation for error code meaning." << std::endl;
    }
    
    return 0;
}
