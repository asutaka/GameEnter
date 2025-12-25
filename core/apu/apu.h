#ifndef NES_APU_H
#define NES_APU_H

#include <cstdint>

namespace nes {

/**
 * @brief Audio Processing Unit - Đơn vị xử lý âm thanh NES
 */
class APU {
public:
    APU();
    ~APU();
    
    void reset();
    void step();
    
    uint8_t read_register(uint16_t address);
    void write_register(uint16_t address, uint8_t value);
    
    // TODO: Implement full APU
};

} // namespace nes

#endif // NES_APU_H
