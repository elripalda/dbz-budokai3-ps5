// DBZ Budokai 3 PS5 - trophies (see hub_trophies.h).
#include "hub_trophies.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <sstream>

namespace budokai3::trophies {

namespace fs = std::filesystem;

namespace {

constexpr char kCatalogName[] = "4E4D0856-trophies.txt";
constexpr char kUnlocksName[] = "4E4D0856.toml";  // (the runtime's)

std::string Clean(std::string s) {
  for (char& c : s)
    if (c == '\t' || c == '\n' || c == '\r') c = ' ';
  return s;
}

std::vector<std::string> Split(const std::string& line, char sep) {
  std::vector<std::string> out;
  size_t a = 0;
  for (;;) {
    const size_t b = line.find(sep, a);
    out.push_back(line.substr(a, b == std::string::npos ? std::string::npos : b - a));
    if (b == std::string::npos) return out;
    a = b + 1;
  }
}

std::string ReadFile(const fs::path& p) {
  std::ifstream in(p, std::ios::binary);
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// The runtime's unlocks: [unlocked.<id>] filetime = <Windows file time>.
std::map<uint32_t, int64_t> Unlocks(const fs::path& file) {
  std::map<uint32_t, int64_t> out;
  std::ifstream in(file);
  int64_t id = -1;
  for (std::string line; std::getline(in, line);) {
    const size_t a = line.find_first_not_of(" \t");
    if (a == std::string::npos || line[a] == '#') continue;
    if (line.compare(a, 10, "[unlocked.") == 0) {
      id = std::strtoll(line.c_str() + a + 10, nullptr, 10);
      if (id > 0) out[uint32_t(id)] = 1;  // (earned; the time below)
      continue;
    }
    if (line[a] == '[') {
      id = -1;
      continue;
    }
    if (id > 0 && line.compare(a, 8, "filetime") == 0) {
      const size_t eq = line.find('=');
      if (eq != std::string::npos) {
        const int64_t t = UnixTime(std::strtoull(line.c_str() + eq + 1, nullptr, 10));
        if (t > 0) out[uint32_t(id)] = t;
      }
    }
  }
  return out;
}

}  // namespace

Grade GradeFor(uint32_t gamerscore) {
  if (gamerscore <= 15) return Grade::kBronze;
  if (gamerscore <= 30) return Grade::kSilver;
  return Grade::kGold;
}

const char* GradeName(Grade grade) {
  switch (grade) {
    case Grade::kBronze: return "Bronze";
    case Grade::kSilver: return "Silver";
    case Grade::kGold: return "Gold";
    case Grade::kPlatinum: return "Platinum";
  }
  return "";
}

bool Here(uint32_t id) { return id >= 21 && id <= 43; }

int64_t UnixTime(uint64_t filetime) {
  constexpr uint64_t kEpoch = 116444736000000000ull;  // 1970 in Windows file time
  return filetime > kEpoch ? int64_t((filetime - kEpoch) / 10000000ull) : 0;
}

int List::Percent() const {
  if (!points) return 0;
  if (Platinum()) return 100;
  const int p = int(std::lround(100.0 * points_earned / points));
  return std::clamp(p, points_earned ? 1 : 0, 99);
}

fs::path Folder(const fs::path& user_data) { return user_data / "achievements"; }

fs::path IconFile(const fs::path& user_data, uint32_t image_id) {
  return Folder(user_data) / "4E4D0856-icons" / (std::to_string(image_id) + ".png");
}

fs::path TitleIconFile(const fs::path& user_data) { return Folder(user_data) / "4E4D0856-icons" / "title.png"; }

List Load(const fs::path& user_data) {
  List list;
  std::ifstream in(Folder(user_data) / kCatalogName);
  for (std::string line; std::getline(in, line);) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line.empty() || line[0] == '#') continue;
    const auto f = Split(line, '\t');
    if (f.size() < 7) continue;
    Trophy t;
    t.id = uint32_t(std::strtoul(f[0].c_str(), nullptr, 10));
    t.gamerscore = uint32_t(std::strtoul(f[1].c_str(), nullptr, 10));
    t.flags = uint32_t(std::strtoul(f[2].c_str(), nullptr, 10));
    t.image_id = uint32_t(std::strtoul(f[3].c_str(), nullptr, 10));
    t.name = f[4], t.description = f[5], t.locked_description = f[6];
    if (Here(t.id)) list.trophies.push_back(std::move(t));
  }
  list.loaded = !list.trophies.empty();
  const auto unlocks = Unlocks(Folder(user_data) / kUnlocksName);
  for (Trophy& t : list.trophies) {
    if (auto it = unlocks.find(t.id); it != unlocks.end()) t.earned = it->second;
    const int g = int(t.grade());
    ++list.earnable, ++list.grade_total[g];
    list.points += t.gamerscore;
    if (t.earned) {
      ++list.earned, ++list.grade_earned[g];
      list.points_earned += t.gamerscore;
      list.last_earned = std::max(list.last_earned, t.earned);
    }
  }
  return list;
}

bool WriteCatalog(const fs::path& user_data, const std::vector<Trophy>& trophies) {
  std::string text =
      "# Budokai 3's achievements (the game writes this when it starts; the hub's Trophies page reads it)\n"
      "# id, gamerscore, flags, image, name, description, description while locked (a tab between each)\n";
  for (const Trophy& t : trophies)
    text += std::to_string(t.id) + "\t" + std::to_string(t.gamerscore) + "\t" + std::to_string(t.flags) + "\t" +
            std::to_string(t.image_id) + "\t" + Clean(t.name) + "\t" + Clean(t.description) + "\t" +
            Clean(t.locked_description) + "\n";
  const fs::path file = Folder(user_data) / kCatalogName;
  if (ReadFile(file) == text) return true;
  std::error_code ec;
  fs::create_directories(file.parent_path(), ec);
  const fs::path tmp = file.string() + ".tmp";
  {
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out << text;
  }
  fs::rename(tmp, file, ec);
  return !ec;
}

// ----------------------------------------------------------------- the cup

namespace {

struct Shades {
  ImU32 base, light, dark;
};

ImU32 Col(int r, int g, int b, float a) { return IM_COL32(r, g, b, int(std::clamp(a, 0.0f, 1.0f) * 255)); }

ImU32 WithAlphaOf(ImU32 c, float k) {
  const int a = int((c >> IM_COL32_A_SHIFT) & 0xFF);
  return (c & ~IM_COL32_A_MASK) | (ImU32(std::clamp(int(a * k), 0, 255)) << IM_COL32_A_SHIFT);
}

// An ellipse as a polygon (from angle a0 to a1).
std::vector<ImVec2> Ellipse(const std::function<ImVec2(float, float)>& P, float cx, float cy, float rx, float ry,
                            float a0 = 0, float a1 = 6.2831853f) {
  std::vector<ImVec2> pts;
  for (int i = 0; i <= 20; ++i) {
    const float t = a0 + (a1 - a0) * float(i) / 20.0f;
    pts.push_back(P(cx + rx * std::cos(t), cy + ry * std::sin(t)));
  }
  return pts;
}

Shades ShadesOf(Grade g, float a) {
  switch (g) {
    case Grade::kBronze: return {Col(206, 128, 78, a), Col(244, 184, 136, a), Col(146, 82, 44, a)};
    case Grade::kSilver: return {Col(186, 192, 204, a), Col(246, 248, 252, a), Col(124, 130, 144, a)};
    case Grade::kGold: return {Col(234, 182, 60, a), Col(255, 232, 146, a), Col(172, 122, 24, a)};
    case Grade::kPlatinum: return {Col(168, 198, 234, a), Col(240, 247, 255, a), Col(104, 134, 176, a)};
  }
  return {};
}

}  // namespace

ImU32 GradeColor(Grade grade, float alpha) { return ShadesOf(grade, alpha).base; }

namespace {

ImU32 MixCol(ImU32 a, ImU32 b, float t) {
  ImU32 out = 0;
  for (int sh = 0; sh < 32; sh += 8) {
    const float ca = float((a >> sh) & 0xFF), cb = float((b >> sh) & 0xFF);
    out |= ImU32(ca + (cb - ca) * t + 0.5f) << sh;
  }
  return out;
}

void CupShapes(ImDrawList* dl, ImVec2 c, float height, const Shades& s, bool shine) {
  const float u = height / 24.0f;
  const std::function<ImVec2(float, float)> P = [&](float x, float y) { return ImVec2(c.x + x * u, c.y + y * u); };
  // the handles (behind the bowl)
  for (int side : {-1, 1}) {
    dl->PathClear();
    const float cx = 6.6f * side, cy = -6.4f;
    const float a0 = side > 0 ? -1.75f : 1.39f, a1 = side > 0 ? 1.75f : 4.89f;
    dl->PathArcTo(P(cx, cy), 3.7f * u, a0, a1, 18);
    dl->PathStroke(s.dark, 0, 1.9f * u);
  }
  // the bowl: a rim, then down to a rounded bottom
  std::vector<ImVec2> bowl;
  for (int i = 0; i <= 24; ++i) {
    const float t = 3.14159265f * float(i) / 24.0f;
    const float k = std::max(0.0f, std::sin(t));  // (sin(pi) is a hair below 0: no NaN from the pow below)
    // (straighter sides near the rim than a half ellipse)
    const float x = 7.6f * std::cos(t) * (0.82f + 0.18f * k), y = -10.8f + 13.2f * std::pow(k, 1.25f);
    bowl.push_back(P(x, y));
  }
  dl->AddConvexPolyFilled(bowl.data(), int(bowl.size()), s.base);
  dl->AddRectFilled(P(-8.4f, -12.2f), P(8.4f, -10.4f), s.light, 0.6f * u);
  if (shine) {  // a slim curve of light down the left of the bowl, and a little shade on the right
    const auto shade = Ellipse(P, 3.3f, -4.8f, 2.2f, 4.8f, -1.5708f, 1.5708f);
    dl->AddConvexPolyFilled(shade.data(), int(shade.size()), WithAlphaOf(s.dark, 0.45f));
    const auto glint = Ellipse(P, -4.0f, -5.6f, 1.1f, 3.4f);
    dl->AddConvexPolyFilled(glint.data(), int(glint.size()), WithAlphaOf(s.light, 0.75f));
  }
  // the stem, its collar and the base
  const ImVec2 stem[] = {P(-1.25f, 2.0f), P(1.25f, 2.0f), P(1.9f, 6.4f), P(-1.9f, 6.4f)};
  dl->AddConvexPolyFilled(stem, 4, s.dark);
  dl->AddRectFilled(P(-3.8f, 5.9f), P(3.8f, 7.5f), s.base, 0.7f * u);
  dl->AddRectFilled(P(-6.4f, 7.8f), P(6.4f, 11.6f), s.base, 1.0f * u);
  if (shine) dl->AddRectFilled(P(-6.4f, 10.2f), P(6.4f, 11.6f), WithAlphaOf(s.dark, 0.6f), 1.0f * u, ImDrawFlags_RoundCornersBottom);
}

}  // namespace

void DrawCup(ImDrawList* dl, ImVec2 c, float height, Grade grade, float alpha, bool lit, bool on_light) {
  Shades s = ShadesOf(grade, alpha);
  if (!lit) {  // not earned: its colours, greyed and quieter
    const ImU32 grey = Col(96, 98, 106, alpha);
    s = {MixCol(s.base, grey, 0.6f), MixCol(s.light, grey, 0.6f), MixCol(s.dark, grey, 0.6f)};
  }
  if (on_light) {  // on a light row: a soft dark edge first, so a silver one still reads
    const ImU32 edge = Col(0, 0, 0, 0.22f * alpha);
    CupShapes(dl, ImVec2(c.x, c.y + height * 0.01f), height * 1.12f, {edge, edge, edge}, false);
  }
  CupShapes(dl, c, height, s, lit);
}

// --------------------------------------------------------------- the card

namespace {

ImU32 WhiteA(float a) { return IM_COL32(255, 255, 255, int(std::clamp(a, 0.0f, 1.0f) * 255)); }
ImU32 BlackA(float a) { return IM_COL32(0, 0, 0, int(std::clamp(a, 0.0f, 1.0f) * 255)); }

// `t` cut to `width` (with "..."), in whole UTF-8 letters.
std::string FitText(ImFont* f, float size, std::string t, float width) {
  auto w = [&](const std::string& s) { return f->CalcTextSizeA(size, FLT_MAX, 0, s.c_str()).x; };
  if (w(t) <= width) return t;
  while (!t.empty() && w(t + "...") > width) {
    t.pop_back();
    while (!t.empty() && (static_cast<unsigned char>(t.back()) & 0xC0) == 0x80) t.pop_back();
  }
  return t + "...";
}

// Letter-spaced text (the small capitals line).
void Spaced(ImDrawList* dl, ImFont* f, float size, ImVec2 p, ImU32 col, const char* t, float spacing) {
  for (const char* c = t; *c; ++c) {
    dl->AddText(f, size, p, col, c, c + 1);
    p.x += f->CalcTextSizeA(size, FLT_MAX, 0, c, c + 1).x + spacing;
  }
}

}  // namespace

void DrawCard(ImDrawList* dl, ImVec2 display, const Card& card, float age, ImFont* body, ImFont* bold) {
  constexpr float kIn = 0.45f, kOut = 0.45f;
  const float show = card.Seconds();
  if (age < 0 || age >= show) return;
  // in from the right, easing out; away to the right, fading
  float alpha = 1, slide = 0;
  const float k = display.y / 1080.0f;
  if (age < kIn) {
    const float t = age / kIn, e = 1 - (1 - t) * (1 - t) * (1 - t);
    alpha = e, slide = (1 - e) * 70 * k;
  } else if (age > show - kOut) {
    const float t = std::clamp((show - age) / kOut, 0.0f, 1.0f);
    alpha = t * t * (3 - 2 * t), slide = (1 - t) * 26 * k;
  }
  const float w = 548 * k, h = 106 * k, r = 18 * k;
  const ImVec2 p0(display.x - 54 * k - w + slide, 50 * k), p1(p0.x + w, p0.y + h);
  // a soft shadow, the card, a fine edge
  for (int i = 1; i <= 6; ++i) {
    const float g = float(i) * 3.0f * k;
    dl->AddRectFilled(ImVec2(p0.x - g, p0.y - g + 5 * k), ImVec2(p1.x + g, p1.y + g + 5 * k), BlackA(0.05f * alpha), r + g);
  }
  dl->AddRectFilled(p0, p1, IM_COL32(30, 31, 36, int(242 * alpha)), r);
  dl->AddRectFilledMultiColor(ImVec2(p0.x + r, p0.y + 1 * k), ImVec2(p1.x - r, p0.y + 2.2f * k), WhiteA(0.07f * alpha),
                              WhiteA(0.07f * alpha), WhiteA(0), WhiteA(0));
  dl->AddRect(p0, p1, WhiteA(0.09f * alpha), r, 0, 1.2f * k);

  // the picture
  const float is = 78 * k;
  const ImVec2 i0(p0.x + 14 * k, p0.y + (h - is) * 0.5f), i1(i0.x + is, i0.y + is);
  if (card.picture) {
    dl->AddImageRounded(card.picture, i0, i1, ImVec2(0, 0), ImVec2(1, 1), WhiteA(alpha), 13 * k);
  } else {
    dl->AddRectFilled(i0, i1, IM_COL32(46, 48, 56, int(255 * alpha)), 13 * k);
    DrawCup(dl, ImVec2((i0.x + i1.x) * 0.5f, (i0.y + i1.y) * 0.5f), is * 0.56f, card.grade, alpha);
  }

  // the words: what it is (with the grade's cup), its name, and its grade
  const float tx = i1.x + 20 * k, tw = p1.x - 22 * k - tx;
  DrawCup(dl, ImVec2(tx + 8 * k, p0.y + 27 * k), 20 * k, card.grade, alpha);
  Spaced(dl, bold, 15 * k, ImVec2(tx + 24 * k, p0.y + 18 * k), WhiteA(0.62f * alpha),
         card.platinum ? "PLATINUM TROPHY" : "TROPHY EARNED", 1.8f * k);
  dl->AddText(bold, 25 * k, ImVec2(tx, p0.y + 40 * k), WhiteA(alpha), FitText(bold, 25 * k, card.name, tw).c_str());
  const std::string sub = card.platinum ? "Every Budokai 3 trophy"
                                        : std::string(GradeName(card.grade)) + "  \xC2\xB7  Dragon Ball Z Budokai 3";
  dl->AddText(body, 17 * k, ImVec2(tx, p0.y + 74 * k), GradeColor(card.grade, 0.9f * alpha),
              FitText(body, 17 * k, sub, tw).c_str());
}

}  // namespace budokai3::trophies
