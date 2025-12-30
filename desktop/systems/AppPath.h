#pragma once
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#endif

namespace nes {

inline std::filesystem::path get_app_dir() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::filesystem::path exe_path(buffer);
    return exe_path.parent_path();
#else
    return std::filesystem::current_path();
#endif
}

} // namespace nes
