#include "input/input.h"

namespace nes {

Input::Input() 
    : controller1_state_(0), controller2_state_(0),
      controller1_shifter_(0), controller2_shifter_(0),
      strobe_(false) {
}

Input::~Input() {
}

void Input::reset() {
    controller1_state_ = 0;
    controller2_state_ = 0;
    controller1_shifter_ = 0;
    controller2_shifter_ = 0;
    strobe_ = false;
}

void Input::write(uint8_t value) {
    // Strobe mechanism:
    // Writing 1 sets strobe mode (continuously reloading state)
    // Writing 0 clears strobe mode (state is latched and can be read out)
    bool prev_strobe = strobe_;
    strobe_ = (value & 0x01) != 0;
    
    if (prev_strobe && !strobe_) {
        // Transition from 1 -> 0: Latch the current state
        controller1_shifter_ = controller1_state_;
        controller2_shifter_ = controller2_state_;
    }
    
    if (strobe_) {
        // While strobe is high, shifter always reflects current state
        controller1_shifter_ = controller1_state_;
        controller2_shifter_ = controller2_state_;
    }
}

uint8_t Input::read_controller1() {
    uint8_t value = 0;
    
    if (strobe_) {
        // If strobe is high, return status of button A (bit 0)
        value = controller1_state_ & 0x01;
    } else {
        // Read next bit from shifter
        value = controller1_shifter_ & 0x01;
        // Shift right to prepare next bit
        controller1_shifter_ >>= 1;
        // Fill with 1s after 8 bits (standard NES controller behavior)
        controller1_shifter_ |= 0x80;
    }
    
    // Standard NES controllers return 0x40 or 0x00 on open bus bits (5-7)
    // Usually bit 0 is data, bits 1-4 are 0, bits 5-7 are open bus
    // For simplicity, we just return the bit 0 data.
    return value;
}

uint8_t Input::read_controller2() {
    uint8_t value = 0;
    
    if (strobe_) {
        value = controller2_state_ & 0x01;
    } else {
        value = controller2_shifter_ & 0x01;
        controller2_shifter_ >>= 1;
        controller2_shifter_ |= 0x80;
    }
    
    return value;
}

void Input::set_button_state(int button, bool pressed) {
    if (button < 0 || button > 7) return;
    
    if (pressed) {
        controller1_state_ |= (1 << button);
    } else {
        controller1_state_ &= ~(1 << button);
    }
}

} // namespace nes
