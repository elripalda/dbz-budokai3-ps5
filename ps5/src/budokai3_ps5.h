// DBZ Budokai 3 PS5 - what the PS5 port adds around NovaPowers' game code:
// the settings file a first start writes, the hub's choices handed to the
// game's own settings, and the runtime settings the PS5 needs.
#pragma once

#include <filesystem>

namespace budokai3 {

// The app's settings file (dbz3.toml beside the app): written when missing or
// from an older test build, the player's choices kept.
void PrepareSettingsFile(const std::filesystem::path& path);

// The hub's choices (Resolution, Text Language, Button Prompts, FPS Counter,
// Anisotropic Filter) passed to the game's own settings (dbz3_user.toml), so
// its start reads them as the PC launcher would leave them.
void ApplyHubChoices();

// After the game's settings reach the runtime (OnPostSetup): the ones the PS5
// needs instead (resolution scale, frame pacing, the plain present).
void ApplyPs5RuntimeSettings();

// "720p", "1440p" or "4K", for the log.
const char* ResolutionName();

}  // namespace budokai3
