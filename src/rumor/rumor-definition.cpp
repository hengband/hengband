#include "rumor/rumor-definition.h"

RumorDefinition::RumorDefinition(RumorType type, std::variant<std::monostate, DungeonId, FixedArtifactId, MonraceId, TownId> id, const std::string &description)
    : type(type)
    , id(id)
    , description(description)
{
}

RumorType RumorDefinition::get_type() const
{
    return this->type;
}

const std::variant<std::monostate, DungeonId, FixedArtifactId, MonraceId, TownId> &RumorDefinition::get_id() const
{
    return this->id;
}

const std::string &RumorDefinition::get_description() const
{
    return this->description;
}
