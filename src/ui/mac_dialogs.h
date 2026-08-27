#pragma once
#include <string>

/// Show a native macOS open-panel and return the chosen path (empty if cancelled).
std::string mac_show_open_panel();

/// Show a native macOS save-panel and return the chosen path (empty if cancelled).
std::string mac_show_save_panel();
