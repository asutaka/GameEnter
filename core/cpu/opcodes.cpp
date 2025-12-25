#include "cpu/cpu.h"
#include <array>

namespace nes {

// Opcode information structure
struct OpcodeInfo {
    const char* name;          // Tên instruction
    void (CPU::*execute)(uint16_t);  // Function pointer
    uint16_t (CPU::*addr_mode)();    // Addressing mode
    int cycles;                // Base cycles
    bool page_cross_penalty;   // +1 cycle nếu cross page boundary
};

// Instruction execution wrappers
void CPU::exec_LDA(uint16_t addr) { LDA(addr); }
void CPU::exec_LDX(uint16_t addr) { LDX(addr); }
void CPU::exec_LDY(uint16_t addr) { LDY(addr); }
void CPU::exec_STA(uint16_t addr) { STA(addr); }
void CPU::exec_STX(uint16_t addr) { STX(addr); }
void CPU::exec_STY(uint16_t addr) { STY(addr); }
void CPU::exec_AND(uint16_t addr) { AND(addr); }
void CPU::exec_EOR(uint16_t addr) { EOR(addr); }
void CPU::exec_ORA(uint16_t addr) { ORA(addr); }
void CPU::exec_BIT(uint16_t addr) { BIT(addr); }
void CPU::exec_ADC(uint16_t addr) { ADC(addr); }
void CPU::exec_SBC(uint16_t addr) { SBC(addr); }
void CPU::exec_CMP(uint16_t addr) { CMP(addr); }
void CPU::exec_CPX(uint16_t addr) { CPX(addr); }
void CPU::exec_CPY(uint16_t addr) { CPY(addr); }
void CPU::exec_INC(uint16_t addr) { INC(addr); }
void CPU::exec_DEC(uint16_t addr) { DEC(addr); }
void CPU::exec_ASL(uint16_t addr) { ASL(addr); }
void CPU::exec_LSR(uint16_t addr) { LSR(addr); }
void CPU::exec_ROL(uint16_t addr) { ROL(addr); }
void CPU::exec_ROR(uint16_t addr) { ROR(addr); }
void CPU::exec_JMP(uint16_t addr) { JMP(addr); }
void CPU::exec_JSR(uint16_t addr) { JSR(addr); }
void CPU::exec_BCC(uint16_t addr) { BCC(addr); }
void CPU::exec_BCS(uint16_t addr) { BCS(addr); }
void CPU::exec_BEQ(uint16_t addr) { BEQ(addr); }
void CPU::exec_BMI(uint16_t addr) { BMI(addr); }
void CPU::exec_BNE(uint16_t addr) { BNE(addr); }
void CPU::exec_BPL(uint16_t addr) { BPL(addr); }
void CPU::exec_BVC(uint16_t addr) { BVC(addr); }
void CPU::exec_BVS(uint16_t addr) { BVS(addr); }

// Implied mode instructions
void CPU::exec_TAX(uint16_t) { TAX(); }
void CPU::exec_TAY(uint16_t) { TAY(); }
void CPU::exec_TXA(uint16_t) { TXA(); }
void CPU::exec_TYA(uint16_t) { TYA(); }
void CPU::exec_TSX(uint16_t) { TSX(); }
void CPU::exec_TXS(uint16_t) { TXS(); }
void CPU::exec_PHA(uint16_t) { PHA(); }
void CPU::exec_PHP(uint16_t) { PHP(); }
void CPU::exec_PLA(uint16_t) { PLA(); }
void CPU::exec_PLP(uint16_t) { PLP(); }
void CPU::exec_INX(uint16_t) { INX(); }
void CPU::exec_INY(uint16_t) { INY(); }
void CPU::exec_DEX(uint16_t) { DEX(); }
void CPU::exec_DEY(uint16_t) { DEY(); }
void CPU::exec_CLC(uint16_t) { CLC(); }
void CPU::exec_CLD(uint16_t) { CLD(); }
void CPU::exec_CLI(uint16_t) { CLI(); }
void CPU::exec_CLV(uint16_t) { CLV(); }
void CPU::exec_SEC(uint16_t) { SEC(); }
void CPU::exec_SED(uint16_t) { SED(); }
void CPU::exec_SEI(uint16_t) { SEI(); }
void CPU::exec_BRK(uint16_t) { BRK(); }
void CPU::exec_NOP(uint16_t) { NOP(); }
void CPU::exec_RTI(uint16_t) { RTI(); }
void CPU::exec_RTS(uint16_t) { RTS(); }

// ASL/LSR/ROL/ROR cho Accumulator mode
void CPU::exec_ASL_A(uint16_t) {
    set_flag(StatusFlag::FLAG_CARRY, (A & 0x80) != 0);
    A <<= 1;
    update_zero_negative(A);
}
void CPU::exec_LSR_A(uint16_t) {
    set_flag(StatusFlag::FLAG_CARRY, (A & 0x01) != 0);
    A >>= 1;
    update_zero_negative(A);
}
void CPU::exec_ROL_A(uint16_t) {
    bool old_carry = get_flag(StatusFlag::FLAG_CARRY);
    set_flag(StatusFlag::FLAG_CARRY, (A & 0x80) != 0);
    A = (A << 1) | (old_carry ? 1 : 0);
    update_zero_negative(A);
}
void CPU::exec_ROR_A(uint16_t) {
    bool old_carry = get_flag(StatusFlag::FLAG_CARRY);
    set_flag(StatusFlag::FLAG_CARRY, (A & 0x01) != 0);
    A = (A >> 1) | (old_carry ? 0x80 : 0);
    update_zero_negative(A);
}

// Illegal opcode wrappers
void CPU::exec_LAX(uint16_t addr) { LAX(addr); }
void CPU::exec_SAX(uint16_t addr) { SAX(addr); }
void CPU::exec_DCP(uint16_t addr) { DCP(addr); }
void CPU::exec_ISC(uint16_t addr) { ISC(addr); }
void CPU::exec_SLO(uint16_t addr) { SLO(addr); }
void CPU::exec_RLA(uint16_t addr) { RLA(addr); }
void CPU::exec_SRE(uint16_t addr) { SRE(addr); }
void CPU::exec_RRA(uint16_t addr) { RRA(addr); }


// Bảng 256 opcodes
// Format: { Tên, Execute, AddrMode, Cycles, PageCrossPenalty }
static const std::array<OpcodeInfo, 256> OPCODE_TABLE = {{
    // 0x00-0x0F
    {"BRK", &CPU::exec_BRK, &CPU::addr_implied, 7, false},      // 0x00
    {"ORA", &CPU::exec_ORA, &CPU::addr_indirect_x, 6, false},   // 0x01
    {"*KIL", &CPU::exec_NOP, &CPU::addr_implied, 2, false},     // 0x02 - Illegal
    {"*SLO", &CPU::exec_NOP, &CPU::addr_indirect_x, 8, false},  // 0x03 - Illegal
    {"*NOP", &CPU::exec_NOP, &CPU::addr_zero_page, 3, false},   // 0x04 - Illegal
    {"ORA", &CPU::exec_ORA, &CPU::addr_zero_page, 3, false},    // 0x05
    {"ASL", &CPU::exec_ASL, &CPU::addr_zero_page, 5, false},    // 0x06
    {"*SLO", &CPU::exec_NOP, &CPU::addr_zero_page, 5, false},   // 0x07 - Illegal
    {"PHP", &CPU::exec_PHP, &CPU::addr_implied, 3, false},      // 0x08
    {"ORA", &CPU::exec_ORA, &CPU::addr_immediate, 2, false},    // 0x09
    {"ASL", &CPU::exec_ASL_A, &CPU::addr_accumulator, 2, false},// 0x0A
    {"*ANC", &CPU::exec_NOP, &CPU::addr_immediate, 2, false},   // 0x0B - Illegal
    {"*NOP", &CPU::exec_NOP, &CPU::addr_absolute, 4, false},    // 0x0C - Illegal
    {"ORA", &CPU::exec_ORA, &CPU::addr_absolute, 4, false},     // 0x0D
    {"ASL", &CPU::exec_ASL, &CPU::addr_absolute, 6, false},     // 0x0E
    {"*SLO", &CPU::exec_NOP, &CPU::addr_absolute, 6, false},    // 0x0F - Illegal
    
    // 0x10-0x1F
    {"BPL", &CPU::exec_BPL, &CPU::addr_relative, 2, true},      // 0x10
    {"ORA", &CPU::exec_ORA, &CPU::addr_indirect_y, 5, true},    // 0x11
    {"*KIL", &CPU::exec_NOP, &CPU::addr_implied, 2, false},     // 0x12 - Illegal
    {"*SLO", &CPU::exec_NOP, &CPU::addr_indirect_y, 8, false},  // 0x13 - Illegal
    {"*NOP", &CPU::exec_NOP, &CPU::addr_zero_page_x, 4, false}, // 0x14 - Illegal
    {"ORA", &CPU::exec_ORA, &CPU::addr_zero_page_x, 4, false},  // 0x15
    {"ASL", &CPU::exec_ASL, &CPU::addr_zero_page_x, 6, false},  // 0x16
    {"*SLO", &CPU::exec_NOP, &CPU::addr_zero_page_x, 6, false}, // 0x17 - Illegal
    {"CLC", &CPU::exec_CLC, &CPU::addr_implied, 2, false},      // 0x18
    {"ORA", &CPU::exec_ORA, &CPU::addr_absolute_y, 4, true},    // 0x19
    {"*NOP", &CPU::exec_NOP, &CPU::addr_implied, 2, false},     // 0x1A - Illegal
    {"*SLO", &CPU::exec_NOP, &CPU::addr_absolute_y, 7, false},  // 0x1B - Illegal
    {"*NOP", &CPU::exec_NOP, &CPU::addr_absolute_x, 4, true},   // 0x1C - Illegal
    {"ORA", &CPU::exec_ORA, &CPU::addr_absolute_x, 4, true},    // 0x1D
    {"ASL", &CPU::exec_ASL, &CPU::addr_absolute_x, 7, false},   // 0x1E
    {"*SLO", &CPU::exec_NOP, &CPU::addr_absolute_x, 7, false},  // 0x1F - Illegal
    
    // 0x20-0x2F
    {"JSR", &CPU::exec_JSR, &CPU::addr_absolute, 6, false},     // 0x20
    {"AND", &CPU::exec_AND, &CPU::addr_indirect_x, 6, false},   // 0x21
    {"*KIL", &CPU::exec_NOP, &CPU::addr_implied, 2, false},     // 0x22 - Illegal
    {"*RLA", &CPU::exec_NOP, &CPU::addr_indirect_x, 8, false},  // 0x23 - Illegal
    {"BIT", &CPU::exec_BIT, &CPU::addr_zero_page, 3, false},    // 0x24
    {"AND", &CPU::exec_AND, &CPU::addr_zero_page, 3, false},    // 0x25
    {"ROL", &CPU::exec_ROL, &CPU::addr_zero_page, 5, false},    // 0x26
    {"*RLA", &CPU::exec_NOP, &CPU::addr_zero_page, 5, false},   // 0x27 - Illegal
    {"PLP", &CPU::exec_PLP, &CPU::addr_implied, 4, false},      // 0x28
    {"AND", &CPU::exec_AND, &CPU::addr_immediate, 2, false},    // 0x29
    {"ROL", &CPU::exec_ROL_A, &CPU::addr_accumulator, 2, false},// 0x2A
    {"*ANC", &CPU::exec_NOP, &CPU::addr_immediate, 2, false},   // 0x2B - Illegal
    {"BIT", &CPU::exec_BIT, &CPU::addr_absolute, 4, false},     // 0x2C
    {"AND", &CPU::exec_AND, &CPU::addr_absolute, 4, false},     // 0x2D
    {"ROL", &CPU::exec_ROL, &CPU::addr_absolute, 6, false},     // 0x2E
    {"*RLA", &CPU::exec_NOP, &CPU::addr_absolute, 6, false},    // 0x2F - Illegal
    
    // 0x30-0x3F
    {"BMI", &CPU::exec_BMI, &CPU::addr_relative, 2, true},      // 0x30
    {"AND", &CPU::exec_AND, &CPU::addr_indirect_y, 5, true},    // 0x31
    {"*KIL", &CPU::exec_NOP, &CPU::addr_implied, 2, false},     // 0x32 - Illegal
    {"*RLA", &CPU::exec_NOP, &CPU::addr_indirect_y, 8, false},  // 0x33 - Illegal
    {"*NOP", &CPU::exec_NOP, &CPU::addr_zero_page_x, 4, false}, // 0x34 - Illegal
    {"AND", &CPU::exec_AND, &CPU::addr_zero_page_x, 4, false},  // 0x35
    {"ROL", &CPU::exec_ROL, &CPU::addr_zero_page_x, 6, false},  // 0x36
    {"*RLA", &CPU::exec_NOP, &CPU::addr_zero_page_x, 6, false}, // 0x37 - Illegal
    {"SEC", &CPU::exec_SEC, &CPU::addr_implied, 2, false},      // 0x38
    {"AND", &CPU::exec_AND, &CPU::addr_absolute_y, 4, true},    // 0x39
    {"*NOP", &CPU::exec_NOP, &CPU::addr_implied, 2, false},     // 0x3A - Illegal
    {"*RLA", &CPU::exec_NOP, &CPU::addr_absolute_y, 7, false},  // 0x3B - Illegal
    {"*NOP", &CPU::exec_NOP, &CPU::addr_absolute_x, 4, true},   // 0x3C - Illegal
    {"AND", &CPU::exec_AND, &CPU::addr_absolute_x, 4, true},    // 0x3D
    {"ROL", &CPU::exec_ROL, &CPU::addr_absolute_x, 7, false},   // 0x3E
    {"*RLA", &CPU::exec_NOP, &CPU::addr_absolute_x, 7, false},  // 0x3F - Illegal
    
    // 0x40-0x4F
    {"RTI", &CPU::exec_RTI, &CPU::addr_implied, 6, false},      // 0x40
    {"EOR", &CPU::exec_EOR, &CPU::addr_indirect_x, 6, false},   // 0x41
    {"*KIL", &CPU::exec_NOP, &CPU::addr_implied, 2, false},     // 0x42 - Illegal
    {"*SRE", &CPU::exec_NOP, &CPU::addr_indirect_x, 8, false},  // 0x43 - Illegal
    {"*NOP", &CPU::exec_NOP, &CPU::addr_zero_page, 3, false},   // 0x44 - Illegal
    {"EOR", &CPU::exec_EOR, &CPU::addr_zero_page, 3, false},    // 0x45
    {"LSR", &CPU::exec_LSR, &CPU::addr_zero_page, 5, false},    // 0x46
    {"*SRE", &CPU::exec_NOP, &CPU::addr_zero_page, 5, false},   // 0x47 - Illegal
    {"PHA", &CPU::exec_PHA, &CPU::addr_implied, 3, false},      // 0x48
    {"EOR", &CPU::exec_EOR, &CPU::addr_immediate, 2, false},    // 0x49
    {"LSR", &CPU::exec_LSR_A, &CPU::addr_accumulator, 2, false},// 0x4A
    {"*ALR", &CPU::exec_NOP, &CPU::addr_immediate, 2, false},   // 0x4B - Illegal
    {"JMP", &CPU::exec_JMP, &CPU::addr_absolute, 3, false},     // 0x4C
    {"EOR", &CPU::exec_EOR, &CPU::addr_absolute, 4, false},     // 0x4D
    {"LSR", &CPU::exec_LSR, &CPU::addr_absolute, 6, false},     // 0x4E
    {"*SRE", &CPU::exec_NOP, &CPU::addr_absolute, 6, false},    // 0x4F - Illegal
    
    // 0x50-0x5F
    {"BVC", &CPU::exec_BVC, &CPU::addr_relative, 2, true},      // 0x50
    {"EOR", &CPU::exec_EOR, &CPU::addr_indirect_y, 5, true},    // 0x51
    {"*KIL", &CPU::exec_NOP, &CPU::addr_implied, 2, false},     // 0x52 - Illegal
    {"*SRE", &CPU::exec_NOP, &CPU::addr_indirect_y, 8, false},  // 0x53 - Illegal
    {"*NOP", &CPU::exec_NOP, &CPU::addr_zero_page_x, 4, false}, // 0x54 - Illegal
    {"EOR", &CPU::exec_EOR, &CPU::addr_zero_page_x, 4, false},  // 0x55
    {"LSR", &CPU::exec_LSR, &CPU::addr_zero_page_x, 6, false},  // 0x56
    {"*SRE", &CPU::exec_NOP, &CPU::addr_zero_page_x, 6, false}, // 0x57 - Illegal
    {"CLI", &CPU::exec_CLI, &CPU::addr_implied, 2, false},      // 0x58
    {"EOR", &CPU::exec_EOR, &CPU::addr_absolute_y, 4, true},    // 0x59
    {"*NOP", &CPU::exec_NOP, &CPU::addr_implied, 2, false},     // 0x5A - Illegal
    {"*SRE", &CPU::exec_NOP, &CPU::addr_absolute_y, 7, false},  // 0x5B - Illegal
    {"*NOP", &CPU::exec_NOP, &CPU::addr_absolute_x, 4, true},   // 0x5C - Illegal
    {"EOR", &CPU::exec_EOR, &CPU::addr_absolute_x, 4, true},    // 0x5D
    {"LSR", &CPU::exec_LSR, &CPU::addr_absolute_x, 7, false},   // 0x5E
    {"*SRE", &CPU::exec_NOP, &CPU::addr_absolute_x, 7, false},  // 0x5F - Illegal
    
    // 0x60-0x6F
    {"RTS", &CPU::exec_RTS, &CPU::addr_implied, 6, false},      // 0x60
    {"ADC", &CPU::exec_ADC, &CPU::addr_indirect_x, 6, false},   // 0x61
    {"*KIL", &CPU::exec_NOP, &CPU::addr_implied, 2, false},     // 0x62 - Illegal
    {"*RRA", &CPU::exec_NOP, &CPU::addr_indirect_x, 8, false},  // 0x63 - Illegal
    {"*NOP", &CPU::exec_NOP, &CPU::addr_zero_page, 3, false},   // 0x64 - Illegal
    {"ADC", &CPU::exec_ADC, &CPU::addr_zero_page, 3, false},    // 0x65
    {"ROR", &CPU::exec_ROR, &CPU::addr_zero_page, 5, false},    // 0x66
    {"*RRA", &CPU::exec_NOP, &CPU::addr_zero_page, 5, false},   // 0x67 - Illegal
    {"PLA", &CPU::exec_PLA, &CPU::addr_implied, 4, false},      // 0x68
    {"ADC", &CPU::exec_ADC, &CPU::addr_immediate, 2, false},    // 0x69
    {"ROR", &CPU::exec_ROR_A, &CPU::addr_accumulator, 2, false},// 0x6A
    {"*ARR", &CPU::exec_NOP, &CPU::addr_immediate, 2, false},   // 0x6B - Illegal
    {"JMP", &CPU::exec_JMP, &CPU::addr_indirect, 5, false},     // 0x6C
    {"ADC", &CPU::exec_ADC, &CPU::addr_absolute, 4, false},     // 0x6D
    {"ROR", &CPU::exec_ROR, &CPU::addr_absolute, 6, false},     // 0x6E
    {"*RRA", &CPU::exec_NOP, &CPU::addr_absolute, 6, false},    // 0x6F - Illegal
    
    // 0x70-0x7F
    {"BVS", &CPU::exec_BVS, &CPU::addr_relative, 2, true},      // 0x70
    {"ADC", &CPU::exec_ADC, &CPU::addr_indirect_y, 5, true},    // 0x71
    {"*KIL", &CPU::exec_NOP, &CPU::addr_implied, 2, false},     // 0x72 - Illegal
    {"*RRA", &CPU::exec_NOP, &CPU::addr_indirect_y, 8, false},  // 0x73 - Illegal
    {"*NOP", &CPU::exec_NOP, &CPU::addr_zero_page_x, 4, false}, // 0x74 - Illegal
    {"ADC", &CPU::exec_ADC, &CPU::addr_zero_page_x, 4, false},  // 0x75
    {"ROR", &CPU::exec_ROR, &CPU::addr_zero_page_x, 6, false},  // 0x76
    {"*RRA", &CPU::exec_NOP, &CPU::addr_zero_page_x, 6, false}, // 0x77 - Illegal
    {"SEI", &CPU::exec_SEI, &CPU::addr_implied, 2, false},      // 0x78
    {"ADC", &CPU::exec_ADC, &CPU::addr_absolute_y, 4, true},    // 0x79
    {"*NOP", &CPU::exec_NOP, &CPU::addr_implied, 2, false},     // 0x7A - Illegal
    {"*RRA", &CPU::exec_NOP, &CPU::addr_absolute_y, 7, false},  // 0x7B - Illegal
    {"*NOP", &CPU::exec_NOP, &CPU::addr_absolute_x, 4, true},   // 0x7C - Illegal
    {"ADC", &CPU::exec_ADC, &CPU::addr_absolute_x, 4, true},    // 0x7D
    {"ROR", &CPU::exec_ROR, &CPU::addr_absolute_x, 7, false},   // 0x7E
    {"*RRA", &CPU::exec_NOP, &CPU::addr_absolute_x, 7, false},  // 0x7F - Illegal
    
    // 0x80-0x8F
    {"*NOP", &CPU::exec_NOP, &CPU::addr_immediate, 2, false},   // 0x80 - Illegal
    {"STA", &CPU::exec_STA, &CPU::addr_indirect_x, 6, false},   // 0x81
    {"*NOP", &CPU::exec_NOP, &CPU::addr_immediate, 2, false},   // 0x82 - Illegal
    {"*SAX", &CPU::exec_NOP, &CPU::addr_indirect_x, 6, false},  // 0x83 - Illegal
    {"STY", &CPU::exec_STY, &CPU::addr_zero_page, 3, false},    // 0x84
    {"STA", &CPU::exec_STA, &CPU::addr_zero_page, 3, false},    // 0x85
    {"STX", &CPU::exec_STX, &CPU::addr_zero_page, 3, false},    // 0x86
    {"*SAX", &CPU::exec_NOP, &CPU::addr_zero_page, 3, false},   // 0x87 - Illegal
    {"DEY", &CPU::exec_DEY, &CPU::addr_implied, 2, false},      // 0x88
    {"*NOP", &CPU::exec_NOP, &CPU::addr_immediate, 2, false},   // 0x89 - Illegal
    {"TXA", &CPU::exec_TXA, &CPU::addr_implied, 2, false},      // 0x8A
    {"*XAA", &CPU::exec_NOP, &CPU::addr_immediate, 2, false},   // 0x8B - Illegal
    {"STY", &CPU::exec_STY, &CPU::addr_absolute, 4, false},     // 0x8C
    {"STA", &CPU::exec_STA, &CPU::addr_absolute, 4, false},     // 0x8D
    {"STX", &CPU::exec_STX, &CPU::addr_absolute, 4, false},     // 0x8E
    {"*SAX", &CPU::exec_NOP, &CPU::addr_absolute, 4, false},    // 0x8F - Illegal
    
    // 0x90-0x9F
    {"BCC", &CPU::exec_BCC, &CPU::addr_relative, 2, true},      // 0x90
    {"STA", &CPU::exec_STA, &CPU::addr_indirect_y, 6, false},   // 0x91
    {"*KIL", &CPU::exec_NOP, &CPU::addr_implied, 2, false},     // 0x92 - Illegal
    {"*AHX", &CPU::exec_NOP, &CPU::addr_indirect_y, 6, false},  // 0x93 - Illegal
    {"STY", &CPU::exec_STY, &CPU::addr_zero_page_x, 4, false},  // 0x94
    {"STA", &CPU::exec_STA, &CPU::addr_zero_page_x, 4, false},  // 0x95
    {"STX", &CPU::exec_STX, &CPU::addr_zero_page_y, 4, false},  // 0x96
    {"*SAX", &CPU::exec_NOP, &CPU::addr_zero_page_y, 4, false}, // 0x97 - Illegal
    {"TYA", &CPU::exec_TYA, &CPU::addr_implied, 2, false},      // 0x98
    {"STA", &CPU::exec_STA, &CPU::addr_absolute_y, 5, false},   // 0x99
    {"TXS", &CPU::exec_TXS, &CPU::addr_implied, 2, false},      // 0x9A
    {"*TAS", &CPU::exec_NOP, &CPU::addr_absolute_y, 5, false},  // 0x9B - Illegal
    {"*SHY", &CPU::exec_NOP, &CPU::addr_absolute_x, 5, false},  // 0x9C - Illegal
    {"STA", &CPU::exec_STA, &CPU::addr_absolute_x, 5, false},   // 0x9D
    {"*SHX", &CPU::exec_NOP, &CPU::addr_absolute_y, 5, false},  // 0x9E - Illegal
    {"*AHX", &CPU::exec_NOP, &CPU::addr_absolute_y, 5, false},  // 0x9F - Illegal
    
    // 0xA0-0xAF
    {"LDY", &CPU::exec_LDY, &CPU::addr_immediate, 2, false},    // 0xA0
    {"LDA", &CPU::exec_LDA, &CPU::addr_indirect_x, 6, false},   // 0xA1
    {"LDX", &CPU::exec_LDX, &CPU::addr_immediate, 2, false},    // 0xA2
    {"*LAX", &CPU::exec_LAX, &CPU::addr_indirect_x, 6, false},  // 0xA3 - Illegal
    {"LDY", &CPU::exec_LDY, &CPU::addr_zero_page, 3, false},    // 0xA4
    {"LDA", &CPU::exec_LDA, &CPU::addr_zero_page, 3, false},    // 0xA5
    {"LDX", &CPU::exec_LDX, &CPU::addr_zero_page, 3, false},    // 0xA6
    {"*LAX", &CPU::exec_LAX, &CPU::addr_zero_page, 3, false},   // 0xA7 - Illegal
    {"TAY", &CPU::exec_TAY, &CPU::addr_implied, 2, false},      // 0xA8
    {"LDA", &CPU::exec_LDA, &CPU::addr_immediate, 2, false},    // 0xA9
    {"TAX", &CPU::exec_TAX, &CPU::addr_implied, 2, false},      // 0xAA
    {"*LAX", &CPU::exec_LAX, &CPU::addr_immediate, 2, false},   // 0xAB - Illegal
    {"LDY", &CPU::exec_LDY, &CPU::addr_absolute, 4, false},     // 0xAC
    {"LDA", &CPU::exec_LDA, &CPU::addr_absolute, 4, false},     // 0xAD
    {"LDX", &CPU::exec_LDX, &CPU::addr_absolute, 4, false},     // 0xAE
    {"*LAX", &CPU::exec_LAX, &CPU::addr_absolute, 4, false},    // 0xAF - Illegal
    
    // 0xB0-0xBF
    {"BCS", &CPU::exec_BCS, &CPU::addr_relative, 2, true},      // 0xB0
    {"LDA", &CPU::exec_LDA, &CPU::addr_indirect_y, 5, true},    // 0xB1
    {"*KIL", &CPU::exec_NOP, &CPU::addr_implied, 2, false},     // 0xB2 - Illegal
    {"*LAX", &CPU::exec_LAX, &CPU::addr_indirect_y, 5, true},   // 0xB3 - Illegal
    {"LDY", &CPU::exec_LDY, &CPU::addr_zero_page_x, 4, false},  // 0xB4
    {"LDA", &CPU::exec_LDA, &CPU::addr_zero_page_x, 4, false},  // 0xB5
    {"LDX", &CPU::exec_LDX, &CPU::addr_zero_page_y, 4, false},  // 0xB6
    {"*LAX", &CPU::exec_LAX, &CPU::addr_zero_page_y, 4, false}, // 0xB7 - Illegal
    {"CLV", &CPU::exec_CLV, &CPU::addr_implied, 2, false},      // 0xB8
    {"LDA", &CPU::exec_LDA, &CPU::addr_absolute_y, 4, true},    // 0xB9
    {"TSX", &CPU::exec_TSX, &CPU::addr_implied, 2, false},      // 0xBA
    {"*LAS", &CPU::exec_NOP, &CPU::addr_absolute_y, 4, true},   // 0xBB - Illegal
    {"LDY", &CPU::exec_LDY, &CPU::addr_absolute_x, 4, true},    // 0xBC
    {"LDA", &CPU::exec_LDA, &CPU::addr_absolute_x, 4, true},    // 0xBD
    {"LDX", &CPU::exec_LDX, &CPU::addr_absolute_y, 4, true},    // 0xBE
    {"*LAX", &CPU::exec_LAX, &CPU::addr_absolute_y, 4, true},   // 0xBF - Illegal
    
    // 0xC0-0xCF
    {"CPY", &CPU::exec_CPY, &CPU::addr_immediate, 2, false},    // 0xC0
    {"CMP", &CPU::exec_CMP, &CPU::addr_indirect_x, 6, false},   // 0xC1
    {"*NOP", &CPU::exec_NOP, &CPU::addr_immediate, 2, false},   // 0xC2 - Illegal
    {"*DCP", &CPU::exec_NOP, &CPU::addr_indirect_x, 8, false},  // 0xC3 - Illegal
    {"CPY", &CPU::exec_CPY, &CPU::addr_zero_page, 3, false},    // 0xC4
    {"CMP", &CPU::exec_CMP, &CPU::addr_zero_page, 3, false},    // 0xC5
    {"DEC", &CPU::exec_DEC, &CPU::addr_zero_page, 5, false},    // 0xC6
    {"*DCP", &CPU::exec_NOP, &CPU::addr_zero_page, 5, false},   // 0xC7 - Illegal
    {"INY", &CPU::exec_INY, &CPU::addr_implied, 2, false},      // 0xC8
    {"CMP", &CPU::exec_CMP, &CPU::addr_immediate, 2, false},    // 0xC9
    {"DEX", &CPU::exec_DEX, &CPU::addr_implied, 2, false},      // 0xCA
    {"*AXS", &CPU::exec_NOP, &CPU::addr_immediate, 2, false},   // 0xCB - Illegal
    {"CPY", &CPU::exec_CPY, &CPU::addr_absolute, 4, false},     // 0xCC
    {"CMP", &CPU::exec_CMP, &CPU::addr_absolute, 4, false},     // 0xCD
    {"DEC", &CPU::exec_DEC, &CPU::addr_absolute, 6, false},     // 0xCE
    {"*DCP", &CPU::exec_NOP, &CPU::addr_absolute, 6, false},    // 0xCF - Illegal
    
    // 0xD0-0xDF
    {"BNE", &CPU::exec_BNE, &CPU::addr_relative, 2, true},      // 0xD0
    {"CMP", &CPU::exec_CMP, &CPU::addr_indirect_y, 5, true},    // 0xD1
    {"*KIL", &CPU::exec_NOP, &CPU::addr_implied, 2, false},     // 0xD2 - Illegal
    {"*DCP", &CPU::exec_NOP, &CPU::addr_indirect_y, 8, false},  // 0xD3 - Illegal
    {"*NOP", &CPU::exec_NOP, &CPU::addr_zero_page_x, 4, false}, // 0xD4 - Illegal
    {"CMP", &CPU::exec_CMP, &CPU::addr_zero_page_x, 4, false},  // 0xD5
    {"DEC", &CPU::exec_DEC, &CPU::addr_zero_page_x, 6, false},  // 0xD6
    {"*DCP", &CPU::exec_NOP, &CPU::addr_zero_page_x, 6, false}, // 0xD7 - Illegal
    {"CLD", &CPU::exec_CLD, &CPU::addr_implied, 2, false},      // 0xD8
    {"CMP", &CPU::exec_CMP, &CPU::addr_absolute_y, 4, true},    // 0xD9
    {"*NOP", &CPU::exec_NOP, &CPU::addr_implied, 2, false},     // 0xDA - Illegal
    {"*DCP", &CPU::exec_NOP, &CPU::addr_absolute_y, 7, false},  // 0xDB - Illegal
    {"*NOP", &CPU::exec_NOP, &CPU::addr_absolute_x, 4, true},   // 0xDC - Illegal
    {"CMP", &CPU::exec_CMP, &CPU::addr_absolute_x, 4, true},    // 0xDD
    {"DEC", &CPU::exec_DEC, &CPU::addr_absolute_x, 7, false},   // 0xDE
    {"*DCP", &CPU::exec_NOP, &CPU::addr_absolute_x, 7, false},  // 0xDF - Illegal
    
    // 0xE0-0xEF
    {"CPX", &CPU::exec_CPX, &CPU::addr_immediate, 2, false},    // 0xE0
    {"SBC", &CPU::exec_SBC, &CPU::addr_indirect_x, 6, false},   // 0xE1
    {"*NOP", &CPU::exec_NOP, &CPU::addr_immediate, 2, false},   // 0xE2 - Illegal
    {"*ISC", &CPU::exec_NOP, &CPU::addr_indirect_x, 8, false},  // 0xE3 - Illegal
    {"CPX", &CPU::exec_CPX, &CPU::addr_zero_page, 3, false},    // 0xE4
    {"SBC", &CPU::exec_SBC, &CPU::addr_zero_page, 3, false},    // 0xE5
    {"INC", &CPU::exec_INC, &CPU::addr_zero_page, 5, false},    // 0xE6
    {"*ISC", &CPU::exec_NOP, &CPU::addr_zero_page, 5, false},   // 0xE7 - Illegal
    {"INX", &CPU::exec_INX, &CPU::addr_implied, 2, false},      // 0xE8
    {"SBC", &CPU::exec_SBC, &CPU::addr_immediate, 2, false},    // 0xE9
    {"NOP", &CPU::exec_NOP, &CPU::addr_implied, 2, false},      // 0xEA
    {"*SBC", &CPU::exec_SBC, &CPU::addr_immediate, 2, false},   // 0xEB - Illegal (same as SBC)
    {"CPX", &CPU::exec_CPX, &CPU::addr_absolute, 4, false},     // 0xEC
    {"SBC", &CPU::exec_SBC, &CPU::addr_absolute, 4, false},     // 0xED
    {"INC", &CPU::exec_INC, &CPU::addr_absolute, 6, false},     // 0xEE
    {"*ISC", &CPU::exec_NOP, &CPU::addr_absolute, 6, false},    // 0xEF - Illegal
    
    // 0xF0-0xFF
    {"BEQ", &CPU::exec_BEQ, &CPU::addr_relative, 2, true},      // 0xF0
    {"SBC", &CPU::exec_SBC, &CPU::addr_indirect_y, 5, true},    // 0xF1
    {"*KIL", &CPU::exec_NOP, &CPU::addr_implied, 2, false},     // 0xF2 - Illegal
    {"*ISC", &CPU::exec_NOP, &CPU::addr_indirect_y, 8, false},  // 0xF3 - Illegal
    {"*NOP", &CPU::exec_NOP, &CPU::addr_zero_page_x, 4, false}, // 0xF4 - Illegal
    {"SBC", &CPU::exec_SBC, &CPU::addr_zero_page_x, 4, false},  // 0xF5
    {"INC", &CPU::exec_INC, &CPU::addr_zero_page_x, 6, false},  // 0xF6
    {"*ISC", &CPU::exec_NOP, &CPU::addr_zero_page_x, 6, false}, // 0xF7 - Illegal
    {"SED", &CPU::exec_SED, &CPU::addr_implied, 2, false},      // 0xF8
    {"SBC", &CPU::exec_SBC, &CPU::addr_absolute_y, 4, true},    // 0xF9
    {"*NOP", &CPU::exec_NOP, &CPU::addr_implied, 2, false},     // 0xFA - Illegal
    {"*ISC", &CPU::exec_NOP, &CPU::addr_absolute_y, 7, false},  // 0xFB - Illegal
    {"*NOP", &CPU::exec_NOP, &CPU::addr_absolute_x, 4, true},   // 0xFC - Illegal
    {"SBC", &CPU::exec_SBC, &CPU::addr_absolute_x, 4, true},    // 0xFD
    {"INC", &CPU::exec_INC, &CPU::addr_absolute_x, 7, false},   // 0xFE
    {"*ISC", &CPU::exec_NOP, &CPU::addr_absolute_x, 7, false},  // 0xFF - Illegal
}};

// Execute opcode mới sử dụng table
void CPU::execute(uint8_t opcode) {
    const OpcodeInfo& info = OPCODE_TABLE[opcode];
    
    // Get address từ addressing mode
    uint16_t addr = (this->*info.addr_mode)();
    
    // Execute instruction
    (this->*info.execute)(addr);
    
    // Set cycles
    cycles_remaining = info.cycles;
    
    // TODO: Add page cross penalty if needed
}

} // namespace nes
