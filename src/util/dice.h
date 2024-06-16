#pragma once

#include <string>
#include <string_view>

class Dice {
public:
    Dice();
    Dice(int num, int sides);

    static int roll(int num, int sides);
    static int maxroll(int num, int sides);
    static double expected_value(int num, int sides);
    static int floored_expected_value(int num, int sides, int mult = 1);
    static std::string to_string(int num, int sides);
    static Dice parse(std::string_view dice_str);

    bool is_valid() const;
    int roll() const;
    int maxroll() const;
    double expected_value() const;
    int floored_expected_value() const;
    int floored_expected_value_multiplied_by(int mult) const;
    std::string to_string() const;

    bool operator==(const Dice &other) const = default;

    int num; //< ダイス数
    int sides; //< ダイスの面数
};
