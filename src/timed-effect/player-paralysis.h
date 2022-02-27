#pragma once

class PlayerParalysis {
public:
    PlayerParalysis() = default;

    short current() const;
    bool is_paralyzed() const;
    void set(short value);
    void reset();

private:
    short paralysis = 0;
};
