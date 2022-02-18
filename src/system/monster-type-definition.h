#pragma once

#include "monster/monster-flag-types.h"
#include "monster/monster-timed-effect-types.h"
#include "monster/smart-learn-types.h"
#include "object/object-index-list.h"
#include "util/flag-group.h"

/*!
 * @brief Monster information, for a specific monster.
 * @Note
 * fy, fx constrain dungeon size to 256x256
 * The "hold_o_idx" field points to the first object of a stack
 * of objects (if any) being carried by the monster (see above).
 */
constexpr int MONSTER_MAXHP = 30000; //!< モンスターの最大HP

struct floor_type;
struct monster_race;
struct monster_type {
	MONRACE_IDX r_idx{};		/*!< モンスターの実種族ID (これが0の時は死亡扱いになる) / Monster race index 0 = dead. */
	MONRACE_IDX ap_r_idx{};	/*!< モンスターの外見種族ID（あやしい影、たぬき、ジュラル星人誤認などにより変化する）Monster race appearance index */
	floor_type *current_floor_ptr{}; /*!< 所在フロアID（現状はfloor_type構造体によるオブジェクトは1つしかないためソースコード設計上の意義以外はない）*/

	/* Sub-alignment flags for neutral monsters */
	#define SUB_ALIGN_NEUTRAL 0x0000 /*!< モンスターのサブアライメント:中立 */
	#define SUB_ALIGN_EVIL    0x0001 /*!< モンスターのサブアライメント:善 */
	#define SUB_ALIGN_GOOD    0x0002 /*!< モンスターのサブアライメント:悪 */
	BIT_FLAGS8 sub_align{};	/*!< 中立属性のモンスターが召喚主のアライメントに従い一時的に立っている善悪陣営 / Sub-alignment for a neutral monster */

	POSITION fy{};		/*!< 所在グリッドY座標 / Y location on map */
	POSITION fx{};		/*!< 所在グリッドX座標 / X location on map */
	int hp{};		/*!< 現在のHP / Current Hit points */
	int maxhp{};		/*!< 現在の最大HP(衰弱効果などにより低下したものの反映) / Max Hit points */
	int max_maxhp{};		/*!< 生成時の初期最大HP / Max Max Hit points */
	int dealt_damage{};		/*!< これまでに蓄積して与えてきたダメージ / Sum of damages dealt by player */
	TIME_EFFECT mtimed[MAX_MTIMED]{};	/*!< 与えられた時限効果の残りターン / Timed status counter */
	byte mspeed{};	        /*!< モンスターの個体加速値 / Monster "speed" */
	ACTION_ENERGY energy_need{};	/*!< モンスター次ターンまでに必要な行動エネルギー / Monster "energy" */
	POSITION cdis{};		/*!< 現在のプレイヤーから距離(逐一計算を避けるためのテンポラリ変数) Current dis from player */
	EnumClassFlagGroup<MonsterTemporaryFlagType> mflag{};	/*!< モンスター個体に与えられた特殊フラグ1 (セーブ不要) / Extra monster flags */
	EnumClassFlagGroup<MonsterConstantFlagType> mflag2{};	/*!< モンスター個体に与えられた特殊フラグ2 (セーブ必要) / Extra monster flags */
	bool ml{};		/*!< モンスターがプレイヤーにとって視認できるか(処理のためのテンポラリ変数) Monster is "visible" */
	ObjectIndexList hold_o_idx_list{};	/*!< モンスターが所持しているアイテムのリスト / Object list being held (if any) */
	POSITION target_y{};		/*!< モンスターの攻撃目標対象Y座標 / Can attack !los player */
	POSITION target_x{};		/*!< モンスターの攻撃目標対象X座標 /  Can attack !los player */
	STR_OFFSET nickname{};	/*!< ペットに与えられた名前の保存先文字列オフセット Monster's Nickname */
	EXP exp{}; /*!< モンスターの現在所持経験値 */

	/* TODO: クローン、ペット、有効化は意義が異なるので別変数に切り離すこと。save/loadのバージョン更新が面倒そうだけど */
	EnumClassFlagGroup<MonsterSmartLearnType> smart{}; /*!< モンスターのプレイヤーに対する学習状態 / Field for "smart_learn" - Some bit-flags for the "smart" field */
	MONSTER_IDX parent_m_idx{}; /*!< 召喚主のモンスターID */
};
