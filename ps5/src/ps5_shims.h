// DBZ Budokai 3 PS5 - force-included into the game's own sources on the PS5.
//
// The PC port runs Python tools and unzip (the launcher's mod pipeline, the mod
// installer) and asks the network interfaces for the LAN address (online). A
// PS5 app can't start programs, and those libc calls aren't ones the console
// is known to have (a missing one traps the first time it's called), so they
// are renamed here, declarations and calls alike, to stubs that say "can't"
// (ps5_shims.cpp). The game itself never needs them.
#pragma once

#if defined(__PROSPERO__)
#define posix_spawn budokai3_ps5_no_posix_spawn
#define posix_spawnp budokai3_ps5_no_posix_spawnp
#define posix_spawn_file_actions_init budokai3_ps5_no_spawn_actions_init
#define posix_spawn_file_actions_destroy budokai3_ps5_no_spawn_actions_destroy
#define posix_spawn_file_actions_addopen budokai3_ps5_no_spawn_actions_addopen
#define posix_spawn_file_actions_adddup2 budokai3_ps5_no_spawn_actions_adddup2
#define posix_spawn_file_actions_addclose budokai3_ps5_no_spawn_actions_addclose
#define waitpid budokai3_ps5_no_waitpid
#define popen budokai3_ps5_no_popen
#define pclose budokai3_ps5_no_pclose
#define getifaddrs budokai3_ps5_no_getifaddrs
#define freeifaddrs budokai3_ps5_no_freeifaddrs
#endif
