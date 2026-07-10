#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "gsl/pointers"

#include "porytiles/domain/config/metatile_attr_field_spec.hpp"

namespace porytiles {

class TextFormatter;

/// @brief One enum member gathered from a base game's fieldmap header.
///
/// @details
/// @c value is absent when the member's value could not be evaluated during the tolerant scan. Members are supplied in
/// declaration order; inference relies on that order rather than on values when values are missing.
struct InferenceEnumMember {
    std::string name;
    std::optional<std::int64_t> value;
};

/// @brief One integer #define gathered from a base game's fieldmap header.
struct InferenceDefine {
    std::string name;
    std::uint32_t value;
};

/// @brief One designated entry of an attribute mask/shift table.
///
/// @details
/// @c index_name is the designator text (for example @c METATILE_ATTRIBUTE_BEHAVIOR). @c value is absent when the entry
/// expression could not be evaluated.
struct InferenceArrayEntry {
    std::string index_name;
    std::optional<std::uint32_t> value;
};

/// @brief The raw facts a project exposes about its metatile attribute layout.
///
/// @details
/// This is the pure, I/O-free input to infer_metatile_attr_fields. The provider gathers these facts from the project's
/// headers and source, then hands them to inference. Inference never reads files; every decision follows from this
/// struct.
struct MetatileAttrScan {
    std::vector<InferenceEnumMember> enum_members; ///< all enum members from the fieldmap header, in declaration order
    std::vector<InferenceDefine> defines;          ///< all integer defines from the fieldmap header
    std::unordered_set<std::string>
        ambiguous_defines; ///< mask defines with conflicting values in an undecidable conditional branch
    std::vector<InferenceArrayEntry> masks_array; ///< entries of the exact-name sMetatileAttrMasks table (may be empty)
    std::vector<InferenceArrayEntry>
        shifts_array;                     ///< entries of the exact-name sMetatileAttrShifts table (may be empty)
    std::size_t detected_attr_size{2};    ///< attribute byte size detected from the metatiles header (2 or 4)
    bool behaviors_header_present{false}; ///< the behaviors header exists and declares at least one MB_ name
};

/// @brief The outcome kind of an inference run.
enum class AttrInferenceStatus {
    valid,        ///< a usable field set was inferred
    invalid,      ///< the project declares fields but a mask could not be determined (fatal at resolution time)
    not_provided, ///< nothing attribute-related was found; other providers should be consulted
};

/// @brief The result of an inference run.
///
/// @details
/// @c fields is populated only when @c status is valid; it includes alternate-only fields (a frlg_mask but no primary
/// mask). @c error_message carries the actionable diagnostic when @c status is invalid. @c warnings holds non-fatal
/// notes (conflicts, missing providers, shift mismatches) regardless of status.
///
/// @c layer_type_mask / @c layer_type_frlg_mask carry the layer-type bit mask as declared by the base game (the
/// primary and FRLG-alternate values respectively), when one was found. The layer type is never emitted as a field,
/// but its mask is now surfaced so downstream resolution can honor a base game's custom layer-type position instead of
/// assuming the size-based default. Either is @c std::nullopt when the base game declares no mask for that layout.
struct MetatileAttrInferenceResult {
    AttrInferenceStatus status{AttrInferenceStatus::not_provided};
    MetatileAttrFieldSpecs fields;
    std::string error_message;
    std::vector<std::string> warnings;
    std::optional<std::uint32_t> layer_type_mask;
    std::optional<std::uint32_t> layer_type_frlg_mask;
};

/// @brief Infers a metatile attribute field schema from a project's raw fieldmap facts.
///
/// @details
/// Runs the three inference phases:
/// - Phase A assembles the field set and each field's primary/FRLG masks from the attribute enum, the
///   METATILE_ATTR_*_MASK defines, and the sMetatileAttrMasks table, applying the dual-layout rule when FRLG mask
///   defines are present.
/// - Phase B names each field and attaches a value-name provider (behavior constants, terrain/encounter enums) where
///   one can be located. The layer-type field is never emitted, but its mask is recorded on the result.
/// - Phase C fills or rejects fields with no mask: the stock two-byte behavior-only case is completed silently, any
///   other missing mask is a fatal, actionable error.
///
/// @param scan The raw facts gathered from the project
/// @param format The formatter used to style diagnostic text
/// @return The inferred field set, an actionable error, or a not-provided outcome
[[nodiscard]] MetatileAttrInferenceResult
infer_metatile_attr_fields(const MetatileAttrScan &scan, gsl::not_null<const TextFormatter *> format);

} // namespace porytiles
