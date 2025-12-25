#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <string>
#include <cstdint>

namespace nes {
    class Memory;

    struct DisassembledInstruction {
        uint8_t opcode;
        int length;           // 1, 2, or 3 bytes
        uint8_t bytes[3];     // Opcode bytes
        std::string mnemonic; // e.g., "LDA"
        std::string operand;  // e.g., "$44", "($80,X)"
        
        std::string to_string() const;
    };

    class Disassembler {
    public:
        static DisassembledInstruction disassemble(uint16_t pc, Memory* memory);
    };
}

#endif // DISASSEMBLER_H
