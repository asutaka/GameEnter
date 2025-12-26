#ifndef NES_MAPPER3_H
#define NES_MAPPER3_H

#include "mappers/mapper.h"
#include <cstdint>

namespace nes {

/**
 * @brief Mapper 3 (CNROM) - Simple CHR banking only
 * 
 * Games: Solomon's Key, Arkanoid, Paperboy, Gradius, etc.
 * 
 * Features:
 * - Fixed 16KB or 32KB PRG ROM (no banking)
 * - Switchable 8KB CHR bank
 */
class Mapper3 : public Mapper {
public:
    Mapper3(uint8_t* prg_rom, size_t prg_size,
            uint8_t* chr_rom, size_t chr_size);
    ~Mapper3() override = default;
    
    uint8_t read(uint16_t address) override;
    void write(uint16_t address, uint8_t value) override;
    void reset() override;

private:
    uint8_t* prg_rom_;
    uint8_t* chr_rom_;
    size_t prg_size_;
    size_t chr_size_;
    
    uint8_t chr_bank_;  // Selected 8KB CHR bank
};

} // namespace nes

#endif // NES_MAPPER3_H
