#include "porytiles2/infra/services/project_artifact_metadata_provider.hpp"

namespace porytiles2 {

std::unordered_map<std::string, std::string>
ProjectArtifactMetadataProvider::compute_porymap_checksums(const Tileset &tileset) const {
    panic("TODO : unimplemented");
}

std::unordered_map<std::string, std::string>
ProjectArtifactMetadataProvider::load_stored_checksums(const std::string &tileset_name) const {
    panic("TODO : unimplemented");
}

Result<void>
ProjectArtifactMetadataProvider::store_checksums(const std::string &tileset_name,
                                                 const std::unordered_map<std::string, std::string> &checksums) const {
    panic("TODO : unimplemented");
}

std::unordered_map<std::string, Timestamp>
ProjectArtifactMetadataProvider::get_porymap_timestamps(const Tileset &tileset) const {
    panic("TODO : unimplemented");
}

std::unordered_map<std::string, Timestamp>
ProjectArtifactMetadataProvider::get_porytiles_timestamps(const Tileset &tileset) const {
    panic("TODO : unimplemented");
}

bool ProjectArtifactMetadataProvider::are_porymap_assets_newer(const std::string &tileset_name) const {
    panic("TODO : unimplemented");
}

} // namespace porytiles2
