// DBZ Budokai 3 PS5 - plain libc functions the earlier ports never imported
// from the console (inet_ntop, inet_pton, atoll, hypotf, islower, isupper),
// built into the app so the console's copies are never asked for. Hidden, so
// they stay the app's own (never exported in place of the console's): this
// file includes no system header, which would declare them first and keep
// them visible.
#if defined(__PROSPERO__)

#define BUDOKAI3_OWN extern "C" __attribute__((visibility("hidden")))

namespace {
constexpr int kAfInet = 2;  // FreeBSD's AF_INET
}

// (arpa/inet.h names them __inet_ntop / __inet_pton) IPv4 only: the online
// code's addresses are IPv4.
BUDOKAI3_OWN const char* __inet_ntop(int af, const void* src, char* dst, unsigned int size) {
  if (af != kAfInet || !src || !dst || size < 16) return nullptr;
  const auto* b = static_cast<const unsigned char*>(src);
  char* o = dst;
  for (int i = 0; i < 4; ++i) {
    const unsigned v = b[i];
    if (v >= 100) *o++ = char('0' + v / 100);
    if (v >= 10) *o++ = char('0' + v / 10 % 10);
    *o++ = char('0' + v % 10);
    if (i < 3) *o++ = '.';
  }
  *o = '\0';
  return dst;
}

BUDOKAI3_OWN int __inet_pton(int af, const char* src, void* dst) {
  if (af != kAfInet) return -1;
  unsigned char out[4];
  int part = 0;
  unsigned value = 0;
  bool digit = false;
  for (const char* p = src;; ++p) {
    if (*p >= '0' && *p <= '9') {
      value = value * 10 + unsigned(*p - '0');
      if (value > 255) return 0;
      digit = true;
    } else if (*p == '.' || *p == '\0') {
      if (!digit || part > 3) return 0;
      out[part++] = static_cast<unsigned char>(value);
      value = 0, digit = false;
      if (*p == '\0') break;
    } else {
      return 0;
    }
  }
  if (part != 4) return 0;
  auto* d = static_cast<unsigned char*>(dst);
  for (int i = 0; i < 4; ++i) d[i] = out[i];
  return 1;
}

BUDOKAI3_OWN long long atoll(const char* s) {
  while (*s == ' ' || (*s >= '\t' && *s <= '\r')) ++s;
  bool neg = false;
  if (*s == '+' || *s == '-') neg = *s++ == '-';
  unsigned long long v = 0;
  while (*s >= '0' && *s <= '9') v = v * 10 + unsigned(*s++ - '0');
  return neg ? -(long long)v : (long long)v;
}

BUDOKAI3_OWN float hypotf(float x, float y) {
  return float(__builtin_sqrt(double(x) * double(x) + double(y) * double(y)));
}

BUDOKAI3_OWN int islower(int c) { return c >= 'a' && c <= 'z'; }
BUDOKAI3_OWN int isupper(int c) { return c >= 'A' && c <= 'Z'; }

#endif
