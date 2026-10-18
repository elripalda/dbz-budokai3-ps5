// DBZ Budokai 3 PS5 - the hub's Trophies page (see hub_ui.h, hub_trophies.h):
// how far along you are, then the trophies - earned (newest first), then not
// yet - with the focused one's details.
#include <algorithm>
#include <cmath>
#include <ctime>
#include <string>

#include "hub_style.h"
#include "hub_ui.h"

namespace budokai3::hub {

namespace {

constexpr float kRowH = 86;

std::string When(int64_t t) {
  if (t <= 0) return "-";
  const std::time_t tt = std::time_t(t);
  std::tm tm{};
  localtime_r(&tt, &tm);
  char buf[48];
  std::strftime(buf, sizeof(buf), "%b %d, %Y  %H:%M", &tm);
  return buf;
}

const char* kGroupNames[] = {"EARNED", "NOT EARNED YET"};

}  // namespace

void Hub::LoadTrophies() {
  trophies_ = trophies::Load(paths_.user_data);
  trophy_items_.clear();
  if (!trophies_.loaded) return;
  const auto& all = trophies_.trophies;
  // earned: the platinum first, then the newest
  std::vector<TrophyItem> earned, rest;
  for (int i = 0; i < int(all.size()); ++i) {
    const auto& t = all[size_t(i)];
    if (t.earned) earned.push_back({i, 0});
    else rest.push_back({i, 1});
  }
  std::stable_sort(earned.begin(), earned.end(), [&](const TrophyItem& a, const TrophyItem& b) {
    return all[size_t(a.index)].earned > all[size_t(b.index)].earned;
  });
  const bool plat = trophies_.Platinum();
  if (plat) trophy_items_.push_back({-1, 0});
  trophy_items_.insert(trophy_items_.end(), earned.begin(), earned.end());
  if (!plat) trophy_items_.push_back({-1, 1});
  trophy_items_.insert(trophy_items_.end(), rest.begin(), rest.end());
  item_[kTrophiesTab] = std::clamp(item_[kTrophiesTab], 0, int(trophy_items_.size()) - 1);
}

// A trophy's picture (the platinum's: the game's own), loaded the first time it's drawn.
Image Hub::TrophyPicture(int index) {
  const uint32_t key = index < 0 ? 0 : std::max(1u, trophies_.trophies[size_t(index)].image_id);
  if (auto it = trophy_pictures_.find(key); it != trophy_pictures_.end()) return it->second;
  Image img;
  if (load_image_)
    img = load_image_(index < 0 ? trophies::TitleIconFile(paths_.user_data)
                                : trophies::IconFile(paths_.user_data, trophies_.trophies[size_t(index)].image_id));
  trophy_pictures_[key] = img;
  return img;
}

// The trophy's picture in a rounded square: dimmed until it's earned, and a
// question mark for a hidden one.
void Hub::TrophyIcon(int index, float x, float y, float size, float over) {
  const bool plat = index < 0;
  const trophies::Trophy* t = plat ? nullptr : &trophies_.trophies[size_t(index)];
  const bool earned = plat ? trophies_.Platinum() : t->earned != 0;
  const float r = size * 0.16f;
  if (t && !earned && t->hidden()) {
    Rect(x, y, size, size, Mix(Rgba(38, 40, 46), Rgba(206, 208, 214), over), r);
    Text(a_.display, size * 0.56f, x + size * 0.5f, y + size * 0.14f, Mix(White(0.5f), Black(0.45f), over), "?", 0.5f);
    return;
  }
  const Image img = TrophyPicture(index);
  if (img) {
    const ImU32 tint = earned ? White(1) : Mix(White(0.34f), White(0.55f), over);
    dl_->AddImageRounded(img.id, P(x, y), P(x + size, y + size), ImVec2(0, 0), ImVec2(1, 1), tint, r * s_);
  } else {
    Rect(x, y, size, size, Mix(Rgba(38, 40, 46), Rgba(206, 208, 214), over), r);
    trophies::DrawCup(dl_, P(x + size * 0.5f, y + size * 0.5f), size * 0.56f * s_,
                      plat ? trophies::Grade::kPlatinum : t->grade(), 1, earned);
  }
  if (!earned) {  // a padlock in the corner
    const float lx = x + size - size * 0.2f, ly = y + size - size * 0.2f, k = size / 64.0f;
    dl_->AddCircleFilled(P(lx, ly), 13 * k * s_, Black(0.72f), 24);
    dl_->AddRectFilled(P(lx - 5.5f * k, ly - 1.5f * k), P(lx + 5.5f * k, ly + 6.5f * k), White(0.85f), 1.5f * k * s_);
    dl_->PathClear();
    dl_->PathArcTo(P(lx, ly - 2 * k), 3.8f * k * s_, 3.14159f, 6.28318f, 12);
    dl_->PathStroke(White(0.85f), 0, 1.8f * k * s_);
    dl_->AddLine(P(lx - 3.8f * k, ly - 2 * k), P(lx - 3.8f * k, ly - 1.5f * k), White(0.85f), 1.8f * k * s_);
    dl_->AddLine(P(lx + 3.8f * k, ly - 2 * k), P(lx + 3.8f * k, ly - 1.5f * k), White(0.85f), 1.8f * k * s_);
  }
}

// How far along: the platinum, the percentage and its bar, and each grade's count (top right).
void Hub::TrophySummary() {
  const float x = kPanelX, w = kPanelW;
  const bool plat = trophies_.Platinum();
  trophies::DrawCup(dl_, P(x + 30, 150), 64 * s_, trophies::Grade::kPlatinum, 1, plat);
  const std::string pct = std::to_string(trophies_.Percent()) + "%";
  Text(a_.display, 56, x + 82, 108, kText, pct);
  // each grade: its cup and earned / all (right-aligned on the percentage's line)
  float rx = x + w;
  for (int g = 0; g < 3; ++g) {  // (bronze at the right, gold at the left)
    const std::string n = std::to_string(trophies_.grade_earned[g]) + "/" + std::to_string(trophies_.grade_total[g]);
    const float tw = TextWidth(a_.body_bold, 22, n);
    Text(a_.body_bold, 22, rx - tw, 130, White(trophies_.grade_earned[g] ? 0.92f : 0.5f), n);
    trophies::DrawCup(dl_, P(rx - tw - 22, 145), 30 * s_, trophies::Grade(g), 1, trophies_.grade_earned[g] > 0);
    rx -= tw + 64;
  }
  // the bar
  const float bx = x + 82, by = 196, bw = x + w - bx;
  Rect(bx, by, bw, 8, White(0.12f), 4);
  const float f = std::clamp(trophies_.points ? float(trophies_.points_earned) / float(trophies_.points) : 0.0f, 0.0f, 1.0f);
  if (f > 0) Rect(bx, by, std::max(8.0f, bw * f), 8, plat ? trophies::GradeColor(trophies::Grade::kPlatinum) : kAccent, 4);
}

// A page with nothing to show yet: what to do.
void Hub::Empty(const char* title, const std::string& text) {
  Text(a_.display, 40, kListX, kListY + 40, kText, title);
  Wrapped(a_.body, 23, kListX, kListY + 110, 820, White(0.65f), text);
}

void Hub::DrawTrophies() {
  if (!trophies_.loaded) {
    PageTitle("TROPHIES", "Budokai 3's achievements, as trophies.");
    Empty("Start the game once",
          "The trophy list comes from the game itself: start it once, and the next time you open the hub your "
          "trophies are here, with the ones you've earned.");
    return;
  }
  PageTitle("TROPHIES", std::to_string(trophies_.earned) + " of " + std::to_string(trophies_.earnable) + " earned" +
                            (trophies_.last_earned ? "  \xC2\xB7  last on " + When(trophies_.last_earned).substr(0, 12) : ""));
  TrophySummary();

  const int n = int(trophy_items_.size());
  const int sel = item_[tab_] = std::clamp(item_[tab_], 0, n - 1);
  std::vector<Row> rows;
  int counts[2] = {};
  for (const TrophyItem& it : trophy_items_) ++counts[it.group];
  for (int i = 0; i < n; ++i) {
    const int g = trophy_items_[size_t(i)].group;
    if (i == 0 || trophy_items_[size_t(i) - 1].group != g)
      rows.push_back({-1, std::string(kGroupNames[g]) + "  " + std::to_string(counts[g])});
    rows.push_back({i});
  }
  const float x = kListX, w = kListW;
  auto name_of = [&](const TrophyItem& it) -> std::string {
    if (it.index < 0) return "Dragon Ball Z Budokai 3 Platinum";
    const auto& t = trophies_.trophies[size_t(it.index)];
    return (!t.earned && t.hidden()) ? "Hidden trophy" : t.name;
  };
  auto text_of = [&](const TrophyItem& it) -> std::string {
    if (it.index < 0) return "Earn every other Budokai 3 trophy (" + std::to_string(trophies_.earnable) + ").";
    const auto& t = trophies_.trophies[size_t(it.index)];
    if (!t.earned && t.hidden()) return "Keep playing to find out what this one is.";
    return t.earned || t.locked_description.empty() ? t.description : t.locked_description;
  };
  List(rows, sel, kRowH, [&](int i, float ry, float over) {
    const TrophyItem& it = trophy_items_[size_t(i)];
    const bool plat = it.index < 0;
    const bool earned = plat ? trophies_.Platinum() : trophies_.trophies[size_t(it.index)].earned != 0;
    TrophyIcon(it.index, x + 20, ry + 11, 58, over);
    const float tx = x + 98, tw = w - 98 - 84;
    Text(a_.body_bold, 23, tx, ry + 13, Mix(White(earned ? 0.95f : 0.78f), kInk, over), Fit(a_.body_bold, 23, name_of(it), tw));
    Text(a_.body, 18, tx, ry + 46, Mix(White(0.42f), Black(0.55f), over), Fit(a_.body, 18, text_of(it), tw));
    const trophies::Grade g = plat ? trophies::Grade::kPlatinum : trophies_.trophies[size_t(it.index)].grade();
    trophies::DrawCup(dl_, P(x + w - 46, ry + 40), 36 * s_, g, 1, earned, over > 0.5f);
  });

  // the focused trophy
  const TrophyItem& it = trophy_items_[size_t(sel)];
  Panel([&](float px, float py, float pw) {
    const bool plat = it.index < 0;
    const trophies::Trophy* t = plat ? nullptr : &trophies_.trophies[size_t(it.index)];
    const bool earned = plat ? trophies_.Platinum() : t->earned != 0;
    const trophies::Grade g = plat ? trophies::Grade::kPlatinum : t->grade();
    TrophyIcon(it.index, px, py, 132, 0);
    const float nx = px + 160, nw = pw - 160;
    trophies::DrawCup(dl_, P(nx + 12, py + 16), 26 * s_, g, 1);
    Text(a_.body_bold, 15, nx + 34, py + 6, trophies::GradeColor(g), Upper(trophies::GradeName(g)) + " TROPHY", 0, 2.5f);
    const float nh = Wrapped(a_.display, 36, nx, py + 36, nw, kText, name_of(it), 1.12f);
    py += std::max(132.0f, 36 + nh) + 26;
    py += Wrapped(a_.body, 22, px, py, pw, White(0.7f), text_of(it)) + 22;
    std::vector<std::pair<std::string, std::string>> facts;
    if (earned) facts.push_back({"EARNED", When(plat ? trophies_.last_earned : t->earned)});
    else if (plat) facts.push_back({"STATUS", std::to_string(trophies_.earnable - trophies_.earned) + " to go"});
    else facts.push_back({"STATUS", "Not earned yet"});
    if (t) facts.push_back({"GAMERSCORE", std::to_string(t->gamerscore) + "G"});
    Facts(px, py, pw, facts);
  });
}

}  // namespace budokai3::hub
