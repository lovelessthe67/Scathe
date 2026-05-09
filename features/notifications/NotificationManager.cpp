#include "NotificationManager.hpp"
#include "core/storage/Globals.hpp"
#include <algorithm>

NotificationManager& NotificationManager::getInstance() {
    static NotificationManager instance;
    return instance;
}

void NotificationManager::push_notif(const std::string& message, float duration) {
    if (!storage::notifications_enabled) return;

    std::lock_guard<std::mutex> lock(mutex);
    notifications.push_back({ message, std::chrono::steady_clock::now(), duration });
}

void NotificationManager::render() {
    if (!storage::notifications_enabled) return;

    std::lock_guard<std::mutex> lock(mutex);
    if (notifications.empty()) return;

    auto now = std::chrono::steady_clock::now();
    size_t visible_count = 0;

    for (auto it = notifications.begin(); it != notifications.end();) {
        float elapsed = std::chrono::duration<float>(now - it->startTime).count();

        if (elapsed > it->duration) {
            it = notifications.erase(it);
            continue;
        }

        if (visible_count >= max_notifications) {
            ++it;
            continue;
        }

        float opacity = 1.0f;
        if (elapsed < fade_in_duration) {
            opacity = elapsed / fade_in_duration;
        } else if (elapsed > (it->duration - fade_out_duration)) {
            opacity = 1.0f - ((elapsed - (it->duration - fade_out_duration)) / fade_out_duration);
        }

        ImVec2 text_size = ImGui::CalcTextSize(it->message.c_str());
        ImVec2 padding = ImVec2(10, 8);
        ImVec2 notification_size = ImVec2(text_size.x + padding.x * 2, text_size.y + padding.y * 2);
        
        // Positioned lower as requested
        ImVec2 pos = ImVec2(10, 80 + visible_count * (notification_size.y + spacing));

        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(notification_size);
        ImGui::Begin(("##Notif_" + std::to_string(visible_count)).c_str(), nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        
        // Background - Dark
        ImU32 bg_color = IM_COL32(20, 20, 20, static_cast<int>(opacity * 255));
        draw_list->AddRectFilled(pos, pos + notification_size, bg_color, 0.0f);
        
        // Layer 1: Black Outer Border
        draw_list->AddRect(pos, pos + notification_size, IM_COL32(0, 0, 0, static_cast<int>(opacity * 255)), 0.0f, 0, 1.0f);

        // Layer 2: Inner Accent Border (Inset by 1 pixel)
        ImU32 accent_color = IM_COL32(137, 207, 240, static_cast<int>(opacity * 255));
        draw_list->AddRect(ImVec2(pos.x + 1, pos.y + 1), ImVec2(pos.x + notification_size.x - 1, pos.y + notification_size.y - 1), accent_color, 0.0f, 0, 1.0f);

        // Progress Bar at the bottom (Sharp)
        float progress = 1.0f - (elapsed / it->duration);
        ImVec2 bar_start = ImVec2(pos.x + 2, pos.y + notification_size.y - 3);
        ImVec2 bar_end = ImVec2(pos.x + 2 + (notification_size.x - 4) * progress, pos.y + notification_size.y - 1);
        draw_list->AddRectFilled(bar_start, bar_end, accent_color, 0.0f);

        // Text
        ImVec2 text_pos = pos + padding;
        draw_list->AddText(text_pos, IM_COL32(255, 255, 255, static_cast<int>(opacity * 255)), it->message.c_str());

        ImGui::End();

        visible_count++;
        ++it;
    }
}
