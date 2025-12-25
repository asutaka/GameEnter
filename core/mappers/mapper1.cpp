#include "mappers/mapper.h"

namespace nes {

// Mapper 1 (MMC1) - Placeholder
class Mapper1 : public Mapper {
public:
    uint8_t read(uint16_t address) override { return 0; }
    void write(uint16_t address, uint8_t value) override {}
    void reset() override {}
};

} // namespace nes
