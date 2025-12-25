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
    P = 0x24; // Interrupt disable = 1, Unused = 1
    
    // Load PC from reset vector ($FFFC-$FFFD)
    if (memory_) {
        PC = read16(0xFFFC);
    } else {
        PC = 0;
    }
    
    cycles_remaining = 7; // Reset mất 7 cycles
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
    if (!get_flag(INTERRUPT)) {
        push16(PC);
        push(P & ~BREAK);
        set_flag(INTERRUPT, true);
        PC = read16(0xFFFE);
        cycles_remaining = 7;
    }
}

void CPU::nmi() {
    push16(PC);
    push(P & ~BREAK);
    set_flag(INTERRUPT, true);
    PC = read16(0xFFFA);
    cycles_remaining = 7;
}

// =====================
// Helper Functions
// =====================

void CPU::set_flag(StatusFlag flag, bool value) {
    if (value) {
        P |= flag;
    } else {
        P &= ~flag;
    }
}

bool CPU::get_flag(StatusFlag flag) const {
    return (P & flag) != 0;
}

void CPU::update_zero_negative(uint8_t value) {
    set_flag(ZERO, value == 0);
    set_flag(NEGATIVE, (value & 0x80) != 0);
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
    uint16_t addr = read16(PC);
    PC += 2;
    return addr + X;
}

uint16_t CPU::addr_absolute_y() {
    uint16_t addr = read16(PC);
    PC += 2;
    return addr + Y;
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
    return read16(ptr);
}

uint16_t CPU::addr_indirect_y() {
    uint8_t ptr = read(PC++);
    uint16_t addr = read16(ptr);
    return addr + Y;
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
    push(P | BREAK | UNUSED);
}

void CPU::PLA() {
    A = pop();
    update_zero_negative(A);
}

void CPU::PLP() {
    P = pop();
    set_flag(BREAK, false);
    set_flag(UNUSED, true);
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
    set_flag(ZERO, (A & value) == 0);
    set_flag(OVERFLOW, (value & 0x40) != 0);
    set_flag(NEGATIVE, (value & 0x80) != 0);
}

// =====================
// Opcodes - Arithmetic
// =====================

void CPU::ADC(uint16_t addr) {
    uint8_t value = read(addr);
    uint16_t result = A + value + (get_flag(CARRY) ? 1 : 0);
    
    set_flag(CARRY, result > 0xFF);
    set_flag(OVERFLOW, (~(A ^ value) & (A ^ result) & 0x80) != 0);
    
    A = result & 0xFF;
    update_zero_negative(A);
}

void CPU::SBC(uint16_t addr) {
    uint8_t value = read(addr);
    uint16_t result = A - value - (get_flag(CARRY) ? 0 : 1);
    
    set_flag(CARRY, result < 0x100);
    set_flag(OVERFLOW, ((A ^ value) & (A ^ result) & 0x80) != 0);
    
    A = result & 0xFF;
    update_zero_negative(A);
}

void CPU::CMP(uint16_t addr) {
    uint8_t value = read(addr);
    uint16_t result = A - value;
    
    set_flag(CARRY, A >= value);
    update_zero_negative(result & 0xFF);
}

void CPU::CPX(uint16_t addr) {
    uint8_t value = read(addr);
    uint16_t result = X - value;
    
    set_flag(CARRY, X >= value);
    update_zero_negative(result & 0xFF);
}

void CPU::CPY(uint16_t addr) {
    uint8_t value = read(addr);
    uint16_t result = Y - value;
    
    set_flag(CARRY, Y >= value);
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
    set_flag(CARRY, (value & 0x80) != 0);
    value <<= 1;
    write(addr, value);
    update_zero_negative(value);
}

void CPU::LSR(uint16_t addr) {
    uint8_t value = read(addr);
    set_flag(CARRY, (value & 0x01) != 0);
    value >>= 1;
    write(addr, value);
    update_zero_negative(value);
}

void CPU::ROL(uint16_t addr) {
    uint8_t value = read(addr);
    bool old_carry = get_flag(CARRY);
    set_flag(CARRY, (value & 0x80) != 0);
    value = (value << 1) | (old_carry ? 1 : 0);
    write(addr, value);
    update_zero_negative(value);
}

void CPU::ROR(uint16_t addr) {
    uint8_t value = read(addr);
    bool old_carry = get_flag(CARRY);
    set_flag(CARRY, (value & 0x01) != 0);
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
    if (!get_flag(CARRY)) {
        PC = addr;
        cycles_remaining++; // Branch taken: +1 cycle
    }
}

void CPU::BCS(uint16_t addr) {
    if (get_flag(CARRY)) {
        PC = addr;
        cycles_remaining++;
    }
}

void CPU::BEQ(uint16_t addr) {
    if (get_flag(ZERO)) {
        PC = addr;
        cycles_remaining++;
    }
}

void CPU::BMI(uint16_t addr) {
    if (get_flag(NEGATIVE)) {
        PC = addr;
        cycles_remaining++;
    }
}

void CPU::BNE(uint16_t addr) {
    if (!get_flag(ZERO)) {
        PC = addr;
        cycles_remaining++;
    }
}

void CPU::BPL(uint16_t addr) {
    if (!get_flag(NEGATIVE)) {
        PC = addr;
        cycles_remaining++;
    }
}

void CPU::BVC(uint16_t addr) {
    if (!get_flag(OVERFLOW)) {
        PC = addr;
        cycles_remaining++;
    }
}

void CPU::BVS(uint16_t addr) {
    if (get_flag(OVERFLOW)) {
        PC = addr;
        cycles_remaining++;
    }
}

// =====================
// Opcodes - Flag
// =====================

void CPU::CLC() { set_flag(CARRY, false); }
void CPU::CLD() { set_flag(DECIMAL, false); }
void CPU::CLI() { set_flag(INTERRUPT, false); }
void CPU::CLV() { set_flag(OVERFLOW, false); }
void CPU::SEC() { set_flag(CARRY, true); }
void CPU::SED() { set_flag(DECIMAL, true); }
void CPU::SEI() { set_flag(INTERRUPT, true); }

// =====================
// Opcodes - System
// =====================

void CPU::BRK() {
    PC++;
    push16(PC);
    push(P | BREAK);
    set_flag(INTERRUPT, true);
    PC = read16(0xFFFE);
}

void CPU::NOP() {
    // Không làm gì
}

void CPU::RTI() {
    P = pop();
    set_flag(BREAK, false);
    set_flag(UNUSED, true);
    PC = pop16();
}

// Execute implementation trong opcodes.cpp

} // namespace nes
