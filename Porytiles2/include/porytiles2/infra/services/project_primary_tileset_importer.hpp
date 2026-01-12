#pragma once

#include "porytiles2/domain/services/primary_tileset_importer.hpp"

namespace porytiles2 {

class ProjectPrimaryTilesetImporter : public PrimaryTilesetImporter {
  public:
    explicit ProjectPrimaryTilesetImporter(
        gsl::not_null<const DomainConfig *> config,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag,
        gsl::not_null<const TilePrinter *> tile_printer,
        gsl::not_null<const PalettePrinter *> pal_printer)
        : PrimaryTilesetImporter{config, format, diag, tile_printer, pal_printer}
    {
    }

    [[nodiscard]] ChainableResult<PorymapTilesetComponent>
    import_from_vanilla(const std::string &tileset_name) const override;
};

} // namespace porytiles2
