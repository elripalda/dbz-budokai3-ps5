// DBZ Budokai 3 PS5 - the stubs ps5_shims.h renames those calls to.
#if defined(__PROSPERO__)
#include "ps5_shims.h"

#include <cerrno>
#include <cstdio>
#include <ifaddrs.h>
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>

extern "C" {

int posix_spawn(pid_t*, const char*, const posix_spawn_file_actions_t*, const posix_spawnattr_t*, char* const[],
                char* const[]) {
  return ENOSYS;
}
int posix_spawnp(pid_t*, const char*, const posix_spawn_file_actions_t*, const posix_spawnattr_t*, char* const[],
                 char* const[]) {
  return ENOSYS;
}
int posix_spawn_file_actions_init(posix_spawn_file_actions_t*) { return 0; }
int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t*) { return 0; }
int posix_spawn_file_actions_addopen(posix_spawn_file_actions_t*, int, const char*, int, mode_t) { return 0; }
int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t*, int, int) { return 0; }
int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t*, int) { return 0; }
pid_t waitpid(pid_t, int* status, int) {
  if (status) *status = 0;
  errno = ECHILD;
  return -1;
}
FILE* popen(const char*, const char*) {
  errno = ENOSYS;
  return nullptr;
}
int pclose(FILE*) { return -1; }
int getifaddrs(struct ifaddrs** out) {
  if (out) *out = nullptr;
  errno = ENOSYS;
  return -1;
}
void freeifaddrs(struct ifaddrs*) {}

}  // extern "C"
#endif
