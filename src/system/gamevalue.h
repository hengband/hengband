#pragma once

#define PENETRATE_INVULNERABILITY 13 /*!< 無敵化が破られる確率(1/x) / 1/x chance of hurting even if invulnerable! */

#define MAX_TELEPORT_DISTANCE 200 /*!< テレポート最大距離 */

/*
 * Refueling constants
 */
#define FUEL_TORCH 5000 /*!< 松明の基本寿命値 / Maximum amount of fuel in a torch */
#define FUEL_LAMP 15000 /*!< ランタンの基本寿命値 / Maximum amount of fuel in a lantern */

#define MAX_SKILLS 10

#define TY_CURSE_CHANCE 200 /*!<太古の怨念の1ターン毎の発動確率(1/n)*/
#define CHAINSWORD_NOISE 100 /*!<チェンソーの1ターン毎の発動確率(1/n)*/

#define SPEAK_CHANCE 8
#define GRINDNOISE 20
#define CYBERNOISE 20

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

/*!< ベースアイテム生成階層が加算される確率 */
constexpr auto CHANCE_BASEITEM_LEVEL_BOOST = 10;

constexpr auto MAX_PLAYER_SIGHT = 20; /*!< プレイヤーの最大視界グリッド数 */
constexpr auto MAX_MONSTER_SENSING = 100; /*!< モンスターの最大感知グリッド数 */

/*!
 * @brief 1フロアに存在可能な、増殖フラグ付きモンスター実体の最大数
 * @details 呪術や突然変異で増殖阻止状態にすると、
 * フロア構造体の「増殖フラグ付きモンスター実体の現在数」が強制的に最大値まで引き上げられる.
 */
constexpr auto MAX_REPRODUCTION = 100;

/*!< 属性攻撃を受けた際に能力値低下を起こす確率(1/n) */
constexpr auto CHANCE_ABILITY_SCORE_DECREASE = 16;

/*!< ランダムアーティファクトにバイアス外の耐性がつき、4を超えるpvalを許可する確率 */
constexpr auto CHANCE_STRENGTHENING = 12;

/* プレイヤー/モンスターの標準速度 (加速0) */
constexpr auto STANDARD_SPEED = 110;

constexpr auto QUEST_DEFINITION_LIST = "QuestDefinitionList.txt";
constexpr auto TOWN_DEFINITION_LIST = "TownDefinitionList.txt";
constexpr auto WILDERNESS_DEFINITION = "WildernessDefinition.txt";
