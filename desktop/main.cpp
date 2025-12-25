#include "../core/emulator.h"
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace nes;

void log_cpu_state(const CPU& cpu, std::ostream& out) {
    // Format giống nestest.log:
    // PC  A  X  Y  P  SP  CYC
    out << std::hex << std::uppercase << std::setfill('0')
        << std::setw(4) << cpu.PC << "  "
        << "A:" << std::setw(2) << (int)cpu.A << " "
        << "X:" << std::setw(2) << (int)cpu.X << " "
        << "Y:" << std::setw(2) << (int)cpu.Y << " "
        << "P:" << std::setw(2) << (int)cpu.P << " "
        << "SP:" << std::setw(2) << (int)cpu.SP << " "
        << "CYC:" << std::dec << cpu.total_cycles
        << std::endl;
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
    // emu.cpu_.PC = 0xC000;
    
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
        log_cpu_state(emu.cpu_, std::cout);
        if (log_file) {
            log_cpu_state(emu.cpu_, log_file);
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
