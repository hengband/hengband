/*!
 * @brief 日記へのメッセージ追加処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "io/write-diary.h"
#include "dungeon/quest.h"
#include "info-reader/fixed-map-parser.h"
#include "io/files-util.h"
#include "market/arena-entry.h"
#include "player/player-status.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-record.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-list.h"
#include "system/floor/floor-info.h"
#include "system/inner-game-data.h"
#include "system/monrace/monrace-definition.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <fmt/format.h>
#include <sstream>

bool write_level; //!< @todo *抹殺* したい…

/*!
 * @brief 日記ファイルを開く
 * @param fff ファイルへのポインタ
 * @param disable_diary 日記への追加を無効化する場合TRUE
 * @return ファイルがあったらTRUE、なかったらFALSE
 * @todo files.c に移すことも検討する？
 */
static bool open_diary_file(FILE **fff, bool *disable_diary)
{
    std::stringstream ss;
    ss << _("playrecord-", "playrec-") << savefile_base.string() << ".txt";
    const auto path = path_build(ANGBAND_DIR_USER, ss.str());
    *fff = angband_fopen(path, FileOpenMode::APPEND);
    if (*fff) {
        return true;
    }

    constexpr auto fmt = _("{} を開くことができませんでした。プレイ記録を一時停止します。", "Failed to open {}. Play-Record is disabled temporarily.");
    const auto &filename = path.string();
    msg_print(fmt, filename);
    msg_erase();
    *disable_diary = true;
    return false;
}

/*!
 * @brief フロア情報を日記に追加する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return クエストIDとレベルノートのペア
 */
static std::pair<QuestId, std::string> write_floor(const FloorType &floor)
{
    auto q_idx = floor.get_quest_id();
    if (!write_level) {
        return std::make_pair(q_idx, std::string());
    }

    if (floor.inside_arena) {
        return std::make_pair(q_idx, std::string(_("アリーナ:", "Arena:")));
    }

    if (!floor.is_underground()) {
        return std::make_pair(q_idx, std::string(_("地上:", "Surface:")));
    }

    if (inside_quest(q_idx) && QuestType::is_fixed(q_idx) && !((q_idx == QuestId::OBERON) || (q_idx == QuestId::SERPENT))) {
        return std::make_pair(q_idx, std::string(_("クエスト:", "Quest:")));
    }

    const auto &dungeon = floor.get_dungeon_definition();
    const auto desc = fmt::format(_("{0}階({1}):", "{1} L{0}:"), floor.dun_level, dungeon.name);
    return std::make_pair(q_idx, desc);
}

/*!
 * @brief 日記の1行の行頭として、時刻と階の表記を書き出す
 * @param fff 日記ファイル
 * @param hour 時
 * @param min 分
 * @param note_level 階の表記
 */
static void print_entry_prefix(FILE *fff, int hour, int min, std::string_view note_level)
{
    fmt::print(fff, " {:2}:{:02} {:>20} ", hour, min, fmt::bytes(note_level));
}

/*!
 * @brief 日記の1行を、時刻と階の表記に続けて書き出す
 * @param fff 日記ファイル
 * @param hour 時
 * @param min 分
 * @param note_level 階の表記
 * @param body_fmt 行頭に続ける内容の書式
 * @param args 書式の引数
 */
template <typename... Args>
static void print_entry(FILE *fff, int hour, int min, std::string_view note_level, fmt::format_string<Args...> body_fmt, Args &&...args)
{
    print_entry_prefix(fff, hour, min, note_level);
    fmt::print(fff, body_fmt, std::forward<Args>(args)...);
}

/*!
 * @brief ペットに関する日記を追加する
 * @param fff 日記ファイル
 * @param num 日記へ追加する内容番号
 * @param note 日記内容のIDに応じた文字列参照ポインタ
 */
static void write_diary_pet(FILE *fff, int num, std::string_view note)
{
    switch (num) {
    case RECORD_NAMED_PET_NAME:
        fmt::print(fff, _("{}を旅の友にすることに決めた。\n", "decided to travel together with {}.\n"), note);
        break;
    case RECORD_NAMED_PET_UNNAME:
        fmt::print(fff, _("{}の名前を消した。\n", "unnamed {}.\n"), note);
        break;
    case RECORD_NAMED_PET_DISMISS:
        fmt::print(fff, _("{}を解放した。\n", "dismissed {}.\n"), note);
        break;
    case RECORD_NAMED_PET_DEATH:
        fmt::print(fff, _("{}が死んでしまった。\n", "{} died.\n"), note);
        break;
    case RECORD_NAMED_PET_MOVED:
        fmt::print(fff, _("{}をおいて別のマップへ移動した。\n", "moved to another map leaving {} behind.\n"), note);
        break;
    case RECORD_NAMED_PET_LOST_SIGHT:
        fmt::print(fff, _("{}とはぐれてしまった。\n", "lost sight of {}.\n"), note);
        break;
    case RECORD_NAMED_PET_DESTROY:
        fmt::print(fff, _("{}が*破壊*によって消え去った。\n", "{} was killed by *destruction*.\n"), note);
        break;
    case RECORD_NAMED_PET_EARTHQUAKE:
        fmt::print(fff, _("{}が岩石に押し潰された。\n", "{} was crushed by falling rocks.\n"), note);
        break;
    case RECORD_NAMED_PET_GENOCIDE:
        fmt::print(fff, _("{}が抹殺によって消え去った。\n", "{} was a victim of genocide.\n"), note);
        break;
    case RECORD_NAMED_PET_WIZ_ZAP:
        fmt::print(fff, _("{}がデバッグコマンドによって消え去った。\n", "{} was removed by debug command.\n"), note);
        break;
    case RECORD_NAMED_PET_TELE_LEVEL:
        fmt::print(fff, _("{}がテレポート・レベルによって消え去った。\n", "{} was lost after teleporting a level.\n"), note);
        break;
    case RECORD_NAMED_PET_BLAST:
        fmt::print(fff, _("{}を爆破した。\n", "blasted {}.\n"), note);
        break;
    case RECORD_NAMED_PET_HEAL_LEPER:
        fmt::print(fff, _("{}の病気が治り旅から外れた。\n", "{} was healed and left.\n"), note);
        break;
    case RECORD_NAMED_PET_COMPACT:
        fmt::print(fff, _("{}がモンスター情報圧縮によって消え去った。\n", "{} was lost when the monster list was pruned.\n"), note);
        break;
    case RECORD_NAMED_PET_LOSE_PARENT:
        fmt::print(fff, _("{}の召喚者が既にいないため消え去った。\n", "{} disappeared because its summoner left.\n"), note);
        break;
    default:
        fputs("\n", fff);
        break;
    }
}

/*!
 * @brief 日記にクエストに関するメッセージを追加する
 * @param dk 日記内容のID
 * @param num 日記内容のIDに応じた番号
 * @return エラーコード
 */
int exe_write_diary_quest(PlayerType *player_ptr, DiaryKind dk, QuestId quest_id)
{
    static auto disable_diary = false;
    const auto &[day, hour, min] = AngbandWorld::get_instance().extract_date_time(InnerGameData::get_instance().get_start_race());
    if (disable_diary) {
        return -1;
    }

    auto &floor = *player_ptr->current_floor_ptr;
    const auto old_quest = floor.quest_number;
    const auto &quests = QuestList::get_instance();
    const auto &quest = quests.get_quest(quest_id);
    floor.quest_number = old_quest;

    const auto &[q_idx, note_level] = write_floor(floor);

    FILE *fff = nullptr;
    if (!open_diary_file(&fff, &disable_diary)) {
        return -1;
    }

    auto do_level = true;
    switch (dk) {
    case DiaryKind::FIX_QUEST_C: {
        if (any_bits(quest.flags, QUEST_FLAG_SILENT)) {
            break;
        }

        constexpr auto fmt = _("クエスト「{}」を達成した。\n", "completed quest '{}'.\n");
        print_entry(fff, hour, min, note_level, fmt, quest.name);
        break;
    }
    case DiaryKind::FIX_QUEST_F: {
        if (any_bits(quest.flags, QUEST_FLAG_SILENT)) {
            break;
        }

        constexpr auto fmt = _("クエスト「{}」から命からがら逃げ帰った。\n", "ran away from quest '{}'.\n");
        print_entry(fff, hour, min, note_level, fmt, quest.name);
        break;
    }
    case DiaryKind::RAND_QUEST_C: {
        constexpr auto fmt = _("ランダムクエスト({})を達成した。\n", "completed random quest '{}'\n");
        print_entry(fff, hour, min, note_level, fmt, quest.get_bounty().name);
        break;
    }
    case DiaryKind::RAND_QUEST_F: {
        constexpr auto fmt = _("ランダムクエスト({})から逃げ出した。\n", "ran away from quest '{}'.\n");
        print_entry(fff, hour, min, note_level, fmt, quest.get_bounty().name);
        break;
    }
    case DiaryKind::TO_QUEST: {
        if (any_bits(quest.flags, QUEST_FLAG_SILENT)) {
            break;
        }

        constexpr auto fmt = _("クエスト「{}」へと突入した。\n", "entered the quest '{}'.\n");
        print_entry(fff, hour, min, note_level, fmt, quest.name);
        break;
    }
    default:
        break;
    }

    angband_fclose(fff);
    if (do_level) {
        write_level = false;
    }

    return 0;
}

/*!
 * @brief 日記にメッセージを追加する
 * @param dk 日記内容のID
 * @param num 日記内容のIDに応じた数値
 * @param note 日記内容のIDに応じた文字列
 */
void exe_write_diary(const FloorType &floor, DiaryKind dk, int num, std::string_view note)
{
    static auto disable_diary = false;
    const auto &[day, hour, min] = AngbandWorld::get_instance().extract_date_time(InnerGameData::get_instance().get_start_race());
    if (disable_diary) {
        return;
    }

    FILE *fff = nullptr;
    if (!open_diary_file(&fff, &disable_diary)) {
        return;
    }

    const auto &[q_idx, note_level] = write_floor(floor);
    auto do_level = true;
    switch (dk) {
    case DiaryKind::DIALY:
        if (day < MAX_DAYS) {
            fmt::print(fff, _("{}日目\n", "Day {}\n"), day);
        } else {
            fputs(_("*****日目\n", "Day *****\n"), fff);
        }

        do_level = false;
        break;
    case DiaryKind::DESCRIPTION:
        if (num) {
            fmt::print(fff, "{}\n", note);
            do_level = false;
        } else {
            print_entry(fff, hour, min, note_level, "{}\n", note);
        }

        break;
    case DiaryKind::ART: {
        constexpr auto fmt = _("{}を発見した。\n", "discovered {}.\n");
        print_entry(fff, hour, min, note_level, fmt, note);
        break;
    }
    case DiaryKind::ART_SCROLL: {
        constexpr auto fmt = _("巻物によって{}を生成した。\n", "created {} by scroll.\n");
        print_entry(fff, hour, min, note_level, fmt, note);
        break;
    }
    case DiaryKind::UNIQUE: {
        constexpr auto fmt = _("{}を倒した。\n", "defeated {}.\n");
        print_entry(fff, hour, min, note_level, fmt, note);
        break;
    }
    case DiaryKind::MAXDEAPTH: {
        constexpr auto fmt = _("{0}の最深階{1}階に到達した。\n", "reached level {1} of {0} for the first time.\n");
        const auto &dungeon = floor.get_dungeon_definition();
        print_entry(fff, hour, min, note_level, fmt, dungeon.name, num);
        break;
    }
    case DiaryKind::TRUMP: {
        constexpr auto fmt = _("{0}{1}の最深階を{2}階にセットした。\n", "reset recall level of {0} to {2} {1}.\n");
        const auto &dungeon = floor.get_dungeon_definition();
        const auto &dungeon_records = DungeonRecords::get_instance();
        const auto dungeon_id = i2enum<DungeonId>(num);
        const auto max_level = dungeon_records.get_record(dungeon_id).get_max_level();
        print_entry(fff, hour, min, note_level, fmt, note, dungeon.name, max_level);
        break;
    }
    case DiaryKind::STAIR: {
        auto to = inside_quest(q_idx) && (QuestType::is_fixed(q_idx) && !((q_idx == QuestId::OBERON) || (q_idx == QuestId::SERPENT)))
                      ? _("地上", "the surface")
                  : !(floor.dun_level + num)
                      ? _("地上", "the surface")
                      : fmt::format(_("{}階", "level {}"), floor.dun_level + num);
        constexpr auto fmt = _("{0}へ{1}。\n", "{1} {0}.\n");
        print_entry(fff, hour, min, note_level, fmt, to, note);
        break;
    }
    case DiaryKind::RECALL:
        if (!num) {
            constexpr auto fmt = _("帰還を使って{0}の{1}階へ下りた。\n", "recalled to dungeon level {1} of {0}.\n");
            const auto &dungeon = floor.get_dungeon_definition();
            const auto &dungeon_records = DungeonRecords::get_instance();
            const auto max_level = dungeon_records.get_record(floor.dungeon_id).get_max_level();
            print_entry(fff, hour, min, note_level, fmt, dungeon.name, max_level);
        } else {
            constexpr auto fmt = _("帰還を使って地上へと戻った。\n", "recalled from dungeon to surface.\n");
            print_entry(fff, hour, min, note_level, fmt);
        }

        break;
    case DiaryKind::TELEPORT_LEVEL: {
        constexpr auto fmt = _("レベル・テレポートで脱出した。\n", "got out using teleport level.\n");
        print_entry(fff, hour, min, note_level, fmt);
        break;
    }
    case DiaryKind::BUY: {
        constexpr auto fmt = _("{}を購入した。\n", "bought {}.\n");
        print_entry(fff, hour, min, note_level, fmt, note);
        break;
    }
    case DiaryKind::SELL: {
        constexpr auto fmt = _("{}を売却した。\n", "sold {}.\n");
        print_entry(fff, hour, min, note_level, fmt, note);
        break;
    }
    case DiaryKind::ARENA: {
        const auto &entries = ArenaEntryList::get_instance();
        const auto defeated_entry = entries.get_defeated_entry();
        if (defeated_entry) {
            constexpr auto fmt = _("闘技場の{0}で、{1}の前に敗れ去った。\n", "beaten by {1} in {0}.\n");
            const auto num_defeated = entries.get_fight_number(false);
            print_entry(fff, hour, min, note_level, fmt, num_defeated, note);
            break;
        }

        constexpr auto fmt = _("闘技場の{}で({})に勝利した。\n", "won {} ({}).\n");
        const auto fight_number = entries.get_fight_number(true);
        print_entry(fff, hour, min, note_level, fmt, fight_number, note);
        if (entries.is_player_true_victor()) {
            constexpr auto mes_true_champion = _("                 最強の挑戦者からタイトルを防衛し、真のチャンピオンとなった。\n",
                "                 won the strongest challenger and became the True Champion.\n");
            fputs(mes_true_champion, fff);
            do_level = false;
            break;
        }

        if (entries.is_player_victor()) {
            constexpr auto mes_champion = _("                 闘技場のすべての敵に勝利し、チャンピオンとなった。\n",
                "                 won all fights to become a Champion.\n");
            fputs(mes_champion, fff);
            do_level = false;
        }

        break;
    }
    case DiaryKind::FOUND: {
        constexpr auto fmt = _("{}を識別した。\n", "identified {}.\n");
        print_entry(fff, hour, min, note_level, fmt, note);
        break;
    }
    case DiaryKind::PAT_TELE: {
        const auto to = !floor.is_underground()
                            ? _("地上", "the surface")
                            : fmt::format(_("{}階({})", "level {} of {}"), floor.dun_level, floor.get_dungeon_definition().name);
        constexpr auto fmt = _("{}へとパターンの力で移動した。\n", "used Pattern to teleport to {}.\n");
        print_entry(fff, hour, min, note_level, fmt, to);
        break;
    }
    case DiaryKind::LEVELUP: {
        constexpr auto fmt = _("レベルが{}に上がった。\n", "reached player level {}.\n");
        print_entry(fff, hour, min, note_level, fmt, num);
        break;
    }
    case DiaryKind::GAMESTART: {
        time_t ct = time((time_t *)0);
        do_level = false;
        if (num) {
            fmt::print(fff, "{} {}", note, ctime(&ct));
        } else {
            print_entry(fff, hour, min, note_level, "{} {}", note, ctime(&ct));
        }

        break;
    }
    case DiaryKind::NAMED_PET:
        print_entry_prefix(fff, hour, min, note_level);
        write_diary_pet(fff, num, note);
        break;
    case DiaryKind::WIZARD_LOG:
        fmt::print(fff, "{}\n", note);
        break;
    default:
        break;
    }

    angband_fclose(fff);
    if (do_level) {
        write_level = false;
    }

    return;
}
