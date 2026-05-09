#pragma once
#pragma once

#include <Windows.h>

#include "imgui_internal.h"
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

const char *const key_namess[] = {"Unknown",
                                  "LBUTTON",
                                  "RBUTTON",
                                  "CANCEL",
                                  "MBUTTON",
                                  "XBUTTON1",
                                  "XBUTTON2",
                                  "Unknown",
                                  "BACK",
                                  "TAB",
                                  "Unknown",
                                  "Unknown",
                                  "CLEAR",
                                  "RETURN",
                                  "Unknown",
                                  "Unknown",
                                  "SHIFT",
                                  "CONTROL",
                                  "MENU",
                                  "PAUSE",
                                  "CAPITAL",
                                  "KANA",
                                  "Unknown",
                                  "JUNJA",
                                  "FINAL",
                                  "KANJI",
                                  "Unknown",
                                  "ESCAPE",
                                  "CONVERT",
                                  "NONCONVERT",
                                  "ACCEPT",
                                  "MODECHANGE",
                                  "SPACE",
                                  "PRIOR",
                                  "NEXT",
                                  "END",
                                  "HOME",
                                  "LEFT",
                                  "UP",
                                  "RIGHT",
                                  "DOWN",
                                  "SELECT",
                                  "PRINT",
                                  "EXECUTE",
                                  "SNAPSHOT",
                                  "INSERT",
                                  "DELETE",
                                  "HELP",
                                  "0",
                                  "1",
                                  "2",
                                  "3",
                                  "4",
                                  "5",
                                  "6",
                                  "7",
                                  "8",
                                  "9",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "A",
                                  "B",
                                  "C",
                                  "D",
                                  "E",
                                  "F",
                                  "G",
                                  "H",
                                  "I",
                                  "J",
                                  "K",
                                  "L",
                                  "M",
                                  "N",
                                  "O",
                                  "P",
                                  "Q",
                                  "R",
                                  "S",
                                  "T",
                                  "U",
                                  "V",
                                  "W",
                                  "X",
                                  "Y",
                                  "Z",
                                  "LWIN",
                                  "RWIN",
                                  "APPS",
                                  "Unknown",
                                  "SLEEP",
                                  "NUMPAD0",
                                  "NUMPAD1",
                                  "NUMPAD2",
                                  "NUMPAD3",
                                  "NUMPAD4",
                                  "NUMPAD5",
                                  "NUMPAD6",
                                  "NUMPAD7",
                                  "NUMPAD8",
                                  "NUMPAD9",
                                  "MULTIPLY",
                                  "ADD",
                                  "SEPARATOR",
                                  "SUBTRACT",
                                  "DECIMAL",
                                  "DIVIDE",
                                  "F1",
                                  "F2",
                                  "F3",
                                  "F4",
                                  "F5",
                                  "F6",
                                  "F7",
                                  "F8",
                                  "F9",
                                  "F10",
                                  "F11",
                                  "F12",
                                  "F13",
                                  "F14",
                                  "F15",
                                  "F16",
                                  "F17",
                                  "F18",
                                  "F19",
                                  "F20",
                                  "F21",
                                  "F22",
                                  "F23",
                                  "F24",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "NUMLOCK",
                                  "SCROLL",
                                  "OEM_NEC_EQUAL",
                                  "OEM_FJ_MASSHOU",
                                  "OEM_FJ_TOUROKU",
                                  "OEM_FJ_LOYA",
                                  "OEM_FJ_ROYA",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "Unknown",
                                  "LSHIFT",
                                  "RSHIFT",
                                  "LCONTROL",
                                  "RCONTROL",
                                  "LMENU",
                                  "RMENU"};

class keybind {
private:
  static std::unordered_map<int, bool> previous_key_states;
  bool last_key_state = false;

public:
  enum c_keybind_type : int { TOGGLE, HOLD, ALWAYS };

  int key = 0;
  c_keybind_type type = TOGGLE;
  const char *name = "[None]";
  bool enabled = false;
  bool waiting_for_input = false;

  keybind(const char *_name) {
    this->name = _name;
    this->type = HOLD;
  }

  keybind(const char *_name, int _key, c_keybind_type _type) {
    this->name = _name;
    this->key = _key;
    this->type = _type;
  }

  void update() {
    if (key == 0) {
      if (type != ALWAYS)
        enabled = false;
      return;
    }

    bool current_key_state = (GetAsyncKeyState(key) & 0x8000) != 0;
    bool key_just_pressed = current_key_state && !last_key_state;

    switch (type) {
    case TOGGLE:
      if (key_just_pressed) {
        enabled = !enabled;
      }
      break;
    case HOLD:
      enabled = current_key_state;
      break;
    case ALWAYS:
      enabled = true;
      break;
    }

    last_key_state = current_key_state;
  }

  std::string get_key_name() {
    if (!key)
      return "None";

    if (key < 0 || key > VK_RMENU) {
      // Handle special keys or ranges if needed, but for now focus on the main
      // array
      return "Unknown";
    }

    return key_namess[key];
  }

  std::string get_name() { return name; }

  std::string get_type() {
    switch (type) {
    case TOGGLE:
      return "toggle";
    case HOLD:
      return "hold";
    case ALWAYS:
      return "always";
    default:
      return "none";
    }
  }

  bool set_key() {
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
      key = 0;
      return true;
    }

    for (int i = 1; i < 5; i++) {
      if (ImGui::GetIO().MouseDown[i]) {
        switch (i) {
        case 1:
          key = VK_RBUTTON;
          break;
        case 2:
          key = VK_MBUTTON;
          break;
        case 3:
          key = VK_XBUTTON1;
          break;
        case 4:
          key = VK_XBUTTON2;
          break;
        }
        return true;
      }
    }

    for (int i = VK_BACK; i <= VK_RMENU; i++) {
      if (i == VK_LBUTTON)
        continue;

      if (GetAsyncKeyState(i) & 0x8000) {
        key = i;
        return true;
      }
    }

    std::vector<int> additional_keys = {
        VK_LWIN,    VK_RWIN,      VK_APPS,     VK_SLEEP,   VK_NUMPAD0,
        VK_NUMPAD1, VK_NUMPAD2,   VK_NUMPAD3,  VK_NUMPAD4, VK_NUMPAD5,
        VK_NUMPAD6, VK_NUMPAD7,   VK_NUMPAD8,  VK_NUMPAD9, VK_MULTIPLY,
        VK_ADD,     VK_SEPARATOR, VK_SUBTRACT, VK_DECIMAL, VK_DIVIDE,
        VK_F1,      VK_F2,        VK_F3,       VK_F4,      VK_F5,
        VK_F6,      VK_F7,        VK_F8,       VK_F9,      VK_F10,
        VK_F11,     VK_F12,       VK_F13,      VK_F14,     VK_F15,
        VK_F16,     VK_F17,       VK_F18,      VK_F19,     VK_F20,
        VK_F21,     VK_F22,       VK_F23,      VK_F24};

    for (int vk : additional_keys) {
      if (GetAsyncKeyState(vk) & 0x8000) {
        key = vk;
        return true;
      }
    }

    return false;
  }
};