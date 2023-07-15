#pragma once

class PlayerConfusion {
public:
    PlayerConfusion() = default;

    short current() const;
    bool is_confused() const;
    void set(short value);
    void reset();

private:
    short confusion = 0;
};
