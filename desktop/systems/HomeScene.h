#pragma once
#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include <functional>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <filesystem>

#include "Slot.h"
#include "UISystem.h"
#include "FontSystem.h"
#include "ReplaySystem.h"
#include "../slot_manager.h" 
#include "../core/config/config_manager.h"
#include "../core/network/network_discovery.h"
#include "../core/network/network_manager.h"
#include "../core/emulator.h"

namespace nes {

// Enum for Home Panels
enum HomePanel { 
    HOME_PANEL_ROM_GRID = 0,
    HOME_PANEL_LIBRARY = 1,
    HOME_PANEL_FAVORITES = 2
};

class HomeScene {
public:
    // State
    int active_panel = HOME_PANEL_ROM_GRID;
    int scroll_y = 0;
    int scroll_speed = 30;
    
    // Popup & Context Menu
    int mouse_down_slot = -1;
    bool showing_delete_popup = false;
    int delete_candidate_index = -1;
    
    bool showing_delete_replay_popup = false;
    int delete_replay_index = -1;
    
    bool showing_context_menu = false;
    int context_menu_slot = -1;
    int menu_x = 0;
    int menu_y = 0;

    // Library
    std::vector<ReplayFileInfo> replay_files;
    int library_scroll_y = 0;
    int selected_replay_index = -1;

    // Duo
    std::string duo_selected_rom_path = "";
    std::string duo_selected_rom_name = "";
    std::string duo_host_name = "player";
    bool duo_rom_selector_open = false;
    int duo_active_input_field = -1;

    // Callbacks
    std::function<bool(std::string)> on_start_game;
    std::function<void(int)> on_delete_slot; // Notify main if needed, or handle here? Main handles save on exit. But immediate delete?
    // We can handle delete here if we have reference to slots.
    
    std::function<void()> on_start_replay; // When replay starts
    std::function<void(std::string, std::string, std::string)> on_create_host; // host_name, rom_name, rom_path
    std::function<void(const NetworkDiscovery::Peer&)> on_connect_host; // Peer

    void init(const std::string& nickname) {
        if (!nickname.empty()) duo_host_name = nickname;
    }

    void handle_event(const SDL_Event& e, std::vector<Slot>& slots, 
                      ConfigManager& config, NetworkDiscovery& discovery, 
                      Emulator& emu, ReplayPlayer& replay_player,
                      int screen_width, int screen_height, int scale) {
        
        int SCREEN_WIDTH = screen_width;
        int SCREEN_HEIGHT = screen_height;
        int SCALE = scale;

        if (e.type == SDL_MOUSEWHEEL) {
            if (active_panel == HOME_PANEL_ROM_GRID) {
                scroll_y -= e.wheel.y * scroll_speed;
                if (scroll_y < 0) scroll_y = 0;
                // Max scroll calc?
                int slot_h = 250; int gap = 20; int cols = 3;
                int rows = (int)((slots.size() + cols - 1) / cols);
                int total_h = rows * (slot_h + gap) + 140; // 140 is header
                if (total_h > SCREEN_HEIGHT * SCALE && scroll_y > total_h - SCREEN_HEIGHT * SCALE) 
                    scroll_y = total_h - SCREEN_HEIGHT * SCALE;
            } else if (active_panel == HOME_PANEL_LIBRARY) {
                 library_scroll_y -= e.wheel.y * scroll_speed;
                 if (library_scroll_y < 0) library_scroll_y = 0;
            }
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN) {
            int mx = e.button.x;
            int my = e.button.y;

            // Header Tabs
            int tab_y = 85; 
            int tab_h = 40;
            // 3 Tabs centered
            int tab_w = 120;
            int total_w = 3 * tab_w;
            int start_x = (SCREEN_WIDTH * SCALE - total_w) / 2;

            // Tab 1: Games
            if (mx >= start_x && mx < start_x + tab_w && my >= tab_y && my < tab_y + tab_h) {
                active_panel = HOME_PANEL_ROM_GRID;
            }
            // Tab 2: Library
            else if (mx >= start_x + tab_w && mx < start_x + 2*tab_w && my >= tab_y && my < tab_y + tab_h) {
                active_panel = HOME_PANEL_LIBRARY;
                // Refresh replays
                replay_files = scan_replay_files();
            }
            // Tab 3: Duo
            else if (mx >= start_x + 2*tab_w && mx < start_x + 3*tab_w && my >= tab_y && my < tab_y + tab_h) {
                active_panel = HOME_PANEL_FAVORITES;
            }
            
            // --- Panel Specific ---
            if (active_panel == HOME_PANEL_ROM_GRID) {
                if (showing_context_menu) {
                     // Check context menu items
                     // 4 items: Play, Add Shortcut InGame, Change Cover, Delete
                     int item_h = 40;
                     int menu_w = 180;
                     int menu_h = 160; // 4 items * 40
                     
                     if (mx >= menu_x && mx <= menu_x + menu_w && my >= menu_y && my <= menu_y + menu_h) {
                         int clicked_item = (my - menu_y) / item_h;
                         if (clicked_item == 0) { // Add Shortcut
                             // .. implementation ..
                             // Needs platform specific shortcut logic. Skipped for now or just log.
                             std::cout << "[Shortcut] Created for " << slots[context_menu_slot].name << std::endl;
                         } else if (clicked_item == 1) { // Add Shortcut InGame
                             std::cout << "[Shortcut InGame] Created for " << slots[context_menu_slot].name << std::endl;
                             // Needs platform specific logic
                         } else if (clicked_item == 2) { // Change Cover
                             std::string path = open_file_dialog("Image Files\0*.png;*.jpg;*.jpeg;*.bmp\0All Files\0*.*\0");
                             if (!path.empty()) {
                                 std::string new_cover = import_cover_image(path, slots[context_menu_slot].name);
                                 slots[context_menu_slot].cover_path = new_cover;
                                 slots[context_menu_slot].cover_texture = load_texture(NULL, new_cover); // Renderer needed? null for now, reload in render?
                                 // Ideally we should pass renderer to handle_event or reload in render
                             }
                         } else if (clicked_item == 3) { // Delete
                             delete_candidate_index = context_menu_slot;
                             showing_delete_popup = true;
                         }
                         showing_context_menu = false;
                     } else {
                         showing_context_menu = false;
                     }
                     return;
                }
                
                if (showing_delete_popup) {
                    int cx = (SCREEN_WIDTH * SCALE)/2; int cy = (SCREEN_HEIGHT * SCALE)/2;
                    int w = 400; int h = 200;
                    SDL_Rect btn_yes = {cx - 110, cy + 20, 100, 40};
                    SDL_Rect btn_no = {cx + 10, cy + 20, 100, 40};
                    
                    if (mx >= btn_yes.x && mx <= btn_yes.x + btn_yes.w && my >= btn_yes.y && my <= btn_yes.y + btn_yes.h) {
                        if (delete_candidate_index != -1) {
                             slots[delete_candidate_index].occupied = false;
                             slots[delete_candidate_index].rom_path = "";
                             slots[delete_candidate_index].name = "";
                             slots[delete_candidate_index].cover_path = "";
                             if (slots[delete_candidate_index].cover_texture) {
                                  SDL_DestroyTexture(slots[delete_candidate_index].cover_texture);
                                  slots[delete_candidate_index].cover_texture = nullptr;
                             }
                             delete_candidate_index = -1;
                        }
                        showing_delete_popup = false;
                    } else if (mx >= btn_no.x && mx <= btn_no.x + btn_no.w && my >= btn_no.y && my <= btn_no.y + btn_no.h) {
                        showing_delete_popup = false;
                        delete_candidate_index = -1;
                    }
                    return;
                }
                
                // Grid Item Clicks
                int slot_w = 200; int slot_h = 250; int gap = 20; int cols = 3;
                int start_x = (SCREEN_WIDTH * SCALE - (cols*slot_w + (cols-1)*gap)) / 2;
                int panel_content_y = 140;
                int start_y = panel_content_y + 10;
                
                // Add Button Index
                int add_btn_index = -1;
                for (size_t k = 0; k < slots.size(); ++k) if (!slots[k].occupied) { add_btn_index = (int)k; break; }
                
                for (size_t i=0; i<slots.size(); i++) {
                     int col = i % cols; int row = (int)i / cols;
                     int sx = start_x + col * (slot_w + gap);
                     int sy = start_y + row * (slot_h + gap) - scroll_y;
                     
                     if (mx >= sx && mx <= sx + slot_w && my >= sy && my <= sy + slot_h) {
                         if (i == add_btn_index) {
                             std::string path = open_file_dialog();
                             if (!path.empty()) {
                                 slots[i].occupied = true;
                                 slots[i].rom_path = path;
                                 slots[i].name = std::filesystem::path(path).stem().string();
                                 std::string cover = find_cover_image(path);
                                 slots[i].cover_path = cover;
                                 // Texture load deferred to render or next frame
                             }
                         } else if (slots[i].occupied) {
                             bool near_dots = (mx >= sx + slot_w - 45 && mx <= sx + slot_w && my >= sy && my <= sy + 55);
                             if (e.button.button == SDL_BUTTON_RIGHT || (e.button.button == SDL_BUTTON_LEFT && near_dots)) {
                                  showing_context_menu = true;
                                  context_menu_slot = (int)i;
                                  menu_x = mx; menu_y = my;
                                  // Bounds check
                                  if (menu_x + 300 > SCREEN_WIDTH * SCALE) menu_x = SCREEN_WIDTH * SCALE - 300;
                                  if (menu_y + 200 > SCREEN_HEIGHT * SCALE) menu_y = SCREEN_HEIGHT * SCALE - 200;
                             } else {
                                  mouse_down_slot = (int)i;
                             }
                         }
                     }
                }

            } 
            else if (active_panel == HOME_PANEL_LIBRARY) {
                // Library Clicks
                if (showing_delete_replay_popup) {
                    int cx = (SCREEN_WIDTH * SCALE)/2; int cy = (SCREEN_HEIGHT * SCALE)/2;
                    int w = 400; int h = 200;
                    SDL_Rect btn_yes = {cx - 110, cy + 20, 100, 40};
                    SDL_Rect btn_no = {cx + 10, cy + 20, 100, 40};
                    
                    if (mx >= btn_yes.x && mx <= btn_yes.x + btn_yes.w && my >= btn_yes.y && my <= btn_yes.y + btn_yes.h) {
                        if (delete_replay_index != -1 && delete_replay_index < (int)replay_files.size()) {
                            std::filesystem::remove(replay_files[delete_replay_index].path);
                            replay_files.erase(replay_files.begin() + delete_replay_index);
                            selected_replay_index = -1;
                        }
                        showing_delete_replay_popup = false;
                    } else if (mx >= btn_no.x && mx <= btn_no.x + btn_no.w && my >= btn_no.y && my <= btn_no.y + btn_no.h) {
                        showing_delete_replay_popup = false;
                    }
                    return;
                }
                
                // Replay List
                int list_start_y = 160;
                int item_height = 80;
                int item_margin = 10;
                int list_x = 40;
                int list_width = SCREEN_WIDTH * SCALE - 80;
                
                for (size_t i = 0; i < replay_files.size(); i++) {
                    int item_y = list_start_y + i * (item_height + item_margin) - library_scroll_y;
                    if (mx >= list_x && mx <= list_x + list_width && my >= item_y && my <= item_y + item_height) {
                        int del_x = list_x + list_width - 40;
                        int icon_x = list_x + 40;
                        
                        // Check Play Icon
                        if (mx >= icon_x - 20 && mx <= icon_x + 20) {
                            // Play Replay
                            std::string game_name_str = replay_files[i].game_name;
                            // Need to find matching slot
                             bool rom_found = false;
                             for (const auto& slot : slots) {
                                if (slot.occupied && (
                                    game_name_str.find(slot.name) != std::string::npos ||
                                    slot.name.find(game_name_str) != std::string::npos)) {
                                    
                                    if (emu.load_rom(slot.rom_path.c_str())) {
                                        emu.reset();
                                        for (int k = 0; k < 10; k++) emu.run_frame();
                                        
                                        replay_player.load_replay(replay_files[i].path);
                                        replay_player.start_playback();
                                        
                                        if (on_start_replay) on_start_replay();
                                        rom_found = true;
                                    }
                                    break;
                                }
                             }
                        } 
                        // Check Delete Icon
                        else if (mx >= del_x - 20 && mx <= del_x + 20) {
                            delete_replay_index = (int)i;
                            showing_delete_replay_popup = true;
                        } else {
                            selected_replay_index = (int)i;
                        }
                    }
                }

            }
            else if (active_panel == HOME_PANEL_FAVORITES) {
                 // Duo Clicks
                int content_x = 40;
                int content_width = SCREEN_WIDTH * SCALE - 80;
                 if (duo_rom_selector_open) {
                      // Selector Logic
                      // Close button
                      int dialog_w = 550; int dialog_h = 450;
                      int dialog_x = (SCREEN_WIDTH * SCALE - dialog_w) / 2;
                      int dialog_y = (SCREEN_HEIGHT * SCALE - dialog_h) / 2;
                      int close_cx = dialog_x + dialog_w - 30; int close_cy = dialog_y + 30;
                      if ((mx-close_cx)*(mx-close_cx) + (my-close_cy)*(my-close_cy) <= 225) {
                          duo_rom_selector_open = false;
                      } else {
                          // List items
                          int list_y = dialog_y + 80; int item_h = 55; int count = 0;
                          for(size_t i=0; i<slots.size() && count < 6; i++) {
                              if (!slots[i].occupied) continue;
                              SDL_Rect r = {dialog_x + 20, list_y, dialog_w - 40, item_h};
                              if (mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h) {
                                  duo_selected_rom_path = slots[i].rom_path;
                                  duo_selected_rom_name = slots[i].name;
                                  duo_rom_selector_open = false;
                              }
                              list_y += item_h + 10; count++;
                          }
                      }
                 } else {
                      // Main Panel
                      int start_y = 130;
                      int row_y = start_y + 40 + 25; // card_y + padding
                      
                      // Browse Button
                      row_y += 20; 
                      SDL_Rect browse_btn = {content_x + content_width - 115, row_y, 90, 40};
                      if (mx >= browse_btn.x && mx <= browse_btn.x + browse_btn.w && my >= browse_btn.y && my <= browse_btn.y + browse_btn.h) {
                          duo_rom_selector_open = true;
                      }
                      
                      row_y += 80;
                      // Host Name
                      SDL_Rect name_field = {content_x + 25, row_y, 250, 40};
                      if (mx >= name_field.x && mx <= name_field.x + name_field.w && my >= name_field.y && my <= name_field.y + name_field.h) {
                          duo_active_input_field = 0;
                          SDL_StartTextInput();
                      } else {
                          if (duo_active_input_field == 0) SDL_StopTextInput();
                          duo_active_input_field = -1;
                      }
                      
                      // Start Hosting
                      SDL_Rect host_btn = {content_x + content_width - 165, row_y, 140, 40};
                      if (mx >= host_btn.x && mx <= host_btn.x + host_btn.w && my >= host_btn.y && my <= host_btn.y + host_btn.h) {
                          if (!duo_selected_rom_path.empty() && !duo_host_name.empty() && on_create_host) {
                              on_create_host(duo_host_name, duo_selected_rom_name, duo_selected_rom_path);
                          }
                      }
                      
                      // Connect Buttons
                      int hy = row_y + 180 + 50 + 40; // Approx
                      auto hosts = discovery.get_peers();
                      for(const auto& host : hosts) {
                          // Connect button
                          SDL_Rect conn_btn = {content_x + content_width - 120, hy + 20, 100, 40};
                          if (mx >= conn_btn.x && mx <= conn_btn.x + conn_btn.w && my >= conn_btn.y && my <= conn_btn.y + conn_btn.h) {
                              bool has_rom = false;
                              for(auto&s:slots) if(s.occupied && s.rom_path == host.rom_path) has_rom = true;
                              if(has_rom && on_connect_host) {
                                  on_connect_host(host);
                              }
                          }
                          hy += 90;
                      }
                 }
            }
        }
        else if (e.type == SDL_MOUSEBUTTONUP) {
            if (active_panel == HOME_PANEL_ROM_GRID && !showing_delete_popup && !showing_context_menu && mouse_down_slot != -1) {
                if (mouse_down_slot >= 0 && mouse_down_slot < (int)slots.size() && slots[mouse_down_slot].occupied) {
                     if (on_start_game) on_start_game(slots[mouse_down_slot].rom_path);
                }
                mouse_down_slot = -1;
            }
        }
        else if (e.type == SDL_TEXTINPUT && duo_active_input_field == 0) {
            duo_host_name += e.text.text;
        }
        else if (e.type == SDL_KEYDOWN && duo_active_input_field == 0) {
            if (e.key.keysym.sym == SDLK_BACKSPACE && !duo_host_name.empty()) duo_host_name.pop_back();
            if (e.key.keysym.sym == SDLK_RETURN) { duo_active_input_field = -1; SDL_StopTextInput(); }
        }
    }

    void render(SDL_Renderer* renderer, std::vector<Slot>& slots, 
                FontSystem& font_title, FontSystem& font_body, FontSystem& font_small,
                int SCREEN_WIDTH, int SCREEN_HEIGHT, int SCALE) {
        
        // --- HEADER TABS ---
        int tab_y = 85; 
        int tab_h = 40;
        int tab_w = 120;
        int total_tabs_w = 3 * tab_w;
        int tabs_start_x = (SCREEN_WIDTH * SCALE - total_tabs_w) / 2;
        
        const char* tabs[] = {"GAMES", "LIBRARY", "DUO"};
        for (int i=0; i<3; i++) {
             int x = tabs_start_x + i * tab_w;
             bool active = (active_panel == i);
             
             if (active) {
                 SDL_Rect line = {x + 30, tab_y + 35, tab_w - 60, 3};
                 SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
                 SDL_RenderFillRect(renderer, &line);
             }
             
             SDL_Color color = active ? SDL_Color{34, 43, 50, 255} : SDL_Color{150, 150, 150, 255};
             float tw = font_body.get_text_width(tabs[i]);
             font_body.draw_text(renderer, tabs[i], x + (tab_w - (int)tw)/2, tab_y + 10, color);
        }
        
        // --- PANEL CONTENT ---
        int panel_content_y = 140;
        
        if (active_panel == HOME_PANEL_ROM_GRID) {
             // --- ROM GRID PANEL ---
             int slot_w = 200;
             int slot_h = 250;
             int gap = 20;
             int cols = 3;
             int start_x = (SCREEN_WIDTH * SCALE - (cols*slot_w + (cols-1)*gap)) / 2;
             int start_y = panel_content_y + 10;

             // Find Add Button Index
             int add_btn_index = -1;
             for (size_t k = 0; k < slots.size(); ++k) if (!slots[k].occupied) { add_btn_index = (int)k; break; }

             for (size_t i=0; i<slots.size(); i++) {
                 int col = i % cols;
                 int row = (int)i / cols;
                 int sx = start_x + col * (slot_w + gap);
                 int sy = start_y + row * (slot_h + gap) - scroll_y;
                 
                 if (sy + slot_h < 140 || sy > SCREEN_HEIGHT * SCALE) continue;

                 SDL_Rect r = {sx, sy, slot_w, slot_h};
                 SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                 SDL_RenderFillRect(renderer, &r);
                 
                 if (i == add_btn_index) {
                     // Add Button Styling
                     int cx = sx + slot_w/2;
                     int cy = sy + slot_h/2 - 20;
                     SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
                     draw_filled_circle_aa(renderer, cx, cy, 40);
                     SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
                     SDL_Rect rv = {cx - 3, cy - 20, 6, 40};
                     SDL_Rect rh = {cx - 20, cy - 3, 40, 6};
                     SDL_RenderFillRect(renderer, &rv);
                     SDL_RenderFillRect(renderer, &rh);
                     font_body.draw_text(renderer, "Add ROM", sx + 60, sy + slot_h - 40, {34, 43, 50, 255});
                 } else if (slots[i].occupied) {
                     // Cover Art / Cartridge Rendering
                     if (!slots[i].cover_texture && !slots[i].cover_path.empty()) {
                         slots[i].cover_texture = load_texture(renderer, slots[i].cover_path);
                     }
                     if (slots[i].cover_texture) {
                          int img_w, img_h;
                          SDL_QueryTexture(slots[i].cover_texture, NULL, NULL, &img_w, &img_h);
                          int max_w = slot_w - 20;
                          int max_h = slot_h - 60;
                          float scale = (std::min)((float)max_w / img_w, (float)max_h / img_h);
                          int final_w = (int)(img_w * scale);
                          int final_h = (int)(img_h * scale);
                          int img_x = sx + (slot_w - final_w) / 2;
                          int img_y = sy + 10 + (max_h - final_h) / 2;
                          
                          SDL_Rect dst = {img_x, img_y, final_w, final_h};
                          
                          // Shadow
                          SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                          SDL_SetRenderDrawColor(renderer, 0, 0, 0, 50);
                          SDL_Rect shad = {img_x + 4, img_y + 4, final_w, final_h};
                          SDL_RenderFillRect(renderer, &shad);
                          SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                          
                          SDL_RenderCopy(renderer, slots[i].cover_texture, NULL, &dst);
                          SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                          SDL_RenderDrawRect(renderer, &dst);
                     } else {
                          draw_nes_cartridge(renderer, sx + slot_w/2, sy + slot_h/2 - 20, 8);
                     }
                     
                     std::string dname = slots[i].name;
                     if (dname.length() > 18) dname = dname.substr(0, 15) + "...";
                     float tw = font_body.get_text_width(dname);
                     font_body.draw_text(renderer, dname, sx + (slot_w - (int)tw)/2, sy + slot_h - 40, {34, 43, 50, 255});

                     // Dots Menu
                     int dx = sx + slot_w - 20;
                     int dy = sy + 25;
                     SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
                     draw_filled_circle_aa(renderer, dx, dy - 6, 3);
                     draw_filled_circle_aa(renderer, dx, dy, 3);
                     draw_filled_circle_aa(renderer, dx, dy + 6, 3);
                 }
             }
             
             // --- CONTEXT MENU (Premium) ---
             if (showing_context_menu) {
                int w = 180; int h = 160; 
                int item_h = 40;
                SDL_Rect menu = {menu_x, menu_y, w, h};
                
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                for (int i = 1; i <= 5; i++) {
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 40 / i);
                    SDL_Rect shadow = {menu_x + i, menu_y + i, w, h};
                    SDL_RenderDrawRect(renderer, &shadow);
                }
                
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderFillRect(renderer, &menu);
                SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
                SDL_RenderDrawRect(renderer, &menu);

                int mouse_x, mouse_y;
                SDL_GetMouseState(&mouse_x, &mouse_y);

                for (int i = 0; i < 4; i++) {
                    int iy = menu_y + i * item_h;
                    SDL_Rect item_rect = {menu_x, iy, w, item_h};
                    if (mouse_x >= menu_x && mouse_x <= menu_x + w && mouse_y >= iy && mouse_y <= iy + item_h) {
                        SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
                        SDL_RenderFillRect(renderer, &item_rect);
                    }
                    
                    int icon_x = menu_x + 12;
                    int text_x = menu_x + 40;
                    int center_y = iy + 20;

                    SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
                    if (i == 0) { // Shortcut
                        SDL_Rect link1 = {icon_x, center_y - 1, 12, 2}; SDL_RenderFillRect(renderer, &link1);
                        SDL_Rect link2 = {icon_x + 2, center_y - 5, 2, 10}; SDL_RenderFillRect(renderer, &link2);
                        font_small.draw_text(renderer, "Add Shortcut", text_x, center_y + 6, {34, 43, 50, 255});
                    } else if (i == 1) { // Shortcut InGame
                        draw_filled_triangle(renderer, icon_x, center_y - 5, icon_x + 10, center_y, icon_x, center_y + 5);
                        font_small.draw_text(renderer, "Add Shortcut InGame", text_x, center_y + 6, {34, 43, 50, 255});
                    } else if (i == 2) { // Change Cover
                        SDL_Rect img = {icon_x, center_y - 5, 14, 10}; SDL_RenderDrawRect(renderer, &img);
                        draw_filled_circle(renderer, icon_x + 4, center_y - 2, 2);
                        font_small.draw_text(renderer, "Change Cover", text_x, center_y + 6, {34, 43, 50, 255});
                    } else if (i == 3) { // Delete
                        SDL_SetRenderDrawColor(renderer, 200, 80, 80, 255);
                        SDL_Rect bin = {icon_x + 2, center_y - 3, 10, 10}; SDL_RenderFillRect(renderer, &bin);
                        SDL_Rect lid = {icon_x, center_y - 5, 14, 2}; SDL_RenderFillRect(renderer, &lid);
                        font_small.draw_text(renderer, "Delete ROM", text_x, center_y + 6, {200, 80, 80, 255});
                    }
                    if (i < 3) {
                        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
                        SDL_RenderDrawLine(renderer, menu_x + 10, iy + item_h, menu_x + w - 10, iy + item_h);
                    }
                }
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
             }
             
             // --- DELETE POPUP (Premium) ---
             if (showing_delete_popup && delete_candidate_index != -1) {
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
                SDL_Rect overlay = {0, 0, SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE};
                SDL_RenderFillRect(renderer, &overlay);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                int cx = (SCREEN_WIDTH * SCALE) / 2;
                int cy = (SCREEN_HEIGHT * SCALE) / 2;
                int pw = 400; int ph = 200;
                SDL_Rect popup = {cx - pw/2, cy - ph/2, pw, ph};
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderFillRect(renderer, &popup);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderDrawRect(renderer, &popup);

                std::string msg = "Delete " + slots[delete_candidate_index].name + "?";
                float tw = font_body.get_text_width(msg);
                font_body.draw_text(renderer, msg, cx - tw/2, cy - 40, {0, 0, 0, 255});

                SDL_Rect btnYes = {cx - 110, cy + 20, 100, 40};
                SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);
                SDL_RenderFillRect(renderer, &btnYes);
                font_body.draw_text(renderer, "Yes", cx - 110 + 35, cy + 48, {255, 255, 255, 255});

                SDL_Rect btnNo = {cx + 10, cy + 20, 100, 40};
                SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
                SDL_RenderFillRect(renderer, &btnNo);
                font_body.draw_text(renderer, "No", cx + 10 + 40, cy + 48, {255, 255, 255, 255});
             }

        } else if (active_panel == HOME_PANEL_LIBRARY) {
             // --- LIBRARY PANEL (Premium) ---
             int list_start_y = panel_content_y + 20;
             int item_height = 80;
             int item_margin = 10;
             int list_x = 40;
             int list_width = SCREEN_WIDTH * SCALE - 80;
             
             if (replay_files.empty()) {
                 int cx = (SCREEN_WIDTH * SCALE) / 2;
                 int cy = (SCREEN_HEIGHT * SCALE) / 2;
                 font_title.draw_text(renderer, "No Replays Found", cx - 120, cy - 30, {150, 150, 150, 255});
                 font_body.draw_text(renderer, "Record gameplay to see replays here", cx - 140, cy + 10, {180, 180, 180, 255});
             } else {
                 for (size_t i = 0; i < replay_files.size(); i++) {
                     int item_y = list_start_y + i * (item_height + item_margin) - library_scroll_y;
                     if (item_y + item_height < panel_content_y || item_y > SCREEN_HEIGHT * SCALE) continue;
                     
                     SDL_Rect item_rect = {list_x, item_y, list_width, item_height};
                     bool is_selected = (selected_replay_index == (int)i);
                     SDL_SetRenderDrawColor(renderer, is_selected ? 235 : 255, is_selected ? 245 : 255, 255, 255);
                     SDL_RenderFillRect(renderer, &item_rect);
                     SDL_SetRenderDrawColor(renderer, is_selected ? 100 : 230, is_selected ? 180 : 230, is_selected ? 255 : 230, 255);
                     SDL_RenderDrawRect(renderer, &item_rect);
                     
                     if (is_selected) {
                         SDL_Rect accent = {list_x, item_y, 6, item_height};
                         SDL_SetRenderDrawColor(renderer, 50, 150, 250, 255);
                         SDL_RenderFillRect(renderer, &accent);
                     }

                     int icon_x = list_x + 40;
                     int icon_y = item_y + item_height / 2;
                     SDL_SetRenderDrawColor(renderer, is_selected ? 50 : 200, is_selected ? 150 : 200, is_selected ? 250 : 200, 255);
                     draw_filled_circle(renderer, icon_x, icon_y, 20);
                     SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                     draw_filled_triangle(renderer, icon_x - 3, icon_y - 8, icon_x + 10, icon_y, icon_x - 3, icon_y + 8);
                     
                     font_body.draw_text(renderer, replay_files[i].display_name, list_x + 80, item_y + 20, {50, 50, 50, 255});
                     
                     float duration = replay_files[i].total_frames / 60.0f;
                     std::stringstream ss; ss << "Time: " << (int)(duration/60) << "m " << (int)duration%60 << "s";
                     font_small.draw_text(renderer, ss.str(), list_x + 80, item_y + 50, {120, 120, 120, 255});
                     
                     int del_x = list_x + list_width - 40;
                     int del_y = item_y + item_height / 2;
                     SDL_SetRenderDrawColor(renderer, 200, 80, 80, 255);
                     SDL_Rect bin = {del_x - 6, del_y - 6, 12, 14}; SDL_RenderFillRect(renderer, &bin);
                     SDL_Rect lid = {del_x - 8, del_y - 8, 16, 2}; SDL_RenderFillRect(renderer, &lid);
                     SDL_Rect hand = {del_x - 3, del_y - 10, 6, 2}; SDL_RenderFillRect(renderer, &hand);
                 }
             }

             if (showing_delete_replay_popup) {
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
                SDL_Rect overlay = {0, 0, SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE};
                SDL_RenderFillRect(renderer, &overlay);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                int cx = (SCREEN_WIDTH * SCALE) / 2;
                int cy = (SCREEN_HEIGHT * SCALE) / 2;
                SDL_Rect pbox = {cx-200, cy-100, 400, 200};
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderFillRect(renderer, &pbox);
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                SDL_RenderDrawRect(renderer, &pbox);
                
                font_title.draw_text(renderer, "Delete Replay?", cx - 100, cy - 60, {34, 43, 50, 255});
                font_body.draw_text(renderer, "Are you sure you want to delete this replay?", cx - 160, cy - 10, {100, 100, 100, 255});
                
                SDL_Rect bY = {cx - 110, cy + 20, 100, 40};
                SDL_SetRenderDrawColor(renderer, 220, 80, 80, 255); SDL_RenderFillRect(renderer, &bY);
                font_body.draw_text(renderer, "Delete", cx - 90, cy + 48, {255, 255, 255, 255});
                
                SDL_Rect bN = {cx + 10, cy + 20, 100, 40};
                SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255); SDL_RenderFillRect(renderer, &bN);
                font_body.draw_text(renderer, "Cancel", cx + 30, cy + 48, {255, 255, 255, 255});
             }

        } else if (active_panel == HOME_PANEL_FAVORITES) {
             // --- DUO PANEL (Premium Redesign) ---
             int content_x = 40;
             int content_width = SCREEN_WIDTH * SCALE - 80;
             int start_y = 130;
             
             int section_y = start_y;
             SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
             draw_filled_circle_aa(renderer, content_x + 10, section_y + 12, 12);
             SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
             draw_filled_circle_aa(renderer, content_x + 10, section_y + 8, 4);
             SDL_Rect body_icon = {content_x + 6, section_y + 13, 8, 4}; SDL_RenderFillRect(renderer, &body_icon);
             font_title.draw_text(renderer, "CREATE HOST", content_x + 35, section_y + 22, {34, 43, 50, 255});
             
             int card_y = section_y + 40;
             SDL_Rect create_card = {content_x, card_y, content_width, 180};
             SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
             SDL_RenderFillRect(renderer, &create_card);
             SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
             SDL_RenderDrawRect(renderer, &create_card);
             
             int padding = 25;
             int row_y = card_y + padding;
             font_small.draw_text(renderer, "SELECT GAME TO HOST", content_x + padding, row_y + 5, {120, 120, 120, 255});
             row_y += 20;
             SDL_Rect r_f = {content_x + padding, row_y, content_width - 150, 40};
             SDL_SetRenderDrawColor(renderer, 248, 249, 250, 255); SDL_RenderFillRect(renderer, &r_f);
             SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255); SDL_RenderDrawRect(renderer, &r_f);
             std::string rt = duo_selected_rom_name.empty() ? "Choose a game..." : duo_selected_rom_name;
             font_body.draw_text(renderer, rt, r_f.x + 12, r_f.y + 26, duo_selected_rom_name.empty() ? SDL_Color{180, 180, 180, 255} : SDL_Color{34, 43, 50, 255});
             
             SDL_Rect bb = {content_x + content_width - 115, row_y, 90, 40};
             SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255); SDL_RenderFillRect(renderer, &bb);
             font_body.draw_text(renderer, "Browse", bb.x + 15, bb.y + 26, {255, 255, 255, 255});
             
             row_y += 60;
             font_small.draw_text(renderer, "HOST NAME", content_x + padding, row_y + 5, {120, 120, 120, 255});
             row_y += 20;
             SDL_Rect nf = {content_x + padding, row_y, 250, 40};
             SDL_SetRenderDrawColor(renderer, 248, 249, 250, 255); SDL_RenderFillRect(renderer, &nf);
             SDL_SetRenderDrawColor(renderer, duo_active_input_field == 0 ? 50 : 220, 150, 250, 255); SDL_RenderDrawRect(renderer, &nf);
             font_body.draw_text(renderer, duo_host_name, nf.x + 12, nf.y + 26, {34, 43, 50, 255});
             if (duo_active_input_field == 0) {
                 float tw = font_body.get_text_width(duo_host_name);
                 SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
                 SDL_RenderDrawLine(renderer, nf.x + 12 + (int)tw, nf.y + 10, nf.x + 12 + (int)tw, nf.y + 30);
             }
             
             SDL_Rect hb = {content_x + content_width - 165, row_y, 140, 40};
             bool ready = !duo_selected_rom_path.empty() && !duo_host_name.empty();
             SDL_SetRenderDrawColor(renderer, ready ? 46 : 200, ready ? 204 : 200, ready ? 113 : 200, 255);
             SDL_RenderFillRect(renderer, &hb);
             font_body.draw_text(renderer, "Start Hosting", hb.x + 15, hb.y + 28, {255, 255, 255, 255});
             
             section_y = card_y + 180 + 50;
             SDL_SetRenderDrawColor(renderer, 34, 43, 50, 255);
             draw_filled_circle_aa(renderer, content_x + 10, section_y + 12, 12);
             font_title.draw_text(renderer, "AVAILABLE HOSTS", content_x + 35, section_y + 22, {34, 43, 50, 255});
             section_y += 50;
             
             auto peers = discovery.get_peers();
             if (peers.empty()) {
                font_body.draw_text(renderer, "Searching for nearby players...", content_x + 20, section_y + 30, {150, 150, 150, 255});
             } else {
                int hy = section_y;
                for (auto& h : peers) {
                    SDL_Rect hc = {content_x, hy, content_width, 80};
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); SDL_RenderFillRect(renderer, &hc);
                    SDL_SetRenderDrawColor(renderer, 235, 235, 235, 255); SDL_RenderDrawRect(renderer, &hc);
                    font_body.draw_text(renderer, h.username, content_x + 50, hy + 35, {34, 43, 50, 255});
                    font_small.draw_text(renderer, "Playing: " + h.game_name, content_x + 50, hy + 58, {120, 120, 120, 255});
                    
                    bool has = false; for(auto&s:slots) if(s.occupied && s.rom_path == h.rom_path) has=true;
                    SDL_Rect cb = {content_x + content_width - 120, hy + 20, 100, 40};
                    SDL_SetRenderDrawColor(renderer, has ? 52 : 231, has ? 152 : 76, has ? 219 : 60, 255);
                    SDL_RenderFillRect(renderer, &cb);
                    font_small.draw_text(renderer, has ? "Connect" : "No ROM", cb.x + 20, cb.y + 26, {255, 255, 255, 255});
                    hy += 90;
                }
             }

             if (duo_rom_selector_open) {
                 SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                 SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
                 SDL_Rect d = {0,0,SCREEN_WIDTH*SCALE, SCREEN_HEIGHT*SCALE}; SDL_RenderFillRect(renderer, &d);
                 SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                 
                 int dw = 550; int dh = 450;
                 int dx = (SCREEN_WIDTH*SCALE - dw)/2; int dy = (SCREEN_HEIGHT*SCALE - dh)/2;
                 SDL_Rect dlg = {dx, dy, dw, dh};
                 SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); SDL_RenderFillRect(renderer, &dlg);
                 font_title.draw_text(renderer, "SELECT GAME", dx + 20, dy + 40, {34, 43, 50, 255});
                 
                 int close_x = dx + dw - 30; int close_y = dy + 30;
                 SDL_SetRenderDrawColor(renderer, 231, 76, 60, 255); draw_filled_circle_aa(renderer, close_x, close_y, 15);
                 
                 int ly = dy + 80; int c = 0;
                 for(auto& s : slots) {
                     if(!s.occupied || c>=6) continue;
                     SDL_Rect itm = {dx + 20, ly, dw - 40, 55};
                     SDL_SetRenderDrawColor(renderer, 252, 252, 252, 255); SDL_RenderFillRect(renderer, &itm);
                     font_body.draw_text(renderer, s.name, itm.x + 55, itm.y + 24, {34, 43, 50, 255});
                     ly += 65; c++;
                 }
             }
        }
    }
};

} // namespace nes
