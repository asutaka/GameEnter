#ifndef NES_MAPPER_H
#define NES_MAPPER_H

#include <cstdint>

namespace nes {

/**
 * @brief Base class cho tất cả mapper
 */
class Mapper {
public:
    virtual ~Mapper() = default;
    
    virtual uint8_t read(uint16_t address) = 0;
    virtual void write(uint16_t address, uint8_t value) = 0;
    virtual void reset() = 0;
};

} // namespace nes

#endif // NES_MAPPER_H
