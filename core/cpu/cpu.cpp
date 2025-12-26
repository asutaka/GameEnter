#include "cpu/cpu.h"
#include "memory/memory.h"
#include <cstring>

namespace nes {

CPU::CPU() 
    : A(0), X(0), Y(0), SP(0xFD), P(0x24),
      PC(0), total_cycles(0), cycles_remaining(0),
      memory_(nullptr) {
}

CPU::~CPU() {
}

void CPU::connect_memory(Memory* memory) {
    memory_ = memory;
}

void CPU::reset() {
    // Reset registers
    A = 0;
    X = 0;
    Y = 0;
    SP = 0xFD;
    P = 0x24; // StatusFlag::FLAG_INTERRUPT disable = 1, StatusFlag::FLAG_UNUSED = 1
    
    // Load PC from reset vector ($FFFC-$FFFD)
    if (memory_) {
        PC = read16(0xFFFC);
    } else {
        PC = 0;
    }
    
    cycles_remaining = 7; // Reset mất 7 cycles
    page_crossed_ = false;
}

int CPU::step() {
    if (cycles_remaining > 0) {
        cycles_remaining--;
        total_cycles++;
        return 1;
    }
    
    // Đọc opcode
    uint8_t opcode = read(PC++);
    
    // Thực thi
    execute(opcode);
    
    return cycles_remaining;
}

void CPU::irq() {
    if (!get_flag(StatusFlag::FLAG_INTERRUPT)) {
        push16(PC);
        push(P & ~static_cast<uint8_t>(StatusFlag::FLAG_BREAK));
        set_flag(StatusFlag::FLAG_INTERRUPT, true);
        PC = read16(0xFFFE);
        cycles_remaining = 7;
    }
}

void CPU::nmi() {
    push16(PC);
    push(P & ~static_cast<uint8_t>(StatusFlag::FLAG_BREAK));
    set_flag(StatusFlag::FLAG_INTERRUPT, true);
    PC = read16(0xFFFA);
    cycles_remaining = 7;
}

// =====================
// Helper Functions
// =====================

void CPU::set_flag(StatusFlag flag, bool value) {
    uint8_t flag_value = static_cast<uint8_t>(flag);
    if (value) {
        P |= flag_value;
    } else {
        P &= ~flag_value;
    }
}

bool CPU::get_flag(StatusFlag flag) const {
    return (P & static_cast<uint8_t>(flag)) != 0;
}

void CPU::update_zero_negative(uint8_t value) {
    set_flag(StatusFlag::FLAG_ZERO, value == 0);
    set_flag(StatusFlag::FLAG_NEGATIVE, (value & 0x80) != 0);
}

// =====================
// Memory Access
// =====================

uint8_t CPU::read(uint16_t address) {
    if (memory_) {
        return memory_->read(address);
    }
    return 0;
}

void CPU::write(uint16_t address, uint8_t value) {
    if (memory_) {
        memory_->write(address, value);
    }
}

uint16_t CPU::read16(uint16_t address) {
    uint8_t lo = read(address);
    uint8_t hi = read(address + 1);
    return (hi << 8) | lo;
}

// =====================
// Stack Operations
// =====================

void CPU::push(uint8_t value) {
    write(0x0100 + SP, value);
    SP--;
}

uint8_t CPU::pop() {
    SP++;
    return read(0x0100 + SP);
}

void CPU::push16(uint16_t value) {
    push(value >> 8);   // High byte
    push(value & 0xFF); // Low byte
}

uint16_t CPU::pop16() {
    uint8_t lo = pop();
    uint8_t hi = pop();
    return (hi << 8) | lo;
}

// Read 16-bit value from zero page with wraparound
// When reading from $FF, it reads $FF and $00 (not $FF and $100)
uint16_t CPU::read16_zp(uint8_t address) {
    uint8_t lo = read(address);
    uint8_t hi = read((address + 1) & 0xFF);  // Wrap around in zero page
    return (hi << 8) | lo;
}

// =====================
// Addressing Modes
// =====================

uint16_t CPU::addr_implied() {
    return 0;
}

uint16_t CPU::addr_accumulator() {
    return 0;
}

uint16_t CPU::addr_immediate() {
    return PC++;
}

uint16_t CPU::addr_zero_page() {
    return read(PC++);
}

uint16_t CPU::addr_zero_page_x() {
    return (read(PC++) + X) & 0xFF;
}

uint16_t CPU::addr_zero_page_y() {
    return (read(PC++) + Y) & 0xFF;
}

uint16_t CPU::addr_absolute() {
    return read16(PC), PC += 2, read16(PC - 2);
}

uint16_t CPU::addr_absolute_x() {
    uint16_t base = read16(PC);
    PC += 2;
    uint16_t addr = base + X;
    page_crossed_ = (base & 0xFF00) != (addr & 0xFF00);
    return addr;
}

uint16_t CPU::addr_absolute_y() {
    uint16_t base = read16(PC);
    PC += 2;
    uint16_t addr = base + Y;
    page_crossed_ = (base & 0xFF00) != (addr & 0xFF00);
    return addr;
}

uint16_t CPU::addr_indirect() {
    uint16_t ptr = read16(PC);
    PC += 2;
    
    // 6502 bug: nếu ptr ở cuối page ($xxFF), high byte wrap về $xx00
    if ((ptr & 0xFF) == 0xFF) {
        uint8_t lo = read(ptr);
        uint8_t hi = read(ptr & 0xFF00);
        return (hi << 8) | lo;
    }
    
    return read16(ptr);
}

uint16_t CPU::addr_indirect_x() {
    uint8_t ptr = read(PC++) + X;
    // Must use zero page wraparound when reading pointer
    return read16_zp(ptr);
}

uint16_t CPU::addr_indirect_y() {
    uint8_t ptr = read(PC++);
    // Must use zero page wraparound when reading pointer
    uint16_t base = read16_zp(ptr);
    uint16_t addr = base + Y;
    page_crossed_ = (base & 0xFF00) != (addr & 0xFF00);
    return addr;
}

uint16_t CPU::addr_relative() {
    int8_t offset = static_cast<int8_t>(read(PC++));
    return PC + offset;
}

// =====================
// Opcodes - Load/Store
// =====================

void CPU::LDA(uint16_t addr) {
    A = read(addr);
    update_zero_negative(A);
}

void CPU::LDX(uint16_t addr) {
    X = read(addr);
    update_zero_negative(X);
}

void CPU::LDY(uint16_t addr) {
    Y = read(addr);
    update_zero_negative(Y);
}

void CPU::STA(uint16_t addr) {
    write(addr, A);
}

void CPU::STX(uint16_t addr) {
    write(addr, X);
}

void CPU::STY(uint16_t addr) {
    write(addr, Y);
}

// =====================
// Opcodes - Transfer
// =====================

void CPU::TAX() {
    X = A;
    update_zero_negative(X);
}

void CPU::TAY() {
    Y = A;
    update_zero_negative(Y);
}

void CPU::TXA() {
    A = X;
    update_zero_negative(A);
}

void CPU::TYA() {
    A = Y;
    update_zero_negative(A);
}

void CPU::TSX() {
    X = SP;
    update_zero_negative(X);
}

void CPU::TXS() {
    SP = X;
}

// =====================
// Opcodes - Stack
// =====================

void CPU::PHA() {
    push(A);
}

void CPU::PHP() {
    push(P | static_cast<uint8_t>(StatusFlag::FLAG_BREAK) | static_cast<uint8_t>(StatusFlag::FLAG_UNUSED));
}

void CPU::PLA() {
    A = pop();
    update_zero_negative(A);
}

void CPU::PLP() {
    P = pop();
    set_flag(StatusFlag::FLAG_BREAK, false);
    set_flag(StatusFlag::FLAG_UNUSED, true);
}

// =====================
// Opcodes - Logical
// =====================

void CPU::AND(uint16_t addr) {
    A &= read(addr);
    update_zero_negative(A);
}

void CPU::EOR(uint16_t addr) {
    A ^= read(addr);
    update_zero_negative(A);
}

void CPU::ORA(uint16_t addr) {
    A |= read(addr);
    update_zero_negative(A);
}

void CPU::BIT(uint16_t addr) {
    uint8_t value = read(addr);
    set_flag(StatusFlag::FLAG_ZERO, (A & value) == 0);
    set_flag(StatusFlag::FLAG_OVERFLOW, (value & 0x40) != 0);
    set_flag(StatusFlag::FLAG_NEGATIVE, (value & 0x80) != 0);
}

// =====================
// Opcodes - Arithmetic
// =====================

void CPU::ADC(uint16_t addr) {
    uint8_t value = read(addr);
    uint16_t result = A + value + (get_flag(StatusFlag::FLAG_CARRY) ? 1 : 0);
    
    set_flag(StatusFlag::FLAG_CARRY, result > 0xFF);
    set_flag(StatusFlag::FLAG_OVERFLOW, (~(A ^ value) & (A ^ result) & 0x80) != 0);
    
    A = result & 0xFF;
    update_zero_negative(A);
}

void CPU::SBC(uint16_t addr) {
    uint8_t value = read(addr);
    uint16_t result = A - value - (get_flag(StatusFlag::FLAG_CARRY) ? 0 : 1);
    
    set_flag(StatusFlag::FLAG_CARRY, result < 0x100);
    set_flag(StatusFlag::FLAG_OVERFLOW, ((A ^ value) & (A ^ result) & 0x80) != 0);
    
    A = result & 0xFF;
    update_zero_negative(A);
}

void CPU::CMP(uint16_t addr) {
    uint8_t value = read(addr);
    uint16_t result = A - value;
    
    set_flag(StatusFlag::FLAG_CARRY, A >= value);
    update_zero_negative(result & 0xFF);
}

void CPU::CPX(uint16_t addr) {
    uint8_t value = read(addr);
    uint16_t result = X - value;
    
    set_flag(StatusFlag::FLAG_CARRY, X >= value);
    update_zero_negative(result & 0xFF);
}

void CPU::CPY(uint16_t addr) {
    uint8_t value = read(addr);
    uint16_t result = Y - value;
    
    set_flag(StatusFlag::FLAG_CARRY, Y >= value);
    update_zero_negative(result & 0xFF);
}

// =====================
// Opcodes - Inc/Dec
// =====================

void CPU::INC(uint16_t addr) {
    uint8_t value = read(addr) + 1;
    write(addr, value);
    update_zero_negative(value);
}

void CPU::INX() {
    X++;
    update_zero_negative(X);
}

void CPU::INY() {
    Y++;
    update_zero_negative(Y);
}

void CPU::DEC(uint16_t addr) {
    uint8_t value = read(addr) - 1;
    write(addr, value);
    update_zero_negative(value);
}

void CPU::DEX() {
    X--;
    update_zero_negative(X);
}

void CPU::DEY() {
    Y--;
    update_zero_negative(Y);
}

// =====================
// Opcodes - Shift
// =====================

void CPU::ASL(uint16_t addr) {
    uint8_t value = read(addr);
    set_flag(StatusFlag::FLAG_CARRY, (value & 0x80) != 0);
    value <<= 1;
    write(addr, value);
    update_zero_negative(value);
}

void CPU::LSR(uint16_t addr) {
    uint8_t value = read(addr);
    set_flag(StatusFlag::FLAG_CARRY, (value & 0x01) != 0);
    value >>= 1;
    write(addr, value);
    update_zero_negative(value);
}

void CPU::ROL(uint16_t addr) {
    uint8_t value = read(addr);
    bool old_carry = get_flag(StatusFlag::FLAG_CARRY);
    set_flag(StatusFlag::FLAG_CARRY, (value & 0x80) != 0);
    value = (value << 1) | (old_carry ? 1 : 0);
    write(addr, value);
    update_zero_negative(value);
}

void CPU::ROR(uint16_t addr) {
    uint8_t value = read(addr);
    bool old_carry = get_flag(StatusFlag::FLAG_CARRY);
    set_flag(StatusFlag::FLAG_CARRY, (value & 0x01) != 0);
    value = (value >> 1) | (old_carry ? 0x80 : 0);
    write(addr, value);
    update_zero_negative(value);
}

// =====================
// Opcodes - Jump/Call
// =====================

void CPU::JMP(uint16_t addr) {
    PC = addr;
}

void CPU::JSR(uint16_t addr) {
    push16(PC - 1);
    PC = addr;
}

void CPU::RTS() {
    PC = pop16() + 1;
}

// =====================
// Opcodes - Branch
// =====================

void CPU::BCC(uint16_t addr) {
    if (!get_flag(StatusFlag::FLAG_CARRY)) {
        cycles_remaining++; // Branch taken: +1 cycle
        if ((PC & 0xFF00) != (addr & 0xFF00)) {
            cycles_remaining++; // Page cross: +1 cycle
        }
        PC = addr;
    }
}

void CPU::BCS(uint16_t addr) {
    if (get_flag(StatusFlag::FLAG_CARRY)) {
        cycles_remaining++;
        if ((PC & 0xFF00) != (addr & 0xFF00)) {
            cycles_remaining++;
        }
        PC = addr;
    }
}

void CPU::BEQ(uint16_t addr) {
    if (get_flag(StatusFlag::FLAG_ZERO)) {
        cycles_remaining++;
        if ((PC & 0xFF00) != (addr & 0xFF00)) {
            cycles_remaining++;
        }
        PC = addr;
    }
}

void CPU::BMI(uint16_t addr) {
    if (get_flag(StatusFlag::FLAG_NEGATIVE)) {
        cycles_remaining++;
        if ((PC & 0xFF00) != (addr & 0xFF00)) {
            cycles_remaining++;
        }
        PC = addr;
    }
}

void CPU::BNE(uint16_t addr) {
    if (!get_flag(StatusFlag::FLAG_ZERO)) {
        cycles_remaining++;
        if ((PC & 0xFF00) != (addr & 0xFF00)) {
            cycles_remaining++;
        }
        PC = addr;
    }
}

void CPU::BPL(uint16_t addr) {
    if (!get_flag(StatusFlag::FLAG_NEGATIVE)) {
        cycles_remaining++;
        if ((PC & 0xFF00) != (addr & 0xFF00)) {
            cycles_remaining++;
        }
        PC = addr;
    }
}

void CPU::BVC(uint16_t addr) {
    if (!get_flag(StatusFlag::FLAG_OVERFLOW)) {
        cycles_remaining++;
        if ((PC & 0xFF00) != (addr & 0xFF00)) {
            cycles_remaining++;
        }
        PC = addr;
    }
}

void CPU::BVS(uint16_t addr) {
    if (get_flag(StatusFlag::FLAG_OVERFLOW)) {
        cycles_remaining++;
        if ((PC & 0xFF00) != (addr & 0xFF00)) {
            cycles_remaining++;
        }
        PC = addr;
    }
}

// =====================
// Opcodes - Flag
// =====================

void CPU::CLC() { set_flag(StatusFlag::FLAG_CARRY, false); }
void CPU::CLD() { set_flag(StatusFlag::FLAG_DECIMAL, false); }
void CPU::CLI() { set_flag(StatusFlag::FLAG_INTERRUPT, false); }
void CPU::CLV() { set_flag(StatusFlag::FLAG_OVERFLOW, false); }
void CPU::SEC() { set_flag(StatusFlag::FLAG_CARRY, true); }
void CPU::SED() { set_flag(StatusFlag::FLAG_DECIMAL, true); }
void CPU::SEI() { set_flag(StatusFlag::FLAG_INTERRUPT, true); }

// =====================
// Opcodes - System
// =====================

void CPU::BRK() {
    PC++;
    push16(PC);
    push(P | static_cast<uint8_t>(StatusFlag::FLAG_BREAK));
    set_flag(StatusFlag::FLAG_INTERRUPT, true);
    PC = read16(0xFFFE);
}

void CPU::NOP() {
    // Không làm gì
}

void CPU::RTI() {
    P = pop();
    set_flag(StatusFlag::FLAG_BREAK, false);
    set_flag(StatusFlag::FLAG_UNUSED, true);
    PC = pop16();
}

// =====================
// Illegal Opcodes
// =====================
// These are undocumented opcodes that combine two operations
// Some games and nestest rely on them

void CPU::LAX(uint16_t addr) {
    // LDA + LDX: Load both A and X with same value
    uint8_t value = read(addr);
    A = value;
    X = value;
    update_zero_negative(value);
}

void CPU::SAX(uint16_t addr) {
    // Store A AND X
    write(addr, A & X);
}

void CPU::DCP(uint16_t addr) {
    // DEC + CMP: Decrement memory then compare with A
    uint8_t value = read(addr) - 1;
    write(addr, value);
    
    // CMP logic
    uint16_t result = A - value;
    set_flag(StatusFlag::FLAG_CARRY, A >= value);
    update_zero_negative(result & 0xFF);
}

void CPU::ISC(uint16_t addr) {
    // INC + SBC: Increment memory then subtract from A
    uint8_t value = read(addr) + 1;
    write(addr, value);
    
    // SBC logic
    uint16_t result = A - value - (get_flag(StatusFlag::FLAG_CARRY) ? 0 : 1);
    set_flag(StatusFlag::FLAG_CARRY, result < 0x100);
    set_flag(StatusFlag::FLAG_OVERFLOW, ((A ^ value) & (A ^ result) & 0x80) != 0);
    A = result & 0xFF;
    update_zero_negative(A);
}

void CPU::SLO(uint16_t addr) {
    // ASL + ORA: Shift left then OR with A
    uint8_t value = read(addr);
    set_flag(StatusFlag::FLAG_CARRY, (value & 0x80) != 0);
    value <<= 1;
    write(addr, value);
    
    // ORA logic
    A |= value;
    update_zero_negative(A);
}

void CPU::RLA(uint16_t addr) {
    // ROL + AND: Rotate left then AND with A
    uint8_t value = read(addr);
    bool old_carry = get_flag(StatusFlag::FLAG_CARRY);
    set_flag(StatusFlag::FLAG_CARRY, (value & 0x80) != 0);
    value = (value << 1) | (old_carry ? 1 : 0);
    write(addr, value);
    
    // AND logic
    A &= value;
    update_zero_negative(A);
}

void CPU::SRE(uint16_t addr) {
    // LSR + EOR: Shift right then XOR with A
    uint8_t value = read(addr);
    set_flag(StatusFlag::FLAG_CARRY, (value & 0x01) != 0);
    value >>= 1;
    write(addr, value);
    
    // EOR logic
    A ^= value;
    update_zero_negative(A);
}

void CPU::RRA(uint16_t addr) {
    // ROR + ADC: Rotate right then add to A
    uint8_t value = read(addr);
    bool old_carry = get_flag(StatusFlag::FLAG_CARRY);
    set_flag(StatusFlag::FLAG_CARRY, (value & 0x01) != 0);
    value = (value >> 1) | (old_carry ? 0x80 : 0);
    write(addr, value);
    
    // ADC logic
    uint16_t result = A + value + (get_flag(StatusFlag::FLAG_CARRY) ? 1 : 0);
    set_flag(StatusFlag::FLAG_CARRY, result > 0xFF);
    set_flag(StatusFlag::FLAG_OVERFLOW, (~(A ^ value) & (A ^ result) & 0x80) != 0);
    A = result & 0xFF;
    update_zero_negative(A);
}

// Execute implementation trong opcodes.cpp

} // namespace nes
