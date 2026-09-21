#pragma once

#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>
#include <tl/optional.hpp>
#include <vector>

class PlayerType;
class Store;
class StoreScreen;
enum class MonraceId : short;

/*!
 * @brief JSONスナップショットの互換性を壊す変更で更新する
 * @details 版ごとの追加・変更・削除キーは tools/bot/README.md の「版3での変更」に記載する。
 */
inline constexpr auto BOT_JSON_PROTOCOL_VERSION = 3;

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

std::string to_json_utf8(std::string_view str);
nlohmann::json make_message_history_json(int count);
nlohmann::json make_bot_json_snapshot(PlayerType *player_ptr, bool include_map = true);
void output_bot_json_snapshot(PlayerType *player_ptr);
void output_bot_json_store_snapshot(PlayerType *player_ptr, const StoreScreen &screen);
void output_bot_json_character_snapshot(PlayerType *player_ptr);
void output_bot_json_knowledge_snapshot(PlayerType *player_ptr, BotKnowledgeCategory category);
void output_bot_json_look_snapshot(PlayerType *player_ptr);
void output_bot_json_spell_list_snapshot(PlayerType *player_ptr, int realm_id, const std::vector<BotSpellListRow> &rows);
void output_bot_json_power_list_snapshot(PlayerType *player_ptr, std::string_view kind, const std::vector<BotPowerListRow> &rows, int page, bool browse_mode);
void output_bot_json_lore_snapshot(PlayerType *player_ptr, MonraceId monrace_id, int lore_mode);
