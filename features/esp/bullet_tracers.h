#pragma once

#include "utils/math/math.h"
#include "imgui.h"
#include <chrono>
#include <vector>

namespace BulletTracers {

struct BulletTracer {
    Engine::Vector3 origin;
    Engine::Vector3 target;
    std::chrono::steady_clock::time_point TimeInserted;
};

extern std::vector<BulletTracer> RegisteredTracers;

void RegisterBulletTracer(const Engine::Vector3& start, const Engine::Vector3& end);
void RenderBulletTracers(ImDrawList* Draw);
void ClearOldTracers();

// Call this in your main loop to check for mouse clicks and register tracers
void CheckAndRegisterTracerOnClick();

} // namespace BulletTracers
