
// A pokeemerald tileset artifact.
// An artifact has a type as well as some optional metadata.
// E.g. an anim frame will belong to a particular anim (name) and have an index.
struct TilesetArtifact {
    enum class Type { metatiles_bin, porytiles_anim_key_frame, porytiles_anim_frame };
    std::optional<std::string> name;
    std::optional<int> index;
};

class TilesetArtifactKeyProvider {
  public:
    // Main key fetch function. It should focus primarily on building keys, not searching the filesystem/database/whatever.
    virtual std::any key_for(const std::string &tileset_name, const TilesetArtifact &artifact) const = 0;

    // The "discover" method family should actually search the backing store to determine which, if any, optional assets were supplied.
    virtual std::set<std::string> discover_porytiles_anims(const std::string &tileset_name) const = 0;
    virtual std::set<std::string> discover_porytiles_anim_frames(const std::string &tileset_name, const std::string &anim_name) const = 0;
    virtual std::set<std::string> discover_porymap_anims(const std::string &tileset_name) const = 0;
    virtual std::set<std::string> discover_porymap_anim_frames(const std::string &tileset_name, const std::string &anim_name) const = 0;

    // Should check if an artifact for the given key actually exists in the backing store.
    virtual bool exists(const std::any &key) const = 0;

    // Implementations of ArtifactKeyProvider (e.g. ProjectArtifactKeyProvider) may choose to build a cache of tilesets in the ctor.
    // That way, the key_for function can focus solely on static path building as opposed to searching a backing store.
};

class TilesetArtifactWriter {
  public:
    virtual write(const std::any &dest_key, const Artifact &artifact, const Tileset &src) = 0;
};

class TilesetArtifactReader {
  public:
    virtual read(Tileset &dest, const std::any &src_key, const Artifact &artifact) = 0;
}

class TilesetRepo {
  public:
    Result<std::unique_ptr<Tileset>> load(const std::string &name) {
        using enum TilesetArtifact::Type;

        // Fail as late as possible
        bool fail_at_exit = false;

        // Confirm tileset exists.
        if (!exists(name)) {
            return std::unexpected{"does not exist"};
        }

        // Init tileset components, to be filled below.
        auto tileset = std::make_unique<ProjectTileset>();
        tileset->name(name);
        tileset.porymap_component(std::make_unique<PorymapTilesetComponent>());
        tileset.porytiles_component(std::make_unique<PorytilesTilesetComponent>());

        // Load artifacts from required keys first. We can check if they exist before performing a read op.
        auto metatiles_key = key_provider_.key_for(tileset.name(), TilesetArtifact{metatiles_bin});
        if (!key_provider_.exists(metatiles_key)) {
            return std::unexpected{"missing required porymap artifact metatiles.bin"};
        }
        reader_.read(metatiles_key, TilesetArtifact{metatiles_bin}, *tileset);

        auto attr_key = key_provider_.key_for(tileset.name(), TilesetArtifact{metatile_attr_bin});
        if (!key_provider_.exists(attr_key)) {
            return std::unexpected{"missing required porymap artifact metatile_attributes.bin"};
        }
        reader_.read(attr_key, TilesetArtifact{metatile_attr_bin}, *tileset);

        auto bottom_png_key = key_provider_.key_for(tileset.name(), TilesetArtifact{bottom_png});
        if (!key_provider_.exists(bottom_png_key)) {
            return std::unexpected{"missing required porytiles artifact bottom.png"};
        }
        // TODO: in this case, bottom.png does not map directly onto a field of PorytilesTilesetComponent, how to handle?
        reader_.read(bottom_png_key, TilesetArtifact{bottom_png}, *tileset);

        // More artifacts...

        // Now load optional artifacts, e.g. attributes csv, pal overrides, anims, etc.
        // attributes.csv
        auto attr_csv_key = key_provider_.key_for(tileset.name(), TilesetArtifact{attributes_csv});
        if (key_provider_.exists(attr_csv_key)) {
            reader_.read(attr_csv_key, TilesetArtifact{attributes_csv}, *tileset);
        } else {
            // Emit warning to user about missing attr csv
        }

        // palette overrides
        for (int i = 0; i < Tileset::num_pals; i++) {
            auto override_key = key_provider_.key_for(tileset.name(), TilesetArtifact{override, i});
            if (key_provider_.exists(override_key)) {
                reader_.read(override_key, TilesetArtifact{override, i}, *tileset);
            }
        }

        // porytiles anims
        std::set<std::string> porytiles_anims = key_provider_.discover_porytiles_anims(tileset.name());
        for (const auto &anim : porytiles_anims) {
            // Read key frame
            auto key_frame_key = key_provider_.key_for(tileset.name(), TilesetArtifact{anim_key_frame, anim});
            if (!key_provider_.exists(key_frame_key)) {
                // TODO: emit validation error: missing required key frame
                fail_at_exit = true;
            }
            reader_.read(key_frame_key, TilesetArtifact{anim_key_frame, anim}, *tileset);

            // Read frame 00.png
            auto frame_00_key = key_provider_.key_for(tileset.name(), TilesetArtifact{anim_frame, anim, 0});
            if (!key_provider_.exists(frame_00_key)) {
                // TODO: emit validation error: missing required 00.png
                fail_at_exit = true;
            }
            reader_.read(frame_00_key, TilesetArtifact{anim_frame, anim, 0}, *tileset);

            // Read the rest of the (optional) frames
            std::set<int> frames = key_provider_.discover_porytiles_anim_frames(tileset.name(), anim);
            int expected_frame = 1;
            for (const auto frame : frames) {
                if (frame != expected_frame) {
                    // TODO: emit validation error: frame {} did not match expected frame {}
                    fail_at_exit = true;
                }
                auto frame_n_key = key_provider_.key_for(tileset.name(), TilesetArtifact{anim_frame, anim, frame});
                reader_.read(frame_n_key, TilesetArtifact{anim_frame, anim, frame}, *tileset);
                expected_frame++;
            }
        }

        // porymap anims would be the same, but without key frame logic

        if (fail_at_exit) {
            return std::unexpected{"error loading tileset"};
        }

        return tileset;
    }

    Result<void> save_tileset(const Tileset &tileset) {
        auto metatiles_key = key_provider_.key_for(tileset.name(), TilesetArtifact{metatiles_bin});
        auto attr_key = key_provider_.key_for(tileset.name(), TilesetArtifact{metatile_attr_bin});

        writer_.write(metatiles_key, TilesetArtifact{metatiles_bin}, tileset);
        writer_.write(attr_key, TilesetArtifact{metatile_attr_bin}, tileset);

         // TODO: we should "clear" the stale contents of the tileset on disk after saving. That way, if the user e.g. removed an anim,
         // the stale Porymap version of the anim doesn't remain on disk and clutter the filesystem.
    }

  private:
    TilesetArtifactKeyProvider key_provider_;
    TilesetArtifactWriter writer_;
    TilesetArtifactReader reader_;
};

