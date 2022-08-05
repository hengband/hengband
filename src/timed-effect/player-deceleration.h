#pragma once

class PlayerDeceleration {
public:
    PlayerDeceleration() = default;

    short current() const;
    bool is_slow() const;
    void set(short value);
    void reset();

private:
    short deceleration = 0;
};
