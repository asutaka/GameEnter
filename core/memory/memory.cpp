#include "memory/memory.h"
#include "ppu/ppu.h"
#include "apu/apu.h"
#include "cartridge/cartridge.h"
#include <cstring>

namespace nes {

Memory::Memory()
    : ppu_(nullptr), apu_(nullptr), cartridge_(nullptr),
      controller1_(0), controller2_(0),
      controller1_snapshot_(0), controller2_snapshot_(0) {
    ram_.fill(0);
}

Memory::~Memory() {
}

void Memory::connect_ppu(PPU* ppu) {
    ppu_ = ppu;
}

void Memory::connect_apu(APU* apu) {
    apu_ = apu;
}

void Memory::connect_cartridge(Cartridge* cartridge) {
    cartridge_ = cartridge;
}

void Memory::reset() {
    ram_.fill(0);
    controller1_ = 0;
    controller2_ = 0;
    controller1_snapshot_ = 0;
    controller2_snapshot_ = 0;
}

uint8_t Memory::read(uint16_t address) {
    // Internal RAM ($0000-$1FFF) - 2KB với 3 mirrors
    if (address < 0x2000) {
        return ram_[address & 0x07FF];
    }
    
    // PPU Registers ($2000-$3FFF) - 8 bytes với mirrors
    if (address < 0x4000) {
        if (ppu_) {
            return ppu_->read_register(0x2000 + (address & 0x0007));
        }
        return 0;
    }
    
    // APU and I/O ($4000-$4017)
    if (address < 0x4018) {
        // Controller 1
        if (address == 0x4016) {
            uint8_t value = (controller1_snapshot_ & 0x80) ? 1 : 0;
            controller1_snapshot_ <<= 1;
            return value;
        }
        
        // Controller 2
        if (address == 0x4017) {
            uint8_t value = (controller2_snapshot_ & 0x80) ? 1 : 0;
            controller2_snapshot_ <<= 1;
            return value;
        }
        
        // APU
        if (apu_) {
            return apu_->read_register(address);
        }
        
        return 0;
    }
    
    // Cartridge space ($4020-$FFFF)
    if (cartridge_) {
        return cartridge_->read(address);
    }
    
    return 0;
}

void Memory::write(uint16_t address, uint8_t value) {
    // Internal RAM ($0000-$1FFF)
    if (address < 0x2000) {
        ram_[address & 0x07FF] = value;
        return;
    }
    
    // PPU Registers ($2000-$3FFF)
    if (address < 0x4000) {
        if (ppu_) {
            ppu_->write_register(0x2000 + (address & 0x0007), value);
        }
        return;
    }
    
    // APU and I/O ($4000-$4017)
    if (address < 0x4018) {
        // OAM DMA ($4014)
        if (address == 0x4014) {
            if (ppu_) {
                // DMA transfer từ CPU RAM sang PPU OAM
                uint16_t dma_addr = value << 8;
                for (int i = 0; i < 256; i++) {
                    ppu_->write_oam_dma(i, read(dma_addr + i));
                }
            }
            return;
        }
        
        // Controller strobe ($4016)
        if (address == 0x4016) {
            if (value & 0x01) {
                // Snapshot controller state
                controller1_snapshot_ = controller1_;
                controller2_snapshot_ = controller2_;
            }
            return;
        }
        
        // APU
        if (apu_) {
            apu_->write_register(address, value);
        }
        
        return;
    }
    
    // Cartridge space ($4020-$FFFF)
    if (cartridge_) {
        cartridge_->write(address, value);
    }
}

} // namespace nes
