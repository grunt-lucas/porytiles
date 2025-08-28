# Staging Area For Noteworthy Topics

## Complete Build Symmetry
Conversion between Porytiles and Porymap format for tilesets and layouts is never lossy.
This means that Porytiles2 can be used to generate Porymap assets from a Porytiles tileset/layout,
and vice versa, seamlessly.

### Artifact Checksums
Porytiles2 will automatically compute checksums for all assets that it manages,
each time the user performs an operation.
This will help users prevent accidental asset clobbering
if they make edits in Porymap or another external tool.

## Incremental Build Support
User can specify an incremental tileset build by specifying `--incremental=keep-unused` at the CLI
or by setting `incremental = keep-unused` in the tileset TOML config.

When `incremental=keep-unused` is set,
compilation will not disturb currently existing Porymap assets.
That is, existing palettes will be fixed,
and existing tiles will be left undisturbed (but reused if possible).
We can throw very specific, helpful error messages if users add tiles/colors
that aren't covered by existing assets.
The compilation pipeline for this type of incremental build
can be a simplified version of the full compilation pipeline,
but with the palette assignment step completely removed. 

Optionally, users can specify additional flags or configurations
if they would like incremental builds to attempt to use available transparent tiles
or "open" palette slots, where "open" is user-defined
(e.g. user could specify that any palette with color '0 0 0' should be considered wildcarded).

It should be noted:
since incremental builds don't disturb existing assets,
that means they also won't remove output assets that aren't used.
That is, if you remove all instances of a given tile from the metatile sheets,
an incremental build will still leave that tile in `tiles.png`.
This is so that incremental builds can be used as a method for editing tilesets
without disturbing anyone who might depend on that tileset.

We could provide `incremental=remove-unused` to modify this behavior.
When `incremental=remove-unused` is set, we leave existing tiles undisturbed like before.
But we sweep the tiles/pals at the end and remove any that are no longer in use by the Porytiles assets.
That means we'll need some kind of usage counter mechanism.
Users should be aware that this may result in the breakage of any dependent tilesets.

One problem we need to solve:
if we pre-populate the final `tiles.png` representation,
we'll need some way to provide a normalized view of the pre-populated tiles.
Consider the case where we do an incremental build on a vanilla tileset.
The tiles in a vanilla `tiles.png` are not normalized,
so in order for Porytiles to successfully avoid duplicating assets
we'll need some kind of normalized view that is also aware of the underlying "true" tile configuration.

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

## Tileset/Layout TOML File
Porytiles2 allows users to configure tileset/layout compilation options using a TOML file.
This will save tons of annoying typing at the CLI.

```toml
# my_secondary_tileset.toml

[tileset]
# This field is required for secondary sets, users won't have to specify paired primary at CLI
partner_primaries = [ "my_cool_primary" ]

[fieldmap_overrides]
num_pals_primary = 7
num_metatiles_primary = 2048
num_metatiles_total = 4096

[palette_assignment]
force_smart_prune = true
```

## Tileset Compilation
TODO: detailed overview of tileset compilation in Porytiles2.

## Tileset Decompilation
TODO: detailed overview of tileset decompilation in Porytiles2.
