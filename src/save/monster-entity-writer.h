#pragma once

#include <cstdint>

class MonsterEntity;
class MonsterEntityWriter {
public:
    MonsterEntityWriter(const MonsterEntity &monster);
    MonsterEntityWriter(MonsterEntityWriter &) = delete;
    MonsterEntityWriter &operator=(const MonsterEntityWriter &) = delete;
    MonsterEntityWriter &operator=(const MonsterEntityWriter &&) = delete;
    void write_to_savedata() const;

private:
    uint32_t write_monster_flags() const;
    void write_monster_info(uint32_t flags) const;
    const MonsterEntity &monster;
};
