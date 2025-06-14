#pragma once

/**
 * @file diagnostics.hpp Definitions for various diagnostic-related types.
 * @copyright Copyright 2025 grunt-lucas. All rights reserved. This project is licensed under the MIT License.
 */

#include <any>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_set>

#include <fmt/color.h>
#include <fmt/ranges.h>

namespace porytiles {

enum class DiagLevel {
    /// Foo
    kIgnored,
    /// Bar
    kNote,
    kRemark,
    kWarning,
    kError,
    kFatal
};

std::string LevelToStr(DiagLevel level);

fmt::terminal_color ColorForLevel(DiagLevel level);

int LevelPriority(DiagLevel level);

// The full definition for this class is in diagnostics/diagnostic_engine.hpp.
class DiagEngine;

/**
 * @brief An alias for a dynamic diagnostic message builder function.
 *
 * @details
 * DynamicMsgBuilder defines a function signature for building diagnostic messages dynamically. The function signature
 * accepts:
 * 1. A boolean flag indicating whether the output is directed to a TTY (terminal).
 * 2. A DiagLevel noting the actual in-flight level of the diagnostic.
 * 3. A `vector` of additional parameters (type `std::any`) that are used to customize the diagnostic message.
 * It returns a `vector` of formatted strings representing the final diagnostic message. Each element in the `vector`
 * represents a single line of output for a DiagConsumer to consume.
 */
using DynamicMsgBuilder = std::function<std::vector<std::string>(const DiagEngine &eng, DiagLevel in_flight_level,
                                                                 const std::vector<std::any> &args)>;

/**
 * @brief Defines a reusable template for standardized diagnostic reporting.
 *
 * @details
 * DiagTempl is used by the DiagEngine to construct the actual diagnostic instance when one is in-flight. A DiagTempl
 * defines a unique name for the diagnostic as well as some default settings. It provides a template for the diagnostic
 * message: either in the form of a static format string or a dynamic builder function. It also contains a list of
 * partner diagnostics. Partner diagnostics are used for warnings/errors that have associated notes.
 */
class DiagTempl {
  public:
    // clang-format off
    explicit DiagTempl(std::string_view name, DiagLevel default_level,
                      DynamicMsgBuilder dynamic_msg_builder) noexcept
        : name_{name},
          default_level_{default_level},
          dynamic_msg_builder_{std::move(dynamic_msg_builder)} {}

    explicit DiagTempl(std::string_view name, DiagLevel default_level,
                        DynamicMsgBuilder dynamic_msg_builder, const std::vector<DiagTempl> &partner_diags) noexcept
        : name_{name},
          default_level_{default_level},
          dynamic_msg_builder_{std::move(dynamic_msg_builder)},
          partner_diags_{partner_diags} {}

    explicit DiagTempl(std::string_view name, DiagLevel default_level,
                    std::string_view static_msg_templ) noexcept
        : name_{name},
          default_level_{default_level},
          static_msg_templ_{static_msg_templ},
          dynamic_msg_builder_{nullptr} {}

    explicit DiagTempl(std::string_view name, DiagLevel default_level,
                        std::string_view static_msg_templ, const std::vector<DiagTempl> &partner_diags) noexcept
        : name_{name},
          default_level_{default_level},
          static_msg_templ_{static_msg_templ},
          dynamic_msg_builder_{nullptr},
          partner_diags_{partner_diags} {}
    // clang-format on

    [[nodiscard]] std::string_view name() const {
        return name_;
    }

    /**
     * @brief Get the default diagnostic level of the template.
     * @return The default DiagLevel.
     */
    [[nodiscard]] DiagLevel level() const {
        return default_level_;
    }

    /**
     * @brief Get the static message template.
     *
     * @details
     * The static message template is only used if a dynamic message builder is not provided.
     *
     * @return A `std::string_view` of the static message template.
     */
    [[nodiscard]] std::string_view static_msg_templ() const {
        return static_msg_templ_;
    }

    /**
     * @brief Build a dynamic message for this DiagTempl based on the configured message builder.
     * @tparam Args The format argument types for the message template.
     * @param eng The calling DiagEngine.
     * @param in_flight_level The in-flight level of this diagnostic.
     * @param args The format arguments for the message template.
     * @return A vector of strings, where each element represents a line in the diagnostic message.
     */
    template <typename... Args>
    std::vector<std::string> BuildDynamicMsg(const DiagEngine &eng, const DiagLevel in_flight_level,
                                             Args &&...args) const {
        if (dynamic_msg_builder_ == nullptr) {
            std::vector<std::string> v{};
            v.push_back(fmt::format(fmt::runtime(static_msg_templ_), std::forward<Args>(args)...));
            return v;
        }
        const std::vector<std::any> v{std::forward<Args>(args)...};
        return dynamic_msg_builder_(eng, in_flight_level, v);
    }

    /**
     * @brief Get a vector of partner DiagTempl for this DiagTempl.
     *
     * @details
     * Partner diagnostics are reported along with the main diagnostic. They are typically used for a DiagLevel::Note
     * that is tied to a specific warning or error.
     *
     * @return A const reference to the vector of partner DiagTempl.
     */
    [[nodiscard]] const std::vector<DiagTempl> &partner_diags() const {
        return partner_diags_;
    }

  private:
    std::string name_;
    DiagLevel default_level_;
    std::string_view static_msg_templ_;
    DynamicMsgBuilder dynamic_msg_builder_;
    std::vector<DiagTempl> partner_diags_;
};

/// @brief Represents an in-flight diagnostic.
///
/// @details
/// InFlightDiag is generated by the DiagEngine by combining the DiagTempl
/// with context from the various user-defined diagnostic parameters (e.g.,
/// warnings as errors, specific warning disables, etc.).
class InFlightDiag {
  public:
    explicit InFlightDiag(const DiagLevel level, std::string msg, DiagTempl templ) noexcept
        : level_{level}, msg_{std::move(msg)}, templ_{std::move(templ)} {}

    [[nodiscard]] DiagLevel level() const noexcept {
        return level_;
    }

    [[nodiscard]] std::string msg() const noexcept {
        return msg_;
    }

    [[nodiscard]] const DiagTempl &templ() const noexcept {
        return templ_;
    }

  private:
    DiagLevel level_;
    std::string msg_;
    DiagTempl templ_;
};

/**
 * @brief A customizable consumer for diagnostic messages.
 *
 * @details
 * DiagEngine must be configured with a concrete DiagConsumer that defines how diagnostic messages are to be processed.
 */
class DiagConsumer {
  public:
    virtual ~DiagConsumer() = default;
    virtual void Consume(const InFlightDiag &diag) = 0;
    [[nodiscard]] virtual bool IsATty() const = 0;
    [[nodiscard]] virtual InFlightDiag ConsumedAt(std::size_t i) const = 0;
    [[nodiscard]] virtual std::uint64_t ConsumedCount() const = 0;
};

/**
 * @brief A DiagConsumer implementation that simply ignores the provided diagnostic.
 */
class IgnoreConsumer final : public DiagConsumer {
  public:
    void Consume(const InFlightDiag &diag) override;
    [[nodiscard]] bool IsATty() const override;
    [[nodiscard]] InFlightDiag ConsumedAt(std::size_t i) const override;
    [[nodiscard]] std::uint64_t ConsumedCount() const override;

  private:
    std::uint64_t consumed_count_{};
};

/**
 * @brief A DiagConsumer implementation that pushes diagnostic messages to `stderr`.
 */
class StderrConsumer final : public DiagConsumer {
  public:
    void Consume(const InFlightDiag &diag) override;
    [[nodiscard]] bool IsATty() const override;
    [[nodiscard]] InFlightDiag ConsumedAt(std::size_t i) const override;
    [[nodiscard]] std::uint64_t ConsumedCount() const override;

  private:
    std::uint64_t consumed_count_{};
};

/**
 * @brief A DiagConsumer implementation that pushes diagnostic messages to an internal `vector`.
 */
class VectorConsumer final : public DiagConsumer {
  public:
    void Consume(const InFlightDiag &diag) override;
    [[nodiscard]] bool IsATty() const override;
    [[nodiscard]] InFlightDiag ConsumedAt(std::size_t i) const override;
    [[nodiscard]] std::uint64_t ConsumedCount() const override;

  private:
    std::vector<InFlightDiag> diags_;
};

// clang-format off

////////////////////////////////////////////////////////////////////////////////
///
/// STANDALONE NOTES
///
////////////////////////////////////////////////////////////////////////////////
constexpr auto NoteGeneric = "note-generic";

////////////////////////////////////////////////////////////////////////////////
///
/// WARNINGS
///
////////////////////////////////////////////////////////////////////////////////
constexpr auto WarnColorPrecisionLoss = "color-precision-loss";
constexpr auto WarnKeyFrameNoMatchingTile = "key-frame-no-matching-tile";
constexpr auto WarnKeyFrameMissingColors = "key-frame-missing-colors";
constexpr auto WarnAttributeFormatMismatch = "attribute-format-mismatch";
constexpr auto WarnMissingAttributesCsv = "missing-attributes-csv";
constexpr auto WarnUnusedAttribute = "unused-attribute";
constexpr auto WarnTransparencyCollapse = "transparency-collapse";
constexpr auto WarnUnusedManualPalColor = "unused-manual-pal-color";
constexpr auto WarnTileIndexOutOfRange = "tile-index-out-of-range";
constexpr auto WarnPaletteIndexOutOfRange = "palette-index-out-of-range";


////////////////////////////////////////////////////////////////////////////////
///
/// ERRORS & FATALS
///
////////////////////////////////////////////////////////////////////////////////
constexpr auto ErrGeneric = "error-generic";
constexpr auto FatalGeneric = "error-fatal-generic";

// clang-format on

/// @brief Retrieves the DiagTempl corresponding to a given diagnostic name.
///
/// This function searches an internal table for the provided diagnostic
/// name. If a DiagTempl with this name is found, the corresponding DiagTempl
/// is returned. If not, the function triggers a panic with an error message
/// indicating an unknown diagnostic name.
///
/// @param name A `std::string_view` of the diagnostic name.
/// @return The DiagTempl associated with the given name.
///
/// @note The function will terminate the program if the diagnostic name is
/// invalid.
DiagTempl DiagFor(std::string_view name);

/// @brief Get an iterable view of all DiagTempl names in the internal table.
///
/// @details The names returned from this function can then be used for lookup
/// via DiagFor. This may be useful for range-based for-loops, or other use
/// cases where the user wants to perform an action for some or all diagnostics.
std::vector<const char *> AllDiagNames();

/// @brief Get an iterable view of all DiagTempl names for a given DiagLevel.
std::vector<const char *> AllDiagNames(DiagLevel level);

} // namespace porytiles

/// @brief Specialization of `std::hash` for `porytiles::DiagTempl`.
///
/// @details
/// This allows porytiles::DiagTempl objects to be used as keys in
/// unordered STL containers like `std::unordered_map` and `std::unordered_set`.
/// The hash value is computed by combining the hash of
/// the template's name and its DiagLevel.
template <>
struct std::hash<porytiles::DiagTempl> {
    std::size_t operator()(const porytiles::DiagTempl &templ) const noexcept {
        std::size_t seed = 0x39A9C07E;
        seed ^= (seed << 6) + (seed >> 2) + 0x6EFC4121 + std::hash<std::string>{}(std::string{templ.name()});
        seed ^= (seed << 6) + (seed >> 2) + 0x14AA7601 + static_cast<std::size_t>(templ.level());
        return seed;
    }
};