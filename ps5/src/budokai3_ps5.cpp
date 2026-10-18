// DBZ Budokai 3 PS5 - see budokai3_ps5.h.
#include "budokai3_ps5.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>

#include <rex/cvar.h>
#include <rex/logging.h>

#include "launcher/settings.h"
#include "ps_button_pack.h"

// The hub's settings (its Settings page), kept in dbz3.toml.
REXCVAR_DEFINE_INT32(budokai3_resolution, 2, "Budokai3",
                     "Picture: 1 = 720p (the Xbox 360's), 2 = 1440p, 3 = 4K (everything drawn at that scale)")
    .range(1, 3);
REXCVAR_DEFINE_BOOL(budokai3_show_fps, true, "Budokai3", "The game's frame rate in the corner");
REXCVAR_DEFINE_INT32(budokai3_language, 1, "Budokai3", "Text language: 1 = English, 4 = French, 5 = Spanish")
    .range(1, 6);
REXCVAR_DEFINE_STRING(budokai3_button_prompts, "playstation", "Budokai3",
                      "The game's button pictures: playstation (DualSense) or xbox (the game's own)");
REXCVAR_DEFINE_INT32(hub_music_volume, 50, "Budokai3", "The hub's menu music, 0-100 (%)");

namespace budokai3 {

namespace {

constexpr char kConfigVersion[] = "(settings v2)";
constexpr char kDefaultConfig[] =
    "# DBZ Budokai 3 - settings (settings v2)\n"
    "gpu_plugin = \"xenos\"\n"
    "# 1 = 720p, 2 = 1440p, 3 = 4K\n"
    "budokai3_resolution = 2\n"
    "anisotropic_override = 3\n"
    "# text language: 1 English, 4 French, 5 Spanish\n"
    "budokai3_language = 1\n"
    "# the game's button pictures: playstation or xbox\n"
    "budokai3_button_prompts = \"playstation\"\n"
    "# the frame rate in the corner\n"
    "budokai3_show_fps = true\n"
    "# the hub's menu music, 0-100 (%)\n"
    "hub_music_volume = 50\n"
    "vsync = true\n"
    "# (a window, not fullscreen: fullscreen gives the hub and overlays a 4:3 screen on the PS5)\n"
    "fullscreen = false\n"
    "# (the PS5's GPU driver: one plain buffer for the Xbox 360's memory, no sparse binding)\n"
    "vulkan_sparse_shared_memory = false\n"
    "vulkan_async_skip_incomplete_frames = false\n"
    "vulkan_allow_present_mode_immediate = false\n";

}  // namespace

void PrepareSettingsFile(const std::filesystem::path& path) {
  std::string text;
  {
    std::ifstream in(path);
    text.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  }
  const std::string first = text.substr(0, text.find('\n'));
  if (first.find(kConfigVersion) != std::string::npos) return;
  std::ofstream(path, std::ios::trunc) << kDefaultConfig;
  REXLOG_INFO("[budokai3] wrote the default settings ({})", path.string());
}

const char* ResolutionName() {
  switch (REXCVAR_GET(budokai3_resolution)) {
    case 1: return "720p";
    case 3: return "4K";
    default: return "1440p";
  }
}

void ApplyHubChoices() {
  const int32_t scale = std::clamp(REXCVAR_GET(budokai3_resolution), 1, 3);
  dbz3::settings::SetResolutionScale(scale);
  rex::cvar::SetFlagByName("dbz3_language", std::to_string(REXCVAR_GET(budokai3_language)));
  rex::cvar::SetFlagByName("user_language", std::to_string(REXCVAR_GET(budokai3_language)));
  dbz3::settings::SetInputGlyphs(REXCVAR_GET(budokai3_button_prompts) == "xbox" ? "xbox" : "playstation");
  rex::cvar::SetFlagByName("dbz3_show_fps", REXCVAR_GET(budokai3_show_fps) ? "true" : "false");
  // the gold borders at the sides of the game's menus stay on (the PC's Golden borders)
  rex::cvar::SetFlagByName("dbz3_ui_side_borders", "true");
  // the PC launcher's anisotropic setting, from the hub's (the runtime's own cvar in dbz3.toml)
  rex::cvar::SetFlagByName("dbz3_anisotropic", rex::cvar::GetFlagByName("anisotropic_override"));
  // (the PC's choices that don't apply on the PS5)
  rex::cvar::SetFlagByName("dbz3_region", "us");
  rex::cvar::SetFlagByName("dbz3_gpu_backend", "vulkan");
  rex::cvar::SetFlagByName("dbz3_present_effect", "bilinear");
  rex::cvar::SetFlagByName("dbz3_frame_cap", "60");
  rex::cvar::SetFlagByName("dbz3_mnk_mode", "false");
  rex::cvar::SetFlagByName("dbz3_dim_unfocused", "false");
  rex::cvar::SetFlagByName("dbz3_update_check", "false");
  dbz3::settings::SaveUserSettings();
  REXLOG_INFO("[budokai3] hub: resolution {}, language {}, buttons {}, fps counter {}", ResolutionName(),
              REXCVAR_GET(budokai3_language), REXCVAR_GET(budokai3_button_prompts),
              REXCVAR_GET(budokai3_show_fps) ? "on" : "off");
}

void ApplyPs5RuntimeSettings() {
  const int32_t scale = std::clamp(REXCVAR_GET(budokai3_resolution), 1, 3);
  rex::cvar::SetFlagByName("resolution_scale", std::to_string(scale));
  // The PS5 sends 4K to the TV and vsync paces the frames: no frame cap of the
  // PC's (it would pace twice), and the plain present (the PC's FSR / CAS are
  // its D3D12 shaders).
  rex::cvar::SetFlagByName("frame_cap", "0");
  rex::cvar::SetFlagByName("present_effect", "bilinear");
  rex::cvar::SetFlagByName("swap_post_effect", "none");
  rex::cvar::SetFlagByName("vsync", "true");
  // Vblanks on time (the default timer fires them up to a millisecond late).
  rex::cvar::SetFlagByName("guest_vblank_hz", "60");
  rex::cvar::SetFlagByName("vulkan_sparse_shared_memory", "false");
  REXLOG_INFO("[budokai3] PS5 runtime settings: resolution_scale={} ({}), vblank 60 Hz", scale, ResolutionName());
  // the PlayStation button pictures (before the GPU first reads the pack list)
  PrepareButtonPack();
}

}  // namespace budokai3
