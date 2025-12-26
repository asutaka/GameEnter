#ifndef NES_APU_H
#define NES_APU_H

#include <cstdint>

namespace nes {

class Memory;

/**
 * @brief NES Audio Processing Unit (Ricoh 2A03 APU)
 * 
 * Generates 5 channels of audio:
 * 1. Pulse 1 (Square wave)
 * 2. Pulse 2 (Square wave)
 * 3. Triangle (Triangle wave)
 * 4. Noise (Pseudo-random noise)
 * 5. DMC (Delta Modulation Channel)
 */
class APU {
public:
    APU();
    ~APU();

    /**
     * @brief Reset APU state
     */
    void reset();

    /**
     * @brief Run one APU cycle (called every CPU cycle)
     * Note: APU runs at CPU frequency, but some components run at half speed.
     */
    void step();

    /**
     * @brief Read from APU register ($4000-$4017)
     */
    uint8_t read_register(uint16_t address);

    /**
     * @brief Write to APU register ($4000-$4017)
     */
    void write_register(uint16_t address, uint8_t value);

    /**
     * @brief Get current audio sample (0.0 to 1.0)
     * Used by the audio backend (SDL) to play sound.
     */
    float get_sample() const;

private:
    // Frame Counter ($4017)
    uint8_t frame_counter_mode_;
    bool irq_inhibit_;
    
    // Status Register ($4015)
    bool enable_pulse1_;
    bool enable_pulse2_;
    bool enable_triangle_;
    bool enable_noise_;
    bool enable_dmc_;
    
    // Internal cycle counter
    uint64_t cycle_count_;
    
    // Frame Counter
    uint8_t frame_step_; // 0-4 or 0-5
    
    // ==========================================
    // Pulse Channel Structure
    // ==========================================
    struct PulseChannel {
        // Registers
        bool enabled;
        
        // Duty / Envelope ($4000/$4004)
        uint8_t duty_mode;      // 0-3 (12.5%, 25%, 50%, 75%)
        bool length_halt;       // Also envelope loop
        bool constant_volume;
        uint8_t volume_envelope; // Period or constant volume
        
        // Sweep ($4001/$4005)
        bool sweep_enabled;
        uint8_t sweep_period;
        bool sweep_negate;
        uint8_t sweep_shift;
        bool sweep_reload;
        
        // Timer Low ($4002/$4006)
        uint8_t timer_low;
        
        // Length / Timer High ($4003/$4007)
        uint8_t length_table_index; // Loaded value
        uint8_t timer_high;
        
        // Internal State
        uint16_t timer_value;       // Current timer countdown
        uint16_t timer_period;      // Reload value (11-bit)
        uint8_t duty_sequence;      // Current position in duty cycle (0-7)
        uint8_t length_counter;     // Current length countdown
        
        // Envelope Unit
        uint8_t envelope_counter;
        uint8_t envelope_period;
        uint8_t envelope_volume;
        bool envelope_start;
        
        // Sweep Unit
        uint8_t sweep_counter;
        uint16_t target_period;     // Calculated target period
        bool sweep_mute;            // Mute flag
        
        // Output
        uint8_t output();
        void step_timer();
        void step_envelope();
        void step_length();
        void step_sweep(bool is_pulse2);
        void calculate_sweep_target(bool is_pulse2);
    };
    
    PulseChannel pulse1_;
    PulseChannel pulse2_;
    
    // ==========================================
    // Triangle Channel Structure
    // ==========================================
    struct TriangleChannel {
        bool enabled;
        
        // Linear Counter ($4008)
        bool length_halt;       // Also control flag
        uint8_t linear_reload;  // Reload value
        bool reload_flag;       // Internal flag
        uint8_t linear_counter; // Internal counter
        
        // Timer Low ($400A)
        uint8_t timer_low;
        
        // Length / Timer High ($400B)
        uint8_t length_table_index;
        uint8_t timer_high;
        
        // Internal State
        uint16_t timer_value;
        uint16_t timer_period;
        uint8_t length_counter;
        uint8_t sequence_index; // 0-31
        
        uint8_t output();
        void step_timer();
        void step_linear();
        void step_length();
    };
    
    TriangleChannel triangle_;
    
    // ==========================================
    // Noise Channel Structure
    // ==========================================
    struct NoiseChannel {
        bool enabled;
        
        // Envelope ($400C)
        bool length_halt;       // Also envelope loop
        bool constant_volume;
        uint8_t volume_envelope;
        
        // Mode / Period ($400E)
        bool mode;              // Loop noise
        uint8_t period_index;   // 0-15
        
        // Length ($400F)
        uint8_t length_table_index;
        
        // Internal State
        uint16_t timer_value;
        uint16_t timer_period;
        uint8_t length_counter;
        uint16_t shift_register; // 15-bit
        
        // Envelope Unit
        uint8_t envelope_counter;
        uint8_t envelope_period;
        uint8_t envelope_volume;
        bool envelope_start;
        
        uint8_t output();
        void step_timer();
        void step_envelope();
        void step_length();
    };
    
    NoiseChannel noise_;
    
    // ==========================================
    // DMC Channel Structure
    // ==========================================
    struct DMCChannel {
        bool enabled;
        
        // Flags / Rate ($4010)
        bool irq_enabled;
        bool loop;
        uint8_t rate_index;
        
        // Direct Load ($4011)
        uint8_t direct_load;
        
        // Address ($4012)
        uint8_t sample_address;
        
        // Length ($4013)
        uint8_t sample_length;
        
        // Internal State
        uint16_t current_address;
        uint16_t bytes_remaining;
        uint8_t sample_buffer;
        bool buffer_empty;
        uint8_t shift_register;
        uint8_t bits_remaining;
        uint16_t timer_value;
        uint16_t timer_period;
        bool silence;
        uint8_t output_level; // 0-127
        
        // IRQ
        bool irq_pending;
        
        uint8_t output();
        void step_timer(Memory* memory);
        void step_reader(Memory* memory);
        void restart();
    };
    
    DMCChannel dmc_;
    
    // Lookup table for length counter
    static const uint8_t LENGTH_TABLE[32];
    
    // Lookup table for noise periods
    static const uint16_t NOISE_PERIOD_TABLE[16];
    
    // Lookup table for DMC rates (NTSC)
    static const uint16_t DMC_RATE_TABLE[16];
    
    // Duty Cycle Sequences
    static const uint8_t DUTY_TABLE[4][8];
    
    // Triangle Sequence
    static const uint8_t TRIANGLE_SEQUENCE[32];
    
    // Helper to clock frame counter
    void step_frame_counter();
    
    // Memory access for DMC
    Memory* memory_;
    
public:
    void connect_memory(Memory* memory);
};

} // namespace nes

#endif // NES_APU_H
