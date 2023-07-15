#pragma once

class PlayerFear {
public:
    PlayerFear() = default;

    short current() const;
    bool is_fearful() const;
    void set(short value);
    void reset();

private:
    short fear = 0;
};
