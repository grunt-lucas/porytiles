#include <algorithm>
#include <bitset>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include "normalize.h"

template<>
struct std::hash<BGR15> {
  std::size_t operator()(BGR15 const& bgr) const noexcept {
    return std::hash<std::uint16_t>{}(bgr.bgr);
  }
};

template<>
struct std::hash<GBATile> {
  std::size_t operator()(GBATile const& g) const noexcept {
    // TODO: Better hash function.
    std::size_t h = 0;
    for (auto p : g.paletteIndexPairs) h ^= std::hash<std::uint8_t>{}(p);
    return h;
  }
};

struct NormalizedPixels {
  std::array<std::uint8_t, 32> paletteIndexPairs;

  auto operator<=>(NormalizedPixels const&) const = default;
};

struct NormalizedPalette {
  int size;
  std::array<BGR15, 16> colors;

  auto operator<=>(NormalizedPalette const&) const = default;
};

struct Normalized {
  NormalizedPixels pixels;
  NormalizedPalette palette;
  bool hFlip;
  bool vFlip;

  bool transparent() const { return palette.size == 1; }
  auto operator<=>(Normalized const&) const = default;
};

template<>
struct std::hash<NormalizedPixels> {
  std::size_t operator()(NormalizedPixels const& k) const noexcept {
    // TODO: Better hash function.
    std::size_t h = 0;
    for (auto p : k.paletteIndexPairs) h ^= std::hash<std::uint8_t>{}(p);
    return h;
  }
};

template<>
struct std::hash<NormalizedPalette> {
  std::size_t operator()(NormalizedPalette const& p) const noexcept {
    // TODO: Better hash function.
    std::size_t h = 0;
    h ^= std::hash<int>{}(p.size);
    for (auto c : p.colors) h ^= std::hash<BGR15>{}(c);
    return h;
  }
};

template<>
struct std::hash<Normalized> {
  std::size_t operator()(Normalized const& n) const noexcept {
    // TODO: Better hash function.
    std::size_t h = 0;
    h ^= std::hash<NormalizedPixels>{}(n.pixels);
    h ^= std::hash<NormalizedPalette>{}(n.palette);
    h ^= std::hash<bool>{}(n.hFlip);
    h ^= std::hash<bool>{}(n.vFlip);
    return h;
  }
};

namespace {

BGR15 toBGR(RGBA32 rgba)
{
  return { std::uint16_t(((rgba.b / 8) << 10) | ((rgba.g / 8) << 5) | (rgba.r / 8)) };
}

int insertRGBA(NormalizedPalette& p, RGBA32 rgba)
{
  if (rgba.a == 0) {
    return 0;
  } else if (rgba.a == 255) {
    auto bgr = toBGR(rgba);
    auto i = std::find(std::begin(p.colors) + 1, std::begin(p.colors) + p.size, bgr) - std::begin(p.colors);
    if (i == p.size) {
      if (p.size == 16) {
        throw "too many colors"; // TODO: Context.
      }
      p.colors[p.size++] = bgr;
    }
    return i;
  } else {
    throw "invalid alpha"; // TODO: Context.
  }
}

// NOTE: This produces a _candidate_ normalized tile (a different choice
// of hFlip/vFlip might be the normal form).
Normalized candidate(RGBATile const& rgba, bool hFlip, bool vFlip)
{
  Normalized normalized;
  normalized.palette.size = 1;
  normalized.palette.colors = {};
  normalized.hFlip = hFlip;
  normalized.vFlip = vFlip;

  for (std::size_t i = 0; i < rgba.pixels.size(); i += 2) {
    std::uint8_t paletteIndexPair = 0;
    paletteIndexPair |= insertRGBA(normalized.palette, rgba.pixels[i + 0]);
    paletteIndexPair |= insertRGBA(normalized.palette, rgba.pixels[i + 1]) << 4;
    normalized.pixels.paletteIndexPairs[i / 2] = paletteIndexPair;
  }

  return normalized;
}

Normalized normalize(RGBATile const& rgba)
{
  auto candidate0 = candidate(rgba, false, false);

  // Short-circuit because transparent tiles are common in metatiles and
  // trivially in normal form.
  if (candidate0.transparent()) return candidate0;

  auto candidate1 = candidate(rgba, true, false);
  auto candidate2 = candidate(rgba, false, true);
  auto candidate3 = candidate(rgba, true, true);

  std::array<Normalized const*, 4> candidates = { &candidate0, &candidate1, &candidate2, &candidate3 };
  auto normalized = std::min_element(std::begin(candidates), std::end(candidates), [](auto c1, auto c2) { return c1->pixels < c2->pixels; });
  return **normalized;
}

GBATile makeTile(Normalized const& n, GBAPalette palette)
{
  GBATile gbaTile;
  std::array<std::uint8_t, 16> paletteIndexes;
  paletteIndexes[0] = 0;
  for (int i = 1; i < n.palette.size; i++) {
    auto it = std::find(std::begin(palette.colors) + 1, std::end(palette.colors), n.palette.colors[i]);
    if (it == std::end(palette.colors)) throw "internal error";
    paletteIndexes[i] = it - std::begin(palette.colors);
  }
  for (std::size_t i = 0; i < n.pixels.paletteIndexPairs.size(); i++) {
    gbaTile.paletteIndexPairs[i]
      = paletteIndexes[n.pixels.paletteIndexPairs[i] & 0xF]
      | (paletteIndexes[n.pixels.paletteIndexPairs[i] >> 4] << 4);
  }
  return gbaTile;
}

}

using DecompiledIndex = std::size_t;

Compiled compile(Config const& config, Decompiled const& decompiled)
{
  Compiled compiled;
  compiled.palettes.resize(config.maxPalettes);
  compiled.assignments.resize(decompiled.tiles.size());

  std::vector<std::pair<Normalized, DecompiledIndex>> normalizeds;
  DecompiledIndex decompiledIndex = 0;
  for (auto const& tile : decompiled.tiles) {
    auto normalized = normalize(tile);
    normalizeds.push_back({normalized, decompiledIndex++});
  }

  std::unordered_map<BGR15, std::size_t> colorIndexes;
  std::size_t colorIndex = 0;
  for (auto const& [normalized, _] : normalizeds) {
    for (int i = 1; i < normalized.palette.size; i++) {
      if (colorIndexes.insert({ normalized.palette.colors[i], colorIndex }).second) {
        colorIndex++;
      }
    }
  }
  if (colorIndex > 15 * config.maxPalettes) {
    throw "too many unique colors";
  }

  using ColorSet = std::bitset<240>;
  auto toColorSet = [&colorIndexes](NormalizedPalette const& p) {
    ColorSet colorSet;
    for (int i = 1; i < p.size; i++) {
      colorSet.set(colorIndexes[p.colors[i]]);
    }
    return colorSet;
  };

  std::vector<std::tuple<Normalized, ColorSet, DecompiledIndex>> normalizeds_;
  std::unordered_set<ColorSet> colorSets;
  for (auto const& [normalized, index] : normalizeds) {
    auto colorSet = toColorSet(normalized.palette);
    normalizeds_.push_back({normalized, colorSet, index});
    colorSets.insert(colorSet);
  }

  // Heuristic: assign the largest color sets first so that a set is
  // processed before its subsets.
  std::vector<ColorSet> assigned;
  std::vector<ColorSet> unassigned;
  std::copy(std::begin(colorSets), std::end(colorSets), std::back_inserter(unassigned));
  std::sort(std::begin(unassigned), std::end(unassigned), [](auto const& cs1, auto const& cs2) { return cs1.count() > cs2.count(); });
  struct AssignState {
    std::vector<ColorSet> assigned;
    std::size_t assigning;
    std::size_t assignTo;
  };
  std::stack<AssignState> assignStates;
  assignStates.push({{config.maxPalettes, {}}, 0, 0});
  while (true) {
    if (assignStates.empty()) {
      throw "no solution found";
    }
    auto& assignState = assignStates.top();
    if (assignState.assigning > unassigned.size()) {
      assigned = assignState.assigned;
      break;
    }
    if (assignState.assignTo > assignState.assigned.size()) {
      assignStates.pop();
      continue;
    }
    AssignState assignState_ = assignState;
    assignState.assignTo++;
    assignState_.assigned[assignState.assignTo] |= unassigned[assignState.assigning];
    assignState_.assigning++;
    assignState_.assignTo = 0;
  }

  // XXX: Actually populate compiled.palettes.

  std::unordered_map<GBATile, std::size_t> tileIndexes;

  for (auto const& [normalized, colorSet, index] : normalizeds_) {
    auto it = std::find_if(std::begin(assigned), std::end(assigned), [&colorSet](auto const& a) {
      return (colorSet & ~a).none();
    });
    if (it == std::end(assigned)) throw "internal error";
    std::size_t paletteIndex = it - std::begin(assigned);

    auto tile = makeTile(normalized, compiled.palettes[paletteIndex]);
    auto ok = tileIndexes.insert({tile, compiled.tiles.size()});
    if (ok.second) compiled.tiles.push_back(tile);
    std::size_t tileIndex = ok.first->second;

    compiled.assignments[index] = { tileIndex, paletteIndex, normalized.hFlip, normalized.vFlip };
  }

  return compiled;
}
