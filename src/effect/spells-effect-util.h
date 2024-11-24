#pragma once

#include "system/angband.h"
#include <string>

enum class MonraceId : short;
class PlayerType;

class CapturedMonsterType {
public:
    CapturedMonsterType();
    MonraceId r_idx;
    byte speed = STANDARD_SPEED;
    short current_hp = 0;
    short max_hp = 0;
    std::string nickname = "";
};

class FallOffHorseEffect {
public:
    FallOffHorseEffect(PlayerType *player_ptr);
    void set_shake_off(int damage);
    void set_fall_off(int damage);
    void apply() const;

private:
    constexpr static int FALL_OFF_DAMAGE_MAX = 200;

    PlayerType *player_ptr;
    int shake_off_damage = 0; /*!< 振り落とされた際のダメージ量 */
    int fall_off_damage = 0; /*!< 落馬した際のダメージ量 */
};

extern bool sukekaku;
extern int project_length; /*!< 投射の射程距離 */

extern int project_m_n; /*!< 魔法効果範囲内にいるモンスターの数 */
extern POSITION project_m_x; /*!< 処理中のモンスターX座標 */
extern POSITION project_m_y; /*!< 処理中のモンスターY座標 */
extern POSITION monster_target_x; /*!< モンスターの攻撃目標X座標 */
extern POSITION monster_target_y; /*!< モンスターの攻撃目標Y座標 */
