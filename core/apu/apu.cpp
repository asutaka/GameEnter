#include "apu/apu.h"
#include "memory/memory.h"
#include <cstring>

namespace nes {

// Length Counter Lookup Table
const uint8_t APU::LENGTH_TABLE[32] = {
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

// Noise Period Lookup Table (NTSC)
const uint16_t APU::NOISE_PERIOD_TABLE[16] = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

// Triangle Sequence
const uint8_t APU::TRIANGLE_SEQUENCE[32] = {
    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

// Duty Cycle Sequences
const uint8_t APU::DUTY_TABLE[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 1, 1, 1, 1, 1}
};

// DMC Rate Lookup Table (NTSC)
const uint16_t APU::DMC_RATE_TABLE[16] = {
    428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54
};

APU::APU() 
    : frame_counter_mode_(0), irq_inhibit_(false),
      enable_pulse1_(false), enable_pulse2_(false),
      enable_triangle_(false), enable_noise_(false), enable_dmc_(false),
      cycle_count_(0), frame_step_(0) {
      
      memory_ = nullptr;
    
    // Initialize channels
    std::memset(&pulse1_, 0, sizeof(PulseChannel));
    std::memset(&pulse2_, 0, sizeof(PulseChannel));
    std::memset(&triangle_, 0, sizeof(TriangleChannel));
    std::memset(&noise_, 0, sizeof(NoiseChannel));
    std::memset(&dmc_, 0, sizeof(DMCChannel));
    
    noise_.shift_register = 1; // Must not be 0
    dmc_.buffer_empty = true;
}

APU::~APU() {
}

void APU::connect_memory(Memory* memory) {
    memory_ = memory;
}

void APU::reset() {
    frame_counter_mode_ = 0;
    irq_inhibit_ = false;
    enable_pulse1_ = false;
    enable_pulse2_ = false;
    enable_triangle_ = false;
    enable_noise_ = false;
    enable_dmc_ = false;
    cycle_count_ = 0;
    frame_step_ = 0;
    
    std::memset(&pulse1_, 0, sizeof(PulseChannel));
    std::memset(&pulse2_, 0, sizeof(PulseChannel));
    std::memset(&triangle_, 0, sizeof(TriangleChannel));
    std::memset(&noise_, 0, sizeof(NoiseChannel));
    std::memset(&dmc_, 0, sizeof(DMCChannel));
    
    noise_.shift_register = 1;
    dmc_.buffer_empty = true;
}

void APU::step() {
    cycle_count_++;
    
    // Pulse channels clock every 2 CPU cycles
    if (cycle_count_ % 2 == 0) {
        pulse1_.step_timer();
        pulse2_.step_timer();
        noise_.step_timer();
        dmc_.step_timer(memory_);
    }
    
    // Triangle clocks every CPU cycle
    triangle_.step_timer();
    
    // Noise timer is clocked every CPU cycle, but decrements based on period
    noise_.step_timer();
    
    // Frame Counter Step (approx 7457 cycles)
    if (cycle_count_ % 7457 == 0) {
        step_frame_counter();
    }
}

void APU::step_frame_counter() {
    frame_step_++;
    
    // Envelope & Linear Counter (Every step)
    pulse1_.step_envelope();
    pulse2_.step_envelope();
    triangle_.step_linear();
    noise_.step_envelope();
    
    // Length & Sweep (Every 2 steps: 2, 4)
    if (frame_step_ == 2 || frame_step_ == 4) {
        pulse1_.step_length();
        pulse1_.step_sweep(false);
        
        pulse2_.step_length();
        pulse2_.step_sweep(true);
        
        triangle_.step_length();
        noise_.step_length();
    }
    
    // Reset step
    if (frame_counter_mode_ == 0) {
        if (frame_step_ >= 4) frame_step_ = 0;
    } else {
        if (frame_step_ >= 5) frame_step_ = 0;
    }
}

uint8_t APU::read_register(uint16_t address) {
    if (address == 0x4015) {
        uint8_t value = 0;
        if (pulse1_.length_counter > 0) value |= 0x01;
        if (pulse2_.length_counter > 0) value |= 0x02;
        if (triangle_.length_counter > 0) value |= 0x04;
        if (noise_.length_counter > 0) value |= 0x08;
        if (dmc_.bytes_remaining > 0) value |= 0x10;
        
        // TODO: Frame interrupt flag
        // TODO: DMC interrupt flag
        
        return value;
    }
    return 0;
}

void APU::write_register(uint16_t address, uint8_t value) {
    switch (address) {
        // ... Pulse/Triangle/Noise cases (unchanged) ...
        // Pulse 1
        case 0x4000:
            pulse1_.duty_mode = (value >> 6) & 0x03;
            pulse1_.length_halt = (value & 0x20) != 0;
            pulse1_.constant_volume = (value & 0x10) != 0;
            pulse1_.volume_envelope = value & 0x0F;
            break;
        case 0x4001:
            pulse1_.sweep_enabled = (value & 0x80) != 0;
            pulse1_.sweep_period = (value >> 4) & 0x07;
            pulse1_.sweep_negate = (value & 0x08) != 0;
            pulse1_.sweep_shift = value & 0x07;
            pulse1_.sweep_reload = true;
            break;
        case 0x4002:
            pulse1_.timer_low = value;
            pulse1_.timer_period = (pulse1_.timer_period & 0x0700) | value;
            break;
        case 0x4003:
            pulse1_.length_table_index = (value >> 3) & 0x1F;
            if (enable_pulse1_) pulse1_.length_counter = LENGTH_TABLE[pulse1_.length_table_index];
            pulse1_.timer_high = value & 0x07;
            pulse1_.timer_period = (pulse1_.timer_period & 0x00FF) | ((uint16_t)pulse1_.timer_high << 8);
            pulse1_.duty_sequence = 0;
            pulse1_.envelope_start = true;
            break;

        // Pulse 2
        case 0x4004:
            pulse2_.duty_mode = (value >> 6) & 0x03;
            pulse2_.length_halt = (value & 0x20) != 0;
            pulse2_.constant_volume = (value & 0x10) != 0;
            pulse2_.volume_envelope = value & 0x0F;
            break;
        case 0x4005:
            pulse2_.sweep_enabled = (value & 0x80) != 0;
            pulse2_.sweep_period = (value >> 4) & 0x07;
            pulse2_.sweep_negate = (value & 0x08) != 0;
            pulse2_.sweep_shift = value & 0x07;
            pulse2_.sweep_reload = true;
            break;
        case 0x4006:
            pulse2_.timer_low = value;
            pulse2_.timer_period = (pulse2_.timer_period & 0x0700) | value;
            break;
        case 0x4007:
            pulse2_.length_table_index = (value >> 3) & 0x1F;
            if (enable_pulse2_) pulse2_.length_counter = LENGTH_TABLE[pulse2_.length_table_index];
            pulse2_.timer_high = value & 0x07;
            pulse2_.timer_period = (pulse2_.timer_period & 0x00FF) | ((uint16_t)pulse2_.timer_high << 8);
            pulse2_.duty_sequence = 0;
            pulse2_.envelope_start = true;
            break;
            
        // Triangle
        case 0x4008:
            triangle_.length_halt = (value & 0x80) != 0;
            triangle_.linear_reload = value & 0x7F;
            break;
        case 0x400A:
            triangle_.timer_low = value;
            triangle_.timer_period = (triangle_.timer_period & 0x0700) | value;
            break;
        case 0x400B:
            triangle_.length_table_index = (value >> 3) & 0x1F;
            if (enable_triangle_) triangle_.length_counter = LENGTH_TABLE[triangle_.length_table_index];
            triangle_.timer_high = value & 0x07;
            triangle_.timer_period = (triangle_.timer_period & 0x00FF) | ((uint16_t)triangle_.timer_high << 8);
            triangle_.reload_flag = true;
            break;
            
        // Noise
        case 0x400C:
            noise_.length_halt = (value & 0x20) != 0;
            noise_.constant_volume = (value & 0x10) != 0;
            noise_.volume_envelope = value & 0x0F;
            break;
        case 0x400E:
            noise_.mode = (value & 0x80) != 0;
            noise_.period_index = value & 0x0F;
            noise_.timer_period = NOISE_PERIOD_TABLE[noise_.period_index];
            break;
        case 0x400F:
            noise_.length_table_index = (value >> 3) & 0x1F;
            if (enable_noise_) noise_.length_counter = LENGTH_TABLE[noise_.length_table_index];
            noise_.envelope_start = true;
            break;

        // DMC
        case 0x4010:
            dmc_.irq_enabled = (value & 0x80) != 0;
            dmc_.loop = (value & 0x40) != 0;
            dmc_.rate_index = value & 0x0F;
            dmc_.timer_period = DMC_RATE_TABLE[dmc_.rate_index];
            if (!dmc_.irq_enabled) dmc_.irq_pending = false;
            break;
        case 0x4011:
            dmc_.direct_load = value & 0x7F;
            dmc_.output_level = dmc_.direct_load; // Direct update
            break;
        case 0x4012:
            dmc_.sample_address = value;
            break;
        case 0x4013:
            dmc_.sample_length = value;
            break;

        // Status
        case 0x4015:
            enable_pulse1_ = (value & 0x01) != 0;
            enable_pulse2_ = (value & 0x02) != 0;
            enable_triangle_ = (value & 0x04) != 0;
            enable_noise_ = (value & 0x08) != 0;
            enable_dmc_ = (value & 0x10) != 0;
            
            if (!enable_pulse1_) pulse1_.length_counter = 0;
            if (!enable_pulse2_) pulse2_.length_counter = 0;
            if (!enable_triangle_) triangle_.length_counter = 0;
            if (!enable_noise_) noise_.length_counter = 0;
            
            if (enable_dmc_) {
                if (dmc_.bytes_remaining == 0) {
                    dmc_.restart();
                }
            } else {
                dmc_.bytes_remaining = 0;
                dmc_.irq_pending = false;
            }
            break;
            
        // Frame Counter
        case 0x4017:
            frame_counter_mode_ = (value & 0x80) >> 7;
            irq_inhibit_ = (value & 0x40) != 0;
            frame_step_ = 0;
            if (frame_counter_mode_ == 1) {
                step_frame_counter();
            }
            break;
    }
}

float APU::get_sample() const {
    // Mixer: linear approximation
    // Pulse Out = 0.00752 * (Pulse1 + Pulse2)
    // TND Out = 0.00851 * Triangle + 0.00494 * Noise + 0.00335 * DMC
    
    uint8_t p1 = const_cast<PulseChannel&>(pulse1_).output();
    uint8_t p2 = const_cast<PulseChannel&>(pulse2_).output();
    uint8_t tr = const_cast<TriangleChannel&>(triangle_).output();
    uint8_t ns = const_cast<NoiseChannel&>(noise_).output();
    uint8_t dm = const_cast<DMCChannel&>(dmc_).output();
    
    float pulse_out = 0.00752f * (p1 + p2);
    float tnd_out = 0.00851f * tr + 0.00494f * ns + 0.00335f * dm;
    
    return pulse_out + tnd_out;
}

// ... Pulse/Triangle/Noise implementations (unchanged) ...
// ==========================================
// Pulse Channel Implementation
// ==========================================

uint8_t APU::PulseChannel::output() {
    if (length_counter == 0) return 0;
    if (DUTY_TABLE[duty_mode][duty_sequence] == 0) return 0;
    if (timer_period < 8) return 0;
    if (sweep_mute) return 0;
    
    if (constant_volume) {
        return volume_envelope;
    } else {
        return envelope_volume;
    }
}

void APU::PulseChannel::step_timer() {
    if (timer_value == 0) {
        timer_value = timer_period;
        duty_sequence = (duty_sequence + 1) & 0x07;
    } else {
        timer_value--;
    }
}

void APU::PulseChannel::step_envelope() {
    if (envelope_start) {
        envelope_start = false;
        envelope_volume = 15;
        envelope_counter = volume_envelope;
    } else {
        if (envelope_counter > 0) {
            envelope_counter--;
        } else {
            envelope_counter = volume_envelope;
            if (envelope_volume > 0) {
                envelope_volume--;
            } else if (length_halt) {
                envelope_volume = 15;
            }
        }
    }
}

void APU::PulseChannel::step_length() {
    if (!length_halt && length_counter > 0) {
        length_counter--;
    }
}

void APU::PulseChannel::step_sweep(bool is_pulse2) {
    calculate_sweep_target(is_pulse2);
    
    if (sweep_reload) {
        sweep_reload = false;
        sweep_counter = sweep_period;
    } else if (sweep_counter > 0) {
        sweep_counter--;
    } else {
        sweep_counter = sweep_period;
        if (sweep_enabled && !sweep_mute) {
            timer_period = target_period;
        }
    }
}

void APU::PulseChannel::calculate_sweep_target(bool is_pulse2) {
    uint16_t change = timer_period >> sweep_shift;
    if (sweep_negate) {
        target_period = timer_period - change;
        if (is_pulse2) target_period--;
    } else {
        target_period = timer_period + change;
    }
    
    sweep_mute = (timer_period < 8) || (target_period > 0x7FF);
}

// ==========================================
// Triangle Channel Implementation
// ==========================================

uint8_t APU::TriangleChannel::output() {
    if (length_counter == 0 || linear_counter == 0) return 0;
    return TRIANGLE_SEQUENCE[sequence_index];
}

void APU::TriangleChannel::step_timer() {
    if (timer_value == 0) {
        timer_value = timer_period;
        if (length_counter > 0 && linear_counter > 0) {
            sequence_index = (sequence_index + 1) & 0x1F;
        }
    } else {
        timer_value--;
    }
}

void APU::TriangleChannel::step_linear() {
    if (reload_flag) {
        linear_counter = linear_reload;
    } else if (linear_counter > 0) {
        linear_counter--;
    }
    
    if (!length_halt) {
        reload_flag = false;
    }
}

void APU::TriangleChannel::step_length() {
    if (!length_halt && length_counter > 0) {
        length_counter--;
    }
}

// ==========================================
// Noise Channel Implementation
// ==========================================

uint8_t APU::NoiseChannel::output() {
    if (length_counter == 0) return 0;
    if (shift_register & 0x01) return 0; // Bit 0 determines output
    
    if (constant_volume) {
        return volume_envelope;
    } else {
        return envelope_volume;
    }
}

void APU::NoiseChannel::step_timer() {
    if (timer_value == 0) {
        timer_value = timer_period;
        
        uint8_t feedback;
        if (mode) {
            feedback = (shift_register & 0x01) ^ ((shift_register >> 6) & 0x01);
        } else {
            feedback = (shift_register & 0x01) ^ ((shift_register >> 1) & 0x01);
        }
        
        shift_register >>= 1;
        shift_register |= (feedback << 14);
    } else {
        timer_value--;
    }
}

void APU::NoiseChannel::step_envelope() {
    if (envelope_start) {
        envelope_start = false;
        envelope_volume = 15;
        envelope_counter = volume_envelope;
    } else {
        if (envelope_counter > 0) {
            envelope_counter--;
        } else {
            envelope_counter = volume_envelope;
            if (envelope_volume > 0) {
                envelope_volume--;
            } else if (length_halt) {
                envelope_volume = 15;
            }
        }
    }
}

void APU::NoiseChannel::step_length() {
    if (!length_halt && length_counter > 0) {
        length_counter--;
    }
}

// ==========================================
// DMC Channel Implementation
// ==========================================

uint8_t APU::DMCChannel::output() {
    return output_level;
}

void APU::DMCChannel::step_timer(Memory* memory) {
    if (timer_value == 0) {
        timer_value = timer_period;
        step_reader(memory);
    } else {
        timer_value--;
    }
}

void APU::DMCChannel::step_reader(Memory* memory) {
    if (!buffer_empty) {
        // Output cycle
        if (bits_remaining <= 0) {
            bits_remaining = 8;
            shift_register = sample_buffer;
            buffer_empty = true;
        }
        
        if (bits_remaining > 0) {
            bool bit_0 = (shift_register & 0x01) != 0;
            if (bit_0) {
                if (output_level <= 125) output_level += 2;
            } else {
                if (output_level >= 2) output_level -= 2;
            }
            shift_register >>= 1;
            bits_remaining--;
        }
    }
    
    // Fetch cycle (if buffer empty and bytes remaining)
    if (buffer_empty && bytes_remaining > 0 && memory) {
        sample_buffer = memory->read(current_address);
        buffer_empty = false;
        
        // Address increment logic: wraps 0xFFFF -> 0x8000
        if (current_address == 0xFFFF) {
            current_address = 0x8000;
        } else {
            current_address++;
        }
        
        bytes_remaining--;
        
        if (bytes_remaining == 0) {
            if (loop) {
                restart();
            } else if (irq_enabled) {
                irq_pending = true;
            }
        }
    }
}

void APU::DMCChannel::restart() {
    current_address = 0xC000 + (sample_address * 64);
    bytes_remaining = (sample_length * 16) + 1;
}

} // namespace nes
