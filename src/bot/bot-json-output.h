#pragma once

#include "object-enchant/tr-flags.h"
#include <functional>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>
#include <tl/optional.hpp>
#include <vector>

class ItemEntity;
class MonsterEntity;
class PlayerType;
class Store;
class StoreScreen;
enum class MonraceId : short;

/*!
 * @brief プレイヤーの特性を供給源ごとに集めたもの
 * @details キャラクター画面の特性のページの列と同じく、装備・本人 (常時と一時)・乗馬に分けて持つ。
 */
struct BotPlayerKnownFlags {
    TrFlags equipment; //!< 鑑定済みの装備の特性 (get_flags_known())
    TrFlags permanent; //!< 種族・職業・突然変異・性格の特性 (player_flags())
    TrFlags temporary; //!< 時限効果と構えの特性 (tim_player_flags())
    TrFlags riding; //!< 乗っている馬で得ている特性 (riding_flags())
    TrFlags riding_negative; //!< 乗っている馬のために失っている特性 (riding_flags())
};

/*!
 * @brief JSONスナップショットのキーを足す・変える・消す変更で更新する
 * @details 版ごとの追加・変更・削除キーは tools/bot/README.md の「版4での変更」「版3での変更」などに記載する。
 */
inline constexpr auto BOT_JSON_PROTOCOL_VERSION = 4;

enum class BotKnowledgeCategory {
    ARTIFACTS_KNOWN,
    ARTIFACTS_IDENTIFIED,
    OBJECTS_KNOWN,
    UNIQUES_ALIVE,
    UNIQUES_DEAD,
    BOUNTY,
    HOME,
    EQUIP_RESISTANCES,
    FEATURES,
    SELF_INFO,
    MUTATIONS,
    WEAPON_EXP,
    SPELL_EXP,
    SKILL_EXP,
    VIRTUES,
    DUNGEONS,
    QUESTS,
    PETS,
    AUTOPICK,
    MONSTERS,
    KILL_COUNT,
    MAX,
};

/*!
 * @brief 呪文一覧 (魔法書の閲覧・詠唱メニュー) の1行
 * @details print_spells() が画面に書いた値そのものを持つ。必殺剣は画面に熟練度・失敗率・効果を
 * 出さないので、それらの項目は空のまま出力しない。
 */
struct BotSpellListRow {
    int spell_id = 0; //!< 呪文ID
    std::string name; //!< 呪文名
    std::string status; //!< available / untried / unknown / forgotten / illegible
    int level = 0; //!< 習得レベル
    int mana = 0; //!< 消費MP
    tl::optional<int> fail; //!< 失敗率 (%)
    std::string proficiency; //!< 熟練度の略記 ("[初心]" 等の画面表記)
    bool proficiency_mark = false; //!< 熟練度の前の '!'
    std::string info; //!< 効果欄 (未知・忘却・未経験の場合はその表記)
};

/*!
 * @brief 特殊能力一覧 (種族/職業/突然変異のパワー、超能力等の職業固有の技) の1行
 */
struct BotPowerListRow {
    char letter; //!< 選択キー
    int page; //!< 表示されるページ
    std::string name; //!< 能力名
    int level; //!< 使用可能レベル
    int cost; //!< 消費MP
    int fail; //!< 失敗率 (%)
    std::string info; //!< 効果欄
};

/*!
 * @brief 開いている画面の情報を、制御サーバの state のスナップショットに足すためのもの
 */
struct BotScreen {
    std::function<void(nlohmann::json &snapshot, PlayerType *player_ptr)> add_to_snapshot; //!< スナップショットの type を変え、画面の情報を足す
    bool include_map = true; //!< 地図を載せてよいか (state の map の指定より優先して地図を省く)
};

/*!
 * @brief 画面を開いている間、その画面を登録する
 * @details 制御サーバの state は、登録された画面があれば、JSONL のその画面のスナップショットと同じく
 * type を変えて画面の情報を載せる。入れ子に登録したときは、最後に登録した画面が有効になる。
 */
class BotScreenScope {
public:
    explicit BotScreenScope(BotScreen screen);
    ~BotScreenScope();

    BotScreenScope(const BotScreenScope &) = delete;
    BotScreenScope &operator=(const BotScreenScope &) = delete;
    BotScreenScope(BotScreenScope &&) = delete;
    BotScreenScope &operator=(BotScreenScope &&) = delete;

private:
    BotScreen registered; //!< 登録した画面
    const BotScreen *previous; //!< 前に登録されていた画面
};

/*!
 * @brief 描いたときに中身が決まる画面 (呪文の一覧など) を、開いている間だけ登録する
 * @details 一覧を出すメニューの間などに置く。一覧を描くと、output_bot_json_*_snapshot() が
 * いちばん内側のこのスコープの登録を、描いた中身に入れ替える。一覧を隠したら hide_bot_drawn_screen() で登録を外す。
 * 登録がない間は、外側で登録された画面 (店など) が有効になる。
 * 中身を入れ替えるときに登録し直すので、このスコープより内側に BotScreenScope を置いたまま一覧を描かないこと。
 * 構築した後に登録を書き換えるので、const で宣言しないこと。
 */
class BotDrawnScreenScope {
public:
    BotDrawnScreenScope();
    ~BotDrawnScreenScope();

    BotDrawnScreenScope(const BotDrawnScreenScope &) = delete;
    BotDrawnScreenScope &operator=(const BotDrawnScreenScope &) = delete;
    BotDrawnScreenScope(BotDrawnScreenScope &&) = delete;
    BotDrawnScreenScope &operator=(BotDrawnScreenScope &&) = delete;

private:
    friend void show_bot_drawn_screen(BotScreen screen);
    friend void hide_bot_drawn_screen();

    tl::optional<BotScreenScope> registered; //!< 描いた中身の登録
    BotDrawnScreenScope *previous; //!< 外側のスコープ
};

void hide_bot_drawn_screen();
BotScreen make_bot_store_screen(const StoreScreen &screen);
BotScreen make_bot_store_screen(StoreScreen &&screen) = delete;
BotScreen make_bot_character_screen();
tl::optional<BotKnowledgeCategory> find_bot_knowledge_category(int key);
BotScreen make_bot_knowledge_screen(BotKnowledgeCategory category);
BotScreen make_bot_look_screen();

std::string to_json_utf8(std::string_view str);
nlohmann::json make_message_history_json(int count);
nlohmann::json make_bot_proficiency_values_json(int exp, int max, bool show_values, bool show_debug, bool cap_exp = false);
nlohmann::json make_bot_character_speed_json(int total, int temporary, bool lightspeed, bool riding);
nlohmann::json make_bot_item_identity_json(const ItemEntity &item, bool store_identified);
nlohmann::json make_bot_found_item_identity_json(const ItemEntity &item, bool is_hallucinated);
nlohmann::json make_bot_pet_row_json(int index, const MonsterEntity &monster, bool is_hallucinated);
nlohmann::json make_bot_player_ability_sources_json(const BotPlayerKnownFlags &flags);
nlohmann::json make_bot_json_snapshot(PlayerType *player_ptr, bool include_map = true);
void output_bot_json_snapshot(PlayerType *player_ptr);
void output_bot_json_screen_snapshot(PlayerType *player_ptr);
void output_bot_json_spell_list_snapshot(PlayerType *player_ptr, int realm_id, const std::vector<BotSpellListRow> &rows);
void output_bot_json_power_list_snapshot(PlayerType *player_ptr, std::string_view kind, const std::vector<BotPowerListRow> &rows, int page, bool browse_mode);
void output_bot_json_lore_snapshot(PlayerType *player_ptr, MonraceId monrace_id, int lore_mode);
