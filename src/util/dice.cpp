#include "util/dice.h"
#include "system/angband-exceptions.h"
#include "term/z-rand.h"
#include "util/string-processor.h"
#include <sstream>

Dice::Dice()
    : num(0)
    , sides(0)
{
}

Dice::Dice(int num, int sides)
    : num(num)
    , sides(sides)
{
}

/*!
 * @brief 面数sidesのダイスをnum個振った出目の合計を返す
 *
 * @param num ダイスの数
 * @param sides ダイスの面数
 * @return 出目の合計
 */
int Dice::roll(int num, int sides)
{
    auto sum = 0;
    for (auto i = 0; i < num; i++) {
        sum += randint1(sides);
    }
    return sum;
}

/*!
 * @brief 面数sidesのダイスをnum個振った時に出る可能性のある出目の合計の最大値を返す
 *
 * @param num ダイスの数
 * @param sides ダイスの面数
 * @return 出目の合計の最大値
 */
int Dice::maxroll(int num, int sides)
{
    return num * sides;
}

/*!
 * @brief 面数sidesのダイスをnum個振った時の出目の合計の期待値を返す
 *
 * @param num ダイスの数
 * @param sides ダイスの面数
 * @return 出目の合計の期待値
 * @note 期待値は整数で表せない可能性があるため戻り値はdouble型で返す。
 * 戻り値を整数として扱う場合は小数点以下が切り捨てられる可能性がある。
 */
double Dice::expected_value(int num, int sides)
{
    return static_cast<double>(num * (sides + 1)) / 2.0;
}

/*!
 * @brief 面数sidesのダイスをnum個振った時の出目の合計の期待値の整数部を返す
 *
 * 小数部を切り捨てたことによる誤差をなくすため、引数で倍率を指定することができる。
 *
 * @param num ダイスの数
 * @param sides ダイスの面数
 * @param mult 期待値の倍率
 * @return 出目の合計の期待値のmult倍の小数点以下を切り捨てた値
 */
int Dice::floored_expected_value(int num, int sides, int mult)
{
    return mult * num * (sides + 1) / 2;
}

/*!
 * @brief ダイスを表す文字列を生成する
 *
 * ダイスの数をN、面数をMとした時、"NdM"の形式の文字列を生成する。
 *
 * @param num ダイスの数
 * @param sides ダイスの面数
 * @return 生成した"NdM"の形式の文字列
 */
std::string Dice::to_string(int num, int sides)
{
    std::stringstream ss;
    ss << num << "d" << sides;
    return ss.str();
}

/*!
 * @brief ダイスを表す文字列からダイスオブジェクトを生成する
 *
 * 引数で与えられたダイスを表す文字列 "XdY" から、
 * Xをダイスの数、Yをダイスの面数とするヒットダイスオブジェクトを生成する。
 *
 * @param dice_str ダイスを表す文字列
 * @return 生成したダイスオブジェクト
 * @throw std::runtime_error dice_strがダイスを表す文字列として解釈できない場合
 */
Dice Dice::parse(std::string_view dice_str)
{
    const auto tokens = str_split(dice_str, 'd');
    if (tokens.size() != 2) {
        THROW_EXCEPTION(std::runtime_error, "Invalid dice string");
    }

    try {
        auto num = std::stoi(tokens[0]);
        auto sides = std::stoi(tokens[1]);
        return Dice(num, sides);
    } catch (const std::exception &) {
        THROW_EXCEPTION(std::runtime_error, "Invalid dice string");
    }
}

int Dice::roll() const
{
    return Dice::roll(this->num, this->sides);
}

int Dice::maxroll() const
{
    return Dice::maxroll(this->num, this->sides);
}

double Dice::expected_value() const
{
    return Dice::expected_value(this->num, this->sides);
}

int Dice::floored_expected_value() const
{
    return Dice::floored_expected_value(this->num, this->sides, 1);
}

int Dice::floored_expected_value_multiplied_by(int mult) const
{
    return Dice::floored_expected_value(this->num, this->sides, mult);
}

std::string Dice::to_string() const
{
    return Dice::to_string(this->num, this->sides);
}
