#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "../core/emulator.h"
#include <iostream>
#include <string>
#include <chrono>

using namespace nes;

// Screen dimensions
const int SCREEN_WIDTH = 256;
const int SCREEN_HEIGHT = 240;
const int SCALE = 3; // 3x scale = 768x720

// Key mapping
void handle_input(Emulator& emu, const Uint8* keys) {
    uint8_t buttons = 0;
    
    // Controller 1 Mapping
    if (keys[SDL_SCANCODE_Z])      buttons |= (1 << Input::BUTTON_A);
    if (keys[SDL_SCANCODE_X])      buttons |= (1 << Input::BUTTON_B);
    if (keys[SDL_SCANCODE_A])      buttons |= (1 << Input::BUTTON_SELECT); // A key for Select
    if (keys[SDL_SCANCODE_S])      buttons |= (1 << Input::BUTTON_START);  // S key for Start
    if (keys[SDL_SCANCODE_UP])     buttons |= (1 << Input::BUTTON_UP);
    if (keys[SDL_SCANCODE_DOWN])   buttons |= (1 << Input::BUTTON_DOWN);
    if (keys[SDL_SCANCODE_LEFT])   buttons |= (1 << Input::BUTTON_LEFT);
    if (keys[SDL_SCANCODE_RIGHT])  buttons |= (1 << Input::BUTTON_RIGHT);
    
    emu.set_controller(0, buttons);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <rom_file>" << std::endl;
        return 1;
    }

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Audio Specification
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_F32;
    want.channels = 1;
    want.samples = 2048;
    want.callback = NULL; // Use queue
    
    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audio_device == 0) {
        std::cerr << "Failed to open audio: " << SDL_GetError() << std::endl;
        // Continue without audio
    } else {
        SDL_PauseAudioDevice(audio_device, 0); // Start playing
    }

    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "NES Emulator - C++ From Scratch",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Create texture
    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH, SCREEN_HEIGHT
    );

    if (!texture) {
        std::cerr << "Texture could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialize Emulator
    Emulator emu;
    if (!emu.load_rom(argv[1])) {
        std::cerr << "Failed to load ROM: " << argv[1] << std::endl;
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        if (audio_device != 0) SDL_CloseAudioDevice(audio_device);
        SDL_Quit();
        return 1;
    }

    std::cout << "ROM loaded successfully! Starting emulation..." << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  D-Pad:  Arrow Keys" << std::endl;
    std::cout << "  A:      Z" << std::endl;
    std::cout << "  B:      X" << std::endl;
    std::cout << "  Select: A" << std::endl;
    std::cout << "  Start:  S" << std::endl;
    std::cout << "  Quit:   ESC" << std::endl;

    emu.reset();
    
    // Run warmup frames to let PPU initialize
    std::cout << "Running warmup frames..." << std::endl;
    for (int i = 0; i < 10; i++) {
        emu.run_frame();
    }
    
    // Don't init palette - let game do it
    // This way game's colors will be correct
    
    std::cout << "Emulator ready!" << std::endl;

    // Main loop
    bool quit = false;
    SDL_Event e;
    
    auto last_time = std::chrono::high_resolution_clock::now();
    int frame_count = 0;
    int total_frames = 0;
    bool auto_started = false;
    
    while (!quit) {
        // Handle events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    quit = true;
                }
                // Reset
                if (e.key.keysym.sym == SDLK_r) {
                    emu.reset();
                    total_frames = 0;
                    auto_started = false;
                    std::cout << "Reset!" << std::endl;
                }
                // Force rendering (F key)
                if (e.key.keysym.sym == SDLK_f) {
                    emu.memory_.write(0x2001, 0x1E);
                    std::cout << "Forced PPUMASK = $1E (rendering enabled)" << std::endl;
                }
            }
        }

        // Handle input
        const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);
        handle_input(emu, currentKeyStates);
        
        // Auto-start: Press START button after 3 seconds (180 frames) to help games initialize
        if (!auto_started && total_frames >= 180 && total_frames <= 190) {
            emu.set_controller(0, 0x08); // START button
            if (total_frames == 190) {
                auto_started = true;
                std::cout << "Auto-pressed START to begin game..." << std::endl;
            }
        }

        // Run one frame
        emu.run_frame();
        total_frames++;
        
        if (total_frames % 60 == 0) {
            std::cout << "Frame: " << total_frames << " (Running...)" << std::endl;
        }
        
        // Audio Output
        if (audio_device != 0) {
            const std::vector<float>& samples = emu.get_audio_samples();
            if (!samples.empty()) {
                // Queue audio
                SDL_QueueAudio(audio_device, samples.data(), samples.size() * sizeof(float));
                
                // Simple sync: if queue is too large, skip a frame or delay?
                // For now, let's just let it run. SDL handles buffering.
                // If queue gets too big (latency), we might want to clear it or speed up.
                // But with VSync enabled, it should be roughly synced.
                
                // Anti-latency: if queue > 0.1s, clear some?
                // uint32_t queued_bytes = SDL_GetQueuedAudioSize(audio_device);
                // if (queued_bytes > 44100 * sizeof(float) / 5) { // > 0.2s
                //     SDL_ClearQueuedAudio(audio_device);
                // }
            }
        }

        // Update texture
        const uint8_t* framebuffer = emu.get_framebuffer();
        SDL_UpdateTexture(texture, NULL, framebuffer, SCREEN_WIDTH * 4);

        // Render
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        
        // FPS Counter
        frame_count++;
        auto current_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = current_time - last_time;
        
        if (elapsed.count() >= 1.0) {
            std::string title = "NES Emulator - FPS: " + std::to_string(frame_count);
            SDL_SetWindowTitle(window, title.c_str());
            frame_count = 0;
            last_time = current_time;
        }
    }

    // Cleanup
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    if (audio_device != 0) SDL_CloseAudioDevice(audio_device);
    SDL_Quit();

    return 0;
}
