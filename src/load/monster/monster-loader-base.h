#pragma once

class MonsterEntity;
class PlayerType;
class MonsterLoaderBase {
public:
    virtual ~MonsterLoaderBase() = default;
    virtual void rd_monster(MonsterEntity &monster) = 0;

protected:
    MonsterLoaderBase() = default;
};
