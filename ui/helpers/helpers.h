#pragma once
#include "core/storage/Globals.hpp"
#include "imgui.h"

struct helpers {
  static inline int active_tab = 0;
  static void add_tab();
  static bool is_tab_clicked(int tab_index, ImVec2 tab_pos, ImVec2 tab_size);
  static void render();
  static void HotkeyButton(keybind &kb, const char *label);
};
