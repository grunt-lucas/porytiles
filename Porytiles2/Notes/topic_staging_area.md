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

## Patch Build Support
User can specify a patch tileset build by specifying `--patch` at the CLI
or by setting `patch.enabled:true` in the tileset YAML config.

When a patch build is set,
compilation will not disturb currently existing Porymap assets.
That is, existing palettes will be fixed,
and existing tiles will be left undisturbed (but reused if possible).
Porytiles can throw very specific, helpful error messages if users add tiles/colors
that aren't covered by existing assets.
The compilation pipeline for this type of patch build
can be a simplified version of the full compilation pipeline,
but with the palette assignment step completely removed.

Optionally, users can specify additional flags or configurations
if they would like patch builds to attempt to use available transparent tiles
or "open" palette slots, where "open" is user-defined
(e.g. user could specify that any palette with color '0 0 0' should be considered wildcarded).

It should be noted:
since patch builds don't disturb existing assets,
that means they also won't remove output assets that aren't used.
That is, if you remove all instances of a given tile from the metatile sheets,
an patch build will still leave that tile in `tiles.png`.
This is so that patch builds can be used as a method for editing tilesets
without disturbing anyone who might depend on that tileset.

### Settings
Patch builds can configure the `tiles` and `pals` assets to be either `fixed` or `free`.

When marked `fixed`, an asset will be considered immutable and cannot be modified by the patch build.
If one of the Porytiles input assets generates any change, the patch build will fail.

When marked `free`, an asset will be considered mutable and could be modified by the patch build,
but only in such a way that won't break dependent assets.

```yaml
compilation:
  patch:
    enabled: true
    tiles: fixed
    pals: fixed
```

## Palette Hints
While Porytiles palettes will remain separate JASC pal files like Porytiles1 palette overrides,
palette primers in Porytiles2 will known as palette hints, and will be specified in the tileset YAML config:

```yaml
compilation:
  palette_hints:
    - name: "foliage"
      colors:
      - [ 12, 190, 20 ]
      - [ 40, 210, 10 ]
```

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
Primary compilation could specify e.g. `--allow-additional-pals=11,12`
which would generate meaningful content in pals 11 and 12 and also allow metatiles to reference these pal indexes without error.
Then, when users compile a secondary tileset paired against this one, it would look at the config and automatically pull in 11,12
as overrides, and error out if the user tries to specify their own overrides.

### See
https://discord.com/channels/419213663107416084/419213762193522708/1439211965724627017
https://github.com/TeamAquasHideout/Team-Aquas-Asset-Repo/tree/main/Tilesets/The%20Great%20Tileset%20Exchange/Full%20Tilesets/LeoB%20ORAS/tilesets/secondary

## Tileset Compilation
TODO: detailed overview of tileset compilation in Porytiles2.

## Tileset Decompilation
TODO: detailed overview of tileset decompilation in Porytiles2.
