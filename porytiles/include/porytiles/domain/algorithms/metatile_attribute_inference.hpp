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

/// @brief The diagnostic tag the infra-layer scanner emits its warnings under.
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

/// @brief What the scan found where the behavior constants header should be.
///
/// @details
/// The scanner reports what it saw, never what it means. The distinction between @c absent and @c unreadable matters
/// downstream: a project with no header genuinely declares no behavior constants, while a header that failed to scan
/// may declare plenty, and telling the user "no constants were found" in the latter case would send them looking at a
/// file that is full of them.
enum class BehaviorsHeaderSource {
    absent,       ///< the project has no include/constants/metatile_behaviors.h
    unreadable,   ///< the header exists but could not be scanned
    no_constants, ///< the header was scanned and declares no MB_ name
    declared,     ///< the header declares at least one MB_ name
};

/// @brief The behavior constants header, as the scan found it.
struct BehaviorsHeaderScan {
    BehaviorsHeaderSource source{BehaviorsHeaderSource::absent};
    std::string path; ///< the path looked at, so a diagnostic can name it
};

/// @brief What the scan found where struct Tileset's metatileAttributes member should be.
///
/// @details
/// The scanner reports what it saw, never what it means: @c declared says a member of that name exists and records the
/// declarator it was written with, leaving it to the domain to decide whether that declarator names a width Porytiles
/// can read.
enum class AttributeDeclarationSource {
    no_fieldmap_header,   ///< the project has no include/global.fieldmap.h
    header_unreadable,    ///< the fieldmap header exists but could not be scanned
    no_tileset_struct,    ///< the header was scanned and declares no struct Tileset
    no_attributes_member, ///< struct Tileset declares no metatileAttributes member
    declared,             ///< the member is declared, and the declarator fields describe how
};

/// @brief struct Tileset's metatileAttributes declaration, as the project wrote it.
///
/// @details
/// The declarator fields carry meaning only when @c source is @c declared. @c element_type is the pointed-to type
/// spelling (@c u16 on pokeemerald, @c u32 on pokefirered, anything at all on a project that renamed it),
/// @c pointer_depth is the number of stars on the declarator, and @c is_const records a const-qualified pointee.
/// Together they reconstruct the declaration for a diagnostic without the scanner having to judge it.
struct AttributeDeclarationScan {
    AttributeDeclarationSource source{AttributeDeclarationSource::no_fieldmap_header};
    std::string element_type;
    std::size_t pointer_depth{0};
    bool is_const{false};
};

/// @brief Renders a declared metatileAttributes member the way the project wrote it.
///
/// @details
/// Rebuilds the declarator from the scan ("const u16 *metatileAttributes"), so a diagnostic can quote the line the
/// user has to go and look at rather than describe it. Only a scan whose source is @c declared has a declarator to
/// render; every other source yields an empty string.
///
/// @param scan The declaration the scan recorded
/// @return The reconstructed declaration, or an empty string when nothing was declared
[[nodiscard]] std::string to_declaration_string(const AttributeDeclarationScan &scan);

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
    BehaviorsHeaderScan behaviors_header; ///< the behavior constants header, or why there is none
    AttributeDeclarationScan declaration; ///< struct Tileset's metatileAttributes declaration, or why there is none
    std::string header_source;      ///< path of the fieldmap header the defines, enum members, and struct came from
    std::string masks_table_source; ///< path of the file the mask table came from, empty when no table was read
    std::vector<std::string>
        unreadable_sources; ///< files that exist but could not be read, so whatever they declare is missing above
};

/// @brief Why one inferred field could not be settled from the project's sources alone.
///
/// @details
/// The provider kinds record a field whose value-name provider could not be located: the behavior field's constants
/// header is absent, unreadable, or empty, or a named field's @c TILE_<X>_ enum probe found nothing. The remaining
/// kinds record two project sources contradicting each other about the same fact: a mask define disagreeing with the
/// mask table entry, or a declared shift (define or table entry) disagreeing with the offset its mask implies.
enum class FieldConflictKind {
    provider_behaviors_absent,       ///< the behavior constants header does not exist
    provider_behaviors_unreadable,   ///< the behavior constants header exists but could not be scanned
    provider_behaviors_no_constants, ///< the header was scanned and declares no MB_ name
    provider_no_matching_enum,       ///< no enum member with the probed prefix exists in the fieldmap header
    mask_define_vs_table,            ///< the field's mask define and mask table entry disagree
    shift_vs_mask,                   ///< the field's declared shift and its mask's bit offset disagree
};

/// @brief One unsettled fact about one inferred field, carried out of inference for the reconciler to rule on.
///
/// @details
/// Inference cannot rule on these itself: it runs before the user's field overrides are merged, and an override can
/// legitimately settle any of them (a stated mask ends a mask or shift dispute, a stated provider, including
/// `provider: null`, ends a provider hunt). The reconciler, which sees the overrides, turns each conflict fatal
/// unless the matching override speaks to it. @c probed is the header path (behavior provider kinds) or the enum
/// prefix (the named-probe kind); @c declared and @c alternate carry the two disagreeing values for the mask and
/// shift kinds (define mask versus table mask, declared shift versus the shift the mask implies).
struct InferredFieldConflict {
    std::string field_name;
    FieldConflictKind kind;
    std::string probed;                     ///< header path or enum prefix (provider kinds only)
    std::optional<std::uint32_t> declared;  ///< define mask, or the declared shift
    std::optional<std::uint32_t> alternate; ///< table mask, or the shift the mask implies
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
/// @c conflicts lists the facts inference could not settle for this set's fields (see InferredFieldConflict); the
/// list is per-set because the bare and FRLG layouts carry different masks, so a conflict in one need not exist in
/// the other, and only the selected set's conflicts should ever matter.
struct MetatileAttributeCandidateSet {
    std::string origin;
    std::string source;
    MetatileAttributeFieldDefinitions fields;
    std::size_t required_bytes{2};
    std::vector<InferredFieldConflict> conflicts;
};

/// @brief The outcome kind of an inference run.
enum class AttributeInferenceStatus {
    valid,        ///< one or more usable candidate sets were inferred
    invalid,      ///< no layout could be determined from what the project declares (fatal at resolution time)
    not_provided, ///< nothing attribute-related was found; other providers should be consulted
};

/// @brief The declared element width of a project's metatile attribute arrays, and the declaration behind it.
///
/// @details
/// @c scan is the declaration the project wrote, carried through so a diagnostic can quote it. @c size is the width it
/// maps to, set only for a single-pointer @c u8, @c u16, or @c u32 element. Every other declarator, and every source
/// with no declaration at all, leaves it unset. Unset is not a default: the declaration width has no second witness
/// (unlike the attribute width, which the masks independently bound), so resolution is fatal unless the user pins the
/// width with the declaration-size knob.
struct InferredAttributeDeclaration {
    AttributeDeclarationScan scan;
    std::optional<std::size_t> size;
};

/// @brief The result of an inference run.
///
/// @details
/// @c candidates is populated only when @c status is valid; a project with both bare and FRLG mask defines yields two
/// sets (bare defines first). @c error_message carries the actionable diagnostic when @c status is invalid. @c
/// declaration is the declared element width and the declaration it was read from; it is populated regardless of
/// status, since the struct declaration is a project fact independent of whether a mask layout could be inferred.
struct MetatileAttributeInferenceResult {
    AttributeInferenceStatus status{AttributeInferenceStatus::not_provided};
    std::vector<MetatileAttributeCandidateSet> candidates;
    std::string error_message;
    InferredAttributeDeclaration declaration;
};

/// @brief Infers the metatile attribute mask candidate sets from a project's raw fieldmap facts.
///
/// @details
/// Runs the three inference phases:
/// - Phase A gathers the field suffixes and per-suffix masks from the attribute enum, the `METATILE_ATTR_*_MASK`
///   defines, and the sMetatileAttrMasks table, then groups them into candidate sets: when `*_MASK_FRLG` defines are
///   present the bare defines form one set and the FRLG defines (plus the table) form another; otherwise everything
///   merges into a single set, with the define winning over a disagreeing table entry (the disagreement is recorded
///   as a conflict on the set for the reconciler to rule on).
/// - Phase B names each field and attaches a value-name provider (behavior constants, terrain/encounter enums) where
///   one can be located. The LAYER_TYPE suffix always names the field "layer_type" and never gets a provider: its
///   values are managed by Porytiles, and the emitted definition carries `FieldRole::layer_type`.
/// - Phase C rejects any field with no mask in any set: a project that declares a field but exposes no mask for it
///   gets a fatal, actionable error. There are no assumed completions; either the masks are readable or the user
///   must declare the layout explicitly.
///
/// An outcome with no usable candidate set is `not_provided` only when the scan read everything it looked at. When the
/// scan lists unreadable sources, the same outcome is `invalid` instead, with an error saying the masks could not be
/// read: a project whose fieldmap files failed to scan must not be told it declares no masks.
///
/// Facts inference could not settle for a set's fields (a missing or unreadable behavior constants header, a named
/// field whose value-name enum probe found nothing, a mask define disagreeing with the mask table, a declared shift
/// disagreeing with its mask's offset) are recorded on that set's conflict list rather than diagnosed here.
/// Inference runs before the user's field overrides are merged, so it cannot know which conflicts an override
/// settles; the reconciler rules on the selected set's conflicts once the overrides are in view.
///
/// @param scan The raw facts gathered from the project
/// @param format The formatter used to style error text
/// @return The inferred candidate sets, an actionable error, or a not-provided outcome
[[nodiscard]] MetatileAttributeInferenceResult
infer_metatile_attribute_candidates(const MetatileAttributeScan &scan, gsl::not_null<const TextFormatter *> format);

} // namespace porytiles
