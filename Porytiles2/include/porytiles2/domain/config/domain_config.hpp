#pragma once

#include <string>

#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/utilities/source_locations.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/config/config_value.hpp"
#include "porytiles2/xcut/panic/panic.hpp"

namespace porytiles2 {

/**
 * @brief Interface that defines a complete domain layer configuration.
 *
 * @details
 * The domain layer operates with this interface - it doesn't need to worry about implementation. Every config value is
 * either virtual (i.e., comes from the user) or defined in terms of other virtual values.
 */
class DomainConfig {
  public:
    virtual ~DomainConfig() = default;

    [[nodiscard]] virtual ConfigValue<std::size_t> num_tiles_primary(const std::string &tileset) const = 0;

    [[nodiscard]] virtual ConfigValue<std::size_t> num_tiles_total(const std::string &tileset) const = 0;

    [[nodiscard]] ConfigValue<std::size_t> num_tiles_secondary(const std::string &tileset) const
    {
        PlainTextFormatter formatter{};
        const auto total = num_tiles_total(tileset);
        const auto total_name = total.name();
        const auto primary = num_tiles_primary(tileset);
        const auto primary_name = primary.name();
        if (total.value() < primary.value()) {
            /*
             * TODO: this should not panic, since it's possible for the user to mistakenly configure this. Any bad state
             * that is user-reachable after valid user intervention should never panic. Thus, we'll need some kind of
             * configuration validation system to run on program init, and fail gracefully when user provides bad
             * configuration.
             */
            const auto msg =
                formatter.format("{}({}) < {}({})", total.name(), total.value(), primary.name(), primary.value());
            panic(msg);
        }
        const std::size_t result = total.value() - primary.value();
        const auto source = formatter.format(
            "Derived: {} ({}) - {} ({})", total.name(), total.source(), primary.name(), primary.source());
        return ConfigValue{result, extract_function_name(), source};
    }

    [[nodiscard]] virtual ConfigValue<std::size_t> num_metatiles_primary(const std::string &tileset) const = 0;

    [[nodiscard]] virtual ConfigValue<std::size_t> num_metatiles_total(const std::string &tileset) const = 0;

    [[nodiscard]] ConfigValue<std::size_t> num_metatiles_secondary(const std::string &tileset) const
    {
        PlainTextFormatter formatter{};
        const auto total = num_metatiles_total(tileset);
        const auto total_name = total.name();
        const auto primary = num_metatiles_primary(tileset);
        const auto primary_name = primary.name();
        if (total.value() < primary.value()) {
            /*
             * TODO: this should not panic, since it's possible for the user to mistakenly configure this. Any bad state
             * that is user-reachable after valid user intervention should never panic. Thus, we'll need some kind of
             * configuration validation system to run on program init, and fail gracefully when user provides bad
             * configuration.
             */
            const auto msg =
                formatter.format("{}({}) < {}({})", total.name(), total.value(), primary.name(), primary.value());
            panic(msg);
        }
        const std::size_t result = total.value() - primary.value();
        const auto source = formatter.format(
            "Derived: {} ({}) - {} ({})", total.name(), total.source(), primary.name(), primary.source());
        return ConfigValue{result, extract_function_name(), source};
    }

    [[nodiscard]] virtual ConfigValue<std::size_t> num_pals_primary(const std::string &tileset) const = 0;

    [[nodiscard]] virtual ConfigValue<std::size_t> num_pals_total(const std::string &tileset) const = 0;

    [[nodiscard]] ConfigValue<std::size_t> num_pals_secondary(const std::string &tileset) const
    {
        PlainTextFormatter formatter{};
        const auto total = num_pals_total(tileset);
        const auto total_name = total.name();
        const auto primary = num_pals_primary(tileset);
        const auto primary_name = primary.name();
        if (total.value() < primary.value()) {
            /*
             * TODO: this should not panic, since it's possible for the user to mistakenly configure this. Any bad state
             * that is user-reachable after valid user intervention should never panic. Thus, we'll need some kind of
             * configuration validation system to run on program init, and fail gracefully when user provides bad
             * configuration.
             */
            const auto msg =
                formatter.format("{}({}) < {}({})", total.name(), total.value(), primary.name(), primary.value());
            panic(msg);
        }
        const std::size_t result = total.value() - primary.value();
        const auto source = formatter.format(
            "Derived: {} ({}) - {} ({})", total.name(), total.source(), primary.name(), primary.source());
        return ConfigValue{result, extract_function_name(), source};
    }

    [[nodiscard]] virtual ConfigValue<std::size_t> max_map_data_size(const std::string &tileset) const = 0;

    [[nodiscard]] virtual ConfigValue<std::size_t> num_tiles_per_metatile(const std::string &tileset) const = 0;

    [[nodiscard]] virtual ConfigValue<Rgba32> extrinsic_transparency(const std::string &tileset) const = 0;
};

} // namespace porytiles2
