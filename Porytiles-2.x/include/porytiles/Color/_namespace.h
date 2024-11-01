#ifndef PORYTILES_COLOR_NAMESPACE_H
#define PORYTILES_COLOR_NAMESPACE_H

/**
 * @brief Porytiles color types and color-related types.
 *
 * @details
 * The core Porytiles color representation can be found in the Color interface. Porytiles also
 * provides some basic Color implementations: Rgba32 and Bgr15. Library users may provide their own
 * for custom use-cases.
 *
 * Colors can be stored in an aggregate object called a Palette. The Palette class provides a
 * minimum possible interface for the palette concept. The Porytiles compiler is designed to work
 * with any palette (and thus any color representation) that conforms to this interface.
 *
 * Each Color implementation must provide its own Palette implementation, since different color
 * formats might have special implementation considerations. The Porytiles library includes palette
 * implementations for Rgba32 and Bgr15, namely Rgba32Palette and Bgr15Palette.
 */
namespace porytiles::color {
}

#endif // PORYTILES_COLOR_NAMESPACE_H
