# Staging Area For Noteworthy Topics

## North Star: Complete Build Symmetry
Conversion between Porytiles and Porymap format for tilesets and layouts is never lossy.
This means that Porytiles2 can be used to generate Porymap assets from a Porytiles tileset/layout,
and vice versa, seamlessly.

Core philosophy: Porytiles should feel like a natural extension of Porymap, not an alternative tool.
Users should be empowered to use whichever editor they want for any given edit,
and be able to seamlessly transition between editors.

## Layout Metatile Generation
Layout compilation runs with default: `--unknown-metatile-policy=reject`.
When the layout compiler encounters a metatile that's not present in the primary or secondary tileset,
it will error out with a diagnostic message.

Users can optionally supply alternatives:
`--unknown-metatile-policy=add-to-primary` and `--unknown-metatile-policy=add-to-secondary`.
When the add-to-primary policy is enabled,
instead of erroring out upon an unknown metatile,
the layout compiler will append the metatile to the end of the primary tileset and continue.
User can specify `--recompile-after=each` or `--recompile-after=all` to control when tileset recompilation happens.
Either after each time the layout compiler updates the tileset, or at the end after all updates have been made.
`--recompile-after=each` is much more CPU intensive, but it can catch issues earlier.

The add-to-secondary policy functions the same way, but appending to the secondary tileset instead.

By combining and recombining these policies through an iterative workflow,
users can build functional layouts and tilesets by simply drawing the maps they want as-is
and generating the necessary metatiles on an as-needed basis.

