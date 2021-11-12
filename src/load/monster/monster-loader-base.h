#pragma once

struct monster_type;
class PlayerType;
class MonsterLoaderBase {
public:
    virtual ~MonsterLoaderBase() = default;
    virtual void rd_monster(monster_type *m_ptr) = 0;

protected:
    MonsterLoaderBase() = default;
};
