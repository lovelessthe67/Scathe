#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <mutex>
#include "imgui.h"

struct Notification {
    std::string message;
    std::chrono::steady_clock::time_point startTime;
    float duration;
};

class NotificationManager {
public:
    static NotificationManager& getInstance();

    void push_notif(const std::string& message, float duration = 2.0f);
    void render();

private:
    NotificationManager() = default;
    ~NotificationManager() = default;
    NotificationManager(const NotificationManager&) = delete;
    NotificationManager& operator=(const NotificationManager&) = delete;

    std::vector<Notification> notifications;
    std::mutex mutex;

    static constexpr float fade_in_duration = 0.3f;
    static constexpr float fade_out_duration = 0.3f;
    static constexpr float spacing = 10.0f;
    static constexpr int max_notifications = 5;
};
