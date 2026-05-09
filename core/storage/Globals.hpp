#pragma once

#include "core/Core.hpp"
#include "utils/configs/configs.hpp"
#include "imgui.h"
#include "ckeybind/keybind.hpp"
#include "utils/wallcheck/Wallcheck.h"
#include "utils/math/math.h"
#include <mutex>
#include <unordered_map>
#include <vector>

namespace globals {
inline float r3dboxcolor[3] = {1.0f, 1.0f, 1.0f};
inline float box_filled_color[3] = {1.0f, 1.0f, 1.0f};
inline bool phantomforceteamcheck = false;
inline bool r3dbox = false;
inline bool g_showLobbyWarning = false;
inline bool frontlineteamcheck = true;
inline bool badbuisnessteamcheck = true;
inline bool botcheck = false;
inline int botmode = 0;
inline bool radar_enabled = false;
inline float radar_pos_x = 20.0f;
inline float radar_pos_y = 60.0f;
inline bool radar_show_distance = false;
inline float radar_size = 180.0f;
inline ImVec2 radar_position = ImVec2(20, 60);
inline bool sonar_enabled = false;
inline float sonar_range = 50.0f;
inline float sonar_thickness = 0.3f;
inline float sonar_color[4] = {1.0f, 1.0f, 1.0f, 0.8f};
inline float sonar_dot_color[4] = {1.0f, 0.0f, 0.0f, 1.0f};
inline bool sonar_show_distance = false;
inline bool sonar_dot_color_dummy = true;
inline bool smoothenable = false;
inline int TypeTextName = 1;
inline int TypeTextDistance = 0;
inline int TypeTextTool = 0;
inline int TypeTextRigType = 0;
inline int TypeHealthBar = 2;

inline std::vector<std::string> ESPFieldOrder = {"tool", "distance", "name"};
inline float health_bar_color_main[4] = {0.0f, 1.0f, 0.0f, 1.0f};
inline float snipline[3] = {1.0f, 1.0f, 1.0f};

inline std::unordered_map<std::string, int> playerStatuses;
inline bool skipFriendlyESP = true;
inline std::vector<std::string> whitelist;
inline std::vector<std::string> blacklist;
inline bool IsPlayerFriendly(const std::string &playerName) {
  auto it = playerStatuses.find(playerName);
  return (it != playerStatuses.end() && it->second == 1);
}
inline int GetPlayerStatus(const std::string &playerName) {
  auto it = playerStatuses.find(playerName);
  return (it != playerStatuses.end()) ? it->second : 0;
}
inline void SetPlayerStatus(const std::string &playerName, int status) {
  playerStatuses[playerName] = status;
}

inline float shakeX = 3.0f;
inline float shakeY = 3.0f;

inline bool beizer = false;
inline bool blatant = false;
inline bool healtbartext = false;
inline bool flickbot = false;
inline bool shake = false;
inline float aimbotSensitivity = 0.5f;
inline float aimbotSmoothing = 5.0f;

inline bool headESP = false;
inline int performance_mode = 0;

inline bool teamcheck = false;
inline bool fov_check = false;
inline bool ko_check = false;

namespace rage {
inline bool exploits = false;
inline bool spreadremove = false;
inline float spreadamount = 0.0f;
inline bool speed_enabled = false;
inline int speed_mode = 0;
inline float walkspeed_amount = 50.0f;
inline float velocity_speed = 100.0f;
inline keybind walkspeed_bind{"walkspeed_bind"};

namespace features {
void walkspeed();
}
} // namespace rage

namespace game {
struct local_player_t {
  Engine::Instance humanoid{};
  Engine::Instance hrp{};
};
inline local_player_t local_player{};
} // namespace game

} // namespace globals

struct storage {
  static inline int speedtype = 0;
  static inline float thickness = 2.0f;
  static inline float Boxthickness = 1.0f;

  static inline Engine::Instance datamodel{};
  static inline Engine::Instance fakedm{};
  static inline Engine::Instance visualengine{};
  static inline Engine::Instance workspace{};
  static inline Engine::Instance players{};
  static inline Engine::Instance game{};
  static inline Engine::Instance camera{};
  static inline Engine::PlayerIns localplayer{};

  static inline bool streamble = false;
  static inline bool streamproof = false;
  static inline bool vsync = true;
  static inline bool watermark_enabled = true;
  static inline bool notifications_enabled = true;
  static inline float watermark_x = 15.0f;
  static inline float watermark_y = 15.0f;
  static inline bool playerlist = false;
  static inline std::uint64_t mouse_service{};
  static inline int_fast64_t gameid = 0;
  static inline int_fast64_t placeid = 0;

  static inline Wallcheck wallcheck;
  static inline Engine::Matrix4x4 g_cached_viewmatrix;
  static inline bool g_has_cached_matrix = false;

  static inline int box_type = 0;
  static inline int mode = 0;
  static inline float glow_size = 30.0f;

  static inline bool glow_box = false;
  static inline float glow_box_intensity = 15.0f;
  static inline float glow_box_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};

  static inline float color_1[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  static inline float color_2[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  static inline float box_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  static inline float box_color1[4] = {1.0f, 1.0f, 1.0f, 0.3f};
  static inline float dot_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  static inline float name_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  static inline float distance_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  static inline float tool_esp_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  static inline float glow_col_dist[3] = {1.0f, 1.0f, 1.0f};
  static inline float health_bar_color[3] = {1.0f, 1.0f, 1.0f};
  static inline float chams_color[4] = {0.0f, 0.0f, 0.0f, 0.42f};

  static inline float silent_prediction_y = 1.0f;
  static inline float silent_prediction_x = 1.0f;
  static inline bool silent_prediction = false;
  static inline float prediction_x = 1.0f;
  static inline float prediction_y = 1.0f;
  static inline bool prediction = false;

  static inline bool aimbot = false;
  static inline bool aimbot_teamcheck = false;
  static inline bool aimbot_fov_check = true;
  static inline bool use_fov = true;
  static inline bool aimbot_knockcheck = false;

  static inline bool camlock_teamcheck = false;
  static inline bool silent_teamcheck = false;

  static inline int aimpart = 0;
  static inline int aimbot_mode = 0;
  static inline int aimbot_type = 0;
  static inline int aimbot_easing_style = 0;
  static inline bool sticky = false;
  static inline bool disable_outside_fov = true;
  static inline float fov = 100.0f;
  static inline bool draw_fov = false;
  static inline float hex_fov_rotation = 0.0f;
  static inline float hex_fov_rotation_speed = 0.1f;
  static inline bool wall_check = false;
  static inline bool silent_wallcheck = false;
  static inline float sensitivity = 1.0f;
  static inline float smoothness_x = 1.0f;
  static inline float smoothness_y = 1.0f;
  static inline bool closest_part = false;
  static inline float max_aimbot_distance = 1000.0f;
  static inline bool max_dist = false;
  static inline bool dis_check = false;

  static inline bool silentaim_spoof = false;
  static inline bool silentaim_closestpart = false;
  static inline bool silent_closest_to_mouse = false;
  static inline bool silent_grabbedcheck = false;

  static inline bool healthcheck = false;
  static inline bool knockcheck = false;
  static inline bool knockhecksilent = false;
  static inline bool knifecheck = false;
  static inline bool localplayercheck = false;
  static inline bool HealthBasedColor = false;
  static inline bool health_bar_text = false;
  static inline float health_x = 29.874f;
  static inline float health_y = 0.0f;
  static inline bool enable_health_glow = false;

  static inline bool offscreen_Check = true;
  static inline bool fill_box = false;
  static inline bool fill_box_gradient = false;
  static inline float fill_box_gradient_color1[4] = {
      1.0f, 1.0f, 1.0f, 0.3f}; // White gradient start
  static inline float fill_box_gradient_color2[4] = {
      0.2f, 0.2f, 0.2f, 0.3f}; // Light black gradient end
  static inline bool fill_box_gradient_2 = false;
  static inline float fill_box_gradient_2_color1[4] = {1.0f, 0.0f, 1.0f,
                                                       0.3f}; // Magenta start
  static inline float fill_box_gradient_2_color2[4] = {0.0f, 0.0f, 0.0f,
                                                       0.3f}; // Black end
  static inline float alpha = 1.0f;

  static inline bool walkspeed = false;
  static inline float walkspeedvalue = 16.0f;
  static inline float walkspeed_speed = 16.0f;
  static inline bool nojumpcooldown = false;
  static inline float jumppower = 50.0f;
  static inline float JumpPower = 50.0f;
  static inline bool cframe = false;

  static inline bool jump_power_enabled = false;

  static inline bool korblox = false;
  static inline bool skin_changer_enabled = false;
  static inline int skin_changer_doublebarrel = 0;
  static inline int skin_changer_revolver = 0;
  static inline int skin_changer_tacticalshotgun = 0;
  static inline int skin_changer_ak47 = 0;
  static inline int skin_changer_smg = 0;
  static inline int skin_changer_lmg = 0;

  static inline bool bladeball_autoparry = false;

  static inline bool camera_prediction = false;
  static inline bool resolver = false;
  static inline float camera_prediction_x = 1.0f;
  static inline float camera_prediction_y = 1.0f;
  static inline float mouse_sensitivity = 0.5f;
  static inline float mouse_smoothness = 5.0f;
  static inline float smoothness_camera = 5.0f;

  static inline bool shake = false;
  static inline float shake_x = 0.0f;
  static inline float shake_y = 0.0f;
  static inline bool chams_rainbow = false;
  static inline int chamstype = 0;

  static inline bool tracers = false;
  static inline int tracer_type = 0;
  static inline float tracers_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};

  static inline bool hsuis = false;
  static inline bool hitsound = false;
  static inline int hitsound_type = 0;
  static inline bool headless = false;

  static inline bool custom_models_enabled = false;
  static inline std::string config_name = "";
  static inline std::vector<std::string> config_list{};

  static inline void save_config() {
    Engine::configs::save(config_name.c_str());
  }
  static inline void load_config() {
    Engine::configs::load(config_name.c_str());
  }
  static inline void update_config_list() {
    config_list = Engine::configs::get_configs();
  }
  static inline void delete_config() {
    Engine::configs::remove(config_name.c_str());
    update_config_list();
  }

  static inline std::string custom_models_path = "game.Workspace";
  static inline keybind menu_key_bind = {"Menu Key", VK_INSERT,
                                         keybind::TOGGLE};
  static inline std::vector<Engine::Instance> custom_models{};

  struct hitbox_expander {
    static inline bool enabled = false;
    static inline bool custom_models = false;
    static inline bool cancollide = false;
    static inline float size_x = 2.2f;
    static inline float size_y = 2.2f;
    static inline float size_z = 1.2f;
    static inline bool visualize = false;
    static inline float visualize_color[4] = {1.0f, 0.0f, 0.0f, 0.3f};
  };

  static inline bool spinbot_enabled = false;

  static inline bool noclip = false;
  static inline int noclip_mode = 0;

  static inline bool emote_changer = false;
  static inline int emote_id = 0;

  static inline bool animation_changer = false;
  static inline int animation_type = 0;

  static inline int idle_animation = 0;
  static inline int run_animation = 0;
  static inline int walk_animation = 0;
  static inline int jump_animation = 0;
  static inline int fall_animation = 0;
  static inline bool fly_enabled = false;
  static inline float fly_speed = 50.0f;
  static inline int flight_type = 0;

  static inline keybind speedkeybind{"speedkeybind"};
  static inline keybind jumppowerkeybind{"jumppowerkeybind"};
  static inline keybind flightkeybind{"flightkeybind"};

  static inline keybind noclipkeybind{"noclipkeybind"};
  static inline bool no_jump_cooldown = false;
  static inline keybind no_jump_cooldown_keybind{"no_jump_cooldown_keybind"};
  static inline keybind aimbotkeybind{"aimbotkeybind"};
  static inline keybind silentaimkeybind{"silentaimkeybind"};
  static inline keybind menukeybind{"menukeybind"};
  static inline keybind triggerbotkeybind{"triggerbotkeybind"};
  static inline keybind spinbotkeybind{"spinbotkeybind"};
  static inline int menukeybind_default_key = VK_INSERT;
  
  static inline bool show_keybind_list = true;

  static inline void update_keybinds() {
    aimbotkeybind.update();
    silentaimkeybind.update();
    speedkeybind.update();
    jumppowerkeybind.update();
    flightkeybind.update();
    noclipkeybind.update();
    no_jump_cooldown_keybind.update();
    triggerbotkeybind.update();
    spinbotkeybind.update();
  }

  static inline void init_menu_keybind() {
    if (menukeybind.key == 0) {
      menukeybind.key = menukeybind_default_key;
    }
  }

  struct visuals {
    static inline bool head_dot = false;
    static inline bool box = false;
    static inline int name_display_mode = 0;
    static inline bool tracers = false;
    static inline bool tracers_style = false;
    static inline bool name = false;
    static inline bool distance = false;
    static inline bool tool = false;
    static inline bool weapon_icon_esp = false;
    static inline float weapon_icon_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    static inline bool teamcheck = false;
    static inline bool chams = false;
    static inline bool forcefield_chams_self = false;
    static inline bool forcefield_chams_global = false;
    static inline bool health_bar = false;
    static inline bool sonar_waves = false;
    static inline bool sonar = false;
    static inline bool china_rotation = false;
    static inline bool china_rainbow = false;
    static inline bool fog = false;
    static inline float fog_start = 0.0f;
    static inline float fog_end = 1000.0f;
    static inline float fog_color[4] = {0.5f, 0.5f, 0.5f, 1.0f};
    
    static inline bool bullet_tracers_enabled = false;
    static inline int bullet_tracer_style = 0; // 0 = Beam, 1 = Swirl, 2 = Lightning
    static inline float bullet_tracer_color[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    static inline float bullet_tracer_thickness = 2.0f;
    static inline float bullet_tracer_lifetime = 2000.0f; // milliseconds
    static inline float bullet_tracer_fade_start = 1500.0f; // when to start fading (ms)
    static inline bool bullet_tracer_outline = true;

    static inline bool crosshair_enabled = false;
    static inline float crosshair_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    static inline float crosshair_size = 10.0f;
    static inline float crosshair_gap = 5.0f;
    static inline bool crosshair_gapTween = false;
    static inline float crosshair_gapSpeed = 1.0f;
    static inline float crosshair_thickness = 1.0f;
    static inline int crosshair_styleIdx = 0;
    static inline float crosshair_baseSpeed = 100.0f;
    static inline float crosshair_fadeDuration = 1.0f;
    static inline bool death_check = false;
    static inline bool allow_local_player = false;
    static inline int player_filter_mode =
        0; // 0 = Everyone Except Local, 1 = Local Player, 2 = Team Check
    static inline bool rig_type = false;
    static inline bool skeleton = false;
    static inline float skeleton_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    static inline float skeleton_thickness = 0.8f;
    static inline bool arsenal_skin_changer = false;
    static inline int selected_melee_index = 0;
    static inline std::vector<std::string> arsenal_melees = {
    "None", "Karambit", "Sickle", "Butterfly Knife", "Tomahawk", "Tactical Knife", "ACT Trophy",
    "Combat Knife", "The Ghostwalker", "SuperSpaceKatana", "Peppermint Slicer", "Blossoming Femur",
    "Handy Candy", "Crucible", "Grumpy Hammer", "Coal Scythe", "Crab Claw", "Divinity",
    "Literal Melee", "When Day Breaks", "Bunny Staff", "Wired Bat", "Skele Scythe", "Big Sip",
    "Death's Blade", "Silver Bell", "Halberd", "Annihilator's Broken Sword", "Pipe Wrench Shank",
    "Synthlight Greatsword", "Harvester", "Rapier", "Moai", "Loaf", "Reliable Hammer",
    "Doublade", "Nomad's Blade", "Rusty Pipe", "Digi-Blade", "Daito", "Delinquent Pop",
    "Spring Greatsword", "Racket", "Sip O' Stink", "Ghost Ripper", "Moderation Hammer",
    "Easter Cleaver", "Kunai", "Stranger's Handblades", "Wooden Spoon", "Pumpkin Bucket",
    "Toy Tree", "Slicecicle", "Garlic Kebab", "Stop Sign", "Brass Knuckles", "Sabre",
    "Machete", "Claws", "Kitchen Knife", "Newspaper", "Endbringer", "Equip", "Katar",
    "ACT Trophy S6", "Icicle", "The Scrambler", "Reclaimer", "The Venomshank", "Crowbar",
    "Frostweaver's Wand", "Katana", "Bouquet", "Guitar", "Gingerbread Knife", "Electro Axe",
    "The Windforce", "Swordfish", "Doodle Sign", "Smug Egg", "Bloxy", "Rubber Hammer",
    "Shovel", "Coal Sword", "Leader's Axe", "Kukri", "The Firebrand", "Bat Axe", "Fish",
    "Slappy", "Bat", "Drill-Shear Skewer", "Electronic Stake", "The Fool's Tool",
    "Balloon Sword", "Beast Hammer", "The Illumina", "Merry Masher", "Seal", "Da Melee",
    "Brick", "Energy Katar", "Baton", "Calculator", "Khopesh", "Egg", "Rebel's Bat",
    "OG Space Katana", "Mop", "The Ice Dagger", "Candle Sword", "Candleabra",
    "Earth Cleaver", "Blade", "Bone Club", "Makeshift Axe", "R.A.M", "Wrench", "Pencil",
    "Heart Break", "Paddle", "Pumpkin Staff", "Peppermint Hammer", "Carrot", "Frog",
    "Stinger", "Hallow's Scythe", "Coral Blade", "Makeshift Saw", "Energy Blade",
    "Pumpkin Axe", "Plane", "Fisticuffs", "Candy Cane Claws", "Roughian's Pipe",
    "FOAM BLADE 3000", "Pan", "Divine Medallions", "Sledgehammer", "Rokia Hammer",
    "Scythe", "Pitchfork", "Golden Rings", "Naginata", "Ban Hammer", "Electric Flail",
    "Night's Edge", "Classic Sword", "Bone Karambit", "Starfire Staff", "Candy Cane",
    "Space KatanaOLD", "Aged Shovel", "The Darkheart", "Skull Pal", "Mittens", "Saber",
    "Assimilator", "Handblades", "Space Katana", "Killbrick Melee", "Chainsaw",
    "Swift End", "Candy Cane Sword", "Banana", "Hero's Sword", "Can Mace",
    "Paint Brush", "Gaster Blaster", "Fire Poker", "Glacier Blade" };
  };

  static inline std::vector<Engine::PlayerIns> player_cache{};
  static inline std::mutex cached_players_mutex;

  static inline std::vector<Engine::PlayerIns> custom_player_cache{};
  static inline std::mutex custom_players_mutex;

  static inline std::vector<bool> aimbot_checks = {
      false, false, false}; // Team Check, Wall Check, FOV Check

  static inline bool dex_explorer_enabled = false;
  static inline bool esp = false;
  static inline bool silent_aim = false;
  static inline bool silent_use_fov = true;
  static inline float silent_fov = 100.f;
  static inline float hitchance = 100.f;
  static inline int silent_hitbox = 0;
  static inline bool show_silent_fov = false;
  static inline float silent_fov_color[4] = {1.f, 1.f, 1.f, 1.f};
  static inline bool skeletons = false;
  static inline float skeleton_color[4] = {1.f, 1.f, 1.f, 1.f};
  static inline bool view_angles = false;
  static inline bool hitbox_visualizer = false;
  static inline float hitbox_color[4] = {1.f, 1.f, 1.f, 1.f};
  static inline int selected_player = -1;
  static inline float spin_speed = 100.f;
  static inline bool ko_check = false;
  static inline bool team_prediction = false;
  static inline bool show_fov = false;
  static inline float fov_color[4] = {1.f, 1.f, 1.f, 1.f};
  static inline float aimbot_fov = 100.f;
  static inline float smoothness = 10.f;
  static inline int aim_hitbox = 0;
  static inline bool sticky_aim = false;
  static inline bool triggerbot = false;
  static inline float triggerbot_delay = 0.0f;
  static inline float triggerbot_range = 10.0f;
  
  static inline bool rapidfire_enabled = false;

  struct discord_rpc {
    static inline bool enabled = true;
    static inline char client_id[32] = "1455713142897250556";
    static inline char details[64] = "discord.gg/scathe";
    static inline char state[64] = "Join Scathe The Best Cheat!";
    static inline char large_image_key[32] = "large_image_key";
    static inline char small_image_key[32] = "small_image_key";
  };

  static inline uintptr_t aimbot_target_addr = 0;
  static inline uintptr_t silent_target_addr = 0;
};