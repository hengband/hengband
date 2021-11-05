#pragma once

#include <memory>

enum class MonsterLoaderVersionType;
class PlayerType;
struct monster_type;
class MonsterLoaderBase;
class MonsterLoaderFactory {
public:
    static std::shared_ptr<MonsterLoaderBase> create_loader(PlayerType *player_ptr);

private:
    MonsterLoaderFactory() = delete;
    static MonsterLoaderVersionType get_version();
};
