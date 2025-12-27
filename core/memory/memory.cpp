#include "memory/memory.h"
#include "ppu/ppu.h"
#include "apu/apu.h"
#include "input/input.h"
#include "cartridge/cartridge.h"
#include <cstring>

namespace nes {

Memory::Memory()
    : ppu_(nullptr), apu_(nullptr), input_(nullptr), cartridge_(nullptr) {
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

void Memory::connect_input(Input* input) {
    input_ = input;
}

void Memory::connect_cartridge(Cartridge* cartridge) {
    cartridge_ = cartridge;
}

void Memory::reset() {
    ram_.fill(0);
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
            if (input_) {
                return input_->read_controller1();
            }
            return 0;
        }
        
        // Controller 2
        if (address == 0x4017) {
            if (input_) {
                return input_->read_controller2();
            }
            return 0;
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
            uint16_t ppu_reg = 0x2000 + (address & 0x0007);
            // DEBUG: Log PPUMASK writes
            // if ((address & 0x0007) == 1) { // PPUMASK
            //     static int ppumask_count = 0;
            //     if (ppumask_count++ < 20 || value != 0) {
            //         printf("[MEM] Write $%04X -> PPU $%04X = $%02X (PPUMASK)\n", 
            //                address, ppu_reg, value);
            //     }
            // }
            ppu_->write_register(ppu_reg, value);
        } else {
            // printf("[MEM] WARNING: Write $%04X but PPU is NULL!\n", address);
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
            if (input_) {
                input_->write(value);
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
