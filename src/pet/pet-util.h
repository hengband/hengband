#pragma once

#include "system/angband.h"

enum pet_command {
    PET_DISMISS = 1, /*!< ペットに関するコマンド: ペットを離す */
    PET_TARGET = 2, /*!< ペットに関するコマンド: ペットのターゲットを指定 */
    PET_STAY_CLOSE = 3, /*!< ペットに関するコマンド: 近くにいろ */
    PET_FOLLOW_ME = 4, /*!< ペットに関するコマンド: ついて来い */
    PET_SEEK_AND_DESTROY = 5, /*!< ペットに関するコマンド: 敵を見つけて倒せ */
    PET_ALLOW_SPACE = 6, /*!< ペットに関するコマンド: 少し離れていろ */
    PET_STAY_AWAY = 7, /*!< ペットに関するコマンド: 離れていろ */
    PET_OPEN_DOORS = 8, /*!< ペットに関するコマンド: ドア解放の許可 */
    PET_TAKE_ITEMS = 9, /*!< ペットに関するコマンド: アイテム取得の許可 */
    PET_TELEPORT = 10, /*!< ペットに関するコマンド: テレポートの許可 */
    PET_ATTACK_SPELL = 11, /*!< ペットに関するコマンド: 攻撃魔法の許可 */
    PET_SUMMON_SPELL = 12, /*!< ペットに関するコマンド: 召喚魔法の許可 */
    PET_BALL_SPELL = 13, /*!< ペットに関するコマンド: プレイヤーを魔法に巻き込む許可 */
    PET_RIDING = 14, /*!< ペットに関するコマンド: ペットに乗る */
    PET_NAME = 15, /*!< ペットに関するコマンド: ペットに名前をつける */
    PET_TWO_HANDS = 16, /*!< ペットに関するコマンド: 騎乗中に両手で武器を使うかどうか */
};

enum pet_follow_distance {
    PET_CLOSE_DIST = 1, /*!<ペットの行動範囲…近くにいろ */
    PET_FOLLOW_DIST = 6, /*!<ペットの行動範囲…ついて来い */
    PET_SEEK_DIST = 10, /*!<ペットの行動範囲…特になし? */
    PET_DESTROY_DIST = 255, /*!<ペットの行動範囲…敵を見つけて倒せ */
    PET_SPACE_DIST = -10, /*!<ペットの行動範囲…少し離れていろ */
    PET_AWAY_DIST = -25, /*!<ペットの行動範囲…離れていろ */
};

enum pet_permission {
    PF_OPEN_DOORS = 0x0001, /*!< ペットの行動許可…ドアを開けてよい */
    PF_PICKUP_ITEMS = 0x0002, /*!< ペットの行動許可…アイテムを拾ってよい */
    PF_TELEPORT = 0x0004, /*!< ペットの行動許可…テレポート魔法を使ってよい */
    PF_ATTACK_SPELL = 0x0008, /*!< ペットの行動許可…攻撃魔法を使ってよい */
    PF_SUMMON_SPELL = 0x0010, /*!< ペットの行動許可…召喚魔法を使ってよい */
    PF_BALL_SPELL = 0x0020, /*!< ペットの行動許可…ボール魔法でプレイヤーを巻き込んでよい */
    PF_TWO_HANDS = 0x0040, /*!< プレイヤーの騎乗フラグ…武器を片手で持つ */
};

extern int total_friends;

class Grid;
class PlayerType;
bool can_player_ride_pet(PlayerType *player_ptr, const Grid *g_ptr, bool now_riding);
PERCENTAGE calculate_upkeep(PlayerType *player_ptr);
