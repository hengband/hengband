#pragma once
/*!
 * @brief 噂定義
 * @date 2026/05/06
 * @author Hourier
 */

#include <string>
#include <variant>

enum class RumorType {
    GOSSIP = 0,
    TOWN = 1,
    SHALLOW_DUNGEON = 2,
    NORMAL_MONSTER = 3,
    SHALLOW_ARTIFACT = 4,
    DEEP_DUNGEON = 5,
    UNIQUE_MONSTER = 6,
    DEEP_ARTIFACT = 7,
    MAX,
};

enum class DungeonId;
enum class FixedArtifactId : short;
enum class MonraceId : short;
enum class TownId;
class RumorDefinition {
public:
    RumorDefinition(RumorType type, std::variant<std::monostate, DungeonId, FixedArtifactId, MonraceId, TownId> id, const std::string &description);

    RumorType get_type() const;
    const std::variant<std::monostate, DungeonId, FixedArtifactId, MonraceId, TownId> &get_id() const;
    const std::string &get_description() const;

private:
    RumorType type;
    std::variant<std::monostate, DungeonId, FixedArtifactId, MonraceId, TownId> id;
    std::string description;
};
