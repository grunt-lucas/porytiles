#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

#include "gsl/pointers"

#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/domain/services/enum_map_provider.hpp"
#include "porytiles/utilities/c_parser/c_parser_facade.hpp"
#include "porytiles/utilities/c_parser/source_position.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

/// @brief Loads a field's name/value mappings from a C/C++ header used by a decomp project.
///
/// @details
/// One provider serves one field. What field it serves, which header it reads, the shared name prefix, the names to
/// skip, and the acceptable declaration styles all come from the EnumSpec passed at construction, so a single class
/// covers behavior, terrain, encounter, and any future field. Two declaration styles are understood:
///
/// Define format:
/// ```
/// #define MB_NORMAL 0x00
/// #define MB_TALL_GRASS 0x02
/// ```
///
/// Enum format:
/// ```
/// MB_NORMAL,
/// MB_TALL_GRASS, // optional comment
/// ```
///
/// In the enum format, values are assigned sequentially starting from 0. Both formats support decimal and hexadecimal
/// values (for define format). The EnumSpec's HeaderFormat selects which styles are read: defines only, enums only, or
/// either (defines first, then enums). During the scan, names that lack the spec's prefix, names in the spec's skip
/// set, and values that exceed the field's maximum are silently ignored, so an unrelated or over-wide constant in a
/// shared header is simply not a mapping. The provider loads lazily on first lookup and caches the result.
class HeaderEnumMapProvider final : public EnumMapProvider {
  public:
    /// @brief Constructs a provider that will load from the specified header file.
    ///
    /// @details
    /// The provider reads mappings from the given header lazily when first accessed via lookup(), then caches them for
    /// subsequent lookups. The EnumSpec fully determines what is scanned and how.
    ///
    /// @param header_path The path to the header declaring this field's value names
    /// @param spec The resolved description of the field's prefix, cap, skip set, format, and display name
    /// @param format The text formatter for styled output
    /// @param diag The user diagnostics for error reporting
    HeaderEnumMapProvider(
        std::filesystem::path header_path,
        EnumSpec spec,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag)
        : header_path_{std::move(header_path)}, spec_{std::move(spec)}, format_{format}, diag_{diag}
    {
    }

    [[nodiscard]] ChainableResult<std::uint32_t> lookup(const std::string &name) const override;

    [[nodiscard]] ChainableResult<std::string> lookup(std::uint32_t value) const override;

  private:
    ChainableResult<void> ensure_loaded() const;

    /// @brief Attempts to add a scanned entry with prefix/skip/range filtering, duplicate detection, and rich error
    /// reporting.
    ///
    /// @details
    /// Uses duck typing to accept any entry type with name(), int_value(), and position() methods. Entries whose name
    /// lacks the spec's prefix, are in the spec's skip set, or whose value falls outside the field's range are filtered
    /// out (returning success without inserting). Otherwise it checks for duplicate names and values, and on a
    /// duplicate produces a rich error showing both source locations.
    ///
    /// This template is defined in the .cpp file since it is only used internally with DefineStatement and EnumMember
    /// types.
    ///
    /// @tparam Entry Type with name(), int_value(), and position() methods (e.g. DefineStatement, EnumMember)
    /// @param entry The entry to add
    /// @return Empty result on success (including filtered-out entries), error on duplicate
    /// @note Despite being const, this method mutates mutable cache members (maps and load_failed_ flag)
    template <typename Entry>
    ChainableResult<void> try_add_entry(const Entry &entry) const;

    std::filesystem::path header_path_;
    EnumSpec spec_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
    mutable bool loaded_{false};
    mutable bool load_failed_{false};
    mutable std::unique_ptr<CParserFacade> driver_;
    mutable std::unordered_map<std::string, std::uint32_t> name_to_value_;
    mutable std::unordered_map<std::uint32_t, std::string> value_to_name_;
    mutable std::unordered_map<std::string, SourcePosition> name_to_position_;
    mutable std::unordered_map<std::uint32_t, SourcePosition> value_to_position_;
};

/// @brief Builds a header provider for every provider-backed field in a schema.
///
/// @details
/// Walks the schema's fields and, for each field with a provider spec, constructs a HeaderEnumMapProvider from the
/// spec's header (resolved against @p project_root) and the EnumSpec derived via ProviderSpec::to_enum_spec. The
/// returned map upholds the ProviderMap membership contract: it contains exactly the schema's has_provider() fields,
/// keyed by field name, so has_provider() and map membership stay equivalent for consumers.
///
/// Providers load lazily, so building the map does no file I/O; a bad header path surfaces on first lookup.
///
/// @param project_root The decomp project root the provider header paths are resolved against
/// @param schema The resolved attribute schema whose provider-backed fields need providers
/// @param format The text formatter for styled output
/// @param diag The user diagnostics for error reporting
/// @return A map from field name to its header-backed provider
[[nodiscard]] ProviderMap build_provider_map(
    const std::filesystem::path &project_root,
    const Schema &schema,
    gsl::not_null<const TextFormatter *> format,
    gsl::not_null<const UserDiagnostics *> diag);

} // namespace porytiles
