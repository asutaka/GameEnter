#include "mappers/mapper7.h"
#include <cstring>

namespace nes {

Mapper7::Mapper7(uint8_t* prg_rom, size_t prg_size,
                 uint8_t* chr_rom, size_t chr_size)
    : prg_rom_(prg_rom), chr_rom_(chr_rom),
      prg_size_(prg_size), chr_size_(chr_size),
      prg_bank_(0), mirror_mode_(MirrorMode::SINGLE_SCREEN) {
    
    std::memset(chr_ram_, 0, sizeof(chr_ram_));
}

void Mapper7::reset() {
    prg_bank_ = 0;
    mirror_mode_ = MirrorMode::SINGLE_SCREEN;
}

uint8_t Mapper7::read(uint16_t address) {
    if (address < 0x2000) {
        // CHR RAM $0000-$1FFF
        return chr_ram_[address];
    }
    else if (address >= 0x8000) {
        // PRG ROM $8000-$FFFF: Switchable 32KB bank
        uint32_t offset = (prg_bank_ * 0x8000) + (address & 0x7FFF);
        if (offset < prg_size_) {
            return prg_rom_[offset];
        }
    }
    
    return 0;
}

void Mapper7::write(uint16_t address, uint8_t value) {
    if (address < 0x2000) {
        // CHR RAM $0000-$1FFF: Writable
        chr_ram_[address] = value;
    }
    else if (address >= 0x8000) {
        // Bank select + mirroring (any write to $8000-$FFFF)
        prg_bank_ = value & 0x07;  // Bits 0-2: PRG bank
        
        // Bit 4: One-screen mirroring select
        // 0 = lower nametable, 1 = upper nametable
        // For simplicity, both use SINGLE_SCREEN mode
        mirror_mode_ = MirrorMode::SINGLE_SCREEN;
    }
}

} // namespace nes
