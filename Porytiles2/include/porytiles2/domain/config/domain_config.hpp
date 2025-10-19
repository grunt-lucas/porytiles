#pragma once

#include <string>

#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/utilities/source_locations.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/config/config_value.hpp"
#include "porytiles2/xcut/panic/panic.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Interface that defines a complete domain layer configuration.
 *
 * @details
 * The domain layer operates with this interface - it doesn't need to worry about implementation. Every config value is
 * either virtual (i.e., comes from the user) or defined in terms of other virtual values (derived).
 */
class DomainConfig {
  public:
    virtual ~DomainConfig() = default;

    [[nodiscard]] virtual ChainableResult<ConfigValue<std::size_t>>
    num_tiles_primary(const std::string &tileset) const = 0;

    [[nodiscard]] virtual ChainableResult<ConfigValue<std::size_t>>
    num_tiles_total(const std::string &tileset) const = 0;

    [[nodiscard]] ChainableResult<ConfigValue<std::size_t>> num_tiles_secondary(const std::string &tileset) const
    {
        const auto name = tileset + ":" + extract_function_name();
        return compute_secondary(
            tileset,
            name,
            [this](const auto &ts) { return num_tiles_total(ts); },
            [this](const auto &ts) { return num_tiles_primary(ts); });
    }

    [[nodiscard]] virtual ChainableResult<ConfigValue<std::size_t>>
    num_metatiles_primary(const std::string &tileset) const = 0;

    [[nodiscard]] virtual ChainableResult<ConfigValue<std::size_t>>
    num_metatiles_total(const std::string &tileset) const = 0;

    [[nodiscard]] ChainableResult<ConfigValue<std::size_t>> num_metatiles_secondary(const std::string &tileset) const
    {
        const auto name = tileset + ":" + extract_function_name();
        return compute_secondary(
            tileset,
            name,
            [this](const auto &ts) { return num_metatiles_total(ts); },
            [this](const auto &ts) { return num_metatiles_primary(ts); });
    }

    [[nodiscard]] virtual ChainableResult<ConfigValue<std::size_t>>
    num_pals_primary(const std::string &tileset) const = 0;

    [[nodiscard]] virtual ChainableResult<ConfigValue<std::size_t>>
    num_pals_total(const std::string &tileset) const = 0;

    [[nodiscard]] ChainableResult<ConfigValue<std::size_t>> num_pals_secondary(const std::string &tileset) const
    {
        const auto name = tileset + ":" + extract_function_name();
        return compute_secondary(
            tileset,
            name,
            [this](const auto &ts) { return num_pals_total(ts); },
            [this](const auto &ts) { return num_pals_primary(ts); });
    }

    [[nodiscard]] virtual ChainableResult<ConfigValue<std::size_t>>
    max_map_data_size(const std::string &tileset) const = 0;

    [[nodiscard]] virtual ChainableResult<ConfigValue<std::size_t>>
    num_tiles_per_metatile(const std::string &tileset) const = 0;

    [[nodiscard]] virtual ChainableResult<ConfigValue<Rgba32>>
    extrinsic_transparency(const std::string &tileset) const = 0;

  private:
    [[nodiscard]] ChainableResult<ConfigValue<std::size_t>>
    compute_secondary(const std::string &tileset, const std::string &name, auto get_total, auto get_primary) const
    {
        PlainTextFormatter formatter{};

        // Unwrap the total value
        auto total_result = get_total(tileset);
        if (!total_result.has_value()) {
            return ChainableResult<ConfigValue<std::size_t>>{
                FormattableError{"Failed to get total config value"}, total_result};
        }
        const auto total = std::move(total_result).value();

        // Unwrap the primary value
        auto primary_result = get_primary(tileset);
        if (!primary_result.has_value()) {
            return ChainableResult<ConfigValue<std::size_t>>{
                FormattableError{"Failed to get primary config value"}, primary_result};
        }
        const auto primary = std::move(primary_result).value();

        // Validate that total >= primary
        if (total.value() < primary.value()) {
            const auto msg =
                formatter.format("{}({}) < {}({})", total.name(), total.value(), primary.name(), primary.value());
            return FormattableError{msg};
        }

        // Compute the secondary value
        const std::size_t result = total.value() - primary.value();
        const auto source = formatter.format(
            "Derived: {} ({}) - {} ({})", total.name(), total.source(), primary.name(), primary.source());
        return ConfigValue{result, name, source};
    }
};

} // namespace porytiles2
