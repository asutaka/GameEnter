#include "mappers/mapper0.h"
#include <cstring>

namespace nes {

Mapper0::Mapper0(uint8_t* prg_rom, size_t prg_size,
                 uint8_t* chr_rom, size_t chr_size)
    : prg_rom_(prg_rom), chr_rom_(chr_rom),
      prg_size_(prg_size), chr_size_(chr_size) {
    
    // Allocate and initialize PRG RAM (8KB)
    prg_ram_ = new uint8_t[0x2000];
    std::memset(prg_ram_, 0, 0x2000);
}

Mapper0::~Mapper0() {
    delete[] prg_ram_;
}

uint8_t Mapper0::read(uint16_t address) {
    if (address < 0x2000) {
        // CHR ROM $0000-$1FFF (8KB)
        if (chr_size_ > 0) {
            return chr_rom_[address % chr_size_];
        }
        // CHR RAM case (some games use RAM instead of ROM)
        return 0;
    }
    else if (address >= 0x6000 && address < 0x8000) {
        // PRG RAM $6000-$7FFF (8KB)
        return prg_ram_[address & 0x1FFF];
    }
    else if (address >= 0x8000) {
        // PRG ROM $8000-$FFFF
        uint32_t index = address - 0x8000;
        
        if (prg_size_ == 0x4000) {
            // 16KB PRG ROM: Mirror to both $8000-$BFFF and $C000-$FFFF
            index &= 0x3FFF;  // 16KB mask
        }
        else {
            // 32KB PRG ROM: Direct mapping
            index &= 0x7FFF;  // 32KB mask
        }
        
        if (index < prg_size_) {
            return prg_rom_[index];
        }
    }
    
    return 0;
}

void Mapper0::write(uint16_t address, uint8_t value) {
    if (address < 0x2000) {
        // CHR ROM: Usually read-only
        // Some games use CHR RAM (8KB) instead
        // TODO: CHR RAM support if needed
    }
    else if (address >= 0x6000 && address < 0x8000) {
        // PRG RAM $6000-$7FFF (8KB)
        prg_ram_[address & 0x1FFF] = value;
    }
    // PRG ROM writes are ignored (read-only)
}

void Mapper0::reset() {
    // Mapper 0 has no internal state to reset
    // PRG RAM is persistent (for save games)
}

} // namespace nes
