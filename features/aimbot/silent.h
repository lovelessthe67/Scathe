#pragma once

namespace rbx
{
	namespace silent
	{
		void silent_aim_1();
		void silent_aim_2();
		void silentaim_main();
	}
}
inline std::unique_ptr<Engine::Instance> g_mouseservice{};

inline bool g_silent_data_ready{ false };
inline bool g_silent_found_target{ false };
inline bool g_silent_target_needs_reset{ false };
inline Engine::Vector2 g_silent_partpos{};
inline Engine::Vector3 g_silent_partpos_3d{}; // 3D position of the target part
inline std::uint64_t g_silent_cached_position_x{ 0 };
inline std::uint64_t g_silent_cached_position_y{ 0 };
inline Engine::PlayerIns g_silent_cached_target{};
inline Engine::PlayerIns g_silent_cached_last_target{};
inline Engine::Instance g_silent_aim_instance{};

inline bool g_silent_aim_enabled{ true };
inline bool g_silent_sticky_aim{ false };
inline bool g_silent_knock_check{ false };
inline bool g_silent_auto_switch{ false };
inline bool g_silent_spoof_mouse{ true };
inline int g_silent_aim_keybind{ 0x51 };
inline bool g_silent_aim_locked{ false };
inline bool g_silent_aim_key_was_pressed{ false };

struct c_silent_help final {
	unsigned long long address = 0;
	static std::uint64_t cached_input_object;
	void set_frame_pos_x(uint64_t position);

	void set_frame_pos_y(uint64_t position);

	void initialize_mouse_service(std::uint64_t address);

	void write_mouse_position(std::uint64_t address, float x, float y);

};
