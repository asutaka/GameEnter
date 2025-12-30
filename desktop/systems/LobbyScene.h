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
        
        // --- 2-COLUMN LAYOUT ---
        int left_col_x = cx - 320;  // Left column start
        int right_col_x = cx + 20;  // Right column start
        int content_y = 220;        // Content start Y
        
        // === LEFT COLUMN: Players Card ===
        SDL_Rect players_box = {left_col_x, content_y, 300, 180};
        
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
        
        // Title
        font_small.draw_text(renderer, "Players", players_box.x + 10, players_box.y + 10, {52, 152, 219, 255});
        
        // Player 1 (Host)
        SDL_SetRenderDrawColor(renderer, 46, 204, 113, 255);
        draw_filled_circle_aa(renderer, players_box.x + 30, players_box.y + 60, 8);
        std::string p1_text = lobby_is_host ? lobby_host_name + " (You)" : lobby_host_name;
        font_body.draw_text(renderer, "P1: " + p1_text, players_box.x + 50, players_box.y + 70, {34, 43, 50, 255});
        
        // Player 2 (Client)
        if (lobby_player2_connected) {
            SDL_SetRenderDrawColor(renderer, 46, 204, 113, 255);
            draw_filled_circle_aa(renderer, players_box.x + 30, players_box.y + 110, 8);
            std::string p2_text = lobby_is_host ? "Player 2" : "You";
            font_body.draw_text(renderer, "P2: " + p2_text, players_box.x + 50, players_box.y + 120, {34, 43, 50, 255});
        } else {
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            draw_filled_circle_aa(renderer, players_box.x + 30, players_box.y + 110, 8);
            font_small.draw_text(renderer, "Waiting for P2...", players_box.x + 50, players_box.y + 120, {150, 150, 150, 255});
        }
        
        // === RIGHT COLUMN: Chat ===
        int chat_w = 300;
        int chat_h = 180;
        SDL_Rect chat_box = {right_col_x, content_y, chat_w, chat_h};
        
        // Shadow
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 15);
        SDL_Rect chat_shadow = {chat_box.x + 4, chat_box.y + 4, chat_w, chat_h};
        SDL_RenderFillRect(renderer, &chat_shadow);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &chat_box);
        SDL_SetRenderDrawColor(renderer, 52, 152, 219, 255);
        SDL_RenderDrawRect(renderer, &chat_box);
        
        // Chat title
        font_small.draw_text(renderer, "Chat", chat_box.x + 10, chat_box.y + 10, {52, 152, 219, 255});
        
        // Chat history (max 5 messages, but smaller)
        int msg_y = chat_box.y + 30;
        int visible_count = 0;
        int max_visible = 4;  // Reduced from 5 to fit better
        
        for (int i = (int)chat_history.size() - 1; i >= 0 && visible_count < max_visible; i--) {
            const auto& msg = chat_history[i];
            std::string display = msg.first + ": " + msg.second;
            
            // Truncate if too long
            if (display.length() > 35) {
                display = display.substr(0, 32) + "...";
            }
            
            SDL_Color msg_color = (visible_count % 2 == 0) ? 
                SDL_Color{34, 43, 50, 255} : SDL_Color{100, 100, 100, 255};
            
            font_small.draw_text(renderer, display, chat_box.x + 10, msg_y, msg_color);
            msg_y += 18;  // Reduced spacing
            visible_count++;
        }
        
        // Input box
        SDL_Rect input_box = {chat_box.x + 10, chat_box.y + chat_h - 30, chat_w - 20, 20};
        SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
        SDL_RenderFillRect(renderer, &input_box);
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        SDL_RenderDrawRect(renderer, &input_box);
        
        // Input text (truncated if too long)
        std::string display_input = chat_input;
        if (display_input.length() > 30) {
            display_input = display_input.substr(display_input.length() - 30);
        }
        
        if (!display_input.empty()) {
            font_small.draw_text(renderer, display_input, input_box.x + 5, input_box.y + 15, {34, 43, 50, 255});
        } else {
            font_small.draw_text(renderer, "Type message...", input_box.x + 5, input_box.y + 15, {150, 150, 150, 255});
        }
        
        // === BUTTONS (Below both columns) ===
        int btn_y = content_y + 200;
        
        if (lobby_is_host) {
            // Cancel Button
            SDL_Rect cancel_btn = {cx - 130, btn_y, 110, 40};
            SDL_SetRenderDrawColor(renderer, 231, 76, 60, 255);
            SDL_RenderFillRect(renderer, &cancel_btn);
            font_body.draw_text(renderer, "Cancel", cancel_btn.x + (110 - font_body.get_text_width("Cancel"))/2, cancel_btn.y + 27, {255, 255, 255, 255});
            
            // Start Button
            SDL_Rect start_btn = {cx + 20, btn_y, 110, 40};
            if (lobby_player2_connected) {
                SDL_SetRenderDrawColor(renderer, 46, 204, 113, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            }
            SDL_RenderFillRect(renderer, &start_btn);
            SDL_Color start_color = lobby_player2_connected ? SDL_Color{255, 255, 255, 255} : SDL_Color{220, 220, 220, 255};
            font_body.draw_text(renderer, "Start", start_btn.x + (110 - font_body.get_text_width("Start"))/2, start_btn.y + 27, start_color);
        } else {
            // Leave Button (Client)
            SDL_Rect leave_btn = {cx - 55, btn_y, 110, 40};
            SDL_SetRenderDrawColor(renderer, 231, 76, 60, 255);
            SDL_RenderFillRect(renderer, &leave_btn);
            font_body.draw_text(renderer, "Leave", leave_btn.x + (110 - font_body.get_text_width("Leave"))/2, leave_btn.y + 27, {255, 255, 255, 255});
        }
    }
    std::function<void()> on_start_multiplayer;
    
    // Chat state
    std::string chat_input;
    std::vector<std::pair<std::string, std::string>> chat_history;  // <sender, message>
    const int MAX_CHAT_MESSAGES = 5;

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
            int content_y = 220;
            int btn_y = content_y + 200;
            
            if (lobby_is_host) {
                // Cancel Button
                SDL_Rect cancel_btn = {cx - 130, btn_y, 110, 40};
                if (mx >= cancel_btn.x && mx <= cancel_btn.x + cancel_btn.w &&
                    my >= cancel_btn.y && my <= cancel_btn.y + cancel_btn.h) {
                    std::cout << "❌ Host cancelled lobby" << std::endl;
                    discovery.stop_advertising();
                    net_manager.disconnect();
                    current_scene = SCENE_HOME;
                    home_active_panel = 2; // HOME_PANEL_FAVORITES (Duo)
                }
                
                // Start Button
                SDL_Rect start_btn = {cx + 20, btn_y, 110, 40};
                if (lobby_player2_connected && mx >= start_btn.x && mx <= start_btn.x + start_btn.w &&
                    my >= start_btn.y && my <= start_btn.y + start_btn.h) {
                    std::cout << "🎮 Host starting game!" << std::endl;
                    net_manager.send_input(0xFFFFFFFF, 0xFF); // MSG_START_GAME
                    if (on_start_multiplayer) on_start_multiplayer();
                }
            } else {
                // Leave Button (Client)
                SDL_Rect leave_btn = {cx - 55, btn_y, 110, 40};
                if (mx >= leave_btn.x && mx <= leave_btn.x + leave_btn.w &&
                    my >= leave_btn.y && my <= leave_btn.y + leave_btn.h) {
                    std::cout << "❌ Client left lobby" << std::endl;
                    net_manager.disconnect();
                    current_scene = SCENE_HOME;
                    home_active_panel = 2; // HOME_PANEL_FAVORITES
                }
            }
        }
        
        // Chat input handling
        if (e.type == SDL_TEXTINPUT) {
            // Add typed character to chat input
            if (chat_input.length() < 100) {  // Max 100 characters
                chat_input += e.text.text;
            }
        }
        
        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_BACKSPACE && !chat_input.empty()) {
                // Remove last character
                chat_input.pop_back();
            }
            else if (e.key.keysym.sym == SDLK_RETURN && !chat_input.empty()) {
                // Send message
                std::string sender = lobby_is_host ? "Host" : "Client";
                
                // Add to local history
                chat_history.push_back({sender, chat_input});
                if (chat_history.size() > MAX_CHAT_MESSAGES) {
                    chat_history.erase(chat_history.begin());
                }
                
                // Send via network
                net_manager.send_chat_message(chat_input);
                
                // Clear input
                chat_input.clear();
            }
        }
    }
};

} // namespace nes
