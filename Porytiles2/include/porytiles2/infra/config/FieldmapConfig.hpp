#pragma once

#include <cstddef>

namespace porytiles {

class FieldmapConfig2 {
public:
  FieldmapConfig2(const std::size_t num_tiles_in_primary, const std::size_t num_tiles_total,
                  const std::size_t num_metatiles_in_primary, const std::size_t num_metatiles_total,
                  const std::size_t num_pals_in_primary, const std::size_t num_pals_total,
                  const std::size_t max_map_data_size, const std::size_t num_tiles_per_metatile)
      : num_tiles_in_primary_(num_tiles_in_primary), num_tiles_total_(num_tiles_total),
        num_metatiles_in_primary_(num_metatiles_in_primary),
        num_metatiles_total_(num_metatiles_total), num_pals_in_primary_(num_pals_in_primary),
        num_pals_total_(num_pals_total), max_map_data_size_(max_map_data_size),
        num_tiles_per_metatile_(num_tiles_per_metatile) {}

  [[nodiscard]] std::size_t num_tiles_in_primary() const { return num_tiles_in_primary_; }

  [[nodiscard]] std::size_t num_tiles_in_secondary() const {
    return num_tiles_total_ - num_tiles_in_primary_;
  }

  [[nodiscard]] std::size_t num_tiles_total() const { return num_tiles_total_; }

  [[nodiscard]] std::size_t num_metatiles_in_primary() const { return num_metatiles_in_primary_; }

  [[nodiscard]] std::size_t num_metatiles_in_secondary() const {
    return num_metatiles_total_ - num_metatiles_in_primary_;
  }

  [[nodiscard]] std::size_t num_metatiles_total() const { return num_metatiles_total_; }

  [[nodiscard]] std::size_t num_pals_in_primary() const { return num_pals_in_primary_; }

  [[nodiscard]] std::size_t num_pals_in_secondary() const {
    return num_pals_total_ - num_pals_in_primary_;
  }

  [[nodiscard]] std::size_t num_pals_total() const { return num_pals_total_; }

  [[nodiscard]] std::size_t max_map_data_size() const { return max_map_data_size_; }

  [[nodiscard]] std::size_t num_tiles_per_metatile() const { return num_tiles_per_metatile_; }

private:
  std::size_t num_tiles_in_primary_;
  std::size_t num_tiles_total_;
  std::size_t num_metatiles_in_primary_;
  std::size_t num_metatiles_total_;
  std::size_t num_pals_in_primary_;
  std::size_t num_pals_total_;
  std::size_t max_map_data_size_;
  std::size_t num_tiles_per_metatile_;
};

} // namespace porytiles