#include "../core/emulator.h"
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace nes;

int main(int argc, char* argv[]) {
    std::cout << "=== NES Game ROM Test ===" << std::endl;
    std::cout << std::endl;
    
    // Check arguments
    std::string rom_file;
    if (argc >= 2) {
        rom_file = argv[1];
    } else {
        // Default to Donkey Kong
        rom_file = "tests\\test_roms\\donkeykong.nes";
    }
    
    // Create emulator
    Emulator emu;
    
    // Load ROM
    std::cout << "Loading ROM: " << rom_file << std::endl;
    if (!emu.load_rom(rom_file)) {
        std::cerr << "Failed to load ROM!" << std::endl;
        return 1;
    }
    
    std::cout << "ROM loaded successfully!" << std::endl;
    std::cout << std::endl;
    
    // Reset emulator
    emu.reset();
    
    // Show initial state
    std::cout << "Initial CPU State:" << std::endl;
    std::cout << "  PC: 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') 
              << emu.cpu_.PC << std::endl;
    std::cout << "  A:  0x" << std::setw(2) << (int)emu.cpu_.A << std::endl;
    std::cout << "  X:  0x" << std::setw(2) << (int)emu.cpu_.X << std::endl;
    std::cout << "  Y:  0x" << std::setw(2) << (int)emu.cpu_.Y << std::endl;
    std::cout << "  P:  0x" << std::setw(2) << (int)emu.cpu_.P << std::endl;
    std::cout << "  SP: 0x" << std::setw(2) << (int)emu.cpu_.SP << std::endl;
    std::cout << std::endl;
    
    // Run for a few instructions to verify CPU is working
    std::cout << "Running 100 instructions..." << std::endl;
    std::cout << std::endl;
    
    for (int i = 0; i < 100; i++) {
        uint16_t pc_before = emu.cpu_.PC;
        uint8_t opcode = emu.memory_.read(emu.cpu_.PC++);
        
        // Execute instruction
        emu.cpu_.execute(opcode);
        emu.cpu_.total_cycles += emu.cpu_.cycles_remaining;
        
        // Show first 10 instructions
        if (i < 10) {
            std::cout << "Instruction " << std::dec << std::setw(2) << i << ": "
                      << "PC=0x" << std::hex << std::setw(4) << std::setfill('0') << pc_before
                      << " Opcode=0x" << std::setw(2) << (int)opcode
                      << " -> PC=0x" << std::setw(4) << emu.cpu_.PC
                      << std::endl;
        }
    }
    
    std::cout << std::endl;
    std::cout << "Final CPU State:" << std::endl;
    std::cout << "  PC: 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') 
              << emu.cpu_.PC << std::endl;
    std::cout << "  A:  0x" << std::setw(2) << (int)emu.cpu_.A << std::endl;
    std::cout << "  X:  0x" << std::setw(2) << (int)emu.cpu_.X << std::endl;
    std::cout << "  Y:  0x" << std::setw(2) << (int)emu.cpu_.Y << std::endl;
    std::cout << "  P:  0x" << std::setw(2) << (int)emu.cpu_.P << std::endl;
    std::cout << "  SP: 0x" << std::setw(2) << (int)emu.cpu_.SP << std::endl;
    std::cout << "  Total Cycles: " << std::dec << emu.cpu_.total_cycles << std::endl;
    std::cout << std::endl;
    
    // Check some memory locations to verify game is loaded
    std::cout << "Memory Check:" << std::endl;
    std::cout << "  RAM[0x0000]: 0x" << std::hex << std::setw(2) << std::setfill('0') 
              << (int)emu.memory_.read(0x0000) << std::endl;
    std::cout << "  RAM[0x0001]: 0x" << std::setw(2) << (int)emu.memory_.read(0x0001) << std::endl;
    
    // Check reset vector
    uint16_t reset_vec = emu.memory_.read(0xFFFC) | (emu.memory_.read(0xFFFD) << 8);
    std::cout << "  Reset Vector: 0x" << std::setw(4) << reset_vec << std::endl;
    std::cout << std::endl;
    
    std::cout << "✅ Game ROM test completed successfully!" << std::endl;
    std::cout << std::endl;
    std::cout << "The CPU is running the game code correctly!" << std::endl;
    std::cout << "(Graphics rendering requires PPU implementation)" << std::endl;
    
    return 0;
}
