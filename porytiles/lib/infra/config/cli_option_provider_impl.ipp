#pragma once

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <exception>
#include <format>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "porytiles/app/config/primary_pairing_mode.hpp"
#include "porytiles/domain/config/anim_key_frame_resolution_strategy.hpp"
#include "porytiles/domain/config/anim_multi_pal_subtile_resolution_strategy.hpp"
#include "porytiles/domain/config/anim_pal_resolution_strategy.hpp"
#include "porytiles/domain/config/artifact_edit_mode.hpp"
#include "porytiles/domain/config/frame_linking.hpp"
#include "porytiles/domain/config/packing_strategy_type.hpp"
#include "porytiles/domain/config/tile_sharing_alignment.hpp"
#include "porytiles/domain/config/tile_sharing_packing.hpp"
#include "porytiles/domain/config/tiles_pal_mode.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/infra/config/frlg_alternate_mask_mode.hpp"
#include "porytiles/infra/config/layer_value.hpp"

// The anonymous namespace ensures internal linkage per translation unit
// This file is intentionally included only in cli_option_provider.cpp
namespace {

using namespace porytiles;

/// @brief Parses a std::size_t from a CLI string option.
///
/// @details
/// Converts a raw string to std::size_t using std::from_chars for robust parsing.
/// Returns LayerValue::invalid() for non-integer strings or out-of-range values.
///
/// @param raw_value The raw string value from CLI, or std::nullopt if not provided
/// @param option_name The CLI option name for error messages (e.g., "--num-tiles-in-primary")
/// @return LayerValue with parsed value, invalid error, or not_provided status
LayerValue<std::size_t> parse_size_t(const std::optional<std::string> &raw_value, const std::string &option_name)
{
    if (!raw_value.has_value()) {
        return LayerValue<std::size_t>::not_provided();
    }

    const auto &str = raw_value.value();

    // Use std::from_chars for efficient, locale-independent parsing
    std::size_t value = 0;
    const auto *begin = str.data();
    const auto *end = str.data() + str.size();
    auto [ptr, ec] = std::from_chars(begin, end, value);

    if (ec == std::errc{} && ptr == end) {
        return LayerValue<std::size_t>::valid(value, option_name, "CLI");
    }

    if (ec == std::errc::result_out_of_range) {
        const auto error = std::format("Invalid value '{}' for '{}': value out of range.", str, option_name);
        return LayerValue<std::size_t>::invalid(error, option_name);
    }

    // Invalid argument or trailing characters
    const auto error = std::format("Invalid value '{}' for '{}': not a valid integer.", str, option_name);
    return LayerValue<std::size_t>::invalid(error, option_name);
}

/// @brief Parses a bool from a CLI string option.
///
/// @details
/// Expects "true" or "false" (case-sensitive, matching CLI11 flag capture).
/// Returns LayerValue::invalid() for any other value.
///
/// @param raw_value The raw string value from CLI, or std::nullopt if not provided
/// @param option_name The CLI option name for error messages (e.g., "--verify-checksums")
/// @return LayerValue with parsed value, invalid error, or not_provided status
LayerValue<bool> parse_bool(const std::optional<std::string> &raw_value, const std::string &option_name)
{
    if (!raw_value.has_value()) {
        return LayerValue<bool>::not_provided();
    }

    const auto &str = raw_value.value();

    if (str == "true") {
        return LayerValue<bool>::valid(true, option_name, "CLI");
    }
    if (str == "false") {
        return LayerValue<bool>::valid(false, option_name, "CLI");
    }

    const auto error = std::format("Invalid value '{}' for '{}': expected 'true' or 'false'.", str, option_name);
    return LayerValue<bool>::invalid(error, option_name);
}

/// @brief Parses an Rgba32 from a CLI string option.
///
/// @details
/// Accepts "R,G,B" or "R,G,B,A" format with values 0-255. Alpha defaults to 255 if not provided.
/// Returns LayerValue::invalid() for malformed input or out-of-range component values.
///
/// @param raw_value The raw string value from CLI, or std::nullopt if not provided
/// @param option_name The CLI option name for error messages (e.g., "--extrinsic-transparency")
/// @return LayerValue with parsed value, invalid error, or not_provided status
LayerValue<Rgba32> parse_rgba32(const std::optional<std::string> &raw_value, const std::string &option_name)
{
    if (!raw_value.has_value()) {
        return LayerValue<Rgba32>::not_provided();
    }

    const auto &input = raw_value.value();
    std::istringstream iss{input};
    std::string token;
    std::vector<int> values;

    while (std::getline(iss, token, ',')) {
        // Use std::from_chars for robust integer parsing
        int val = 0;
        const auto *begin = token.data();
        const auto *end = token.data() + token.size();
        auto [ptr, ec] = std::from_chars(begin, end, val);

        if (ec != std::errc{} || ptr != end) {
            const auto error =
                std::format("Invalid value '{}' for '{}': '{}' is not a valid integer.", input, option_name, token);
            return LayerValue<Rgba32>::invalid(error, option_name);
        }

        if (val < 0 || val > 255) {
            const auto error = std::format(
                "Invalid value '{}' for '{}': component {} is out of range (must be 0-255).", input, option_name, val);
            return LayerValue<Rgba32>::invalid(error, option_name);
        }

        values.push_back(val);
    }

    if (values.size() == 3) {
        Rgba32 result{
            static_cast<std::uint8_t>(values[0]),
            static_cast<std::uint8_t>(values[1]),
            static_cast<std::uint8_t>(values[2]),
            255};
        return LayerValue<Rgba32>::valid(result, option_name, "CLI");
    }

    if (values.size() == 4) {
        Rgba32 result{
            static_cast<std::uint8_t>(values[0]),
            static_cast<std::uint8_t>(values[1]),
            static_cast<std::uint8_t>(values[2]),
            static_cast<std::uint8_t>(values[3])};
        return LayerValue<Rgba32>::valid(result, option_name, "CLI");
    }

    const auto error = std::format(
        "Invalid value '{}' for '{}': expected R,G,B or R,G,B,A format (got {} components).",
        input,
        option_name,
        values.size());
    return LayerValue<Rgba32>::invalid(error, option_name);
}

/// @brief Pass-through parser for string CLI options.
///
/// @details
/// Simply wraps the raw string in a LayerValue. No transformation is needed.
///
/// @param raw_value The raw string value from CLI, or std::nullopt if not provided
/// @param option_name The CLI option name for error messages
/// @return LayerValue with the string value or not_provided status
LayerValue<std::string> parse_string(const std::optional<std::string> &raw_value, const std::string &option_name)
{
    if (!raw_value.has_value()) {
        return LayerValue<std::string>::not_provided();
    }

    return LayerValue<std::string>::valid(raw_value.value(), option_name, "CLI");
}

/// @brief Parses a std::vector<std::string> from CLI multi-value options.
///
/// @details
/// CLI11 natively handles populating the vector. This function simply wraps the
/// result in a LayerValue. An empty vector is treated as "not provided" because
/// CLI11 leaves the vector empty when the option is not used. This is safe because
/// the default value for all vector config values is also an empty vector, so
/// "not provided" and "empty" are semantically equivalent in the current config system.
///
/// @param raw_values The vector populated by CLI11
/// @param option_name The CLI option name for source info
/// @return LayerValue with the vector or not_provided status
LayerValue<std::vector<std::string>>
parse_string_vector(const std::vector<std::string> &raw_values, const std::string &option_name)
{
    if (raw_values.empty()) {
        return LayerValue<std::vector<std::string>>::not_provided();
    }
    return LayerValue<std::vector<std::string>>::valid(raw_values, option_name, "CLI");
}

/// @brief Parses an ArtifactEditMode from a CLI string option.
///
/// @details
/// Uses the unified artifact_edit_mode_from_str() which provides fuzzy matching.
/// Returns LayerValue::invalid() for unrecognized values, allowing consistent
/// error formatting through LazyLayeredConfig.
///
/// @param raw_value The raw string value from CLI, or std::nullopt if not provided
/// @param option_name The CLI option name for error messages (e.g., "--tiles-edit-mode")
/// @return LayerValue with parsed value, invalid error, or not_provided status
LayerValue<ArtifactEditMode>
parse_artifact_edit_mode(const std::optional<std::string> &raw_value, const std::string &option_name)
{
    if (!raw_value.has_value()) {
        return LayerValue<ArtifactEditMode>::not_provided();
    }

    const auto &str = raw_value.value();
    const auto result = artifact_edit_mode_from_str(str);

    if (result.has_value()) {
        return LayerValue<ArtifactEditMode>::valid(result.value(), option_name, "CLI");
    }

    const auto error = std::format("Invalid value '{}' for '{}'.", str, option_name);
    return LayerValue<ArtifactEditMode>::invalid(error, option_name);
}

/// @brief Parses a TilesPalMode from a CLI string option.
///
/// @details
/// Uses the unified tiles_pal_mode_from_str() which provides fuzzy matching.
/// Returns LayerValue::invalid() for unrecognized values.
///
/// @param raw_value The raw string value from CLI, or std::nullopt if not provided
/// @param option_name The CLI option name for error messages
/// @return LayerValue with parsed value, invalid error, or not_provided status
LayerValue<TilesPalMode>
parse_tiles_pal_mode(const std::optional<std::string> &raw_value, const std::string &option_name)
{
    if (!raw_value.has_value()) {
        return LayerValue<TilesPalMode>::not_provided();
    }

    const auto &str = raw_value.value();
    const auto result = tiles_pal_mode_from_str(str);

    if (result.has_value()) {
        return LayerValue<TilesPalMode>::valid(result.value(), option_name, "CLI");
    }

    const auto error = std::format("Invalid value '{}' for '{}'.", str, option_name);
    return LayerValue<TilesPalMode>::invalid(error, option_name);
}

/// @brief Parses an AnimPalResolutionStrategy from a CLI string option.
///
/// @details
/// Uses the unified anim_pal_resolution_strategy_from_str() which provides fuzzy matching.
/// Returns LayerValue::invalid() for unrecognized values.
///
/// @param raw_value The raw string value from CLI, or std::nullopt if not provided
/// @param option_name The CLI option name for error messages
/// @return LayerValue with parsed value, invalid error, or not_provided status
LayerValue<AnimPalResolutionStrategy>
parse_anim_pal_resolution_strategy(const std::optional<std::string> &raw_value, const std::string &option_name)
{
    if (!raw_value.has_value()) {
        return LayerValue<AnimPalResolutionStrategy>::not_provided();
    }

    const auto &str = raw_value.value();
    const auto result = anim_pal_resolution_strategy_from_str(str);

    if (result.has_value()) {
        return LayerValue<AnimPalResolutionStrategy>::valid(result.value(), option_name, "CLI");
    }

    const auto error = std::format("Invalid value '{}' for '{}'.", str, option_name);
    return LayerValue<AnimPalResolutionStrategy>::invalid(error, option_name);
}

/// @brief Parses an AnimKeyFrameResolutionStrategy from a CLI string option.
///
/// @details
/// Uses the unified anim_key_frame_resolution_strategy_from_str() which provides fuzzy matching.
/// Returns LayerValue::invalid() for unrecognized values.
///
/// @param raw_value The raw string value from CLI, or std::nullopt if not provided
/// @param option_name The CLI option name for error messages
/// @return LayerValue with parsed value, invalid error, or not_provided status
LayerValue<AnimKeyFrameResolutionStrategy>
parse_anim_key_frame_resolution_strategy(const std::optional<std::string> &raw_value, const std::string &option_name)
{
    if (!raw_value.has_value()) {
        return LayerValue<AnimKeyFrameResolutionStrategy>::not_provided();
    }

    const auto &str = raw_value.value();
    const auto result = anim_key_frame_resolution_strategy_from_str(str);

    if (result.has_value()) {
        return LayerValue<AnimKeyFrameResolutionStrategy>::valid(result.value(), option_name, "CLI");
    }

    const auto error = std::format("Invalid value '{}' for '{}'.", str, option_name);
    return LayerValue<AnimKeyFrameResolutionStrategy>::invalid(error, option_name);
}

/// @brief Parses an AnimMultiPalSubtileResolutionStrategy value from a CLI option string.
///
/// @details
/// Uses the unified anim_multi_pal_subtile_resolution_strategy_from_str() which provides fuzzy matching.
/// Returns LayerValue::invalid() for unrecognized values.
///
/// @param raw_value The raw string value from CLI, or std::nullopt if not provided
/// @param option_name The CLI option name for error messages
/// @return LayerValue with parsed value, invalid error, or not_provided status
LayerValue<AnimMultiPalSubtileResolutionStrategy> parse_anim_multi_pal_subtile_resolution_strategy(
    const std::optional<std::string> &raw_value, const std::string &option_name)
{
    if (!raw_value.has_value()) {
        return LayerValue<AnimMultiPalSubtileResolutionStrategy>::not_provided();
    }

    const auto &str = raw_value.value();
    const auto result = anim_multi_pal_subtile_resolution_strategy_from_str(str);

    if (result.has_value()) {
        return LayerValue<AnimMultiPalSubtileResolutionStrategy>::valid(result.value(), option_name, "CLI");
    }

    const auto error = std::format("Invalid value '{}' for '{}'.", str, option_name);
    return LayerValue<AnimMultiPalSubtileResolutionStrategy>::invalid(error, option_name);
}

/// @brief Parses a FrameLinking value from a CLI option string.
///
/// @details
/// Returns LayerValue::not_provided() if raw_value is std::nullopt.
/// Returns LayerValue::invalid() for unrecognized values.
///
/// @param raw_value The raw string value from CLI, or std::nullopt if not provided
/// @param option_name The CLI option name for error messages
/// @return LayerValue with parsed value, invalid error, or not_provided status
LayerValue<FrameLinking>
parse_frame_linking(const std::optional<std::string> &raw_value, const std::string &option_name)
{
    if (!raw_value.has_value()) {
        return LayerValue<FrameLinking>::not_provided();
    }

    const auto &str = raw_value.value();
    const auto result = frame_linking_from_str(str);

    if (result.has_value()) {
        return LayerValue<FrameLinking>::valid(result.value(), option_name, "CLI");
    }

    const auto error = std::format("Invalid value '{}' for '{}'.", str, option_name);
    return LayerValue<FrameLinking>::invalid(error, option_name);
}

/// @brief Parses a TileSharingPacking value from a CLI option string.
///
/// @details
/// Uses the unified tile_sharing_packing_from_str() which provides fuzzy matching.
/// Returns LayerValue::invalid() for unrecognized values.
///
/// @param raw_value The raw string value from CLI, or std::nullopt if not provided
/// @param option_name The CLI option name for error messages
/// @return LayerValue with parsed value, invalid error, or not_provided status
LayerValue<TileSharingPacking>
parse_tile_sharing_packing(const std::optional<std::string> &raw_value, const std::string &option_name)
{
    if (!raw_value.has_value()) {
        return LayerValue<TileSharingPacking>::not_provided();
    }

    const auto &str = raw_value.value();
    const auto result = tile_sharing_packing_from_str(str);

    if (result.has_value()) {
        return LayerValue<TileSharingPacking>::valid(result.value(), option_name, "CLI");
    }

    const auto error = std::format("Invalid value '{}' for '{}'.", str, option_name);
    return LayerValue<TileSharingPacking>::invalid(error, option_name);
}

/// @brief Parses a TileSharingAlignment value from a CLI option string.
///
/// @details
/// Uses the unified tile_sharing_alignment_from_str() which provides fuzzy matching.
/// Returns LayerValue::invalid() for unrecognized values.
///
/// @param raw_value The raw string value from CLI, or std::nullopt if not provided
/// @param option_name The CLI option name for error messages
/// @return LayerValue with parsed value, invalid error, or not_provided status
LayerValue<TileSharingAlignment>
parse_tile_sharing_alignment(const std::optional<std::string> &raw_value, const std::string &option_name)
{
    if (!raw_value.has_value()) {
        return LayerValue<TileSharingAlignment>::not_provided();
    }

    const auto &str = raw_value.value();
    const auto result = tile_sharing_alignment_from_str(str);

    if (result.has_value()) {
        return LayerValue<TileSharingAlignment>::valid(result.value(), option_name, "CLI");
    }

    const auto error = std::format("Invalid value '{}' for '{}'.", str, option_name);
    return LayerValue<TileSharingAlignment>::invalid(error, option_name);
}

LayerValue<PackingStrategyType>
parse_packing_strategy_type(const std::optional<std::string> &raw_value, const std::string &option_name)
{
    if (!raw_value.has_value()) {
        return LayerValue<PackingStrategyType>::not_provided();
    }

    const auto &str = raw_value.value();
    const auto result = packing_strategy_type_from_str(str);

    if (result.has_value()) {
        return LayerValue<PackingStrategyType>::valid(result.value(), option_name, "CLI");
    }

    const auto error = std::format("Invalid value '{}' for '{}'.", str, option_name);
    return LayerValue<PackingStrategyType>::invalid(error, option_name);
}

LayerValue<PrimaryPairingMode>
parse_primary_pairing_mode(const std::optional<std::string> &raw_value, const std::string &option_name)
{
    if (!raw_value.has_value()) {
        return LayerValue<PrimaryPairingMode>::not_provided();
    }

    const auto &str = raw_value.value();
    const auto result = primary_pairing_mode_from_str(str);

    if (result.has_value()) {
        return LayerValue<PrimaryPairingMode>::valid(result.value(), option_name, "CLI");
    }

    const auto error = std::format("Invalid value '{}' for '{}'.", str, option_name);
    return LayerValue<PrimaryPairingMode>::invalid(error, option_name);
}

LayerValue<FrlgAlternateMaskMode>
parse_frlg_alternate_mask_mode(const std::optional<std::string> &raw_value, const std::string &option_name)
{
    if (!raw_value.has_value()) {
        return LayerValue<FrlgAlternateMaskMode>::not_provided();
    }

    const auto &str = raw_value.value();
    const auto result = frlg_alternate_mask_mode_from_str(str);

    if (result.has_value()) {
        return LayerValue<FrlgAlternateMaskMode>::valid(result.value(), option_name, "CLI");
    }

    const auto error = std::format("Invalid value '{}' for '{}'.", str, option_name);
    return LayerValue<FrlgAlternateMaskMode>::invalid(error, option_name);
}

/// @brief Parses an optional layer-type mask from a CLI string option.
///
/// @details
/// Accepts hexadecimal (0x...), decimal, and octal literals (via std::stoul base 0), mirroring the YAML mask parser.
/// A parsed value, including 0 (which disables the layer type), yields a present optional. Absence of the option
/// yields not_provided so lower-priority providers and the size-based default can supply the value instead.
///
/// @param raw_value The raw string value from CLI, or std::nullopt if not provided
/// @param option_name The CLI option name for error messages (e.g., "--metatile-layer-type-mask")
/// @return LayerValue with the parsed mask, an invalid error, or not_provided status
LayerValue<std::optional<std::uint32_t>>
parse_layer_type_mask(const std::optional<std::string> &raw_value, const std::string &option_name)
{
    if (!raw_value.has_value()) {
        return LayerValue<std::optional<std::uint32_t>>::not_provided();
    }

    const auto &str = raw_value.value();
    try {
        std::size_t consumed = 0;
        const unsigned long parsed = std::stoul(str, &consumed, 0);
        if (consumed == str.size() && parsed <= 0xFFFFFFFFUL) {
            return LayerValue<std::optional<std::uint32_t>>::valid(
                std::optional<std::uint32_t>{static_cast<std::uint32_t>(parsed)}, option_name, "CLI");
        }
    }
    catch (const std::exception &) {
        // Fall through to the invalid path below.
    }

    const auto error = std::format(
        "Invalid value '{}' for '{}': expected a 32-bit integer mask (for example 0xF000); use 0 to disable the layer "
        "type.",
        str,
        option_name);
    return LayerValue<std::optional<std::uint32_t>>::invalid(error, option_name);
}

} // namespace
