// DBZ Budokai 3 PS5 - see ps_button_pack.h.
#include "ps_button_pack.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <mutex>
#include <string>

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/logging.h>
#include <rex/ui/image_decode.h>

#include "ingame/native_ui.h"
#include "region.h"

REXCVAR_DECLARE(std::string, budokai3_button_prompts);

namespace budokai3 {
namespace fs = std::filesystem;

namespace {

// The BACK texture: data_usi 2033, 256x64 (its button at 76-127 x 0-24, the
// words BACK below it and ": Help" to the right).
constexpr uint64_t kBackHash = 0x9607A868D438558Eull;
constexpr int kBackEntry = 2033;
constexpr int kBackW = 256, kBackH = 64;
constexpr int kScale = 4;                                   // the pack's
constexpr char kBackFile[] = "9607A868D438558E_256x64_RGBA8.dds";

struct Picture {
  int w = 0, h = 0;
  std::vector<uint8_t> px;   // RGBA
};

std::mutex g_mu;
Picture g_back;   // the BACK picture made at start, for the port's screens

fs::path Dir() { return rex::filesystem::GetExecutableFolder() / "ps_buttons"; }
fs::path PackDir() { return Dir() / "pack"; }

bool PlayStation() { return REXCVAR_GET(budokai3_button_prompts) != "xbox"; }

bool LoadImage(const fs::path& path, Picture& out) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return false;
  const std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  out.px = rex::ui::DecodeImageRGBA(data.data(), data.size(), out.w, out.h);
  return !out.px.empty();
}

// A picture at another size, in premultiplied alpha (clean edges): bilinear
// when it grows, the average of each block when it shrinks by a whole factor.
Picture Resized(const Picture& src, int w, int h) {
  Picture out{w, h, std::vector<uint8_t>(size_t(w) * h * 4)};
  auto pm = [&](int x, int y, float* c) {
    x = std::clamp(x, 0, src.w - 1);
    y = std::clamp(y, 0, src.h - 1);
    const uint8_t* p = &src.px[(size_t(y) * src.w + x) * 4];
    const float a = p[3] / 255.0f;
    c[0] = p[0] * a;
    c[1] = p[1] * a;
    c[2] = p[2] * a;
    c[3] = p[3];
  };
  const bool shrink = src.w >= w && src.w % w == 0 && src.h % h == 0 && src.w / w == src.h / h;
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      float acc[4] = {0, 0, 0, 0}, c[4];
      if (shrink) {
        const int k = src.w / w;
        for (int j = 0; j < k; ++j)
          for (int i = 0; i < k; ++i) {
            pm(x * k + i, y * k + j, c);
            for (int n = 0; n < 4; ++n) acc[n] += c[n] / float(k * k);
          }
      } else {
        const float sx = (x + 0.5f) * src.w / w - 0.5f, sy = (y + 0.5f) * src.h / h - 0.5f;
        const int x0 = int(std::floor(sx)), y0 = int(std::floor(sy));
        const float fx = sx - x0, fy = sy - y0;
        const float wt[4] = {(1 - fx) * (1 - fy), fx * (1 - fy), (1 - fx) * fy, fx * fy};
        const int xs[4] = {x0, x0 + 1, x0, x0 + 1}, ys[4] = {y0, y0, y0 + 1, y0 + 1};
        for (int q = 0; q < 4; ++q) {
          pm(xs[q], ys[q], c);
          for (int n = 0; n < 4; ++n) acc[n] += c[n] * wt[q];
        }
      }
      uint8_t* o = &out.px[(size_t(y) * w + x) * 4];
      const float a = acc[3] / 255.0f;
      for (int n = 0; n < 3; ++n) o[n] = uint8_t(std::clamp(a > 0.001f ? acc[n] / a : 0.0f, 0.0f, 255.0f) + 0.5f);
      o[3] = uint8_t(std::clamp(acc[3], 0.0f, 255.0f) + 0.5f);
    }
  }
  return out;
}

// `top` over `base` (same size, straight alpha).
void Over(Picture& base, const Picture& top) {
  for (size_t i = 0; i < base.px.size(); i += 4) {
    const float ta = top.px[i + 3] / 255.0f, ba = base.px[i + 3] / 255.0f;
    const float a = ta + ba * (1 - ta);
    for (int n = 0; n < 3; ++n) {
      const float c = a > 0.0f ? (top.px[i + n] * ta + base.px[i + n] * ba * (1 - ta)) / a : 0.0f;
      base.px[i + n] = uint8_t(std::clamp(c, 0.0f, 255.0f) + 0.5f);
    }
    base.px[i + 3] = uint8_t(std::clamp(a * 255.0f, 0.0f, 255.0f) + 0.5f);
  }
}

// An HD pack's own BACK texture (its PNG), when one is installed: its words
// are sharper than the game's.
bool HdPackBack(Picture& out) {
  const std::string list = rex::cvar::GetFlagByName("dbz3_texture_packs");
  size_t pos = 0;
  while (pos < list.size()) {
    size_t sep = list.find(';', pos);
    if (sep == std::string::npos) sep = list.size();
    const fs::path dir = fs::u8path(list.substr(pos, sep - pos));
    pos = sep + 1;
    std::error_code ec;
    for (auto it = fs::recursive_directory_iterator(dir, ec); !ec && it != fs::recursive_directory_iterator();
         it.increment(ec)) {
      const std::string n = it->path().filename().string();
      if (n.size() < 20 || it->path().extension() != ".png") continue;
      std::string stem = n.substr(0, 16);
      std::transform(stem.begin(), stem.end(), stem.begin(), [](unsigned char c) { return char(std::toupper(c)); });
      if (stem == "9607A868D438558E" && LoadImage(it->path(), out) && out.w * kBackH == out.h * kBackW) {
        REXLOG_INFO("[budokai3] PS buttons: BACK from the HD pack ({})", it->path().string());
        return true;
      }
    }
  }
  return false;
}

// A 32-bit RGBA DDS (the pack reads it as it reads its PNGs).
bool WriteDds(const fs::path& path, const Picture& p) {
  uint8_t h[128] = {};
  auto put = [&](int at, uint32_t v) { std::memcpy(h + at, &v, 4); };
  std::memcpy(h, "DDS ", 4);
  put(4, 124);
  put(8, 0x100F);                       // caps, height, width, pitch, pixel format
  put(12, uint32_t(p.h));
  put(16, uint32_t(p.w));
  put(20, uint32_t(p.w) * 4);
  put(76, 32);
  put(80, 0x41);                        // RGB + alpha
  put(88, 32);
  put(92, 0x000000FF);
  put(96, 0x0000FF00);
  put(100, 0x00FF0000);
  put(104, 0xFF000000);
  put(108, 0x1000);                     // a texture
  const fs::path tmp = fs::path(path).concat(".tmp");
  {
    std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(h), sizeof(h));
    f.write(reinterpret_cast<const char*>(p.px.data()), std::streamsize(p.px.size()));
    if (!f) return false;
  }
  std::error_code ec;
  fs::rename(tmp, path, ec);
  return !ec;
}

void MakeBack() {
  Picture touch_pad;
  if (!LoadImage(Dir() / "touch_pad.png", touch_pad) || touch_pad.w != kBackW * kScale ||
      touch_pad.h != kBackH * kScale) {
    REXLOG_WARN("[budokai3] PS buttons: no touch_pad.png; BACK stays the game's");
    return;
  }
  Picture base;
  if (!HdPackBack(base)) {
    dbz3::ingame::native_ui::Image img;
    if (!dbz3::ingame::native_ui::ReadPiece(dbz3::HostDataRoot() / "us", kBackEntry, kBackHash, img) ||
        img.w != kBackW || img.h != kBackH) {
      REXLOG_WARN("[budokai3] PS buttons: the game's BACK texture wasn't found; it stays the game's");
      return;
    }
    base = {img.w, img.h, std::move(img.rgba)};
  }
  // the old button out (rows 0-24 of 76-127, at the picture's scale), the touch pad in
  const int k = base.w / kBackW;
  for (int y = 0; y < 25 * k && y < base.h; ++y)
    std::memset(&base.px[(size_t(y) * base.w + 76 * k) * 4], 0, size_t(52 * k) * 4);
  Picture back = Resized(base, kBackW * kScale, kBackH * kScale);
  Over(back, touch_pad);
  if (!WriteDds(PackDir() / kBackFile, back)) {
    REXLOG_WARN("[budokai3] PS buttons: couldn't write {}", (PackDir() / kBackFile).string());
  }
  std::lock_guard<std::mutex> lock(g_mu);
  g_back = std::move(back);
}

}  // namespace

namespace {
// A built-in pack first in the game's list (a small pack wins over an HD one).
void UsePack(const fs::path& dir) {
  std::string list = rex::cvar::GetFlagByName("dbz3_texture_packs");
  const std::string d = rex::path_to_utf8(dir);
  if (list.find(d) == std::string::npos) {
    list = list.empty() ? d : d + ";" + list;
    rex::cvar::SetFlagByName("dbz3_texture_packs", list);
  }
}
}  // namespace

void PrepareButtonPack() {
  std::error_code ec;
  if (!PlayStation() || !fs::is_directory(PackDir(), ec)) {
    REXLOG_INFO("[budokai3] PS buttons: off ({})", PlayStation() ? "no ps_buttons/pack" : "Xbox prompts");
    return;
  }
  MakeBack();
  UsePack(PackDir());
  REXLOG_INFO("[budokai3] PS buttons: on ('{}')", rex::cvar::GetFlagByName("dbz3_texture_packs"));
}

bool PsButtonPicture(const char* name, std::vector<uint8_t>& rgba, int& width, int& height, float& game_width,
                     float& game_height) {
  if (!PlayStation()) return false;
  struct Piece {
    const char* name;
    const char* file;
    float w, h;
  };
  static const Piece kPieces[] = {{"botones", "AEA34DDB47477372_512x64_RGBA8.png", 512, 64},
                                  {"boton_start", "E56FC5E5DEFE917B_64x64_RGBA8.png", 64, 64},
                                  {"boton_back", nullptr, float(kBackW), float(kBackH)}};
  for (const Piece& p : kPieces) {
    if (std::strcmp(p.name, name) != 0) continue;
    Picture pic;
    if (p.file) {
      if (!LoadImage(PackDir() / p.file, pic)) return false;
    } else {
      std::lock_guard<std::mutex> lock(g_mu);
      if (g_back.px.empty()) return false;
      pic = g_back;
    }
    width = pic.w;
    height = pic.h;
    rgba = std::move(pic.px);
    game_width = p.w;
    game_height = p.h;
    return true;
  }
  return false;
}

}  // namespace budokai3
