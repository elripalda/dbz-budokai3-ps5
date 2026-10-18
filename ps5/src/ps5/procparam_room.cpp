// [ps5 port] Room at the start of the data segment (tools/ps5-pie-budokai3.ld:
// .ps5_room) for the PS5 process parameters ps5-native-tool writes into the
// file just before it, so they can't reach the game's data. Never read.
extern "C" {
__attribute__((section(".ps5_room"), used, aligned(16))) char budokai3_ps5_procparam_room[0x1000] = {1};
}
