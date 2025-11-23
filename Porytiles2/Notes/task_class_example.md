```c++
/**
 * @brief Task encapsulating the compile_patch operation for primary tilesets.
 *
 * @details
 * Breaks the monolithic compile_patch logic into discrete phases:
 * 1. process_porytiles_input() - metatileize, validate, decompose Porytiles layers
 * 2. process_porymap_input() - triple-layerize, decompile, decompose Porymap data
 * 3. setup_working_data() - initialize palettes, workspace, and output component
 * 4. match_tiles() - main loop matching Porytiles tiles to Porymap tiles/palettes
 * 5. assemble_output() - finalize output with dual-layer conversion, attributes, exports
 */
class PatchCompilerTask {
  public:
    PatchCompilerTask(
        TextFormatter *format,
        UserDiagnostics *diag,
        TilePrinter *tile_printer,
        PalettePrinter *pal_printer,
        const DomainConfig &config,
        const Tileset &tileset,
        PatchTilesMode tiles_mode,
        PatchPalMode pal_mode)
        : format_{format}, diag_{diag}, tile_printer_{tile_printer}, pal_printer_{pal_printer}, config_{config},
          tileset_{tileset}, tiles_mode_{tiles_mode}, pal_mode_{pal_mode}
    {
    }

    [[nodiscard]] ChainableResult<std::unique_ptr<Tileset>> run();

  private:
    [[nodiscard]] ChainableResult<void> process_porytiles_input();
    [[nodiscard]] ChainableResult<void> process_porymap_input();
    void setup_working_data();
    [[nodiscard]] ChainableResult<void> match_tiles();
    [[nodiscard]] std::unique_ptr<Tileset> assemble_output();

    // Dependencies
    TextFormatter *format_;
    UserDiagnostics *diag_;
    TilePrinter *tile_printer_;
    PalettePrinter *pal_printer_;
    const DomainConfig &config_;
    const Tileset &tileset_;
    PatchTilesMode tiles_mode_;
    PatchPalMode pal_mode_;

    // Config values (populated in run())
    Rgba32 extrinsic_transparency_{};
    unsigned int num_pals_primary_{};
    unsigned int num_pals_total_{};
    unsigned int num_metatiles_primary_{};
    unsigned int num_tiles_primary_{};
    unsigned int num_tiles_per_metatile_{};

    // Intermediate state - Porytiles
    std::vector<Metatile<Rgba32>> porytiles_metatiles_{};
    std::vector<PixelTile<Rgba32>> porytiles_pixel_rgba_{};
    std::vector<CanonicalPixelTile<Rgba32>> porytiles_canonical_pixel_rgba_{};

    // Intermediate state - Porymap
    std::vector<TilemapEntry> porymap_tilemap_entries_{};
    std::vector<Metatile<Rgba32>> porymap_metatiles_{};
    std::vector<PixelTile<Rgba32>> porymap_pixel_rgba_{};
    std::vector<CanonicalPixelTile<Rgba32>> porymap_canonical_pixel_rgba_{};
    std::vector<Palette<Rgba32>> porymap_pals_{};

    // Working data
    std::unique_ptr<PorymapTilesetComponent> new_porymap_component_{};
    std::unique_ptr<TilesPngWorkspace> tiles_workspace_{};
};

ChainableResult<std::unique_ptr<Tileset>> PatchCompilerTask::run()
{
    // Unwrap config values
    PT_UNWRAP_TILESET_CONFIG_REF(config_, extrinsic_transparency, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, num_pals_primary, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, num_pals_total, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, num_metatiles_primary, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, num_tiles_primary, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, num_tiles_per_metatile, tileset_.name(), std::unique_ptr<Tileset>);

    extrinsic_transparency_ = extrinsic_transparency.value();
    num_pals_primary_ = num_pals_primary.value();
    num_pals_total_ = num_pals_total.value();
    num_metatiles_primary_ = num_metatiles_primary.value();
    num_tiles_primary_ = num_tiles_primary.value();
    num_tiles_per_metatile_ = num_tiles_per_metatile.value();

    // Execute phases
    PT_TRY_CALL_CHAIN_ERR(process_porytiles_input(), "failed to process Porytiles input", std::unique_ptr<Tileset>);
    PT_TRY_CALL_CHAIN_ERR(process_porymap_input(), "failed to process Porymap input", std::unique_ptr<Tileset>);
    setup_working_data();
    PT_TRY_CALL_CHAIN_ERR(match_tiles(), "failed to match tiles", std::unique_ptr<Tileset>);

    return assemble_output();
}

ChainableResult<void> PatchCompilerTask::process_porytiles_input()
{
    LayerImageMetatileizer<Rgba32> metatileizer{};
    MetatileValidator validator{format_, diag_, tile_printer_, pal_printer_, &config_, tileset_.name()};

    // Read Porytiles layer images into metatile vector
    PT_TRY_ASSIGN_CHAIN_ERR(
        metatiles,
        metatileizer.metatileize(
            tileset_.porytiles_component().bottom(),
            tileset_.porytiles_component().middle(),
            tileset_.porytiles_component().top()),
        "failed to metatileize input layer images for " + tileset_.name(),
        void);
    porytiles_metatiles_ = std::move(metatiles);

    // Run validation on Porytiles metatiles
    PT_TRY_CALL_CHAIN_ERR(
        validator.validate_primary(porytiles_metatiles_),
        "encountered error(s) while validating Porytiles metatiles",
        void);

    // Decompose Porytiles metatiles and generate canonical versions
    porytiles_pixel_rgba_ = metatile::decompose(porytiles_metatiles_);
    porytiles_canonical_pixel_rgba_ = transform<CanonicalPixelTile<Rgba32>>(porytiles_pixel_rgba_);

    return {};
}

ChainableResult<void> PatchCompilerTask::process_porymap_input()
{
    LayerModeConverter layer_mode_converter{format_, diag_, tile_printer_};
    MetatileDecompiler metatile_decompiler{format_, diag_, tile_printer_, extrinsic_transparency_};

    // Decompile Porymap tilemap entries and decompose into tile vector
    PT_TRY_ASSIGN_CHAIN_ERR(
        tilemap_entries,
        layer_mode_converter.triple_layerize(tileset_.porymap_component()),
        "failed to triple-layerize Porymap component for tileset " + tileset_.name(),
        void);
    porymap_tilemap_entries_ = std::move(tilemap_entries);

    PT_TRY_ASSIGN_CHAIN_ERR(
        metatiles,
        metatile_decompiler.decompile_metatiles(
            porymap_tilemap_entries_, tileset_.porymap_component().tiles_png(), tileset_.porymap_component().pals()),
        "failed to decompile Porymap component for tileset " + tileset_.name(),
        void);
    porymap_metatiles_ = std::move(metatiles);

    // Decompose Porymap metatiles and generate canonical versions
    porymap_pixel_rgba_ = metatile::decompose(porymap_metatiles_);
    porymap_canonical_pixel_rgba_ = transform<CanonicalPixelTile<Rgba32>>(porymap_pixel_rgba_);

    return {};
}

void PatchCompilerTask::setup_working_data()
{
    // Collect primary palettes from existing Porymap component
    porymap_pals_.reserve(num_pals_primary_);
    for (unsigned int i = 0; i < num_pals_primary_; i++) {
        porymap_pals_.push_back(tileset_.porymap_component().pals()[i]);
    }

    // Create new Porymap component for output
    new_porymap_component_ = std::make_unique<PorymapTilesetComponent>();

    // Preconditions: all decomposed tile vectors have the same size
    assert_or_panic(
        porytiles_pixel_rgba_.size() == porymap_pixel_rgba_.size(),
        "porytiles_pixel_rgba_.size() != porymap_pixel_rgba_.size()");
    assert_or_panic(
        porytiles_pixel_rgba_.size() == porytiles_canonical_pixel_rgba_.size(),
        "porytiles_pixel_rgba_.size() != porytiles_canonical_pixel_rgba_.size()");
    assert_or_panic(
        porymap_pixel_rgba_.size() == porymap_canonical_pixel_rgba_.size(),
        "porymap_pixel_rgba_.size() != porymap_canonical_pixel_rgba_.size()");
    assert_or_panic(
        porymap_tilemap_entries_.size() == porymap_pixel_rgba_.size(),
        "porymap_tilemap_entries_.size() != porymap_pixel_rgba_.size()");

    // Create tiles workspace
    tiles_workspace_ =
        std::make_unique<TilesPngWorkspace>(tileset_.porymap_component().tiles_png(), num_tiles_primary_);
}

ChainableResult<void> PatchCompilerTask::match_tiles()
{
    bool matched_all_tiles = true;
    for (std::size_t i = 0; i < porytiles_pixel_rgba_.size(); i++) {
        const auto &porytiles_tile = porytiles_pixel_rgba_[i];
        const auto &porymap_tile = porymap_pixel_rgba_[i];
        const auto &canonical_porytiles_tile = porytiles_canonical_pixel_rgba_[i];
        const auto &canonical_porymap_tile = porymap_canonical_pixel_rgba_[i];
        const auto &porymap_tilemap_entry = porymap_tilemap_entries_[i];

        // CASE: Porytiles component tile exactly matches Porymap component
        if (porytiles_tile.equals_ignoring_transparency(porymap_tile, extrinsic_transparency_)) {
            new_porymap_component_->push_back_tilemap_entry(porymap_tilemap_entry);
        }

        // CASE: Porytiles component tile matches Porymap component under flip transformation
        else if (canonical_porytiles_tile.equals_ignoring_transparency(
                     canonical_porymap_tile, extrinsic_transparency_)) {
            // XOR flip bits to compute transformation from Porytiles orientation to Porymap orientation
            const bool pt_to_pm_hflip = canonical_porytiles_tile.h_flip() ^ canonical_porymap_tile.h_flip();
            const bool pt_to_pm_vflip = canonical_porytiles_tile.v_flip() ^ canonical_porymap_tile.v_flip();
            TilemapEntry new_entry{
                porymap_tilemap_entry.tile_index(),
                porymap_tilemap_entry.pal_index(),
                static_cast<bool>(porymap_tilemap_entry.h_flip() ^ pt_to_pm_hflip),
                static_cast<bool>(porymap_tilemap_entry.v_flip() ^ pt_to_pm_vflip)};
            new_porymap_component_->push_back_tilemap_entry(new_entry);
        }

        // CASE: New tile, compute which pal to use, compute (or create) tile to use
        else {
            auto [metatile_index, layer, subtile] = metatile::from_tile_index(i);
            auto matches = match_or_best(porytiles_tile, porymap_pals_, extrinsic_transparency_, 1);

            if (matches.at(0).is_covered) {
                const auto pal_index = matches.at(0).pal_index;
                const auto &matched_pal = porymap_pals_.at(pal_index);
                const auto index_tile =
                    index_tile_from_color_tile(porytiles_tile, matched_pal, extrinsic_transparency_);
                CanonicalPixelTile canonical_index_tile{index_tile};
                const auto maybe_tile_index = tiles_workspace_->first_occurrence_of(canonical_index_tile);

                if (maybe_tile_index.has_value()) {
                    const auto tile_index = maybe_tile_index.value();
                    const auto canonical_tile = tiles_workspace_->tile_at(tile_index);
                    const bool pt_to_pm_hflip = canonical_index_tile.h_flip() ^ canonical_tile.h_flip();
                    const bool pt_to_pm_vflip = canonical_index_tile.v_flip() ^ canonical_tile.v_flip();
                    TilemapEntry new_entry{tile_index, pal_index, pt_to_pm_hflip, pt_to_pm_vflip};
                    new_porymap_component_->push_back_tilemap_entry(new_entry);
                }
                else if (!maybe_tile_index.has_value() && tiles_mode_ == PatchTilesMode::tiles_fixed) {
                    matched_all_tiles = false;
                    std::vector<std::string> no_match_err{};
                    no_match_err.emplace_back(format_->format(
                        "{}: no matching tiles found",
                        FormatParam{metatile::message_header(*format_, metatile_index, layer, subtile), Style::bold}));
                    no_match_err.emplace_back();
                    std::ranges::copy(
                        tile_printer_->print_metatile(porytiles_metatiles_.at(metatile_index), layer, subtile),
                        std::back_inserter(no_match_err));
                    diag_->err("no-matching-tiles", no_match_err);
                }
                else {
                    panic("implement PatchTilesMode::tiles_free add tile case");
                }
            }
            else {
                matched_all_tiles = false;
                std::vector<std::string> no_match_err{};
                no_match_err.emplace_back(format_->format(
                    "{}: no matching palettes found",
                    FormatParam{metatile::message_header(*format_, metatile_index, layer, subtile), Style::bold}));
                no_match_err.emplace_back();
                std::ranges::copy(
                    tile_printer_->print_metatile(porytiles_metatiles_.at(metatile_index), layer, subtile),
                    std::back_inserter(no_match_err));
                diag_->err("no-matching-palettes", no_match_err);

                std::vector<std::string> closest_n_note{};
                closest_n_note.emplace_back("closest N match(es):");
                int match_index = 0;
                for (const auto &match : matches) {
                    if (match_index != 0) {
                        closest_n_note.emplace_back();
                    }
                    closest_n_note.push_back(format_->format(
                        "Palette match candidate: {}",
                        FormatParam{pad_two_digits(match.pal_index) + std::string{".pal"}, Style::bold}));
                    std::ranges::copy(
                        pal_printer_->print_rgba_palette_covered_missing(
                            porymap_pals_.at(match.pal_index), match.covered_colors, match.missing_colors),
                        std::back_inserter(closest_n_note));
                    closest_n_note.emplace_back();
                    closest_n_note.push_back(format_->format(
                        "Uncovered pixels with {}:",
                        FormatParam{pad_two_digits(match.pal_index) + std::string{".pal"}, Style::bold}));
                    closest_n_note.emplace_back();
                    std::ranges::copy(
                        tile_printer_->print_tile_pixel_highlights(porytiles_tile, match.uncovered_pixel_indices),
                        std::back_inserter(closest_n_note));
                    match_index++;
                }
                diag_->note("no-matching-palettes", closest_n_note);
            }
        }
    }

    if (!matched_all_tiles) {
        return ChainableResult<void>{FormattableError{"failed to match all Porytiles tiles"}};
    }

    return {};
}

std::unique_ptr<Tileset> PatchCompilerTask::assemble_output()
{
    // No changes here, this is a compilation operation - no writebacks into input assets
    auto new_porytiles_component = std::make_unique<PorytilesTilesetComponent>(tileset_.porytiles_component());

    // If user is requesting dual-layer, use the input Porytiles-format metatiles to infer the LayerType
    // for each metatile and remove the relevant tilemap entries
    LayerModeConverter layer_mode_converter{format_, diag_, tile_printer_};
    const auto configured_layer_mode = layer_mode_from_val(num_tiles_per_metatile_);
    if (configured_layer_mode == LayerMode::dual) {
        const auto &dual_layerized =
            layer_mode_converter.dual_layerize(new_porymap_component_->metatiles_bin(), porytiles_metatiles_);
        new_porymap_component_->metatiles_bin(dual_layerized);
    }

    // Copy metatile attributes from original
    for (const auto &attr : tileset_.porymap_component().metatile_attributes_bin()) {
        new_porymap_component_->push_back_attribute(attr);
    }

    // Export tiles in original form
    new_porymap_component_->tiles_png(
        tiles_workspace_->export_image(ExportFlipMode::original, ExportTrimMode::trim_trailing_transparent));

    // Copy primary palettes from our processed porymap_pals vector
    for (unsigned int i = 0; i < num_pals_primary_; i++) {
        new_porymap_component_->set_pal(i, porymap_pals_[i]);
    }

    // Copy remaining secondary palettes from the original component
    for (unsigned int i = num_pals_primary_; i < num_pals_total_; i++) {
        new_porymap_component_->set_pal(i, tileset_.porymap_component().pals()[i]);
    }

    // Copy junk pals (13.pal, 14.pal, 15.pal - reserved by game engine)
    for (unsigned int i = num_pals_total_; i < pal::num_pals; i++) {
        new_porymap_component_->set_pal(i, tileset_.porymap_component().pals()[i]);
    }

    // Create the full Tileset and return
    return std::make_unique<Tileset>(
        tileset_.name(), std::move(new_porytiles_component), std::move(new_porymap_component_));
}
```