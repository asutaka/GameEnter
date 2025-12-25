#include "disassembler.h"
#include "../core/memory/memory.h"
#include <sstream>
#include <iomanip>

namespace nes {

// Opcode lookup table với instruction name và length
struct OpcodeEntry {
    const char* name;
    int length;
    const char* addr_mode;
};

static const OpcodeEntry OPCODE_TABLE[256] = {
    // 0x00-0x0F
    {"BRK", 1, "impl"}, {"ORA", 2, "X,ind"}, {"*KIL", 1, "impl"}, {"*SLO", 2, "X,ind"},
    {"*NOP", 2, "zpg"}, {"ORA", 2, "zpg"}, {"ASL", 2, "zpg"}, {"*SLO", 2, "zpg"},
    {"PHP", 1, "impl"}, {"ORA", 2, "#"}, {"ASL", 1, "A"}, {"*ANC", 2, "#"},
    {"*NOP", 3, "abs"}, {"ORA", 3, "abs"}, {"ASL", 3, "abs"}, {"*SLO", 3, "abs"},
    
    // 0x10-0x1F
    {"BPL", 2, "rel"}, {"ORA", 2, "ind,Y"}, {"*KIL", 1, "impl"}, {"*SLO", 2, "ind,Y"},
    {"*NOP", 2, "zpg,X"}, {"ORA", 2, "zpg,X"}, {"ASL", 2, "zpg,X"}, {"*SLO", 2, "zpg,X"},
    {"CLC", 1, "impl"}, {"ORA", 3, "abs,Y"}, {"*NOP", 1, "impl"}, {"*SLO", 3, "abs,Y"},
    {"*NOP", 3, "abs,X"}, {"ORA", 3, "abs,X"}, {"ASL", 3, "abs,X"}, {"*SLO", 3, "abs,X"},
    
    // 0x20-0x2F
    {"JSR", 3, "abs"}, {"AND", 2, "X,ind"}, {"*KIL", 1, "impl"}, {"*RLA", 2, "X,ind"},
    {"BIT", 2, "zpg"}, {"AND", 2, "zpg"}, {"ROL", 2, "zpg"}, {"*RLA", 2, "zpg"},
    {"PLP", 1, "impl"}, {"AND", 2, "#"}, {"ROL", 1, "A"}, {"*ANC", 2, "#"},
    {"BIT", 3, "abs"}, {"AND", 3, "abs"}, {"ROL", 3, "abs"}, {"*RLA", 3, "abs"},
    
    // 0x30-0x3F
    {"BMI", 2, "rel"}, {"AND", 2, "ind,Y"}, {"*KIL", 1, "impl"}, {"*RLA", 2, "ind,Y"},
    {"*NOP", 2, "zpg,X"}, {"AND", 2, "zpg,X"}, {"ROL", 2, "zpg,X"}, {"*RLA", 2, "zpg,X"},
    {"SEC", 1, "impl"}, {"AND", 3, "abs,Y"}, {"*NOP", 1, "impl"}, {"*RLA", 3, "abs,Y"},
    {"*NOP", 3, "abs,X"}, {"AND", 3, "abs,X"}, {"ROL", 3, "abs,X"}, {"*RLA", 3, "abs,X"},
    
    // 0x40-0x4F
    {"RTI", 1, "impl"}, {"EOR", 2, "X,ind"}, {"*KIL", 1, "impl"}, {"*SRE", 2, "X,ind"},
    {"*NOP", 2, "zpg"}, {"EOR", 2, "zpg"}, {"LSR", 2, "zpg"}, {"*SRE", 2, "zpg"},
    {"PHA", 1, "impl"}, {"EOR", 2, "#"}, {"LSR", 1, "A"}, {"*ALR", 2, "#"},
    {"JMP", 3, "abs"}, {"EOR", 3, "abs"}, {"LSR", 3, "abs"}, {"*SRE", 3, "abs"},
    
    // 0x50-0x5F
    {"BVC", 2, "rel"}, {"EOR", 2, "ind,Y"}, {"*KIL", 1, "impl"}, {"*SRE", 2, "ind,Y"},
    {"*NOP", 2, "zpg,X"}, {"EOR", 2, "zpg,X"}, {"LSR", 2, "zpg,X"}, {"*SRE", 2, "zpg,X"},
    {"CLI", 1, "impl"}, {"EOR", 3, "abs,Y"}, {"*NOP", 1, "impl"}, {"*SRE", 3, "abs,Y"},
    {"*NOP", 3, "abs,X"}, {"EOR", 3, "abs,X"}, {"LSR", 3, "abs,X"}, {"*SRE", 3, "abs,X"},
    
    // 0x60-0x6F
    {"RTS", 1, "impl"}, {"ADC", 2, "X,ind"}, {"*KIL", 1, "impl"}, {"*RRA", 2, "X,ind"},
    {"*NOP", 2, "zpg"}, {"ADC", 2, "zpg"}, {"ROR", 2, "zpg"}, {"*RRA", 2, "zpg"},
    {"PLA", 1, "impl"}, {"ADC", 2, "#"}, {"ROR", 1, "A"}, {"*ARR", 2, "#"},
    {"JMP", 3, "ind"}, {"ADC", 3, "abs"}, {"ROR", 3, "abs"}, {"*RRA", 3, "abs"},
    
    // 0x70-0x7F
    {"BVS", 2, "rel"}, {"ADC", 2, "ind,Y"}, {"*KIL", 1, "impl"}, {"*RRA", 2, "ind,Y"},
    {"*NOP", 2, "zpg,X"}, {"ADC", 2, "zpg,X"}, {"ROR", 2, "zpg,X"}, {"*RRA", 2, "zpg,X"},
    {"SEI", 1, "impl"}, {"ADC", 3, "abs,Y"}, {"*NOP", 1, "impl"}, {"*RRA", 3, "abs,Y"},
    {"*NOP", 3, "abs,X"}, {"ADC", 3, "abs,X"}, {"ROR", 3, "abs,X"}, {"*RRA", 3, "abs,X"},
    
    // 0x80-0x8F
    {"*NOP", 2, "#"}, {"STA", 2, "X,ind"}, {"*NOP", 2, "#"}, {"*SAX", 2, "X,ind"},
    {"STY", 2, "zpg"}, {"STA", 2, "zpg"}, {"STX", 2, "zpg"}, {"*SAX", 2, "zpg"},
    {"DEY", 1, "impl"}, {"*NOP", 2, "#"}, {"TXA", 1, "impl"}, {"*XAA", 2, "#"},
    {"STY", 3, "abs"}, {"STA", 3, "abs"}, {"STX", 3, "abs"}, {"*SAX", 3, "abs"},
    
    // 0x90-0x9F
    {"BCC", 2, "rel"}, {"STA", 2, "ind,Y"}, {"*KIL", 1, "impl"}, {"*AHX", 2, "ind,Y"},
    {"STY", 2, "zpg,X"}, {"STA", 2, "zpg,X"}, {"STX", 2, "zpg,Y"}, {"*SAX", 2, "zpg,Y"},
    {"TYA", 1, "impl"}, {"STA", 3, "abs,Y"}, {"TXS", 1, "impl"}, {"*TAS", 3, "abs,Y"},
    {"*SHY", 3, "abs,X"}, {"STA", 3, "abs,X"}, {"*SHX", 3, "abs,Y"}, {"*AHX", 3, "abs,Y"},
    
    // 0xA0-0xAF
    {"LDY", 2, "#"}, {"LDA", 2, "X,ind"}, {"LDX", 2, "#"}, {"*LAX", 2, "X,ind"},
    {"LDY", 2, "zpg"}, {"LDA", 2, "zpg"}, {"LDX", 2, "zpg"}, {"*LAX", 2, "zpg"},
    {"TAY", 1, "impl"}, {"LDA", 2, "#"}, {"TAX", 1, "impl"}, {"*LAX", 2, "#"},
    {"LDY", 3, "abs"}, {"LDA", 3, "abs"}, {"LDX", 3, "abs"}, {"*LAX", 3, "abs"},
    
    // 0xB0-0xBF
    {"BCS", 2, "rel"}, {"LDA", 2, "ind,Y"}, {"*KIL", 1, "impl"}, {"*LAX", 2, "ind,Y"},
    {"LDY", 2, "zpg,X"}, {"LDA", 2, "zpg,X"}, {"LDX", 2, "zpg,Y"}, {"*LAX", 2, "zpg,Y"},
    {"CLV", 1, "impl"}, {"LDA", 3, "abs,Y"}, {"TSX", 1, "impl"}, {"*LAS", 3, "abs,Y"},
    {"LDY", 3, "abs,X"}, {"LDA", 3, "abs,X"}, {"LDX", 3, "abs,Y"}, {"*LAX", 3, "abs,Y"},
    
    // 0xC0-0xCF
    {"CPY", 2, "#"}, {"CMP", 2, "X,ind"}, {"*NOP", 2, "#"}, {"*DCP", 2, "X,ind"},
    {"CPY", 2, "zpg"}, {"CMP", 2, "zpg"}, {"DEC", 2, "zpg"}, {"*DCP", 2, "zpg"},
    {"INY", 1, "impl"}, {"CMP", 2, "#"}, {"DEX", 1, "impl"}, {"*AXS", 2, "#"},
    {"CPY", 3, "abs"}, {"CMP", 3, "abs"}, {"DEC", 3, "abs"}, {"*DCP", 3, "abs"},
    
    // 0xD0-0xDF
    {"BNE", 2, "rel"}, {"CMP", 2, "ind,Y"}, {"*KIL", 1, "impl"}, {"*DCP", 2, "ind,Y"},
    {"*NOP", 2, "zpg,X"}, {"CMP", 2, "zpg,X"}, {"DEC", 2, "zpg,X"}, {"*DCP", 2, "zpg,X"},
    {"CLD", 1, "impl"}, {"CMP", 3, "abs,Y"}, {"*NOP", 1, "impl"}, {"*DCP", 3, "abs,Y"},
    {"*NOP", 3, "abs,X"}, {"CMP", 3, "abs,X"}, {"DEC", 3, "abs,X"}, {"*DCP", 3, "abs,X"},
    
    // 0xE0-0xEF
    {"CPX", 2, "#"}, {"SBC", 2, "X,ind"}, {"*NOP", 2, "#"}, {"*ISC", 2, "X,ind"},
    {"CPX", 2, "zpg"}, {"SBC", 2, "zpg"}, {"INC", 2, "zpg"}, {"*ISC", 2, "zpg"},
    {"INX", 1, "impl"}, {"SBC", 2, "#"}, {"NOP", 1, "impl"}, {"*SBC", 2, "#"},
    {"CPX", 3, "abs"}, {"SBC", 3, "abs"}, {"INC", 3, "abs"}, {"*ISC", 3, "abs"},
    
    // 0xF0-0xFF
    {"BEQ", 2, "rel"}, {"SBC", 2, "ind,Y"}, {"*KIL", 1, "impl"}, {"*ISC", 2, "ind,Y"},
    {"*NOP", 2, "zpg,X"}, {"SBC", 2, "zpg,X"}, {"INC", 2, "zpg,X"}, {"*ISC", 2, "zpg,X"},
    {"SED", 1, "impl"}, {"SBC", 3, "abs,Y"}, {"*NOP", 1, "impl"}, {"*ISC", 3, "abs,Y"},
    {"*NOP", 3, "abs,X"}, {"SBC", 3, "abs,X"}, {"INC", 3, "abs,X"}, {"*ISC", 3, "abs,X"}
};

DisassembledInstruction Disassembler::disassemble(uint16_t pc, Memory* memory) {
    DisassembledInstruction inst;
    
    inst.opcode = memory->read(pc);
    const OpcodeEntry& entry = OPCODE_TABLE[inst.opcode];
    
    inst.length = entry.length;
    inst.mnemonic = entry.name;
    inst.bytes[0] = inst.opcode;
    
    // Read operand bytes
    if (inst.length >= 2) {
        inst.bytes[1] = memory->read(pc + 1);
    }
    if (inst.length == 3) {
        inst.bytes[2] = memory->read(pc + 2);
    }
    
    // Format operand based on addressing mode
    std::string mode = entry.addr_mode;
    std::ostringstream oss;
    
    if (mode == "impl" || mode == "A") {
        inst.operand = "";
    }
    else if (mode == "#") {
        oss << "#$" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') 
            << (int)inst.bytes[1];
        inst.operand = oss.str();
    }
    else if (mode == "zpg") {
        oss << "$" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') 
            << (int)inst.bytes[1];
        inst.operand = oss.str();
    }
    else if (mode == "zpg,X") {
        oss << "$" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') 
            << (int)inst.bytes[1] << ",X";
        inst.operand = oss.str();
    }
    else if (mode == "zpg,Y") {
        oss << "$" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') 
            << (int)inst.bytes[1] << ",Y";
        inst.operand = oss.str();
    }
    else if (mode == "abs") {
        uint16_t addr = inst.bytes[1] | (inst.bytes[2] << 8);
        oss << "$" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') 
            << addr;
        inst.operand = oss.str();
    }
    else if (mode == "abs,X") {
        uint16_t addr = inst.bytes[1] | (inst.bytes[2] << 8);
        oss << "$" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') 
            << addr << ",X";
        inst.operand = oss.str();
    }
    else if (mode == "abs,Y") {
        uint16_t addr = inst.bytes[1] | (inst.bytes[2] << 8);
        oss << "$" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') 
            << addr << ",Y";
        inst.operand = oss.str();
    }
    else if (mode == "ind") {
        uint16_t addr = inst.bytes[1] | (inst.bytes[2] << 8);
        oss << "($" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') 
            << addr << ")";
        inst.operand = oss.str();
    }
    else if (mode == "X,ind") {
        oss << "($" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') 
            << (int)inst.bytes[1] << ",X)";
        inst.operand = oss.str();
    }
    else if (mode == "ind,Y") {
        oss << "($" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') 
            << (int)inst.bytes[1] << "),Y";
        inst.operand = oss.str();
    }
    else if (mode == "rel") {
        int8_t offset = (int8_t)inst.bytes[1];
        uint16_t target = pc + 2 + offset;
        oss << "$" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') 
            << target;
        inst.operand = oss.str();
    }
    
    return inst;
}

std::string DisassembledInstruction::to_string() const {
    std::ostringstream oss;
    
    // Opcode bytes (padded to 9 chars for alignment - nestest format)
    oss << std::hex << std::uppercase << std::setfill('0');
    for (int i = 0; i < length; i++) {
        oss << std::setw(2) << (int)bytes[i];
        if (i < length - 1) oss << " ";
    }
    
    // Pad to 9 characters (includes trailing space after bytes)
    std::string bytes_str = oss.str();
    while (bytes_str.length() < 9) {
        bytes_str += " ";
    }
    
    // Instruction
    std::string instr = mnemonic;
    if (!operand.empty()) {
        instr += " " + operand;
    }
    
    return bytes_str + instr;
}

} // namespace nes
