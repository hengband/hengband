#pragma once

struct monster_type;
struct player_type;
class MonsterLoaderBase {
public:
    virtual ~MonsterLoaderBase() = default;
    virtual void rd_monster(monster_type *m_ptr) = 0;

protected:
    MonsterLoaderBase() = default;
};
