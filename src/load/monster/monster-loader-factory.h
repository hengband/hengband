#pragma once

#include <memory>

enum class MonsterLoaderVersionType;
class PlayerType;
class MonsterEntity;
class MonsterLoaderBase;
class MonsterLoaderFactory {
public:
    static std::unique_ptr<MonsterLoaderBase> create_loader(PlayerType *player_ptr);

private:
    MonsterLoaderFactory() = delete;
    static MonsterLoaderVersionType get_version();
};
