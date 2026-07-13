#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "gsl/pointers"

#include "porytiles/domain/algorithms/metatile_attribute_inference.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"

namespace porytiles {

/// @brief The product of scanning a decomp project's fieldmap sources for metatile attribute facts.
///
/// @details
/// @c fieldmap_present is true when the fieldmap header exists and both tolerant scans over it succeeded; when it is
/// false, @c scan holds no usable facts and downstream inference should be skipped. @c scan carries the raw facts for
/// infer_metatile_attribute_candidates. @c source is the fieldmap header path, used as the provenance/origin string
/// for values derived from the scan. @c warnings holds the recoverable scan diagnostics (unreadable files,
/// conflicting redefinitions in undecidable regions, etc.) as data, so the caller decides which sink they reach.
struct MetatileAttributeScanOutcome {
    bool fieldmap_present{false};
    MetatileAttributeScan scan;
    std::string source;
    std::vector<std::string> warnings;
};

/// @brief Scans a decomp project's fieldmap sources for the raw metatile attribute facts.
///
/// @details
/// This is pure fact gathering: file I/O and tolerant C parsing with zero interpretation. The scanner reads three
/// well-known files:
///
/// - @c include/global.fieldmap.h: every integer #define and enum member, the ambiguous-define set, and the raw
///   pointed-to type of struct Tileset's metatileAttributes member.
/// - @c src/fieldmap.c: the exact-name sMetatileAttrMasks and sMetatileAttrShifts tables, seeded with the header's
///   symbols so FRLG-macro entries resolve. A missing file or missing table simply leaves the tables empty.
/// - @c include/constants/metatile_behaviors.h: whether it declares at least one MB_ name (as #defines on
///   pokefirered, enum members on pokeemerald).
///
/// What the facts mean (candidate mask layouts, field names, widths) is decided by the domain inference and
/// reconciliation, never here. A missing fieldmap header is not an error: the outcome simply reports
/// fieldmap_present false and the caller treats the project as stating nothing about its attribute layout.
class MetatileAttributeScanner {
  public:
    /// @brief Constructs a MetatileAttributeScanner.
    ///
    /// @param project_root The root directory of the decomp project
    /// @param format The formatter used for warning text
    MetatileAttributeScanner(std::filesystem::path project_root, gsl::not_null<const TextFormatter *> format);

    /// @brief Scans the project's fieldmap sources and returns the gathered facts.
    ///
    /// @return The scan outcome: raw facts, the provenance source, and any recoverable warnings
    [[nodiscard]] MetatileAttributeScanOutcome scan_project() const;

  private:
    std::filesystem::path project_root_;
    const TextFormatter *format_;
};

} // namespace porytiles
