#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <string>

#include "gsl/pointers"

#include "porytiles/domain/models/metatile_attribute.hpp"
#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/domain/services/enum_map_provider.hpp"
#include "porytiles/infra/config/infra_config.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/file_highlight_printer.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

/// @brief The result of loading an attributes CSV: the per-metatile attributes plus the per-role pin-column.
///
/// @details
/// `attributes` maps metatile id to its parsed attribute. `active_pin_column_present` has one entry per configured
/// role pin: true when that role's active pin column was present in the CSV header, false when it was absent. A stale
/// column matching a role's default name but not currently active does not count as present. The artifact reader turns
/// these facts into each role's PriorPinColumnState on the loaded component, which the decompiler's round-trip merge
/// then consumes.
struct AttributesCsvLoadResult {
    std::map<std::size_t, MetatileAttribute> attributes;
    std::map<FieldRole, bool> active_pin_column_present;
};

/// @brief A service that loads metatile attributes from a CSV file.
///
/// @details
/// AttributesCsvLoader parses the Porytiles attributes CSV format, whose columns come from the resolved metatile
/// attribute schema: the header is @c id followed by every schema field name in schema order. A provider-backed field's
/// cells hold value constant names resolved through the field's provider; a raw field's cells hold plain unsigned
/// integers capped at the field's maximum value. For the stock emerald schema this is:
///
/// @code
/// id,behavior
/// 0,MB_NORMAL
/// 1,MB_TALL_GRASS
/// @endcode
///
/// The header row is cross-checked against the resolved schema, so a CSV written for a different schema fails with a
/// diagnostic naming the offending column. Rich error messages with file context highlighting are produced via
/// FileHighlightPrinter.
class AttributesCsvLoader {
  public:
    /// @brief Constructs an AttributesCsvLoader with required dependencies.
    ///
    /// @param format The text formatter for styled output
    /// @param config A pointer to the InfraConfig for the loader
    /// @param diag A pointer to the UserDiagnostics for the loader
    AttributesCsvLoader(
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const InfraConfig *> config,
        gsl::not_null<const UserDiagnostics *> diag)
        : format_{format}, config_{config}, diag_{diag}, file_printer_{std::make_unique<FileHighlightPrinter>(format)}
    {
    }

    /// @brief Loads metatile attributes from a CSV file.
    ///
    /// @details
    /// Parses the CSV file, validates the header row against the given schema, resolves each field cell (constant
    /// names through the field's provider, integers for raw fields), and returns a map of metatile ID to
    /// MetatileAttribute. All attributes are created with LayerType::normal.
    ///
    /// The schema and providers must belong to the tileset that owns the CSV: when compiling a secondary, the paired
    /// primary's CSV parses against the primary's own resolved schema, which can differ from the secondary's. A
    /// provider-backed field missing from the ProviderMap is an internal invariant violation and panics.
    ///
    /// @param path The path to the attributes CSV file
    /// @param schema The owning tileset's resolved attribute schema the CSV columns are validated and parsed against
    /// @param providers The provider map holding one provider per provider-backed schema field
    /// @param tileset_name The tileset whose config scope resolves the role_pins. When compiling a secondary this is
    /// the primary's name for the primary's CSV, so the config resolves under the file's owning tileset.
    /// @pre File must exist and be readable
    /// @return Map of metatile IDs to their attributes, or an error with file context
    [[nodiscard]] ChainableResult<AttributesCsvLoadResult> load(
        const std::filesystem::path &path,
        const Schema &schema,
        const ProviderMap &providers,
        const std::string &tileset_name) const;

  private:
    const TextFormatter *format_;
    const InfraConfig *config_;
    const UserDiagnostics *diag_;
    const std::unique_ptr<FileHighlightPrinter> file_printer_;
};

} // namespace porytiles
