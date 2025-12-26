#include "mappers/mapper1.h"
#include <cstring>

namespace nes {

Mapper1::Mapper1(uint8_t* prg_rom, size_t prg_size,
                 uint8_t* chr_rom, size_t chr_size)
    : prg_rom_(prg_rom), chr_rom_(chr_rom),
      prg_size_(prg_size), chr_size_(chr_size),
      shift_register_(0), shift_count_(0),
      chr_bank_0_(0), chr_bank_1_(0), prg_bank_(0),
      prg_ram_enabled_(true),
      mirror_mode_(MirrorMode::HORIZONTAL) {
    
    std::memset(&control_, 0, sizeof(control_));
    std::memset(prg_ram_, 0, sizeof(prg_ram_));
    
    reset();
}

void Mapper1::reset() {
    shift_register_ = 0x10;  // Bit 4 set
    shift_count_ = 0;
    
    // Reset control register to default
    control_.mirroring = 3;  // Horizontal
    control_.prg_mode = 3;   // Fix last bank
    control_.chr_mode = 0;   // 8KB mode
    
    chr_bank_0_ = 0;
    chr_bank_1_ = 0;
    prg_bank_ = 0;
    
    prg_ram_enabled_ = true;
    update_mirroring();
}

uint8_t Mapper1::read(uint16_t address) {
    if (address < 0x2000) {
        // CHR ROM $0000-$1FFF
        uint32_t offset = get_chr_bank_offset(address);
        if (offset < chr_size_) {
            return chr_rom_[offset];
        }
        return 0;
    }
    else if (address >= 0x6000 && address < 0x8000) {
        // PRG RAM $6000-$7FFF
        if (prg_ram_enabled_) {
            return prg_ram_[address & 0x1FFF];
        }
        return 0;
    }
    else if (address >= 0x8000) {
        // PRG ROM $8000-$FFFF
        uint32_t offset = get_prg_bank_offset(address);
        if (offset < prg_size_) {
            return prg_rom_[offset];
        }
        return 0;
    }
    
    return 0;
}

void Mapper1::write(uint16_t address, uint8_t value) {
    if (address < 0x2000) {
        // CHR RAM write (if CHR RAM instead of CHR ROM)
        if (chr_size_ == 0) {
            // TODO: CHR RAM support
        }
    }
    else if (address >= 0x6000 && address < 0x8000) {
        // PRG RAM $6000-$7FFF
        if (prg_ram_enabled_) {
            prg_ram_[address & 0x1FFF] = value;
        }
    }
    else if (address >= 0x8000) {
        // MMC1 register write
        
        if (value & 0x80) {
            // Bit 7 set: reset shift register
            shift_register_ = 0x10;
            shift_count_ = 0;
            control_.prg_mode = 3;  // Fix last bank
        }
        else {
            // Write bit 0 to shift register
            shift_register_ >>= 1;
            shift_register_ |= (value & 0x01) << 4;
            shift_count_++;
            
            if (shift_count_ == 5) {
                // 5 writes complete: update target register
                uint8_t reg_value = shift_register_ & 0x1F;
                
                if (address < 0xA000) {
                    // $8000-$9FFF: Control
                    write_control(reg_value);
                }
                else if (address < 0xC000) {
                    // $A000-$BFFF: CHR bank 0
                    chr_bank_0_ = reg_value;
                }
                else if (address < 0xE000) {
                    // $C000-$DFFF: CHR bank 1
                    chr_bank_1_ = reg_value;
                }
                else {
                    // $E000-$FFFF: PRG bank
                    prg_bank_ = reg_value & 0x0F;
                    prg_ram_enabled_ = !(reg_value & 0x10);
                }
                
                // Reset shift register
                shift_register_ = 0x10;
                shift_count_ = 0;
            }
        }
    }
}

void Mapper1::write_control(uint8_t value) {
    control_.mirroring = value & 0x03;
    control_.prg_mode = (value >> 2) & 0x03;
    control_.chr_mode = (value >> 4) & 0x01;
    
    update_mirroring();
}

void Mapper1::update_mirroring() {
    switch (control_.mirroring) {
        case 0: mirror_mode_ = MirrorMode::SINGLE_SCREEN; break;  // One-screen lower
        case 1: mirror_mode_ = MirrorMode::SINGLE_SCREEN; break;  // One-screen upper
        case 2: mirror_mode_ = MirrorMode::VERTICAL; break;
        case 3: mirror_mode_ = MirrorMode::HORIZONTAL; break;
    }
}

uint32_t Mapper1::get_prg_bank_offset(uint16_t address) {
    uint32_t bank_number = 0;
    uint32_t offset_in_bank = address & 0x3FFF;  // 16KB bank size
    
    if (address < 0xC000) {
        // $8000-$BFFF (first 16KB)
        switch (control_.prg_mode) {
            case 0:
            case 1:
                // 32KB mode: use prg_bank bits 0-3, ignore bit 0
                bank_number = (prg_bank_ & 0x0E) >> 1;
                offset_in_bank = address & 0x7FFF;  // 32KB
                break;
            case 2:
                // Fix first bank at $8000
                bank_number = 0;
                break;
            case 3:
                // Switch 16KB bank at $8000
                bank_number = prg_bank_;
                break;
        }
    }
    else {
        // $C000-$FFFF (second 16KB)
        switch (control_.prg_mode) {
            case 0:
            case 1:
                // 32KB mode: handled above
                bank_number = (prg_bank_ & 0x0E) >> 1;
                offset_in_bank = (address & 0x7FFF);
                break;
            case 2:
                // Switch 16KB bank at $C000
                bank_number = prg_bank_;
                offset_in_bank = address & 0x3FFF;
                break;
            case 3:
                // Fix last bank at $C000
                bank_number = (prg_size_ / 0x4000) - 1;  // Last bank
                offset_in_bank = address & 0x3FFF;
                break;
        }
    }
    
    return bank_number * 0x4000 + offset_in_bank;
}

uint32_t Mapper1::get_chr_bank_offset(uint16_t address) {
    uint32_t bank_number = 0;
    uint32_t offset_in_bank = address & 0x0FFF;  // 4KB bank size
    
    if (control_.chr_mode == 0) {
        // 8KB mode: use chr_bank_0, ignore bit 0
        bank_number = (chr_bank_0_ & 0x1E) >> 1;
        offset_in_bank = address & 0x1FFF;  // 8KB
    }
    else {
        // 4KB mode
        if (address < 0x1000) {
            // $0000-$0FFF: use chr_bank_0
            bank_number = chr_bank_0_;
        }
        else {
            // $1000-$1FFF: use chr_bank_1
            bank_number = chr_bank_1_;
        }
    }
    
    return bank_number * 0x1000 + offset_in_bank;
}

} // namespace nes
