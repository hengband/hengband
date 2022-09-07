#pragma once

class PlayerBlindness {
public:
    PlayerBlindness() = default;

    short current() const;
    bool is_blind() const;
    void set(short value);
    void reset();

private:
    short blindness = 0;
};
