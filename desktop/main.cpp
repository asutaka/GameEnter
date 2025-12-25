#include "../core/emulator.h"
#include "disassembler.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace nes;

void log_cpu_state(Emulator& emu, std::ostream& out) {
    const CPU& cpu = emu.cpu_;
    Memory* mem = &emu.memory_;  // Get pointer to memory object
    
    // Disassemble current instruction
    DisassembledInstruction inst = Disassembler::disassemble(cpu.PC, mem);
    
    // Format: PC  BYTES  INSTRUCTION                       A:XX X:XX Y:XX P:XX SP:XX PPU:XXX,XXX CYC:X
    out << std::hex << std::uppercase << std::setfill('0');
    
    // PC (4 hex digits)
    out << std::setw(4) << cpu.PC << "  ";
    
    // Instruction bytes and disassembly (padded to 47 chars with spaces)
    std::string instr_str = inst.to_string();
    out << instr_str;
    
    // Pad to ensure alignment (nestest uses 48 chars total for PC + instruction)
    int padding = 48 - 6 - instr_str.length(); // 48 total - 6 for "XXXX  "
    for (int i = 0; i < padding; i++) {
        out << ' ';
    }
    
    // Registers (with zero-fill for hex values)
    out << "A:" << std::setw(2) << (int)cpu.A
        << " X:" << std::setw(2) << (int)cpu.X
        << " Y:" << std::setw(2) << (int)cpu.Y
        << " P:" << std::setw(2) << (int)cpu.P
        << " SP:" << std::setw(2) << (int)cpu.SP;
    
    // PPU state (with space-fill for decimal values)
    int ppu_scanline = 0;
    int ppu_cycle = (cpu.total_cycles * 3) % 341; // PPU runs at 3x CPU speed
    out << " PPU:" << std::setw(3) << std::setfill(' ') << ppu_scanline 
        << "," << std::setw(3) << ppu_cycle;
    
    // CPU cycles (decimal)
    out << " CYC:" << std::dec << cpu.total_cycles;
    
    out << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "=== NES Emulator - CPU Test ===" << std::endl;
    
    // Check arguments
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <rom_file.nes>" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Example:" << std::endl;
        std::cerr << "  " << argv[0] << " nestest.nes" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Download nestest.nes from:" << std::endl;
        std::cerr << "  https://github.com/christopherpow/nes-test-roms" << std::endl;
        return 1;
    }
    
    std::string rom_file = argv[1];
    
    // Tạo emulator
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
    
    // Nestest.nes specific: Start at $C000 for automated test
    // Uncomment nếu run nestest.nes:
    emu.cpu_.PC = 0xC000;
    
    std::cout << "Starting CPU test..." << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << std::endl;
    
    // Open log file
    std::ofstream log_file("cpu_trace.log");
    if (!log_file) {
        std::cerr << "Warning: Cannot create log file" << std::endl;
    }
    
    // Run emulator và log CPU state
    int instruction_count = 0;
    const int MAX_INSTRUCTIONS = 10000;  // Giới hạn để không chạy vô tận
    
    while (instruction_count < MAX_INSTRUCTIONS) {
        // Log state trước khi execute
        log_cpu_state(emu, std::cout);
        if (log_file) {
            log_cpu_state(emu, log_file);
        }
        
        // Execute 1 instruction
        emu.cpu_.step();
        
        // Wait for instruction to complete
        while (emu.cpu_.cycles_remaining > 0) {
            emu.cpu_.step();
        }
        
        instruction_count++;
        
        // Nestest.nes kết thúc khi PC = $C66E với test status tại $0002-$0003
        // Kiểm tra nếu đang chạy nestest
        if (emu.cpu_.PC == 0xC66E) {
            std::cout << std::endl;
            std::cout << "Nestest completed!" << std::endl;
            
            // TODO: Read test status từ $0002-$0003
            // uint8_t status_lo = emu.memory_.read(0x0002);
            // uint8_t status_hi = emu.memory_.read(0x0003);
            
            std::cout << "Check cpu_trace.log and compare with nestest.log" << std::endl;
            break;
        }
        
        // Stop nếu infinite loop detected
        if (emu.cpu_.PC == 0x0000 || emu.cpu_.PC == 0xFFFF) {
            std::cout << std::endl;
            std::cout << "Possible infinite loop detected at PC=" 
                      << std::hex << emu.cpu_.PC << std::endl;
            break;
        }
    }
    
    std::cout << std::endl;
    std::cout << "Test completed!" << std::endl;
    std::cout << "Instructions executed: " << instruction_count << std::endl;
    std::cout << "Total cycles: " << emu.cpu_.total_cycles << std::endl;
    
    if (log_file) {
        log_file.close();
        std::cout << "Log saved to: cpu_trace.log" << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "Compare output with nestest.log:" << std::endl;
    std::cout << "  Download from: https://www.qmtpro.com/~nes/misc/nestest.log" << std::endl;
    
    return 0;
}
