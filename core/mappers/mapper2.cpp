#include "mappers/mapper2.h"
#include <cstring>

namespace nes {

Mapper2::Mapper2(uint8_t* prg_rom, size_t prg_size,
                 uint8_t* chr_rom, size_t chr_size)
    : prg_rom_(prg_rom), chr_rom_(chr_rom),
      prg_size_(prg_size), chr_size_(chr_size),
      prg_bank_(0) {
    
    std::memset(chr_ram_, 0, sizeof(chr_ram_));
}

void Mapper2::reset() {
    prg_bank_ = 0;
}

uint8_t Mapper2::read(uint16_t address) {
    if (address < 0x2000) {
        // CHR RAM $0000-$1FFF
        return chr_ram_[address];
    }
    else if (address >= 0x8000 && address < 0xC000) {
        // PRG ROM $8000-$BFFF: Switchable 16KB bank
        uint32_t offset = (prg_bank_ * 0x4000) + (address & 0x3FFF);
        if (offset < prg_size_) {
            return prg_rom_[offset];
        }
    }
    else if (address >= 0xC000) {
        // PRG ROM $C000-$FFFF: Fixed to last 16KB bank
        uint32_t last_bank = (prg_size_ / 0x4000) - 1;
        uint32_t offset = (last_bank * 0x4000) + (address & 0x3FFF);
        if (offset < prg_size_) {
            return prg_rom_[offset];
        }
    }
    
    return 0;
}

void Mapper2::write(uint16_t address, uint8_t value) {
    if (address < 0x2000) {
        // CHR RAM $0000-$1FFF: Writable
        chr_ram_[address] = value;
    }
    else if (address >= 0x8000) {
        // PRG bank select (any write to $8000-$FFFF)
        prg_bank_ = value & 0x0F;  // Up to 16 banks (256KB)
    }
}

} // namespace nes
