#ifndef NES_INPUT_H
#define NES_INPUT_H

#include <cstdint>

namespace nes {

/**
 * @brief NES Controller Input System
 * 
 * Handles standard NES controller input (A, B, Select, Start, Up, Down, Left, Right).
 * Manages the strobe mechanism and serial data reading via $4016/$4017.
 */
class Input {
public:
    Input();
    ~Input();

    /**
     * @brief Reset input state
     */
    void reset();

    /**
     * @brief Write to Input register ($4016)
     * Controls the strobe latch.
     * Writing 1 then 0 latches the current button state.
     */
    void write(uint8_t value);

    /**
     * @brief Read from Controller 1 ($4016)
     * Returns the next bit of the button state.
     */
    uint8_t read_controller1();

    /**
     * @brief Read from Controller 2 ($4017)
     * Returns the next bit of the button state.
     */
    uint8_t read_controller2();

    /**
     * @brief Set the state of a specific button
     * @param controller_id 0 for Player 1, 1 for Player 2
     * @param button Index of button (0-7)
     * @param pressed True if pressed, false otherwise
     */
    void set_button_state(int controller_id, int button, bool pressed);

    // Button mapping indices
    static constexpr int BUTTON_A      = 0;
    static constexpr int BUTTON_B      = 1;
    static constexpr int BUTTON_SELECT = 2;
    static constexpr int BUTTON_START  = 3;
    static constexpr int BUTTON_UP     = 4;
    static constexpr int BUTTON_DOWN   = 5;
    static constexpr int BUTTON_LEFT   = 6;
    static constexpr int BUTTON_RIGHT  = 7;

private:
    // Current live state of buttons (0 = not pressed, 1 = pressed)
    // Bit 0=A, 1=B, 2=Select, 3=Start, 4=Up, 5=Down, 6=Left, 7=Right
    uint8_t controller1_state_;
    uint8_t controller2_state_;

    // Latched state for serial reading
    uint8_t controller1_shifter_;
    uint8_t controller2_shifter_;

    // Strobe state (latch control)
    bool strobe_;
};

} // namespace nes

#endif // NES_INPUT_H
