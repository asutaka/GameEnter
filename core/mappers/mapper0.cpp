#include "mappers/mapper.h"
#include <cstdint>
#include <cstddef>

namespace nes {

/**
 * @brief Mapper 0 (NROM) - Mapper đơn giản nhất
 * 
 * Games: Donkey Kong, Super Mario Bros, Ice Climber, Excitebike, etc.
 * 
 * CPU Memory Map:
 * $6000-$7FFF: PRG RAM (8KB) - Optional
 * $8000-$BFFF: First 16KB of PRG ROM
 * $C000-$FFFF: Last 16KB của PRG ROM (hoặc mirror của first 16KB nếu chỉ có 16KB)
 * 
 * PPU Memory Map:
 * $0000-$1FFF: CHR ROM (8KB)
 */
class Mapper0 : public Mapper {
public:
    Mapper0(uint8_t* prg_rom, size_t prg_size,
            uint8_t* chr_rom, size_t chr_size)
        : prg_rom_(prg_rom), chr_rom_(chr_rom),
          prg_size_(prg_size), chr_size_(chr_size) {
        
        // PRG RAM (optional)
        prg_ram_ = new uint8_t[8192];
        for (int i = 0; i < 8192; i++) {
            prg_ram_[i] = 0;
        }
    }
    
    ~Mapper0() {
        delete[] prg_ram_;
    }
    
    uint8_t read(uint16_t address) override {
        if (address < 0x2000) {
            // CHR ROM
            return chr_rom_[address % chr_size_];
        }
        else if (address >= 0x6000 && address < 0x8000) {
            // PRG RAM
            return prg_ram_[address - 0x6000];
        }
        else if (address >= 0x8000) {
            // PRG ROM
            uint16_t index = address - 0x8000;
            
            if (prg_size_ == 16384) {
                // 16KB: Mirror vào cả $8000-$BFFF và $C000-$FFFF
                index %= 16384;
            } else {
                // 32KB: Map trực tiếp
                index %= prg_size_;
            }
            
            return prg_rom_[index];
        }
        
        return 0;
    }
    
    void write(uint16_t address, uint8_t value) override {
        if (address >= 0x6000 && address < 0x8000) {
            // PRG RAM
            prg_ram_[address - 0x6000] = value;
        }
        // CHR ROM: Read-only (không write được)
        // Một số game có CHR RAM thay vì CHR ROM, nhưng Mapper 0 thường là ROM
    }
    
    void reset() override {
        // Mapper 0 không có state cần reset
    }

private:
    uint8_t* prg_rom_;
    uint8_t* chr_rom_;
    uint8_t* prg_ram_;
    size_t prg_size_;
    size_t chr_size_;
};

} // namespace nes
