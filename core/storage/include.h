#pragma once

#include <algorithm>
#include <fstream>
#include <iostream>

extern bool shouldExit;
#include <string>
#include <thread>
#include <tlhelp32.h>
#include <vector>
#include <windows.h>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

template <typename T> inline const T &min(const T &a, const T &b) {
  return std::min(a, b);
}

template <typename T> inline const T &max(const T &a, const T &b) {
  return std::max(a, b);
}

#include "core/Core.hpp"

namespace hooks {
void hook_aimbot();
void esp();
static bool render();
void hvh();
void silentaim();
// void Speed();
} // namespace hooks

void speed();
void speed();
void headless_thread();
void korblox_thread();
void fly_thread();
void animation_changer_thread();
void emote_changer_thread();
void arsenal_skinchanger();
void jump();
void noclip();
void hitsound_thread();

extern Engine::PlayerIns aimbot_target;
extern Engine::PlayerIns silentaim_target;
