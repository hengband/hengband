#pragma once

/*!
 * @brief ベースアイテム生成階層が加算される確率
 * @details
 * There is a 1/10 (10%) chance of inflating the requested object_level
 * during the creation of an object (see "get_obj_num()" in "object.c").
 * Lower values yield better objects more often.
 */
#define GREAT_OBJ 10

/*!
 * @brief 深層モンスターが生成される(NASTY生成)の基本確率(1/x)
 * @details
 * There is a 1/25 (4%) chance of inflating the requested monster_level
 * during the creation of a monsters (see "get_mon_num()" in "monster.c").
 * Lower values yield harder monsters more often.
 */
#define NASTY_MON_BASE 25
#define NASTY_MON_MAX 3 /*!< 深層モンスターが1フロアに生成される最大数  */
#define NASTY_MON_PLUS_MAX 25 /*!< 深層モンスターの階層加算最大量 */

#define PENETRATE_INVULNERABILITY 13 /*!< 無敵化が破られる確率(1/x) / 1/x chance of hurting even if invulnerable! */

#define MAX_TELEPORT_DISTANCE 200 /*!< テレポート最大距離 */

/*
 * Refueling constants
 */
#define FUEL_TORCH 5000 /*!< 松明の基本寿命値 / Maximum amount of fuel in a torch */
#define FUEL_LAMP 15000 /*!< ランタンの基本寿命値 / Maximum amount of fuel in a lantern */

/*
 * More maximum values
 */
#define MAX_SIGHT 20 /*!< プレイヤーの最大視界範囲(マス) / Maximum view distance */

#define AAF_LIMIT 100 /*!< モンスターの限界感知範囲(マス) Limit of sensing radius */

#define MIN_M_ALLOC_TD 4 /*!< 街(昼間)の最低住人配置数 / The town starts out with 4 residents during the day */
#define MIN_M_ALLOC_TN 8 /*!< 街(夜間)の最低住人配置数 / The town starts out with 8 residents during the night */

#define MAX_SKILLS 10

/*!
 * @brief モンスター増殖の最大数
 * @details
 * A monster can only "multiply" (reproduce) if there are fewer than 100
 * monsters on the level capable of such spontaneous reproduction.  This
 * is a hack which prevents the "m_list[]" array from exploding due to
 * reproducing monsters.  Messy, but necessary.
 */
#define MAX_REPRO 100

#define BIAS_LUCK 20 /*!< 1/nの確率でrandom_resistance()で付加する元素耐性が免疫になる */
#define IM_LUCK 7 /*!< 1/nの確率でrandom_resistance()で複数免疫の除去処理が免除される */

#define TY_CURSE_CHANCE 200 /*!<太古の怨念の1ターン毎の発動確率(1/n)*/
#define CHAINSWORD_NOISE 100 /*!<チェンソーの1ターン毎の発動確率(1/n)*/

#define SPEAK_CHANCE 8
#define GRINDNOISE 20
#define CYBERNOISE 20

#define GROUP_MAX 32 /*!< place_monster_group() 関数によるモンスターのGROUP生成時の配置最大数 / Maximum size of a group of monsters */

/* ToDo: Make this global */
#define HURT_CHANCE 16 /*!< 属性攻撃を受けた際に能力値低下を起こす確率(1/X) / 1/x chance of reducing stats (for elemental attacks) */

/*
 * Misc constants
 */
#define TOWN_DAWN 10000 /*!< 1日分のターン / Number of ticks from dawn to dawn XXX */
#define TURNS_PER_TICK 10L /*!< 時間経過処理を行うターン数の刻み / Number of energy-gain-turns per ticks */
#define INN_DUNGEON_TURN_ADJ 10 /*!< 宿屋で時間をつぶした場合に増える dungeon_turn の倍率 */
#define MAX_DAYS 20000 /*!< 内部処理中で保持される最大日数 / Maximum days */
#define BREAK_RUNE_PROTECTION 550 /*!< 守りのルーンの強靭度 / Rune of protection resistance */
#define BREAK_RUNE_EXPLOSION 299 /*!< 爆発のルーンの発動しやすさ / For explosive runes */
#define BTH_PLUS_ADJ 3 /*!< 武器経験値及びプレイヤーの打撃/射撃能力に応じた修正値倍率 / Adjust BTH per plus-to-hit */
#define MON_MULT_ADJ 8 /*!< モンスターの増殖しにくさの基本倍率 / High value slows ENERGY_MULTIPLICATION */
#define MON_SUMMON_ADJ 2 /*!< 現在未使用 Adjust level of summoned creatures */
#define MON_DRAIN_LIFE 2 /*!< モンスターの打撃によるプレイヤーの経験値吸収基本倍率(%) / Percent of player exp drained per hit */
#define USE_DEVICE 3 /*!< 魔道具の最低失敗基準値 x> Harder devices x< Easier devices     */

/*!< ランダムアーティファクトにバイアス外の耐性がつき、4を超えるpvalを許可する確率 */
constexpr auto CHANCE_STRENGTHENING = 12;

/* プレイヤー/モンスターの標準速度 (加速0) */
constexpr auto STANDARD_SPEED = 110;

constexpr auto QUEST_DEFINITION_LIST = "QuestDefinitionList.txt";
constexpr auto TOWN_DEFINITION_LIST = "TownDefinitionList.txt";
constexpr auto WILDERNESS_DEFINITION = "WildernessDefinition.txt";
