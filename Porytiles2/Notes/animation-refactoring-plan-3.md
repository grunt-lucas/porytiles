- [Animation Loading Refactoring Plan (Revision 3)](#animation-loading-refactoring-plan-revision-3)
  - [Goal](#goal)
  - [Revision 2 Gaps Analysis](#revision-2-gaps-analysis)
    - [Key Computation Fails To Account For Net-New Animations](#key-computation-fails-to-account-for-net-new-animations)
  - [Animation Components](#animation-components)

# Animation Loading Refactoring Plan (Revision 3)

## Goal
Refactor TilesetRepo and affiliated helper services so that Animations are a first-class concept with coherent loading.

## Revision 2 Gaps Analysis
Revision 2 of this plan plus implementation got us closer, but there are still some critical gaps. This is a hard problem.

### Key Computation Fails To Account For Net-New Animations
If the user adds a new Porymap-component anim (or updates frame count) and imports
OR
the user adds a new Porytiles-component anim (or updates frame count) and compiles
the TilesetRepo::save will fail.

This is because the key provider will fail to find a key for the new artifact, and no write will take place.

E.g. suppose the user added a new anim to anim.yaml and added the new frame data to the Porytiles component.
The load and compile will work fine. But then when we get to TilesetRepo::save, it will try to compute the key for the new anim frames.
This computation will fail, since the key computation is parsing generated_anim_code.h to figure out the key.
And this new anim won't yet be present there until after the save operation finishes:

```c++
/*
 * We'll hit the new anim / new frame in this loop, assuming the compilation task correctly copied it over
 * into the Porymap component.
 */
for (const auto &porymap_anim : tileset.porymap_component().anims() | std::views::values) {
    for (std::size_t i = 0; i < porymap_anim.frame_count(); i++) {
        const auto frame_name = std::to_string(i);
        /*
         * This will fail to find the key: key_for_porymap_anim_frame searches for anim frame keys by parsing the INCBIN
         * definitions in generated_anim_code.h or tileset_anims.c. These files won't have INCBIN definitions for new
         * frames until after save finishes successfully, so this will fail.
         */
        PT_TRY_ASSIGN_CHAIN_ERR(
            frame_key,
            key_provider_->key_for_porymap_anim_frame(tileset.name(), porymap_anim.name(), frame_name),
            "tileset save failed",
            void);
        if (auto result = writer_->write_porymap_anim_frame(frame_key, tileset, porymap_anim.name(), frame_name);
            !result.has_value()) {
            std::ignore = writer_->rollback();
            auto failed = FormattableError{"{}: save failed", FormatParam{frame_key.key(), Style::bold}};
            return ChainableResult<void>{failed, result};
        }
    }
}
```

The same is true in reverse for the import case.

Basically, we need some way for the key provider to "fall back" and look for the key elsewhere if it's not present in anim.yaml / generated_anim_code.h
I.e. "if not present in params file, check the opposing component and see if a new animation exists that matches the requested params."
Something like that?

The problem with doing that is it breaks the TilesetRepo::load case.
When loading a tileset, we're implicitly assuming it was properly saved.
Which means that all animation frames will be present in INCBINs in either generated_anim_code.h or tileset_anims.c.
So if one doesn't exist, that's an error condition. We shouldn't return a "fall back" path.

Idea: perhaps we should have key_for_porymap_anim_frame_write and key_for_porymap_anim_frame_read?
And same thing for Porytiles anim frames.
That way they can work differently. Does this make sense?

## Animation Components