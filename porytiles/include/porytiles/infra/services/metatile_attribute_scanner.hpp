#pragma once

#include <filesystem>

#include "gsl/pointers"

#include "porytiles/domain/algorithms/metatile_attribute_inference.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

/// @brief Scans a decomp project's fieldmap sources for the raw metatile attribute facts.
///
/// @details
/// This class just gathers facts about the sources, it does not try to interpret those facts. The scanner reads three
/// pre-defined files:
///
/// - `include/global.fieldmap.h`: every integer #define and enum member, the ambiguous-define set, and the raw
///   pointed-to type of `struct Tileset`'s `metatileAttributes` member.
/// - `src/fieldmap.c`: the exact-name `sMetatileAttrMasks` and `sMetatileAttrShifts` tables, seeded with the header's
///   symbols so FRLG-macro entries resolve. A missing file or missing table simply leaves the tables empty.
/// - `include/constants/metatile_behaviors.h`: whether it declares at least one `MB_` name (as #defines on
///   pokefirered, enum members on pokeemerald).
///
/// What the facts mean (candidate mask layouts, field names, widths) is decided by the domain inference and
/// reconciliation. A missing fieldmap header is not an error: the scan simply comes back empty and the caller can
/// treat the project as stating nothing about its attribute layout. A file that exists but cannot be read is a
/// different matter, and one the scanner records rather than flattening into "absent": it warns, and lists the path
/// in `unreadable_sources` so the missing facts are not mistaken for facts the project never stated.
class MetatileAttributeScanner {
  public:
    /// @brief Constructs a `MetatileAttributeScanner`.
    ///
    /// @param project_root The root directory of the decomp project
    /// @param format The formatter used by the tolerant C parser
    /// @param diag The sink the recoverable scan warnings are emitted to
    MetatileAttributeScanner(
        std::filesystem::path project_root,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag);

    /// @brief Scans the project's fieldmap sources and returns the gathered facts.
    ///
    /// @details
    /// Recoverable scan problems (an unreadable file, a conflicting redefinition in an undecidable region) are emitted
    /// to diagnostics under the "metatile-attribute-inference" tag.
    ///
    /// @return The raw facts and the paths they were read from
    [[nodiscard]] MetatileAttributeScan scan_project() const;

  private:
    std::filesystem::path project_root_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
};

} // namespace porytiles
