#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "gsl/pointers"

#include "porytiles/domain/config/metatile_attribute_field_definition.hpp"

namespace porytiles {

class TextFormatter;
class UserDiagnostics;

/// @brief The diagnostic tag the inference and its infra-layer scanner emit their warnings under.
inline constexpr auto metatile_attr_inference_tag = "metatile-attribute-inference";

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
/// This is the pure, I/O-free input to infer_metatile_attribute_candidates. The infra-layer scanner gathers these
/// facts from the project's headers and source, then hands them to inference. Inference never reads files; every
/// decision follows from this struct.
struct MetatileAttributeScan {
    std::vector<InferenceEnumMember> enum_members; ///< all enum members from the fieldmap header, in declaration order
    std::vector<InferenceDefine> defines;          ///< all integer defines from the fieldmap header
    std::unordered_set<std::string>
        ambiguous_defines; ///< mask defines with conflicting values in an undecidable conditional branch
    std::vector<InferenceArrayEntry> masks_array; ///< entries of the exact-name sMetatileAttrMasks table (may be empty)
    std::vector<InferenceArrayEntry>
        shifts_array;                     ///< entries of the exact-name sMetatileAttrShifts table (may be empty)
    bool behaviors_header_present{false}; ///< the behaviors header exists and declares at least one MB_ name
    std::optional<std::string>
        attributes_element_type; ///< raw pointed-to type of struct Tileset's metatileAttributes member, when declared
    std::string header_source;   ///< path of the fieldmap header the defines, enum members, and struct came from
    std::string masks_table_source; ///< path of the file the mask table came from, empty when no table was read
};

/// @brief One complete metatile attribute mask layout a project declares.
///
/// @details
/// A project exposes one candidate set per discovered mask layout: the bare METATILE_ATTR_*_MASK defines (merged with
/// the sMetatileAttrMasks table when no FRLG defines exist) form one set, and the *_MASK_FRLG defines plus the table
/// form another. @c origin is a human-readable description of where the set's masks came from, used in size-selection
/// errors and dump output. @c source is the matching file path list: exactly the files @c origin names, comma
/// separated, so the two never disagree (a table-only layout points at src/fieldmap.c, not the fieldmap header). It
/// is empty when the scan recorded no paths. @c fields is the set's field definitions (single-mask, display order);
/// the layer-type field, when the layout declares its mask, appears here as an ordinary definition named "layer_type"
/// carrying FieldRole::layer_type, and a set with no field beyond that role field is discarded as unusable.
/// @c required_bytes is the smallest of 1, 2, or 4 bytes that covers every mask in the set.
struct MetatileAttributeCandidateSet {
    std::string origin;
    std::string source;
    MetatileAttributeFieldDefinitions fields;
    std::size_t required_bytes{2};
};

/// @brief The outcome kind of an inference run.
enum class AttributeInferenceStatus {
    valid,        ///< one or more usable candidate sets were inferred
    invalid,      ///< the project declares fields but a mask could not be determined (fatal at resolution time)
    not_provided, ///< nothing attribute-related was found; other providers should be consulted
};

/// @brief The result of an inference run.
///
/// @details
/// @c candidates is populated only when @c status is valid; a project with both bare and FRLG mask defines yields two
/// sets (bare defines first). @c error_message carries the actionable diagnostic when @c status is invalid. @c
/// declaration_size is the declared element width mapped from the scan's attributes_element_type (u8/u16/u32 map to
/// 1/2/4; anything else stays nullopt); it is populated regardless of status, since the struct declaration is a
/// project fact independent of whether a mask layout could be inferred.
struct MetatileAttributeInferenceResult {
    AttributeInferenceStatus status{AttributeInferenceStatus::not_provided};
    std::vector<MetatileAttributeCandidateSet> candidates;
    std::string error_message;
    std::optional<std::size_t> declaration_size;
};

/// @brief Infers the metatile attribute mask candidate sets from a project's raw fieldmap facts.
///
/// @details
/// Runs the three inference phases:
/// - Phase A gathers the field suffixes and per-suffix masks from the attribute enum, the `METATILE_ATTR_*_MASK`
///   defines, and the sMetatileAttrMasks table, then groups them into candidate sets: when `*_MASK_FRLG` defines are
///   present the bare defines form one set and the FRLG defines (plus the table) form another; otherwise everything
///   merges into a single set, with the define winning over a disagreeing table entry (warned).
/// - Phase B names each field and attaches a value-name provider (behavior constants, terrain/encounter enums) where
///   one can be located. The LAYER_TYPE suffix always names the field "layer_type" and never gets a provider: its
///   values are managed by Porytiles, and the emitted definition carries `FieldRole::layer_type`.
/// - Phase C rejects any field with no mask in any set: a project that declares a field but exposes no mask for it
///   gets a fatal, actionable error. There are no assumed completions; either the masks are readable or the user
///   must declare the layout explicitly.
///
/// Non-fatal findings (a missing behavior constants header, a mask define disagreeing with the mask table, a shift
/// table entry disagreeing with its mask) are emitted to `diag` under the "metatile-attribute-inference" tag as they
/// are discovered.
///
/// @param scan The raw facts gathered from the project
/// @param format The formatter used to style diagnostic text
/// @param diag The sink the non-fatal warnings are emitted to
/// @return The inferred candidate sets, an actionable error, or a not-provided outcome
[[nodiscard]] MetatileAttributeInferenceResult infer_metatile_attribute_candidates(
    const MetatileAttributeScan &scan,
    gsl::not_null<const TextFormatter *> format,
    gsl::not_null<const UserDiagnostics *> diag);

} // namespace porytiles
