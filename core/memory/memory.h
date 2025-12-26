#ifndef NES_MEMORY_H
#define NES_MEMORY_H

#include <cstdint>
#include <array>

namespace nes {

// Forward declarations
class PPU;
class APU;
class Input;
class Cartridge;

/**
 * @brief Bộ nhớ chính của NES (CPU Memory Map)
 * 
 * NES CPU Memory Map (16KB addressable):
 * $0000-$07FF: 2KB Internal RAM
 * $0800-$1FFF: Mirrors of $0000-$07FF (3 lần)
 * $2000-$2007: PPU Registers
 * $2008-$3FFF: Mirrors of $2000-$2007
 * $4000-$4017: APU and I/O Registers
 * $4018-$401F: APU and I/O (thường disabled)
 * $4020-$FFFF: Cartridge space (PRG ROM, PRG RAM, Mapper)
 */
class Memory {
public:
    Memory();
    ~Memory();
    
    /**
     * @brief Kết nối các component với memory bus
     */
    void connect_ppu(PPU* ppu);
    void connect_apu(APU* apu);
    void connect_input(Input* input);
    void connect_cartridge(Cartridge* cartridge);
    
    /**
     * @brief Đọc 1 byte từ địa chỉ
     */
    uint8_t read(uint16_t address);
    
    /**
     * @brief Ghi 1 byte vào địa chỉ
     */
    void write(uint16_t address, uint8_t value);
    
    /**
     * @brief Reset bộ nhớ
     */
    void reset();

private:
    // 2KB Internal RAM
    std::array<uint8_t, 0x0800> ram_;
    
    // Connected components
    PPU* ppu_;
    APU* apu_;
    Input* input_;
    Cartridge* cartridge_;
};

} // namespace nes

#endif // NES_MEMORY_H
