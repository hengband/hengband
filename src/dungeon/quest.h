#pragma once

#include "system/angband.h"
#include "util/enum-converter.h"
#include <map>
#include <string>
#include <vector>

// clang-format off

/*
 * Quest constants
 */
enum class QuestId : int16_t{
	NONE = 0, /* クエストなし */
	THIEF = 1, /*<! 盗賊の隠れ家 */
	SEWER = 2, /*<! 下水道 */
	LOGURUS = 3, /*<! ログルス使い */
	VAULT = 4, /*<! 宝物庫 */
	TOWER1 = 5, /*<! 塔クエスト(第1階層)に割り振るクエストID */
	TOWER2 = 6, /*<! 塔クエスト(第2階層)に割り振るクエストID */
	TOWER3 = 7, /*<! 塔クエスト(第3階層)に割り振るクエストID */
	OBERON = 8, /*<! オベロン打倒クエストに割り振るクエストID */
	SERPENT = 9, /*<! サーペント打倒クエストに割り振るクエストID */
	SORCERER = 10, /*<! 仙術エネルギー特異点 */
	CHAOS = 11, /*<! カオスの特異点 */
	NATURE = 12, /*<! 自然魔術の特異点 */
	WARG = 14, /*<! ワーグを殲滅せよ */
	ERIC = 15, /*<! エリックの要塞 */
	MONSALVAT = 16, /*<! モンサルヴァト城への侵攻 */
	CITY_SEA = 17, /*<! 海底都市(現在は没) */
	WATER_CAVE = 18, /*<! 湖の洞窟 */
	DOOM_1 = 19, /*<! 破滅のクエスト１ */
	VAPOR = 20, /*<! 謎の瘴気 */
	DOOM_2 = 21, /*<! 破滅のクエスト２ */
	ORC_CAMP = 22, /*<! オークのキャンプ */
	SPAWNING = 23, /*<! 増殖地獄 */
	MS = 24, /*<! マイクロンフトの興亡 */
	HAUNTED_HOUCE = 25, /*<! 幽霊屋敷 */
	KILLING_FIELDS = 26, /*<! 激戦場 */
	OLD_CASTLE = 27, /*<! 古い城 */
	ROYAL_CRYPT = 28, /*<! 王家の墓 */
	MIMIC = 29, /*<! ミミックの財宝 */
	TENGU = 30, /*<! テングとデスソード */
	WILLOW = 31, /*<! 柳じじい */
	DARK_ELF = 32, /*<! ダークエルフの王 */
	CLONE = 33, /*<! クローン地獄 */
	DUMP_WITNESS = 34, /*<! もの言えぬ証人 */
	RANDOM_QUEST1 = 40, /*<! ランダムクエストを割り当てるクエストIDの開始値 */
	RANDOM_QUEST2 = 41,
	RANDOM_QUEST3 = 42,
	RANDOM_QUEST4 = 43,
	RANDOM_QUEST5 = 44,
	RANDOM_QUEST6 = 45,
	RANDOM_QUEST7 = 46,
	RANDOM_QUEST8 = 47,
	RANDOM_QUEST9 = 48,
	RANDOM_QUEST10 = 49, /*<! ランダムクエストを割り当てるクエストIDの終了値 */
};

constexpr auto MIN_RANDOM_QUEST = enum2i(QuestId::RANDOM_QUEST1);
constexpr auto MAX_RANDOM_QUEST = enum2i(QuestId::RANDOM_QUEST10);

constexpr uint QUEST_FLAG_SILENT = 0x01; /*!< クエストフラグ: クエスト進行に関する情報表示を抑止する / no messages from completion */
constexpr uint QUEST_FLAG_PRESET = 0x02; /*!< クエストフラグ: クエストがダンジョン外で発生する / quest is outside the main dungeon */
constexpr uint QUEST_FLAG_ONCE   = 0x04; /*!< クエストフラグ: クエストがフロアを出た時点で完了する / quest is marked finished after leaving */
constexpr uint QUEST_FLAG_TOWER  = 0x08; /*!< クエストフラグ: クエスト:塔の形式で進行する / Tower quest is special */

enum class QuestStatusType : short {
	UNTAKEN = 0,         /*!< クエストステータス状態：未発生*/
	TAKEN = 1,           /*!< クエストステータス状態：発生中*/
	COMPLETED=  2,       /*!< クエストステータス状態：達成*/
	REWARDED = 3,        /*!< クエストステータス状態：報酬受け取り前*/
	FINISHED = 4,        /*!< クエストステータス状態：完了*/
	FAILED = 5,          /*!< クエストステータス状態：失敗*/
	FAILED_DONE = 6,     /*!< クエストステータス状態：失敗完了*/
	STAGE_COMPLETED = 7, /*!< クエストステータス状態：ステージ毎達成*/
};

enum class QuestKindType : short {
	NONE = 0,                      /*!< ダミー、クエストではない */
	KILL_LEVEL = 1,     /*!< クエスト目的: 特定のユニークモンスターを倒す */
	KILL_ANY_LEVEL = 2, /*!< クエスト目的: イベント受託時点でランダムで選ばれた特定のユニークモンスターを倒す */
	FIND_ARTIFACT = 3,  /*!< クエスト目的: 特定のアーティファクトを発見する */
	FIND_EXIT = 4,      /*!< クエスト目的: 脱出する */
	KILL_NUMBER = 5,    /*!< クエスト目的: モンスターを無差別に特定数倒す */
	KILL_ALL = 6,       /*!< クエスト目的: エリア中のすべてのモンスターを全て倒す */
	RANDOM = 7,         /*!< クエスト目的: ランダムクエストとして選ばれたユニーク1体を倒す */
	TOWER = 8,          /*!< クエスト目的: 複数のエリアの全てのモンスターを倒す */
};

// clang-format on

/*!
 * @brief クエスト情報の構造体 / Structure for the "quests".
 */
enum class FixedArtifactId : short;
enum class MonsterRaceId : int16_t;
class ArtifactType;
class QuestType {
public:
    QuestType() = default;
    virtual ~QuestType() = default;
    QuestStatusType status{}; /*!< クエストの進行ステータス / Is the quest taken, completed, finished? */
    QuestKindType type{}; /*!< クエストの種別 / The quest type */

    std::string name = ""; /*!< クエスト名 / Quest name */
    DEPTH level = 0; /*!< 処理階層 / Dungeon level */
    MonsterRaceId r_idx{}; /*!< クエスト対象のモンスターID / Monster race */

    MONSTER_NUMBER cur_num = 0; /*!< 撃破したモンスターの数 / Number killed */
    MONSTER_NUMBER max_num = 0; /*!< 求められるモンスターの撃破数 / Number required */

    FixedArtifactId reward_artifact_idx{}; /*!< クエスト対象のアイテムID / object index */
    MONSTER_NUMBER num_mon = 0; /*!< QuestKindTypeがKILL_NUMBER時の目標撃破数 number of monsters on level */

    BIT_FLAGS flags = 0; /*!< クエストに関するフラグビット / quest flags */
    DUNGEON_IDX dungeon = 0; /*!< クエスト対象のダンジョンID / quest dungeon */

    PLAYER_LEVEL complev = 0; /*!< クリア時プレイヤーレベル / player level (complete) */
    REAL_TIME comptime = 0; /*!< クリア時ゲーム時間 /  quest clear time*/

    static bool is_fixed(QuestId quest_idx);
    bool has_reward() const;
    ArtifactType &get_reward() const;
};

class QuestList final {
public:
    using iterator = std::map<QuestId, QuestType>::iterator;
    using reverse_iterator = std::map<QuestId, QuestType>::reverse_iterator;
    using const_iterator = std::map<QuestId, QuestType>::const_iterator;
    using const_reverse_iterator = std::map<QuestId, QuestType>::const_reverse_iterator;
    static QuestList &get_instance();
    QuestType &operator[](QuestId id);
    const QuestType &operator[](QuestId id) const;
    iterator begin();
    const_iterator begin() const;
    iterator end();
    const_iterator end() const;
    reverse_iterator rbegin();
    const_reverse_iterator rbegin() const;
    reverse_iterator rend();
    const_reverse_iterator rend() const;
    iterator find(QuestId id);
    const_iterator find(QuestId id) const;
    size_t size() const;
    void initialize();
    QuestList(const QuestList &) = delete;
    QuestList(QuestList &&) = delete;
    QuestList &operator=(const QuestList &) = delete;
    QuestList &operator=(QuestList &&) = delete;

private:
    bool initialized = false;
    std::map<QuestId, QuestType> quest_data;
    QuestList() = default;
    ~QuestList() = default;
};

extern char quest_text[10][80];
extern int quest_text_line;
extern QuestId leaving_quest;

class ItemEntity;
class PlayerType;
void determine_random_questor(PlayerType *player_ptr, QuestType *q_ptr);
void record_quest_final_status(QuestType *q_ptr, PLAYER_LEVEL lev, QuestStatusType stat);
void complete_quest(PlayerType *player_ptr, QuestId quest_num);
void check_find_art_quest_completion(PlayerType *player_ptr, ItemEntity *o_ptr);
void quest_discovery(QuestId q_idx);
QuestId quest_number(PlayerType *player_ptr, DEPTH level);
QuestId random_quest_number(PlayerType *player_ptr, DEPTH level);
void leave_quest_check(PlayerType *player_ptr);
void leave_tower_check(PlayerType *player_ptr);
void exe_enter_quest(PlayerType *player_ptr, QuestId quest_idx);
void do_cmd_quest(PlayerType *player_ptr);
bool inside_quest(QuestId id);
