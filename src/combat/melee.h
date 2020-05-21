#include "combat/combat-options-type.h"

 /*!
  * @note モンスターの攻撃効果 / New monster blow effects
  */
#define RBE_HURT         1 /*!< モンスターの攻撃効果: 攻撃する*/
#define RBE_POISON       2 /*!< モンスターの攻撃効果: 毒をくらわす*/
#define RBE_UN_BONUS     3 /*!< モンスターの攻撃効果: 劣化させる*/
#define RBE_UN_POWER     4 /*!< モンスターの攻撃効果: 充填魔力を吸収する*/
#define RBE_EAT_GOLD     5 /*!< モンスターの攻撃効果: 金を盗む*/
#define RBE_EAT_ITEM     6 /*!< モンスターの攻撃効果: アイテムを盗む*/
#define RBE_EAT_FOOD     7 /*!< モンスターの攻撃効果: あなたの食糧を食べる*/
#define RBE_EAT_LITE     8 /*!< モンスターの攻撃効果: 明かりを吸収する*/
#define RBE_ACID         9 /*!< モンスターの攻撃効果: 酸を飛ばす*/
#define RBE_ELEC        10 /*!< モンスターの攻撃効果: 感電させる*/
#define RBE_FIRE        11 /*!< モンスターの攻撃効果: 燃やす*/
#define RBE_COLD        12 /*!< モンスターの攻撃効果: 凍らせる*/
#define RBE_BLIND       13 /*!< モンスターの攻撃効果: 盲目にする*/
#define RBE_CONFUSE     14 /*!< モンスターの攻撃効果: 混乱させる*/
#define RBE_TERRIFY     15 /*!< モンスターの攻撃効果: 恐怖させる*/
#define RBE_PARALYZE    16 /*!< モンスターの攻撃効果: 麻痺させる*/
#define RBE_LOSE_STR    17 /*!< モンスターの攻撃効果: 腕力を減少させる*/
#define RBE_LOSE_INT    18 /*!< モンスターの攻撃効果: 知能を減少させる*/
#define RBE_LOSE_WIS    19 /*!< モンスターの攻撃効果: 賢さを減少させる*/
#define RBE_LOSE_DEX    20 /*!< モンスターの攻撃効果: 器用さを減少させる*/
#define RBE_LOSE_CON    21 /*!< モンスターの攻撃効果: 耐久力を減少させる*/
#define RBE_LOSE_CHR    22 /*!< モンスターの攻撃効果: 魅力を減少させる*/
#define RBE_LOSE_ALL    23 /*!< モンスターの攻撃効果: 全ステータスを減少させる*/
#define RBE_SHATTER     24 /*!< モンスターの攻撃効果: 粉砕する*/
#define RBE_EXP_10      25 /*!< モンスターの攻撃効果: 経験値を減少(10d6+)させる*/
#define RBE_EXP_20      26 /*!< モンスターの攻撃効果: 経験値を減少(20d6+)させる*/
#define RBE_EXP_40      27 /*!< モンスターの攻撃効果: 経験値を減少(40d6+)させる*/
#define RBE_EXP_80      28 /*!< モンスターの攻撃効果: 経験値を減少(80d6+)させる*/
#define RBE_DISEASE     29 /*!< モンスターの攻撃効果: 病気にする*/
#define RBE_TIME        30 /*!< モンスターの攻撃効果: 時間を逆戻りさせる*/
#define RBE_DR_LIFE     31 /*!< モンスターの攻撃効果: 生命力を吸収する*/
#define RBE_DR_MANA     32 /*!< モンスターの攻撃効果: 魔力を奪う*/
#define RBE_SUPERHURT   33 /*!< モンスターの攻撃効果: 強力に攻撃する*/
#define RBE_INERTIA     34 /*!< モンスターの攻撃効果: 減速させる*/
#define RBE_STUN        35 /*!< モンスターの攻撃効果: 朦朧とさせる*/

extern bool test_hit_norm(player_type *attacker_ptr, HIT_RELIABILITY chance, ARMOUR_CLASS ac, bool visible);
extern PERCENTAGE hit_chance(player_type *attacker_ptr, HIT_RELIABILITY chance, ARMOUR_CLASS ac);
extern HIT_POINT tot_dam_aux(player_type *attacker_ptr, object_type *o_ptr, HIT_POINT tdam, monster_type *m_ptr, combat_options mode, bool thrown);
extern HIT_POINT critical_norm(player_type *attacker_ptr, WEIGHT weight, int plus, HIT_POINT dam, s16b meichuu, combat_options mode);
extern bool do_cmd_attack(player_type *attacker_ptr, POSITION y, POSITION x, combat_options mode);
extern bool make_attack_normal(player_type *targer_ptr, MONSTER_IDX m_idx);
extern void mon_take_hit_mon(player_type *player_ptr, MONSTER_IDX m_idx, HIT_POINT dam, bool *dead, bool *fear, concptr note, MONSTER_IDX who);
extern bool monst_attack_monst(player_type *subject_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx);
