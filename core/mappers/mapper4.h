#ifndef NES_MAPPER4_H
#define NES_MAPPER4_H

#include "mappers/mapper.h"
#include "cartridge/cartridge.h"
#include <cstdint>

namespace nes {

/**
 * @brief Mapper 4 (MMC3) - Nintendo's advanced mapper with IRQ support
 * 
 * Games: Contra, Mega Man 3-6, Super Mario Bros 2/3, Kirby's Adventure, etc.
 * 
 * Features:
 * - 8KB PRG ROM banking
 * - 2KB/1KB CHR ROM banking  
 * - Scanline counter IRQ
 * - Mirroring control
 * - PRG RAM protection
 */
class Mapper4 : public Mapper {
public:
    Mapper4(uint8_t* prg_rom, size_t prg_size,
            uint8_t* chr_rom, size_t chr_size);
    ~Mapper4() override = default;
    
    uint8_t read(uint16_t address) override;
    void write(uint16_t address, uint8_t value) override;
    void reset() override;
    
    MirrorMode get_mirroring() const { return mirror_mode_; }
    
    // PPU calls this on A12 rising edge (for scanline counter)
    void notify_scanline();
    bool irq_pending() const { return irq_flag_; }
    void clear_irq() { irq_flag_ = false; }

private:
    // ROM pointers
    uint8_t* prg_rom_;
    uint8_t* chr_rom_;
    size_t prg_size_;
    size_t chr_size_;
    
    // Bank select ($8000-$9FFE, even)
    uint8_t bank_select_;      // Target bank number
    bool prg_mode_;            // PRG banking mode
    bool chr_a12_inversion_;   // CHR A12 inversion
    
    // Bank data ($8001-$9FFF, odd)
    uint8_t bank_registers_[8];  // R0-R7
    
    // Mirroring ($A000-$BFFE, even)
    MirrorMode mirror_mode_;
    
    // PRG RAM protect ($A001-$BFFF, odd)
    bool prg_ram_enabled_;
    bool prg_ram_write_protect_;
    
    // IRQ latch ($C000-$DFFE, even)
    uint8_t irq_latch_;
    
    // IRQ reload ($C001-$DFFF, odd)
    // Writing here reloads counter
    
    // IRQ disable ($E000-$FFFE, even)
    bool irq_enabled_;
    
    // IRQ enable ($E001-$FFFF, odd)
    
    // IRQ state
    uint8_t irq_counter_;
    bool irq_reload_;
    bool irq_flag_;
    
    // PRG RAM
    uint8_t prg_ram_[0x2000];  // 8KB PRG RAM
    
    // Helper functions
    uint32_t get_prg_bank_offset(uint16_t address);
    uint32_t get_chr_bank_offset(uint16_t address);
};

} // namespace nes

#endif // NES_MAPPER4_H
