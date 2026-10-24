/*!
 * @brief ステータス補正の合計と上下限の回帰テスト
 */

#include "player-status/player-status-base.h"
#include <doctest/doctest.h>
#include <limits>

namespace {

// プレイヤーや装備を用意せず、補正の合計処理だけを検証する。
class TestPlayerStatus : public PlayerStatusBase {
public:
    TestPlayerStatus()
        : PlayerStatusBase(nullptr)
    {
        this->default_value = 110;
        this->min_value = 11;
        this->max_value = 209;
    }

    int16_t equipment = 0;
    int16_t weight = 0;
    int16_t timed = 0;

protected:
    void set_locals() override {}
    int16_t equipments_bonus() override
    {
        return this->equipment;
    }
    int16_t inventory_weight_bonus() override
    {
        return this->weight;
    }
    int16_t time_effect_bonus() override
    {
        return this->timed;
    }
};

class TestExceptionStatus : public TestPlayerStatus {
public:
    TestExceptionStatus()
    {
        this->min_value = -128;
        this->max_value = 127;
    }

    int received = 0;

protected:
    int set_exception_bonus(int value) override
    {
        this->received = value;
        return value / 1000;
    }
};

}

TEST_CASE("PlayerStatusBase clamps the complete sum without int16 overflow")
{
    TestPlayerStatus status;
    SUBCASE("ordinary bonuses remain unchanged")
    {
        status.equipment = 20;
        status.weight = -5;
        status.timed = 10;
        CHECK(status.get_value() == 135);
    }
    SUBCASE("positive overflow reaches the upper bound")
    {
        status.equipment = std::numeric_limits<int16_t>::max();
        CHECK(status.get_value() == 209);
    }
    SUBCASE("negative overflow reaches the lower bound")
    {
        status.equipment = -120;
        status.weight = std::numeric_limits<int16_t>::min();
        CHECK(status.get_value() == 11);
    }
    // 次の 2 つは、途中の合計が int16_t の範囲を外れても最終的な合計が範囲内に戻る場合の仕様を確かめる。
    // int16_t の足し算でも 2^16 を法として同じ値に戻るので、修正前のコードでも通る (回帰テストではない)。
    SUBCASE("positive intermediate overflow can be cancelled")
    {
        status.equipment = std::numeric_limits<int16_t>::max();
        status.weight = std::numeric_limits<int16_t>::min();
        CHECK(status.get_value() == 109);
    }
    SUBCASE("negative intermediate overflow can be cancelled")
    {
        status.equipment = -120;
        status.weight = std::numeric_limits<int16_t>::min();
        status.timed = std::numeric_limits<int16_t>::max();
        CHECK(status.get_value() == 11);
    }
}

TEST_CASE("PlayerStatusBase applies exceptions before clamping or narrowing")
{
    TestExceptionStatus status;
    SUBCASE("exception receives a positive sum beyond int16")
    {
        status.equipment = std::numeric_limits<int16_t>::max();
        CHECK(status.get_value() == 32);
        CHECK(status.received == 32877);
    }
    SUBCASE("exception receives a negative sum beyond int16")
    {
        status.equipment = -120;
        status.weight = std::numeric_limits<int16_t>::min();
        CHECK(status.get_value() == -32);
        CHECK(status.received == -32778);
    }
}
