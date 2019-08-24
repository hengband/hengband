#pragma once
#include "grid.h"

extern bool do_cmd_riding(player_type *creature_ptr, bool force);
extern PERCENTAGE calculate_upkeep(player_type *creature_ptr);
extern void do_cmd_pet_dismiss(player_type *creature_pt);
extern void do_cmd_pet(player_type *creature_ptr);
extern bool player_can_ride_aux(grid_type *g_ptr, bool now_riding);
extern bool rakuba(HIT_POINT dam, bool force);

extern int total_friends;

/*** Pet constants ***/


/*
 * ペットに関するコマンド群 / Pet Commands
 */
#define PET_DISMISS				1  /*!< ペットに関するコマンド: ペットを離す */
#define PET_TARGET				2  /*!< ペットに関するコマンド: ペットのターゲットを指定 */
#define PET_STAY_CLOSE			3  /*!< ペットに関するコマンド: 近くにいろ */
#define PET_FOLLOW_ME			4  /*!< ペットに関するコマンド: ついて来い */
#define PET_SEEK_AND_DESTROY	5  /*!< ペットに関するコマンド: 敵を見つけて倒せ */
#define PET_ALLOW_SPACE			6  /*!< ペットに関するコマンド: 少し離れていろ */
#define PET_STAY_AWAY			7  /*!< ペットに関するコマンド: 離れていろ */
#define PET_OPEN_DOORS			8  /*!< ペットに関するコマンド: ドア解放の許可 */
#define PET_TAKE_ITEMS			9  /*!< ペットに関するコマンド: アイテム取得の許可 */
#define PET_TELEPORT			10 /*!< ペットに関するコマンド: テレポートの許可 */
#define PET_ATTACK_SPELL		11 /*!< ペットに関するコマンド: 攻撃魔法の許可 */
#define PET_SUMMON_SPELL		12 /*!< ペットに関するコマンド: 召喚魔法の許可 */
#define PET_BALL_SPELL			13 /*!< ペットに関するコマンド: プレイヤーを魔法に巻き込む許可 */
#define PET_RIDING				14 /*!< ペットに関するコマンド: ペットに乗る */
#define PET_NAME				15 /*!< ペットに関するコマンド: ペットに名前をつける */
#define PET_RYOUTE				16 /*!< ペットに関するコマンド: 騎乗中に両手で武器を使うかどうか */

 /*
  * Follow distances
  */
#define PET_CLOSE_DIST				1		/*!<ペットの行動範囲…近くにいろ */
#define PET_FOLLOW_DIST				6		/*!<ペットの行動範囲…ついて来い */
#define PET_SEEK_DIST				10		/*!<ペットの行動範囲…特になし? */
#define PET_DESTROY_DIST			255		/*!<ペットの行動範囲…敵を見つけて倒せ */
#define PET_SPACE_DIST				(-10)	/*!<ペットの行動範囲…少し離れていろ */
#define PET_AWAY_DIST				(-25)	/*!<ペットの行動範囲…離れていろ */

#define PF_OPEN_DOORS   0x0001 /*!< ペットの行動許可…ドアを開けてよい */
#define PF_PICKUP_ITEMS 0x0002 /*!< ペットの行動許可…アイテムを拾ってよい */
#define PF_TELEPORT     0x0004 /*!< ペットの行動許可…テレポート魔法を使ってよい */
#define PF_ATTACK_SPELL 0x0008 /*!< ペットの行動許可…攻撃魔法を使ってよい */
#define PF_SUMMON_SPELL 0x0010 /*!< ペットの行動許可…召喚魔法を使ってよい */
#define PF_BALL_SPELL   0x0020 /*!< ペットの行動許可…ボール魔法でプレイヤーを巻き込んでよい */
#define PF_RYOUTE       0x0040 /*!< プレイヤーの騎乗フラグ…武器を片手で持つ */

