#pragma once

class PlayerAcceleration {
public:
    PlayerAcceleration() = default;

    short current() const;
    bool is_fast() const;
    void set(short value);
    void reset();

private:
    short acceleration = 0;
};
