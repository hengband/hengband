/*
 * @brief ゲーム内で広範に使われる定数群の定義
 * @author Hourier
 * @date 2022/11/02
 */

#pragma once

constexpr auto PENETRATE_INVULNERABILITY = 13; /*!< 無敵化が破られる確率(1/n) */
constexpr auto FUEL_TORCH = 5000; /*!< 松明の基本寿命値 */
constexpr auto FUEL_LAMP = 15000; /*!< ランタンの基本寿命値 */
constexpr auto TOWN_DAWN = 10000; /*!< 1日分のターン */
constexpr auto TURNS_PER_TICK = 10L; /*!< 時間経過処理を行うターン数の刻み */
constexpr auto MAX_DAYS = 20000; /*!< 内部処理中で保持される最大日数 */
constexpr auto BTH_PLUS_ADJ = 3; /*!< 武器経験値及びプレイヤーの打撃/射撃能力に応じた修正値倍率 */
constexpr auto MON_DRAIN_LIFE = 2; /*!< モンスターの打撃によるプレイヤーの経験値吸収基本倍率(%) */
constexpr auto USE_DEVICE = 3; /*!< 魔道具の最低失敗基準値 */
constexpr auto CHANCE_BASEITEM_LEVEL_BOOST = 10; /*!< ベースアイテム生成階層が加算される確率 */
constexpr auto MAX_PLAYER_SIGHT = 20; /*!< プレイヤーの最大視界グリッド数 */
constexpr auto MAX_MONSTER_SENSING = 100; /*!< モンスターの最大感知グリッド数 */
constexpr auto CHANCE_ABILITY_SCORE_DECREASE = 16; /*!< 属性攻撃を受けた際に能力値低下を起こす確率(1/n) */
constexpr auto CHANCE_STRENGTHENING = 12; /*!< ランダムアーティファクトにバイアス外の耐性がつき、4を超えるpvalを許可する確率 */
constexpr auto STANDARD_SPEED = 110; /* プレイヤー/モンスターの標準速度 (加速0) */

/*!
 * @brief 1フロアに存在可能な、増殖フラグ付きモンスター実体の最大数
 * @details 呪術や突然変異で増殖阻止状態にすると、
 * フロア構造体の「増殖フラグ付きモンスター実体の現在数」が強制的に最大値まで引き上げられる.
 */
constexpr auto MAX_REPRODUCTION = 100;

constexpr auto QUEST_DEFINITION_LIST = "QuestDefinitionList.txt";
constexpr auto TOWN_DEFINITION_LIST = "TownDefinitionList.txt";
constexpr auto WILDERNESS_DEFINITION = "WildernessDefinition.txt";
