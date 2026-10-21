#pragma once

#include "info-reader/parse-error-types.h"
#include <nlohmann/json_fwd.hpp>
#include <string_view>
#include <tl/expected.hpp>

class TerrainType;

class TerrainReader {
public:
    explicit TerrainReader(const nlohmann::json &terrain_data);
    TerrainReader(nlohmann::json &&) = delete;
    TerrainReader(const TerrainReader &) = delete;
    TerrainReader(TerrainReader &&) = delete;
    TerrainReader &operator=(const TerrainReader &) = delete;
    TerrainReader &operator=(TerrainReader &&) = delete;

    tl::expected<void, ParseErrorType> read() const;

private:
    bool grab_one_feat_flag(TerrainType &terrain, std::string_view what) const;
    tl::expected<void, ParseErrorType> set_terrain_symbol(TerrainType &terrain) const;
    tl::expected<void, ParseErrorType> set_terrain_conversion(TerrainType &terrain) const;
    tl::expected<void, ParseErrorType> set_terrain_generation(TerrainType &terrain) const;
    tl::expected<void, ParseErrorType> set_terrain_trap(TerrainType &terrain) const;
    tl::expected<void, ParseErrorType> set_terrain_door(TerrainType &terrain) const;
    tl::expected<void, ParseErrorType> set_terrain_tunnel(TerrainType &terrain) const;
    tl::expected<void, ParseErrorType> set_terrain_pattern(TerrainType &terrain) const;
    tl::expected<void, ParseErrorType> set_terrain_store(TerrainType &terrain) const;
    tl::expected<void, ParseErrorType> set_terrain_building(TerrainType &terrain) const;
    tl::expected<void, ParseErrorType> set_terrain_interactions(TerrainType &terrain) const;

    const nlohmann::json &terrain_data;
};
