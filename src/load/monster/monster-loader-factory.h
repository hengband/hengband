#pragma once

#include <memory>

enum class MonsterLoaderVersionType;
struct player_type;
struct monster_type;
class MonsterLoaderBase;
class MonsterLoaderFactory {
public:
    static std::shared_ptr<MonsterLoaderBase> create_loader(player_type *player_ptr);

private:
    MonsterLoaderFactory() = delete;
    static MonsterLoaderVersionType get_version();
};
