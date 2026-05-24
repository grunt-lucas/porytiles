#include "porytiles/domain/packing/models/shape_group_metadata.hpp"

namespace porytiles {

ShapeGroupMetadata build_shape_group_metadata(
    const std::vector<ShapeGroup<Rgba32>> &shape_groups, const std::vector<PackableTile::Id> &combined_index_to_id)
{
    ShapeGroupMetadata metadata;

    for (const auto &group : shape_groups) {
        std::vector<PackableTile::Id> member_ids;
        member_ids.reserve(group.members.size());

        for (const auto &member : group.members) {
            const auto &id = combined_index_to_id.at(member.tile_index);
            // Primary tiles are never packed, so they don't need biased packing metadata
            if (std::holds_alternative<PackableTile::PrimaryTileId>(id)) {
                continue;
            }
            member_ids.push_back(id);
        }

        if (member_ids.empty()) {
            continue;
        }

        std::size_t group_index = metadata.group_members.size();
        for (const auto &id : member_ids) {
            metadata.tile_id_to_group[id] = group_index;
        }

        metadata.group_members.push_back(std::move(member_ids));
    }

    return metadata;
}

} // namespace porytiles
