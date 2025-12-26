#include "mappers/mapper3.h"

namespace nes {

Mapper3::Mapper3(uint8_t* prg_rom, size_t prg_size,
                 uint8_t* chr_rom, size_t chr_size)
    : prg_rom_(prg_rom), chr_rom_(chr_rom),
      prg_size_(prg_size), chr_size_(chr_size),
      chr_bank_(0) {
}

void Mapper3::reset() {
    chr_bank_ = 0;
}

uint8_t Mapper3::read(uint16_t address) {
    if (address < 0x2000) {
        // CHR ROM $0000-$1FFF: Switchable 8KB bank
        uint32_t offset = (chr_bank_ * 0x2000) + address;
        if (offset < chr_size_) {
            return chr_rom_[offset];
        }
    }
    else if (address >= 0x8000) {
        // PRG ROM $8000-$FFFF: Fixed
        uint32_t offset = address - 0x8000;
        
        if (prg_size_ == 0x4000) {
            // 16KB: Mirror
            offset &= 0x3FFF;
        }
        
        if (offset < prg_size_) {
            return prg_rom_[offset];
        }
    }
    
    return 0;
}

void Mapper3::write(uint16_t address, uint8_t value) {
    if (address >= 0x8000) {
        // CHR bank select (any write to $8000-$FFFF)
        chr_bank_ = value & 0x03;  // Up to 4 banks (32KB CHR)
    }
}

} // namespace nes
