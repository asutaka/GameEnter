#include "mappers/mapper.h"

namespace nes {

/**
 * @brief Mapper 4 (MMC3) - Cho game Contra
 * 
 * Đây là mapper phức tạp với:
 * - PRG ROM banking
 * - CHR ROM banking
 * - Scanline counter (cho IRQ)
 * - Mirroring control
 * 
 * TODO: Implement đầy đủ sau khi CPU/PPU hoàn thành
 */
class Mapper4 : public Mapper {
public:
    uint8_t read(uint16_t address) override { return 0; }
    void write(uint16_t address, uint8_t value) override {}
    void reset() override {}
    
    // TODO: Implement scanline IRQ
};

} // namespace nes
