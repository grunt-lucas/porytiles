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

## Multiple Partner Primary Support
Niche use case, but would be cool. A secondary could specify multiple partner primaries like:
```toml
# my_secondary_tileset.toml

[tileset]
partner_primaries = [ "general_cave_brown", "general_cave_grey" ]
```
The secondary would then supply separate versions of each layer PNG, one for each partner primary.
E.g. `bottom.general_cave_brown.png`, `bottom.general_cave_grey.png`, etc.

The tileset compiler would enforce that
e.g., metatile 8 as seen in the `general_cave_brown` version of the layer PNGs
generates the same metatile data as metatile 8 in `general_cave_grey` version.

This means we must provide some way for users to massage the output ordering of their primary tilesets.
That way these guarantees can be made.
I am not sure if this is something that can be done entirely computationally,
without user intervention.

## Primary Palette Fixing
Couldn't think of a better name for this.

But some community users do a clever thing where they steal one of the secondary palettes for use with the primary tileset
without actually changeing num_pals_primary or any engine palette loading logic.
The way it works is that for every single secondary tileset paired with a given primary, they fix one of the palettes
to have identical content (e.g. 12.pal). That way, whenever the primary set is loaded, you're guaranteed to have a secondary set
loaded with the right pal infor in 12.pal, so primary metatiles that reference pal 12 look normal.

We could support this with some configuration options + Porytiles palettes.
There are multiple ways it could be done, let's think about which would be the easiest to use.

### Idea 1
Primary compilation could specify e.g. `--out-of-band-pals=11,12`
which would generate meaningful content in pals 11 and 12 and also allow metatiles to reference these pal indexes without error.
Then, when users compile a secondary tileset paired against this one, it would look at the config and automatically pull in 11,12
as overrides, and error out if the user tries to specify their own overrides.

### See
https://discord.com/channels/419213663107416084/419213762193522708/1439211965724627017
https://github.com/TeamAquasHideout/Team-Aquas-Asset-Repo/tree/main/Tilesets/The%20Great%20Tileset%20Exchange/Full%20Tilesets/LeoB%20ORAS/tilesets/secondary
