#pragma once

class PlayerPoison {
public:
    PlayerPoison() = default;

    short current() const;
    bool is_poisoned() const;
    void set(short value);
    void reset();

private:
    short poison = 0;
};
