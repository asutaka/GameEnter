#include "../core/emulator.h"
#include "disassembler.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>

using namespace nes;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <rom_file>" << std::endl;
        return 1;
    }

    Emulator emu;
    if (!emu.load_rom(argv[1])) {
        std::cerr << "Failed to load ROM" << std::endl;
        return 1;
    }

    // Set PC to Reset Vector (Standard startup)
    emu.reset();
    // emu.cpu_.PC = 0xC000; // Don't force PC
    
    // Log file
    std::ofstream log("my_nestest_startup.log");
    
    // We need to track total cycles
    uint64_t total_cycles = 7; // Initial cycles
    
    // Run 200000 instructions
    for (int i = 0; i < 200000; i++) {
        uint16_t pc = emu.cpu_.PC;
        
        // Disassemble
        DisassembledInstruction inst = Disassembler::disassemble(pc, &emu.memory_);
        
        // Format: "C000  4C F5 C5  JMP $C5F5                       A:00 X:00 Y:00 P:24 SP:FD PPU:  0, 21 CYC:7"
        
        log << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << pc << "  ";
        
        // Bytes and instruction
        std::string disasm = inst.to_string();
        // disasm is "4C F5 C5 JMP $C5F5" (padded)
        // We need to pad it to 48 chars
        log << std::left << std::setw(48) << std::setfill(' ') << disasm << std::right;
        
        log << "A:" << std::setw(2) << std::setfill('0') << (int)emu.cpu_.A << " ";
        log << "X:" << std::setw(2) << (int)emu.cpu_.X << " ";
        log << "Y:" << std::setw(2) << (int)emu.cpu_.Y << " ";
        log << "P:" << std::setw(2) << (int)emu.cpu_.P << " ";
        log << "SP:" << std::setw(2) << (int)emu.cpu_.SP << " ";
        
        // PPU
        log << "PPU:" << std::setw(3) << std::setfill(' ') << emu.ppu_.get_scanline() << "," 
            << std::setw(3) << emu.ppu_.get_cycle() << " ";
            
        log << "CYC:" << std::dec << total_cycles << std::endl;
        
        // Execute
        int cycles = emu.cpu_.step();
        total_cycles += cycles;
        
        // Step PPU 3x
        for(int j=0; j<cycles*3; j++) emu.ppu_.step();
    }
    
    std::cout << "Generated my_nestest.log with 1000 instructions." << std::endl;
    
    return 0;
}
