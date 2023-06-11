/*!
 * @brief 日記へのメッセージ追加処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "io/write-diary.h"
#include "dungeon/quest.h"
#include "info-reader/fixed-map-parser.h"
#include "io/files-util.h"
#include "market/arena-info-table.h"
#include "monster-race/monster-race.h"
#include "player/player-status.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "term/z-form.h"
#include "util/angband-files.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <sstream>

bool write_level; //!< @todo *抹殺* したい…

#ifdef JP
#else
/*!
 * @brief Return suffix of ordinal number
 * @param num number
 * @return pointer of suffix string.
 */
std::string get_ordinal_number_suffix(int num)
{
    num = std::abs(num) % 100;
    switch (num % 10) {
    case 1:
        return (num == 11) ? "th" : "st";
    case 2:
        return (num == 12) ? "th" : "nd";
    case 3:
        return (num == 13) ? "th" : "rd";
    default:
        return "th";
    }
}
#endif

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
    ss << _("playrecord-", "playrec-") << savefile_base << ".txt";
    const auto &path = path_build(ANGBAND_DIR_USER, ss.str());
    *fff = angband_fopen(path, FileOpenMode::APPEND);
    if (*fff) {
        return true;
    }

    constexpr auto mes = _("%s を開くことができませんでした。プレイ記録を一時停止します。", "Failed to open %s. Play-Record is disabled temporarily.");
    const auto &filename = path.string();
    msg_format(mes, filename.data());
    msg_print(nullptr);
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
    auto q_idx = quest_number(floor, floor.dun_level);
    if (!write_level) {
        return make_pair(q_idx, std::string());
    }

    if (floor.inside_arena) {
        return make_pair(q_idx, std::string(_("アリーナ:", "Arena:")));
    } else if (!floor.dun_level) {
        return make_pair(q_idx, std::string(_("地上:", "Surface:")));
    } else if (inside_quest(q_idx) && QuestType::is_fixed(q_idx) && !((q_idx == QuestId::OBERON) || (q_idx == QuestId::SERPENT))) {
        return make_pair(q_idx, std::string(_("クエスト:", "Quest:")));
    } else {
        char desc[40];
        const auto &dungeon = floor.get_dungeon_definition();
#ifdef JP
        strnfmt(desc, sizeof(desc), "%d階(%s):", (int)floor.dun_level, dungeon.name.data());
#else
        strnfmt(desc, sizeof(desc), "%s L%d:", dungeon.name.data(), (int)floor.dun_level);
#endif
        return make_pair(q_idx, std::string(desc));
    }
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
        fprintf(fff, _("%sを旅の友にすることに決めた。\n", "decided to travel together with %s.\n"), note.data());
        break;
    case RECORD_NAMED_PET_UNNAME:
        fprintf(fff, _("%sの名前を消した。\n", "unnamed %s.\n"), note.data());
        break;
    case RECORD_NAMED_PET_DISMISS:
        fprintf(fff, _("%sを解放した。\n", "dismissed %s.\n"), note.data());
        break;
    case RECORD_NAMED_PET_DEATH:
        fprintf(fff, _("%sが死んでしまった。\n", "%s died.\n"), note.data());
        break;
    case RECORD_NAMED_PET_MOVED:
        fprintf(fff, _("%sをおいて別のマップへ移動した。\n", "moved to another map leaving %s behind.\n"), note.data());
        break;
    case RECORD_NAMED_PET_LOST_SIGHT:
        fprintf(fff, _("%sとはぐれてしまった。\n", "lost sight of %s.\n"), note.data());
        break;
    case RECORD_NAMED_PET_DESTROY:
        fprintf(fff, _("%sが*破壊*によって消え去った。\n", "%s was killed by *destruction*.\n"), note.data());
        break;
    case RECORD_NAMED_PET_EARTHQUAKE:
        fprintf(fff, _("%sが岩石に押し潰された。\n", "%s was crushed by falling rocks.\n"), note.data());
        break;
    case RECORD_NAMED_PET_GENOCIDE:
        fprintf(fff, _("%sが抹殺によって消え去った。\n", "%s was a victim of genocide.\n"), note.data());
        break;
    case RECORD_NAMED_PET_WIZ_ZAP:
        fprintf(fff, _("%sがデバッグコマンドによって消え去った。\n", "%s was removed by debug command.\n"), note.data());
        break;
    case RECORD_NAMED_PET_TELE_LEVEL:
        fprintf(fff, _("%sがテレポート・レベルによって消え去った。\n", "%s was lost after teleporting a level.\n"), note.data());
        break;
    case RECORD_NAMED_PET_BLAST:
        fprintf(fff, _("%sを爆破した。\n", "blasted %s.\n"), note.data());
        break;
    case RECORD_NAMED_PET_HEAL_LEPER:
        fprintf(fff, _("%sの病気が治り旅から外れた。\n", "%s was healed and left.\n"), note.data());
        break;
    case RECORD_NAMED_PET_COMPACT:
        fprintf(fff, _("%sがモンスター情報圧縮によって消え去った。\n", "%s was lost when the monster list was pruned.\n"), note.data());
        break;
    case RECORD_NAMED_PET_LOSE_PARENT:
        fprintf(fff, _("%sの召喚者が既にいないため消え去った。\n", "%s disappeared because its summoner left.\n"), note.data());
        break;
    default:
        fprintf(fff, "\n");
        break;
    }
}

/*!
 * @brief 日記にクエストに関するメッセージを追加する
 * @param dk 日記内容のID
 * @param num 日記内容のIDに応じた番号
 * @return エラーコード
 */
int exe_write_diary_quest(PlayerType *player_ptr, DiaryKind dk, QuestId num)
{
    static bool disable_diary = false;

    int day, hour, min;
    extract_day_hour_min(player_ptr, &day, &hour, &min);

    if (disable_diary) {
        return -1;
    }

    auto &floor = *player_ptr->current_floor_ptr;
    auto old_quest = floor.quest_number;
    const auto &quest_list = QuestList::get_instance();
    const auto &quest = quest_list[num];
    floor.quest_number = (quest.type == QuestKindType::RANDOM) ? QuestId::NONE : num;
    init_flags = INIT_NAME_ONLY;
    parse_fixed_map(player_ptr, QUEST_DEFINITION_LIST, 0, 0, 0, 0);
    floor.quest_number = old_quest;

    const auto [q_idx, note_level] = write_floor(floor);

    FILE *fff = nullptr;
    if (!open_diary_file(&fff, &disable_diary)) {
        return -1;
    }

    bool do_level = true;

    switch (dk) {
    case DiaryKind::FIX_QUEST_C: {
        if (any_bits(quest.flags, QUEST_FLAG_SILENT)) {
            break;
        }

        constexpr auto mes = _(" %2d:%02d %20s クエスト「%s」を達成した。\n", " %2d:%02d %20s completed quest '%s'.\n");
        fprintf(fff, mes, hour, min, note_level.data(), quest.name.data());
        break;
    }
    case DiaryKind::FIX_QUEST_F: {
        if (any_bits(quest.flags, QUEST_FLAG_SILENT)) {
            break;
        }

        constexpr auto mes = _(" %2d:%02d %20s クエスト「%s」から命からがら逃げ帰った。\n", " %2d:%02d %20s ran away from quest '%s'.\n");
        fprintf(fff, mes, hour, min, note_level.data(), quest.name.data());
        break;
    }
    case DiaryKind::RAND_QUEST_C: {
        constexpr auto mes = _(" %2d:%02d %20s ランダムクエスト(%s)を達成した。\n", " %2d:%02d %20s completed random quest '%s'\n");
        fprintf(fff, mes, hour, min, note_level.data(), monraces_info[quest.r_idx].name.data());
        break;
    }
    case DiaryKind::RAND_QUEST_F: {
        constexpr auto mes = _(" %2d:%02d %20s ランダムクエスト(%s)から逃げ出した。\n", " %2d:%02d %20s ran away from quest '%s'.\n");
        fprintf(fff, mes, hour, min, note_level.data(), monraces_info[quest.r_idx].name.data());
        break;
    }
    case DiaryKind::TO_QUEST: {
        if (any_bits(quest.flags, QUEST_FLAG_SILENT)) {
            break;
        }

        constexpr auto mes = _(" %2d:%02d %20s クエスト「%s」へと突入した。\n", " %2d:%02d %20s entered the quest '%s'.\n");
        fprintf(fff, mes, hour, min, note_level.data(), quest.name.data());
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
void exe_write_diary(PlayerType *player_ptr, DiaryKind dk, int num, std::string_view note)
{
    static bool disable_diary = false;

    int day, hour, min;
    extract_day_hour_min(player_ptr, &day, &hour, &min);

    if (disable_diary) {
        return;
    }

    FILE *fff = nullptr;
    if (!open_diary_file(&fff, &disable_diary)) {
        return;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto [q_idx, note_level] = write_floor(floor);
    auto do_level = true;
    switch (dk) {
    case DiaryKind::DIALY:
        if (day < MAX_DAYS) {
            fprintf(fff, _("%d日目\n", "Day %d\n"), day);
        } else {
            fputs(_("*****日目\n", "Day *****\n"), fff);
        }

        do_level = false;
        break;
    case DiaryKind::DESCRIPTION:
        if (num) {
            fprintf(fff, "%s\n", note.data());
            do_level = false;
        } else {
            fprintf(fff, " %2d:%02d %20s %s\n", hour, min, note_level.data(), note.data());
        }

        break;
    case DiaryKind::ART: {
        constexpr auto mes = _(" %2d:%02d %20s %sを発見した。\n", " %2d:%02d %20s discovered %s.\n");
        fprintf(fff, mes, hour, min, note_level.data(), note.data());
        break;
    }
    case DiaryKind::ART_SCROLL: {
        constexpr auto mes = _(" %2d:%02d %20s 巻物によって%sを生成した。\n", " %2d:%02d %20s created %s by scroll.\n");
        fprintf(fff, mes, hour, min, note_level.data(), note.data());
        break;
    }
    case DiaryKind::UNIQUE: {
        constexpr auto mes = _(" %2d:%02d %20s %sを倒した。\n", " %2d:%02d %20s defeated %s.\n");
        fprintf(fff, mes, hour, min, note_level.data(), note.data());
        break;
    }
    case DiaryKind::MAXDEAPTH: {
        constexpr auto mes = _(" %2d:%02d %20s %sの最深階%d階に到達した。\n", " %2d:%02d %20s reached level %d of %s for the first time.\n");
        const auto &dungeon = floor.get_dungeon_definition();
        fprintf(fff, mes, hour, min, note_level.data(), _(dungeon.name.data(), num), _(num, dungeon.name.data()));
        break;
    }
    case DiaryKind::TRUMP: {
        constexpr auto mes = _(" %2d:%02d %20s %s%sの最深階を%d階にセットした。\n", " %2d:%02d %20s reset recall level of %s to %d %s.\n");
        const auto &dungeon = floor.get_dungeon_definition();
        fprintf(fff, mes, hour, min, note_level.data(), note.data(), _(dungeon.name.data(), (int)max_dlv[num]), _((int)max_dlv[num], dungeon.name.data()));
        break;
    }
    case DiaryKind::STAIR: {
        auto to = inside_quest(q_idx) && (QuestType::is_fixed(q_idx) && !((q_idx == QuestId::OBERON) || (q_idx == QuestId::SERPENT)))
                      ? _("地上", "the surface")
                  : !(player_ptr->current_floor_ptr->dun_level + num)
                      ? _("地上", "the surface")
                      : format(_("%d階", "level %d"), player_ptr->current_floor_ptr->dun_level + num);
        constexpr auto mes = _(" %2d:%02d %20s %sへ%s。\n", " %2d:%02d %20s %s %s.\n");
        fprintf(fff, mes, hour, min, note_level.data(), _(to.data(), note.data()), _(note.data(), to.data()));
        break;
    }
    case DiaryKind::RECALL:
        if (!num) {
            constexpr auto mes = _(" %2d:%02d %20s 帰還を使って%sの%d階へ下りた。\n", " %2d:%02d %20s recalled to dungeon level %d of %s.\n");
            const auto &dungeon = floor.get_dungeon_definition();
            fprintf(fff, mes, hour, min, note_level.data(), _(dungeon.name.data(), (int)max_dlv[floor.dungeon_idx]), _((int)max_dlv[floor.dungeon_idx], dungeon.name.data()));
        } else {
            constexpr auto mes = _(" %2d:%02d %20s 帰還を使って地上へと戻った。\n", " %2d:%02d %20s recalled from dungeon to surface.\n");
            fprintf(fff, mes, hour, min, note_level.data());
        }

        break;
    case DiaryKind::TELEPORT_LEVEL: {
        constexpr auto mes = _(" %2d:%02d %20s レベル・テレポートで脱出した。\n", " %2d:%02d %20s got out using teleport level.\n");
        fprintf(fff, mes, hour, min, note_level.data());
        break;
    }
    case DiaryKind::BUY: {
        constexpr auto mes = _(" %2d:%02d %20s %sを購入した。\n", " %2d:%02d %20s bought %s.\n");
        fprintf(fff, mes, hour, min, note_level.data(), note.data());
        break;
    }
    case DiaryKind::SELL: {
        constexpr auto mes = _(" %2d:%02d %20s %sを売却した。\n", " %2d:%02d %20s sold %s.\n");
        fprintf(fff, mes, hour, min, note_level.data(), note.data());
        break;
    }
    case DiaryKind::ARENA: {
        if (num < 0) {
            int n = -num;
            constexpr auto mes = _(" %2d:%02d %20s 闘技場の%d%s回戦で、%sの前に敗れ去った。\n", " %2d:%02d %20s beaten by %s in the %d%s fight.\n");
            fprintf(fff, mes, hour, min, note_level.data(), _(n, note.data()), _("", n), _(note.data(), get_ordinal_number_suffix(n).data()));
            break;
        }

        constexpr auto mes = _(" %2d:%02d %20s 闘技場の%d%s回戦(%s)に勝利した。\n", " %2d:%02d %20s won the %d%s fight (%s).\n");
        fprintf(fff, mes, hour, min, note_level.data(), num, _("", get_ordinal_number_suffix(num).data()), note.data());

        if (num == MAX_ARENA_MONS) {
            constexpr auto mes_champion = _("                 闘技場のすべての敵に勝利し、チャンピオンとなった。\n",
                "                 won all fights to become a Champion.\n");
            fprintf(fff, mes_champion);
            do_level = false;
        }

        break;
    }
    case DiaryKind::FOUND: {
        constexpr auto mes = _(" %2d:%02d %20s %sを識別した。\n", " %2d:%02d %20s identified %s.\n");
        fprintf(fff, mes, hour, min, note_level.data(), note.data());
        break;
    }
    case DiaryKind::PAT_TELE: {
        const auto &floor_ref = *player_ptr->current_floor_ptr;
        auto to = !floor_ref.is_in_dungeon()
                      ? _("地上", "the surface")
                      : format(_("%d階(%s)", "level %d of %s"), floor.dun_level, floor.get_dungeon_definition().name.data());
        constexpr auto mes = _(" %2d:%02d %20s %sへとパターンの力で移動した。\n", " %2d:%02d %20s used Pattern to teleport to %s.\n");
        fprintf(fff, mes, hour, min, note_level.data(), to.data());
        break;
    }
    case DiaryKind::LEVELUP: {
        constexpr auto mes = _(" %2d:%02d %20s レベルが%dに上がった。\n", " %2d:%02d %20s reached player level %d.\n");
        fprintf(fff, mes, hour, min, note_level.data(), num);
        break;
    }
    case DiaryKind::GAMESTART: {
        time_t ct = time((time_t *)0);
        do_level = false;
        if (num) {
            fprintf(fff, "%s %s", note.data(), ctime(&ct));
        } else {
            fprintf(fff, " %2d:%02d %20s %s %s", hour, min, note_level.data(), note.data(), ctime(&ct));
        }

        break;
    }
    case DiaryKind::NAMED_PET:
        fprintf(fff, " %2d:%02d %20s ", hour, min, note_level.data());
        write_diary_pet(fff, num, note.data());
        break;
    case DiaryKind::WIZARD_LOG:
        fprintf(fff, "%s\n", note.data());
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
