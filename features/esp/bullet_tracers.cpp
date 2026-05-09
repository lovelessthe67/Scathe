#include <Windows.h>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef StartPos
#undef StartPos
#endif
#ifdef EndPos
#undef EndPos
#endif

#include "bullet_tracers.h"
#include "core/storage/Globals.hpp"
#include "core/storage/include.h"
#include <cmath>

#define M_PI 3.14159265358979323846

namespace BulletTracers {

std::vector<BulletTracer> RegisteredTracers;

void RegisterBulletTracer(const Engine::Vector3& start, const Engine::Vector3& end) {
    BulletTracer tracer;
    tracer.origin = start;
    tracer.target = end;
    tracer.TimeInserted = std::chrono::steady_clock::now();
    RegisteredTracers.push_back(tracer);
}

void ClearOldTracers() {
    std::chrono::steady_clock::time_point CurrentTime = std::chrono::steady_clock::now();
    for (int i = 0; i < RegisteredTracers.size(); ++i) {
        std::chrono::milliseconds ElapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            CurrentTime - RegisteredTracers[i].TimeInserted);
        if (ElapsedTime.count() >= storage::visuals::bullet_tracer_lifetime) {
            RegisteredTracers.erase(RegisteredTracers.begin() + i);
            --i;
        }
    }
}

void DrawSwirlLine(ImDrawList* Draw, const Engine::Vector2& start, const Engine::Vector2& end, ImU32 color, float thickness, bool drawOutline) {
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float distance = sqrtf(dx * dx + dy * dy);
    
    if (distance < 1.0f) return;
    
    int segments = static_cast<int>(distance / 10.0f);
    if (segments < 5) segments = 5;
    if (segments > 50) segments = 50;
    
    float swirlFrequency = 0.3f;
    float swirlAmplitude = 8.0f;
    
    Engine::Vector2 prevPoint = start;
    
    for (int i = 1; i <= segments; ++i) {
        float t = static_cast<float>(i) / segments;
        
        float baseX = start.x + dx * t;
        float baseY = start.y + dy * t;
        
        float perpX = -dy / distance;
        float perpY = dx / distance;
        
        float offset = sinf(t * M_PI * 2.0f * swirlFrequency * 5.0f) * swirlAmplitude;
        
        Engine::Vector2 currentPoint;
        currentPoint.x = baseX + perpX * offset;
        currentPoint.y = baseY + perpY * offset;
        
        if (drawOutline) {
            Draw->AddLine(
                {prevPoint.x, prevPoint.y},
                {currentPoint.x, currentPoint.y},
                IM_COL32(0, 0, 0, 255),
                thickness + 2.0f
            );
        }
        Draw->AddLine(
            {prevPoint.x, prevPoint.y},
            {currentPoint.x, currentPoint.y},
            color,
            thickness
        );
        
        prevPoint = currentPoint;
    }
}

void DrawLightningLine(ImDrawList* Draw, const Engine::Vector2& start, const Engine::Vector2& end, ImU32 color, float thickness, bool drawOutline) {
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float distance = sqrtf(dx * dx + dy * dy);
    
    if (distance < 1.0f) return;
    
    int segments = static_cast<int>(distance / 15.0f);
    if (segments < 4) segments = 4;
    if (segments > 30) segments = 30;
    
    float jitterAmount = 12.0f;
    
    Engine::Vector2 prevPoint = start;
    
    for (int i = 1; i <= segments; ++i) {
        float t = static_cast<float>(i) / segments;
        
        float baseX = start.x + dx * t;
        float baseY = start.y + dy * t;
        
        float jitterX = 0.0f;
        float jitterY = 0.0f;
        
        if (i != segments) {
            jitterX = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * jitterAmount * 2.0f;
            jitterY = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * jitterAmount * 2.0f;
        }
        
        Engine::Vector2 currentPoint;
        currentPoint.x = baseX + jitterX;
        currentPoint.y = baseY + jitterY;
        
        if (drawOutline) {
            Draw->AddLine(
                {prevPoint.x, prevPoint.y},
                {currentPoint.x, currentPoint.y},
                IM_COL32(0, 0, 0, 255),
                thickness + 2.0f
            );
        }
        Draw->AddLine(
            {prevPoint.x, prevPoint.y},
            {currentPoint.x, currentPoint.y},
            color,
            thickness
        );
        
        Draw->AddLine(
            {prevPoint.x, prevPoint.y},
            {currentPoint.x, currentPoint.y},
            IM_COL32(
                (color >> 0) & 0xFF,
                (color >> 8) & 0xFF,
                (color >> 16) & 0xFF,
                80
            ),
            thickness + 4.0f
        );
        
        prevPoint = currentPoint;
    }
}

void DrawBeamLine(ImDrawList* Draw, const Engine::Vector2& start, const Engine::Vector2& end, ImU32 color, float thickness, bool drawOutline) {
    Draw->AddLine(
        {start.x, start.y},
        {end.x, end.y},
        IM_COL32(
            (color >> 0) & 0xFF,
            (color >> 8) & 0xFF,
            (color >> 16) & 0xFF,
            50
        ),
        thickness + 6.0f
    );
    
    Draw->AddLine(
        {start.x, start.y},
        {end.x, end.y},
        IM_COL32(
            (color >> 0) & 0xFF,
            (color >> 8) & 0xFF,
            (color >> 16) & 0xFF,
            120
        ),
        thickness + 3.0f
    );
    
    if (drawOutline) {
        Draw->AddLine(
            {start.x, start.y},
            {end.x, end.y},
            IM_COL32(0, 0, 0, 255),
            thickness + 1.0f
        );
    }
    
    Draw->AddLine(
        {start.x, start.y},
        {end.x, end.y},
        color,
        thickness
    );
}

void RenderBulletTracers(ImDrawList* Draw) {
    if (!storage::visuals::bullet_tracers_enabled)
        return;
    
    if (RegisteredTracers.size() == 0)
        return;
    
    ClearOldTracers();
    
    std::chrono::steady_clock::time_point CurrentTime = std::chrono::steady_clock::now();
    
    Engine::Vector2 dimensions = storage::visualengine.GetDimensions();
    Engine::Matrix4x4 viewMatrix = storage::visualengine.GetViewMatrix();
    
    for (int i = 0; i < RegisteredTracers.size(); ++i) {
        std::chrono::milliseconds ElapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            CurrentTime - RegisteredTracers[i].TimeInserted);
        
        // Calculate alpha based on fade start time
        float alpha = 1.0f;
        if (ElapsedTime.count() >= storage::visuals::bullet_tracer_fade_start) {
            float fadeProgress = (ElapsedTime.count() - storage::visuals::bullet_tracer_fade_start) / 
                                (storage::visuals::bullet_tracer_lifetime - storage::visuals::bullet_tracer_fade_start);
            alpha = 1.0f - fadeProgress;
        }
        if (alpha < 0.0f) alpha = 0.0f;
        
        Engine::Vector2 One = Engine::WorldToScreen(
            RegisteredTracers[i].origin,
            dimensions,
            viewMatrix
        );
        
        Engine::Vector2 Two = Engine::WorldToScreen(
            RegisteredTracers[i].target,
            dimensions,
            viewMatrix
        );
        
        if (One.x == -1 || Two.x == -1)
            continue;
        
        ImU32 tracerColor = IM_COL32(
            static_cast<int>(storage::visuals::bullet_tracer_color[0] * 255),
            static_cast<int>(storage::visuals::bullet_tracer_color[1] * 255),
            static_cast<int>(storage::visuals::bullet_tracer_color[2] * 255),
            static_cast<int>(storage::visuals::bullet_tracer_color[3] * alpha * 255)
        );
        
        float thickness = storage::visuals::bullet_tracer_thickness;
        bool drawOutline = storage::visuals::bullet_tracer_outline;
        
        switch (storage::visuals::bullet_tracer_style) {
            case 0:
                DrawBeamLine(Draw, One, Two, tracerColor, thickness, drawOutline);
                break;
                
            case 1:
                DrawSwirlLine(Draw, One, Two, tracerColor, thickness, drawOutline);
                break;
                
            case 2:
                DrawLightningLine(Draw, One, Two, tracerColor, thickness, drawOutline);
                break;
                
            default:
                DrawBeamLine(Draw, One, Two, tracerColor, thickness, drawOutline);
                break;
        }
    }
}

} // namespace BulletTracers
