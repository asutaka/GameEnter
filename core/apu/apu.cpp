#include "apu/apu.h"

namespace nes {

APU::APU() {}
APU::~APU() {}

void APU::reset() {}
void APU::step() {}

uint8_t APU::read_register(uint16_t address) {
    return 0;
}

void APU::write_register(uint16_t address, uint8_t value) {
}

} // namespace nes
