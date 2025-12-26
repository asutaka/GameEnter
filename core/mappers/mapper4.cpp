#include "mappers/mapper4.h"
#include <cstring>

namespace nes {

Mapper4::Mapper4(uint8_t* prg_rom, size_t prg_size,
                 uint8_t* chr_rom, size_t chr_size)
    : prg_rom_(prg_rom), chr_rom_(chr_rom),
      prg_size_(prg_size), chr_size_(chr_size),
      bank_select_(0), prg_mode_(false), chr_a12_inversion_(false),
      prg_ram_enabled_(true), prg_ram_write_protect_(false),
      irq_latch_(0), irq_enabled_(false),
      irq_counter_(0), irq_reload_(false), irq_flag_(false),
      mirror_mode_(MirrorMode::HORIZONTAL) {
    
    std::memset(bank_registers_, 0, sizeof(bank_registers_));
    std::memset(prg_ram_, 0, sizeof(prg_ram_));
    
    reset();
}

void Mapper4::reset() {
    bank_select_ = 0;
    prg_mode_ = false;
    chr_a12_inversion_ = false;
    
    std::memset(bank_registers_, 0, sizeof(bank_registers_));
    
    prg_ram_enabled_ = true;
    prg_ram_write_protect_ = false;
    
    irq_latch_ = 0;
    irq_enabled_ = false;
    irq_counter_ = 0;
    irq_reload_ = false;
    irq_flag_ = false;
    
    mirror_mode_ = MirrorMode::HORIZONTAL;
}

uint8_t Mapper4::read(uint16_t address) {
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

void Mapper4::write(uint16_t address, uint8_t value) {
    if (address < 0x2000) {
        // CHR RAM write (if CHR RAM)
        // TODO: CHR RAM support
    }
    else if (address >= 0x6000 && address < 0x8000) {
        // PRG RAM $6000-$7FFF
        if (prg_ram_enabled_ && !prg_ram_write_protect_) {
            prg_ram_[address & 0x1FFF] = value;
        }
    }
    else if (address >= 0x8000) {
        // MMC3 registers
        if (address < 0xA000) {
            if (address & 0x01) {
                // $8001-$9FFF odd: Bank data
                bank_registers_[bank_select_ & 0x07] = value;
            }
            else {
                // $8000-$9FFE even: Bank select
                bank_select_ = value & 0x07;
                prg_mode_ = (value & 0x40) != 0;
                chr_a12_inversion_ = (value & 0x80) != 0;
            }
        }
        else if (address < 0xC000) {
            if (address & 0x01) {
                // $A001-$BFFF odd: PRG RAM protect
                prg_ram_write_protect_ = (value & 0x40) != 0;
                prg_ram_enabled_ = (value & 0x80) != 0;
            }
            else {
                // $A000-$BFFE even: Mirroring
                mirror_mode_ = (value & 0x01) ? MirrorMode::HORIZONTAL : MirrorMode::VERTICAL;
            }
        }
        else if (address < 0xE000) {
            if (address & 0x01) {
                // $C001-$DFFF odd: IRQ reload
                irq_reload_ = true;
            }
            else {
                // $C000-$DFFE even: IRQ latch
                irq_latch_ = value;
            }
        }
        else {
            if (address & 0x01) {
                // $E001-$FFFF odd: IRQ enable
                irq_enabled_ = true;
            }
            else {
                // $E000-$FFFE even: IRQ disable
                irq_enabled_ = false;
                irq_flag_ = false;
            }
        }
    }
}

void Mapper4::notify_scanline() {
    // MMC3 scanline counter
    if (irq_counter_ == 0 || irq_reload_) {
        irq_counter_ = irq_latch_;
        irq_reload_ = false;
    }
    else {
        irq_counter_--;
    }
    
    if (irq_counter_ == 0 && irq_enabled_) {
        irq_flag_ = true;
    }
}

uint32_t Mapper4::get_prg_bank_offset(uint16_t address) {
    uint32_t bank_number = 0;
    
    if (address < 0xA000) {
        // $8000-$9FFF (8KB)
        if (prg_mode_) {
            // Mode 1: Fixed to second-last bank
            bank_number = (prg_size_ / 0x2000) - 2;
        }
        else {
            // Mode 0: Switchable via R6
            bank_number = bank_registers_[6];
        }
    }
    else if (address < 0xC000) {
        // $A000-$BFFF (8KB) - Always R7
        bank_number = bank_registers_[7];
    }
    else if (address < 0xE000) {
        // $C000-$DFFF (8KB)
        if (prg_mode_) {
            // Mode 1: Switchable via R6
            bank_number = bank_registers_[6];
        }
        else {
            // Mode 0: Fixed to second-last bank
            bank_number = (prg_size_ / 0x2000) - 2;
        }
    }
    else {
        // $E000-$FFFF (8KB) - Fixed to last bank
        bank_number = (prg_size_ / 0x2000) - 1;
    }
    
    return (bank_number * 0x2000) + (address & 0x1FFF);
}

uint32_t Mapper4::get_chr_bank_offset(uint16_t address) {
    uint32_t bank_number = 0;
    
    // Apply CHR A12 inversion
    uint16_t effective_addr = address;
    if (chr_a12_inversion_) {
        effective_addr ^= 0x1000;  // Swap $0000-$0FFF with $1000-$1FFF
    }
    
    if (effective_addr < 0x0800) {
        // $0000-$07FF (2KB) - Use R0 (ignore bit 0)
        bank_number = bank_registers_[0] & 0xFE;
        return (bank_number * 0x0400) + (effective_addr & 0x07FF);
    }
    else if (effective_addr < 0x1000) {
        // $0800-$0FFF (2KB) - Use R1 (ignore bit 0)
        bank_number = bank_registers_[1] & 0xFE;
        return (bank_number * 0x0400) + (effective_addr & 0x07FF);
    }
    else if (effective_addr < 0x1400) {
        // $1000-$13FF (1KB) - Use R2
        bank_number = bank_registers_[2];
        return (bank_number * 0x0400) + (effective_addr & 0x03FF);
    }
    else if (effective_addr < 0x1800) {
        // $1400-$17FF (1KB) - Use R3
        bank_number = bank_registers_[3];
        return (bank_number * 0x0400) + (effective_addr & 0x03FF);
    }
    else if (effective_addr < 0x1C00) {
        // $1800-$1BFF (1KB) - Use R4
        bank_number = bank_registers_[4];
        return (bank_number * 0x0400) + (effective_addr & 0x03FF);
    }
    else {
        // $1C00-$1FFF (1KB) - Use R5
        bank_number = bank_registers_[5];
        return (bank_number * 0x0400) + (effective_addr & 0x03FF);
    }
}

} // namespace nes
