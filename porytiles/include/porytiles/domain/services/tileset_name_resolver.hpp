#pragma once

#include <set>
#include <string>

#include "gsl/pointers"

#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"

namespace porytiles {

/// @brief Resolves a user-supplied tileset name to the canonical full name declared in the project.
///
/// @details
/// The canonical full name (e.g. "gTileset_SecretBase") is the unique moniker for a tileset's identity. This resolver
/// is the single place where fuzzy user input can be decoded into that canonical form. This keeps case conversions and
/// other messiness from leaking into the layers below.
///
/// A name matches when its case-folded shorthand (via @c DynamicCasedName canonical equality) equals the case-folded
/// form of a declared tileset's shorthand or full name. So "gTileset_SecretBase", "SecretBase", "secret_base",
/// "secretBase", and "secretbase" all resolve to "gTileset_SecretBase". An input that exactly equals a declared name
/// short-circuits without fuzzy decoding, so two fuzzy-equal but genuinely unique tilesets (e.g. "gTileset_SecretBase"
/// and "gTileset_Secret_Base") each stay addressable by their exact name.
///
/// @param input The user-supplied tileset name in any supported form
/// @param tileset_names The canonical tileset names declared in the project
/// @param format Text formatter for styled error output
/// @return The canonical tileset name, or an error when the input matches no tileset or more than one
[[nodiscard]] ChainableResult<std::string> resolve_tileset_name(
    const std::string &input, const std::set<std::string> &tileset_names, gsl::not_null<const TextFormatter *> format);

} // namespace porytiles
