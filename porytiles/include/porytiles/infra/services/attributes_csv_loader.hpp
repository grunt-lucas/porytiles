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

/**
 * @brief A service that loads metatile attributes from a CSV file.
 *
 * @details
 * AttributesCsvLoader parses the Porytiles attributes CSV format, whose columns come from the resolved metatile
 * attribute schema: the header is @c id followed by every schema field name in schema order. A provider-backed field's
 * cells hold value constant names resolved through the field's provider; a raw field's cells hold plain unsigned
 * integers capped at the field's maximum value. For the stock emerald schema this is:
 *
 * @code
 * id,behavior
 * 0,MB_NORMAL
 * 1,MB_TALL_GRASS
 * @endcode
 *
 * The header row is cross-checked against the resolved schema, so a CSV written for a different schema fails with a
 * diagnostic naming the offending column. Rich error messages with file context highlighting are produced via
 * FileHighlightPrinter.
 */
class AttributesCsvLoader {
  public:
    /**
     * @brief Constructs an AttributesCsvLoader with required dependencies.
     *
     * @details
     * The schema dictates the expected columns and how each cell parses; the provider map must uphold the ProviderMap
     * membership contract for that schema (one provider per has_provider() field). A provider-backed field missing
     * from the map is an internal invariant violation and panics rather than degrading to raw parsing.
     *
     * @param format The text formatter for styled output
     * @param schema The resolved attribute schema the CSV columns are validated and parsed against
     * @param providers The provider map holding one provider per provider-backed schema field
     * @param config The config used to resolve the write_layer_type_column knob at tileset scope
     * @param diag The diagnostics sink for the "column ignored" warning when the knob is off
     */
    AttributesCsvLoader(
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const Schema *> schema,
        gsl::not_null<const ProviderMap *> providers,
        gsl::not_null<const InfraConfig *> config,
        gsl::not_null<const UserDiagnostics *> diag)
        : format_{format}, schema_{schema}, providers_{providers}, config_{config}, diag_{diag},
          file_printer_{std::make_unique<FileHighlightPrinter>(format)}
    {
    }

    /**
     * @brief Loads metatile attributes from a CSV file.
     *
     * @details
     * Parses the CSV file, validates the header row against the resolved schema, resolves each field cell (constant
     * names through the field's provider, integers for raw fields), and returns a map of metatile ID to
     * MetatileAttribute. All attributes are created with LayerType::normal.
     *
     * When a @c layerType column is present, its handling depends on the write_layer_type_column knob resolved at the
     * scope of @p tileset_name: with the knob on, a filled cell pins the attribute's layer type and a blank cell leaves
     * it inferred; with the knob off, the column is ignored and a single warning is emitted for the file.
     *
     * @param path The path to the attributes CSV file
     * @param tileset_name The tileset whose config scope resolves the write_layer_type_column knob. When compiling a
     * secondary this is the primary's name for the primary's CSV, so the knob resolves under the file's owning tileset.
     * @pre File must exist and be readable
     * @return Map of metatile IDs to their attributes, or an error with file context
     */
    [[nodiscard]] ChainableResult<std::map<std::size_t, MetatileAttribute>>
    load(const std::filesystem::path &path, const std::string &tileset_name) const;

  private:
    const TextFormatter *format_;
    const Schema *schema_;
    const ProviderMap *providers_;
    const InfraConfig *config_;
    const UserDiagnostics *diag_;
    const std::unique_ptr<FileHighlightPrinter> file_printer_;
};

} // namespace porytiles
