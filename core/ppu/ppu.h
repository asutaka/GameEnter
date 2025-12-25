#ifndef NES_PPU_H
#define NES_PPU_H

#include <cstdint>

namespace nes {

/**
 * @brief Picture Processing Unit - Đơn vị xử lý đồ họa của NES
 * 
 * PPU render màn hình 256x240 pixels @ 60Hz (NTSC)
 * Palette: 64 màu, hiển thị tối đa 25 màu đồng thời
 */
class PPU {
public:
    PPU();
    ~PPU();
    
    void reset();
    void step();  // 1 PPU cycle
    
    uint8_t read_register(uint16_t address);
    void write_register(uint16_t address, uint8_t value);
    void write_oam_dma(uint8_t index, uint8_t value);
    
    // TODO: Implement full PPU
private:
    // Registers, VRAM, OAM, etc.
};

} // namespace nes

#endif // NES_PPU_H
