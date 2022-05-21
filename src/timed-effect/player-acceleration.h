#pragma once

class PlayerAcceleration {
public:
    PlayerAcceleration() = default;

    short current() const;
    bool is_fast() const;
    void set(short value);
    void add(short value); /*!< 減産も負値を引数に入れれば可能 */
    void reset();

private:
    short acceleration = 0;
};
