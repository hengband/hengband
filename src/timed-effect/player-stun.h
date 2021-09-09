#pragma once

class PlayerStun {
public:
    PlayerStun() = default;
    virtual ~PlayerStun() = default;

    short current();
    void set(short value);

private:
    short stun = 0;
};
