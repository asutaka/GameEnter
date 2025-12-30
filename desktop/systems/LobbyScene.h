#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include "../core/network/network_discovery.h"
#include "../core/network/network_manager.h"
#include "FontSystem.h"
#include "UISystem.h"
#include "Scene.h"

namespace nes {

class LobbyScene {
public:
    void render(SDL_Renderer* renderer, 
                bool lobby_is_host, 
                const std::string& lobby_host_name, 
                const std::string& lobby_rom_name, 
                bool lobby_player2_connected,
                FontSystem& font_title, 
                FontSystem& font_body, 
                FontSystem& font_small,
                int SCREEN_WIDTH, int SCREEN_HEIGHT, int SCALE) {
        
        int cx = (SCREEN_WIDTH * SCALE) / 2;
        int cy = (SCREEN_HEIGHT * SCALE) / 2;
        
        // Background
        SDL_SetRenderDrawColor(renderer, 245, 246, 247, 255);
        SDL_RenderClear(renderer);
        
        // Lobby Info (Below Header)
        std::string title = lobby_is_host ? "Hosting: " + lobby_host_name : "Joining: " + lobby_host_name;
        font_title.draw_text(renderer, title, cx - font_title.get_text_width(title)/2, 140, {34, 43, 50, 255});
        
        // ROM Info
        std::string rom_info = "Game: " + lobby_rom_name;
        font_body.draw_text(renderer, rom_info, cx - font_body.get_text_width(rom_info)/2, 175, {100, 100, 100, 255});
        
        // Players Card (Premium Style)
        SDL_Rect players_box = {cx - 220, cy - 100, 440, 200};
        // Shadow
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 15);
        SDL_Rect shadow = {players_box.x + 4, players_box.y + 4, players_box.w, players_box.h};
        SDL_RenderFillRect(renderer, &shadow);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &players_box);
        SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
        SDL_RenderDrawRect(renderer, &players_box);
        
        // Player 1 (Host)
        SDL_SetRenderDrawColor(renderer, 46, 204, 113, 255); // Emerald Green
        draw_filled_circle_aa(renderer, players_box.x + 40, players_box.y + 60, 8);
        std::string p1_text = lobby_is_host ? lobby_host_name + " (You)" : lobby_host_name;
        font_body.draw_text(renderer, "Player 1: " + p1_text, players_box.x + 65, players_box.y + 70, {34, 43, 50, 255});
        
        // Player 2 (Client)
        if (lobby_player2_connected) {
            SDL_SetRenderDrawColor(renderer, 46, 204, 113, 255);
            draw_filled_circle_aa(renderer, players_box.x + 40, players_box.y + 130, 8);
            std::string p2_text = lobby_is_host ? "Player 2" : "You";
            font_body.draw_text(renderer, "Player 2: " + p2_text, players_box.x + 65, players_box.y + 140, {34, 43, 50, 255});
        } else {
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            draw_filled_circle_aa(renderer, players_box.x + 40, players_box.y + 130, 8);
            font_body.draw_text(renderer, "Waiting for Player 2...", players_box.x + 65, players_box.y + 140, {150, 150, 150, 255});
        }
        
        // Buttons
        if (lobby_is_host) {
            // Cancel Button
            SDL_Rect cancel_btn = {cx - 180, cy + 150, 120, 45};
            SDL_SetRenderDrawColor(renderer, 231, 76, 60, 255); // Alizarin Red
            SDL_RenderFillRect(renderer, &cancel_btn);
            font_body.draw_text(renderer, "Cancel", cancel_btn.x + (120 - font_body.get_text_width("Cancel"))/2, cancel_btn.y + 30, {255, 255, 255, 255});
            
            // Start Button
            SDL_Rect start_btn = {cx + 60, cy + 150, 120, 45};
            if (lobby_player2_connected) {
                SDL_SetRenderDrawColor(renderer, 46, 204, 113, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            }
            SDL_RenderFillRect(renderer, &start_btn);
            SDL_Color start_color = lobby_player2_connected ? SDL_Color{255, 255, 255, 255} : SDL_Color{220, 220, 220, 255};
            font_body.draw_text(renderer, "Start", start_btn.x + (120 - font_body.get_text_width("Start"))/2, start_btn.y + 30, start_color);
        } else {
            // Leave Button (Client)
            SDL_Rect leave_btn = {cx - 60, cy + 150, 120, 45};
            SDL_SetRenderDrawColor(renderer, 231, 76, 60, 255);
            SDL_RenderFillRect(renderer, &leave_btn);
            font_body.draw_text(renderer, "Leave", leave_btn.x + (120 - font_body.get_text_width("Leave"))/2, leave_btn.y + 30, {255, 255, 255, 255});
        }
    }
    std::function<void()> on_start_multiplayer;

    void handle_event(const SDL_Event& e, 
                      bool& lobby_is_host, 
                      bool& lobby_player2_connected,
                      Scene& current_scene,
                      int& home_active_panel,
                      NetworkDiscovery& discovery,
                      NetworkManager& net_manager,
                      int SCREEN_WIDTH, int SCREEN_HEIGHT, int SCALE) {
        
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            int mx = e.button.x;
            int my = e.button.y;
            int cx = (SCREEN_WIDTH * SCALE) / 2;
            int cy = (SCREEN_HEIGHT * SCALE) / 2;
            
            if (lobby_is_host) {
                // Cancel Button
                SDL_Rect cancel_btn = {cx - 180, cy + 150, 120, 45};
                if (mx >= cancel_btn.x && mx <= cancel_btn.x + cancel_btn.w &&
                    my >= cancel_btn.y && my <= cancel_btn.y + cancel_btn.h) {
                    std::cout << "❌ Host cancelled lobby" << std::endl;
                    discovery.stop_advertising();
                    net_manager.disconnect();
                    current_scene = SCENE_HOME;
                    home_active_panel = 2; // HOME_PANEL_FAVORITES (Duo)
                }
                
                // Start Button
                SDL_Rect start_btn = {cx + 60, cy + 150, 120, 45};
                if (lobby_player2_connected && mx >= start_btn.x && mx <= start_btn.x + start_btn.w &&
                    my >= start_btn.y && my <= start_btn.y + start_btn.h) {
                    std::cout << "🎮 Host starting game!" << std::endl;
                    net_manager.send_input(0xFFFFFFFF, 0xFF); // MSG_START_GAME
                    if (on_start_multiplayer) on_start_multiplayer();
                }
            } else {
                // Leave Button (Client)
                SDL_Rect leave_btn = {cx - 60, cy + 150, 120, 45};
                if (mx >= leave_btn.x && mx <= leave_btn.x + leave_btn.w &&
                    my >= leave_btn.y && my <= leave_btn.y + leave_btn.h) {
                    std::cout << "❌ Client left lobby" << std::endl;
                    net_manager.disconnect();
                    current_scene = SCENE_HOME;
                    home_active_panel = 2; // HOME_PANEL_FAVORITES
                }
            }
        }
    }
};

} // namespace nes
