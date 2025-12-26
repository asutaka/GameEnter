#ifndef NES_MAPPER_H
#define NES_MAPPER_H

#include <cstdint>

namespace nes {

// Forward declare MirrorMode
enum class MirrorMode;

/**
 * @brief Base class cho tất cả mapper
 */
class Mapper {
public:
    virtual ~Mapper() = default;
    
    virtual uint8_t read(uint16_t address) = 0;
    virtual void write(uint16_t address, uint8_t value) = 0;
    virtual void reset() = 0;
    
    // Some mappers can change mirroring dynamically (like MMC1)
    virtual MirrorMode get_mirroring() const { 
        return static_cast<MirrorMode>(0);  // Default: HORIZONTAL
    }
};

} // namespace nes

#endif // NES_MAPPER_H
