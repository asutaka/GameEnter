#ifndef NES_CPU_H
#define NES_CPU_H

#include <cstdint>
#include <functional>

namespace nes {

// Forward declaration
class Memory;

/**
 * @brief Ricoh 2A03 CPU (6502 variant)
 * 
 * CPU NES sử dụng là biến thể của 6502 với clock 1.789773 MHz (NTSC)
 * hoặc 1.662607 MHz (PAL). Không có chế độ decimal trong NES CPU.
 */
class CPU {
public:
    CPU();
    ~CPU();
    
    /**
     * @brief Kết nối CPU với bus bộ nhớ
     */
    void connect_memory(Memory* memory);
    
    /**
     * @brief Reset CPU về trạng thái ban đầu
     */
    void reset();
    
    /**
     * @brief Thực thi một chu kỳ clock
     * @return Số cycles đã sử dụng
     */
    int step();
    
    /**
     * @brief Kích hoạt IRQ (Interrupt ReQuest)
     */
    void irq();
    
    /**
     * @brief Kích hoạt NMI (Non-Maskable Interrupt)
     */
    void nmi();
    
    // Registers (8-bit)
    uint8_t A;   // Accumulator
    uint8_t X;   // Index Register X
    uint8_t Y;   // Index Register Y
    uint8_t SP;  // Stack Pointer
    uint8_t P;   // Processor Status
    
    // Program Counter (16-bit)
    uint16_t PC;
    
    // Tổng số cycles đã thực thi
    uint64_t total_cycles;
    
    // Cycles còn lại của lệnh hiện tại
    int cycles_remaining;
    
    // Flag to track page boundary crossing
    bool page_crossed_;

    // Friend declarations for opcode table
    friend struct OpcodeInfo;
    friend void initialize_opcode_table();

    // Addressing modes - Made public for opcode table access
    uint16_t addr_implied();
    uint16_t addr_accumulator();
    uint16_t addr_immediate();
    uint16_t addr_zero_page();
    uint16_t addr_zero_page_x();
    uint16_t addr_zero_page_y();
    uint16_t addr_absolute();
    uint16_t addr_absolute_x();
    uint16_t addr_absolute_y();
    uint16_t addr_indirect();
    uint16_t addr_indirect_x();
    uint16_t addr_indirect_y();
    uint16_t addr_relative();

    // Execute opcode - Made public for opcode table access
    void execute(uint8_t opcode);
    
    // Exec wrappers for opcode table - Made public for opcode table access
    void exec_LDA(uint16_t addr);
    void exec_LDX(uint16_t addr);
    void exec_LDY(uint16_t addr);
    void exec_STA(uint16_t addr);
    void exec_STX(uint16_t addr);
    void exec_STY(uint16_t addr);
    void exec_AND(uint16_t addr);
    void exec_EOR(uint16_t addr);
    void exec_ORA(uint16_t addr);
    void exec_BIT(uint16_t addr);
    void exec_ADC(uint16_t addr);
    void exec_SBC(uint16_t addr);
    void exec_CMP(uint16_t addr);
    void exec_CPX(uint16_t addr);
    void exec_CPY(uint16_t addr);
    void exec_INC(uint16_t addr);
    void exec_DEC(uint16_t addr);
    void exec_ASL(uint16_t addr);
    void exec_LSR(uint16_t addr);
    void exec_ROL(uint16_t addr);
    void exec_ROR(uint16_t addr);
    void exec_JMP(uint16_t addr);
    void exec_JSR(uint16_t addr);
    void exec_BCC(uint16_t addr);
    void exec_BCS(uint16_t addr);
    void exec_BEQ(uint16_t addr);
    void exec_BMI(uint16_t addr);
    void exec_BNE(uint16_t addr);
    void exec_BPL(uint16_t addr);
    void exec_BVC(uint16_t addr);
    void exec_BVS(uint16_t addr);
    void exec_TAX(uint16_t = 0);
    void exec_TAY(uint16_t = 0);
    void exec_TXA(uint16_t = 0);
    void exec_TYA(uint16_t = 0);
    void exec_TSX(uint16_t = 0);
    void exec_TXS(uint16_t = 0);
    void exec_PHA(uint16_t = 0);
    void exec_PHP(uint16_t = 0);
    void exec_PLA(uint16_t = 0);
    void exec_PLP(uint16_t = 0);
    void exec_INX(uint16_t = 0);
    void exec_INY(uint16_t = 0);
    void exec_DEX(uint16_t = 0);
    void exec_DEY(uint16_t = 0);
    void exec_CLC(uint16_t = 0);
    void exec_CLD(uint16_t = 0);
    void exec_CLI(uint16_t = 0);
    void exec_CLV(uint16_t = 0);
    void exec_SEC(uint16_t = 0);
    void exec_SED(uint16_t = 0);
    void exec_SEI(uint16_t = 0);
    void exec_BRK(uint16_t = 0);
    void exec_NOP(uint16_t = 0);
    void exec_RTI(uint16_t = 0);
    void exec_RTS(uint16_t = 0);
    void exec_ASL_A(uint16_t = 0);
    void exec_LSR_A(uint16_t = 0);
    void exec_ROL_A(uint16_t = 0);
    void exec_ROR_A(uint16_t = 0);
    
    // Exec wrappers for illegal opcodes
    void exec_LAX(uint16_t addr);
    void exec_SAX(uint16_t addr);
    void exec_DCP(uint16_t addr);
    void exec_ISC(uint16_t addr);
    void exec_SLO(uint16_t addr);
    void exec_RLA(uint16_t addr);
    void exec_SRE(uint16_t addr);
    void exec_RRA(uint16_t addr);

private:
    Memory* memory_;
    
    // Processor Status Flags
    enum class StatusFlag : uint8_t {
        FLAG_CARRY     = 0x01,  // Bit 0: Carry
        FLAG_ZERO      = 0x02,  // Bit 1: Zero
        FLAG_INTERRUPT = 0x04,  // Bit 2: Interrupt Disable
        FLAG_DECIMAL   = 0x08,  // Bit 3: Decimal Mode (không dùng trong NES)
        FLAG_BREAK     = 0x10,  // Bit 4: Break Command
        FLAG_UNUSED    = 0x20,  // Bit 5: Unused (luôn = 1)
        FLAG_OVERFLOW  = 0x40,  // Bit 6: Overflow
        FLAG_NEGATIVE  = 0x80   // Bit 7: Negative
    };
    
    // Helper functions cho flags
    void set_flag(StatusFlag flag, bool value);
    bool get_flag(StatusFlag flag) const;
    void update_zero_negative(uint8_t value);
    
    // Memory access
    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);
    uint16_t read16(uint16_t address);
    
    // Stack operations
    void push(uint8_t value);
    uint8_t pop();
    void push16(uint16_t value);
    uint16_t pop16();
    
    // Helper for reading 16-bit values from zero page with wraparound
    uint16_t read16_zp(uint8_t address);

private:
    
    // Opcodes - Load/Store
    void LDA(uint16_t addr); // Load Accumulator
    void LDX(uint16_t addr); // Load X
    void LDY(uint16_t addr); // Load Y
    void STA(uint16_t addr); // Store Accumulator
    void STX(uint16_t addr); // Store X
    void STY(uint16_t addr); // Store Y
    
    // Opcodes - Transfer
    void TAX(); // Transfer A to X
    void TAY(); // Transfer A to Y
    void TXA(); // Transfer X to A
    void TYA(); // Transfer Y to A
    void TSX(); // Transfer SP to X
    void TXS(); // Transfer X to SP
    
    // Opcodes - Stack
    void PHA(); // Push Accumulator
    void PHP(); // Push Processor Status
    void PLA(); // Pull Accumulator
    void PLP(); // Pull Processor Status
    
    // Opcodes - Logical
    void AND(uint16_t addr); // AND
    void EOR(uint16_t addr); // Exclusive OR
    void ORA(uint16_t addr); // OR
    void BIT(uint16_t addr); // Bit Test
    
    // Opcodes - Arithmetic
    void ADC(uint16_t addr); // Add with Carry
    void SBC(uint16_t addr); // Subtract with Carry
    void CMP(uint16_t addr); // Compare Accumulator
    void CPX(uint16_t addr); // Compare X
    void CPY(uint16_t addr); // Compare Y
    
    // Opcodes - Increment/Decrement
    void INC(uint16_t addr); // Increment Memory
    void INX(); // Increment X
    void INY(); // Increment Y
    void DEC(uint16_t addr); // Decrement Memory
    void DEX(); // Decrement X
    void DEY(); // Decrement Y
    
    // Opcodes - Shift
    void ASL(uint16_t addr); // Arithmetic Shift Left
    void LSR(uint16_t addr); // Logical Shift Right
    void ROL(uint16_t addr); // Rotate Left
    void ROR(uint16_t addr); // Rotate Right
    
    // Opcodes - Jump/Call
    void JMP(uint16_t addr); // Jump
    void JSR(uint16_t addr); // Jump to Subroutine
    void RTS(); // Return from Subroutine
    
    // Opcodes - Branch
    void BCC(uint16_t addr); // Branch if Carry Clear
    void BCS(uint16_t addr); // Branch if Carry Set
    void BEQ(uint16_t addr); // Branch if Equal (Zero Set)
    void BMI(uint16_t addr); // Branch if Minus (Negative Set)
    void BNE(uint16_t addr); // Branch if Not Equal (Zero Clear)
    void BPL(uint16_t addr); // Branch if Plus (Negative Clear)
    void BVC(uint16_t addr); // Branch if Overflow Clear
    void BVS(uint16_t addr); // Branch if Overflow Set
    
    // Opcodes - Flag
    void CLC(); // Clear Carry
    void CLD(); // Clear Decimal
    void CLI(); // Clear Interrupt
    void CLV(); // Clear Overflow
    void SEC(); // Set Carry
    void SED(); // Set Decimal
    void SEI(); // Set Interrupt
    
    // Opcodes - System
    void BRK(); // Break
    void NOP(); // No Operation
    void RTI(); // Return from Interrupt
    
    // Illegal Opcodes (used by some games and nestest)
    void LAX(uint16_t addr); // LDA + LDX
    void SAX(uint16_t addr); // Store A AND X
    void DCP(uint16_t addr); // DEC + CMP
    void ISC(uint16_t addr); // INC + SBC
    void SLO(uint16_t addr); // ASL + ORA
    void RLA(uint16_t addr); // ROL + AND
    void SRE(uint16_t addr); // LSR + EOR
    void RRA(uint16_t addr); // ROR + ADC
    
};

} // namespace nes

#endif // NES_CPU_H
