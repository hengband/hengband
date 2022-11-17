#pragma once

class MonsterEntity;
class PlayerType;
class MonsterLoaderBase {
public:
    virtual ~MonsterLoaderBase() = default;
    virtual void rd_monster(MonsterEntity *m_ptr) = 0;

protected:
    MonsterLoaderBase() = default;
};
