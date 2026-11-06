/*!
 * @brief モンスターの思い出を表示する処理
 * @date 2020/06/09
 * @author Hourier
 */

#include "view/display-lore.h"
#include "bot/bot-json-output.h"
#include "game-option/cheat-options.h"
#include "game-option/text-display-options.h"
#include "locale/english.h"
#include "locale/japanese.h"
#include "lore/lore-calculator.h"
#include "lore/lore-util.h"
#include "lore/monster-lore.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-sex.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monrace/monrace-record.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "tracking/lore-tracker.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "view/display-symbol.h"
#include "world/world.h"
#include <cstdint>
#include <fmt/format.h>

/*!
 * @brief モンスター情報のヘッダを記述する
 * @param monrace_id モンスターの種族ID
 */
void roff_top(MonraceId monrace_id)
{
    term_erase(0, 0);
    term_gotoxy(0, 0);

    const auto &monrace = MonraceList::get_instance().get_monrace(monrace_id);
#ifdef JP
#else
    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE)) {
        term_addstr(-1, TERM_WHITE, "The ");
    }
#endif

    if (AngbandWorld::get_instance().wizard || cheat_know) {
        term_addstr(-1, TERM_WHITE, "[");
        term_addstr(-1, TERM_L_BLUE, fmt::format("{}", enum2i(monrace_id)));
        term_addstr(-1, TERM_WHITE, "] ");
    }

    term_addstr(-1, TERM_WHITE, monrace.name);

    term_addstr(-1, TERM_WHITE, " ('");
    term_add_bigch(monrace.symbol_definition);
    term_addstr(-1, TERM_WHITE, "')");

    term_addstr(-1, TERM_WHITE, "/('");
    term_add_bigch(monrace.symbol_config);
    term_addstr(-1, TERM_WHITE, "'):");
}

/*!
 * @brief  モンスター情報の表示と共に画面を一時消去するサブルーチン /
 * Hack -- describe the given monster race at the top of the screen
 * @param r_idx モンスターの種族ID
 * @param mode 表示オプション
 */
void screen_roff(PlayerType *player_ptr, MonraceId r_idx, monster_lore_mode mode)
{
    output_bot_json_lore_snapshot(player_ptr, r_idx, mode);
    msg_erase();
    term_erase(0, 1);
    hook_c_roff = c_roff;
    process_monster_lore(player_ptr, r_idx, mode);
    roff_top(r_idx);
}

/*!
 * @brief モンスター情報の現在のウィンドウに表示する /
 * Hack -- describe the given monster race in the current "term" window
 * @param r_idx モンスターの種族ID
 */
void display_roff(PlayerType *player_ptr)
{
    for (int y = 0; y < game_term->hgt; y++) {
        term_erase(0, y);
    }

    term_gotoxy(0, 1);
    hook_c_roff = c_roff;
    const auto &tracker = LoreTracker::get_instance();
    if (!tracker.is_tracking()) {
        return;
    }

    const auto monrace_id = tracker.get_trackee();
    process_monster_lore(player_ptr, monrace_id, MONSTER_LORE_NORMAL);
    roff_top(monrace_id);
}

/*!
 * @brief モンスター詳細情報を自動スポイラー向けに出力する /
 * Hack -- output description of the given monster race
 * @param r_idx モンスターの種族ID
 * @param roff_func 出力処理を行う関数ポインタ
 * @todo ここのroff_funcの引数にFILE* を追加しないとspoiler_file をローカル関数化することができないと判明した、保留.
 */
void output_monster_spoiler(MonraceId r_idx, hook_c_roff_pf roff_func)
{
    hook_c_roff = roff_func;
    PlayerType dummy;

    dummy.lev = 1;
    dummy.max_plv = 1;
    process_monster_lore(&dummy, r_idx, MONSTER_LORE_DEBUG);
}

static void display_killed(lore_type *lore_ptr)
{
#ifdef JP
    hooked_roff(fmt::format("このモンスターはあなたの先祖を {} 人葬っている", lore_ptr->monrace->r_deaths));
#else
    const auto present_perfect_tense = lore_ptr->monrace->r_deaths == 1 ? "has" : "have";
    hooked_roff(fmt::format("{} of your ancestors {} been killed by this creature, ", lore_ptr->monrace->r_deaths, present_perfect_tense));
#endif
    if (lore_ptr->monrace->r_pkills) {
        hooked_roff(fmt::format(_("が、あなたはこのモンスターを少なくとも {} 体は倒している。", "and you have exterminated at least {} of the creatures.  "), lore_ptr->monrace->r_pkills));
    } else if (lore_ptr->monrace->r_tkills) {
        hooked_roff(fmt::format(_("が、あなたの先祖はこのモンスターを少なくとも {} 体は倒している。", "and your ancestors have exterminated at least {} of the creatures.  "), lore_ptr->monrace->r_tkills));
    } else {
        hooked_roff(fmt::format(_("が、まだ{}を倒したことはない。", "and {} is not ever known to have been defeated.  "), Who::who(lore_ptr->msex)));
    }
}

static void display_no_killed(lore_type *lore_ptr)
{
    if (lore_ptr->monrace->r_pkills) {
        hooked_roff(fmt::format(_("あなたはこのモンスターを少なくとも {} 体は殺している。", "You have killed at least {} of these creatures.  "), lore_ptr->monrace->r_pkills));
    } else if (lore_ptr->monrace->r_tkills) {
        hooked_roff(fmt::format(_("あなたの先祖はこのモンスターを少なくとも {} 体は殺している。", "Your ancestors have killed at least {} of these creatures.  "), lore_ptr->monrace->r_tkills));
    } else {
        hooked_roff(_("このモンスターを倒したことはない。", "No battles to the death are recalled.  "));
    }
}

/*!
 * @brief 生存数制限のあるモンスターの最大生存数を表示する
 * @param lore_ptr モンスターの思い出構造体への参照ポインタ
 * @details
 * 一度も倒したことのないモンスターの情報は不明。
 */
static void display_number_of_nazguls(lore_type *lore_ptr)
{
    if (lore_ptr->mode != MONSTER_LORE_DEBUG && lore_ptr->monrace->r_tkills == 0) {
        return;
    }
    if (!lore_ptr->monrace->population_flags.has(MonsterPopulationType::NAZGUL)) {
        return;
    }

    const auto remain = lore_ptr->monrace->max_num;
    const auto killed = lore_ptr->monrace->r_akills;
    if (remain == 0) {
        const auto whom = Who::whom(lore_ptr->msex, (killed > 1));
        hooked_roff(fmt::format(_("{1}はかつて {0} 体存在した。", "You already killed all {0} of {1}.  "), killed, whom));
    } else {
        const auto whom = Who::whom(lore_ptr->msex, (remain + killed > 1));
#ifdef JP
        hooked_roff(fmt::format("{}はまだ {} 体生きている。", whom, remain));
#else
        std::string be((remain > 1) ? "are" : "is");
        hooked_roff(fmt::format("{} of {} {} still alive.  ", remain, whom, be));
#endif
    }
}

void display_kill_numbers(lore_type *lore_ptr)
{
    if ((lore_ptr->mode & 0x02) != 0) {
        return;
    }

    const auto kill_unique_description = lore_ptr->build_kill_unique_description();
    if (kill_unique_description) {
        for (const auto &[text, color] : *kill_unique_description) {
            hook_c_roff(color, text);
        }
        return;
    }

    if (lore_ptr->monrace->r_deaths == 0) {
        display_no_killed(lore_ptr);
    } else {
        display_killed(lore_ptr);
    }

    display_number_of_nazguls(lore_ptr);

    hooked_roff("\n");
}

void display_where_to_appear_summary(lore_type *lore_ptr)
{
    if (lore_ptr->monrace->level == 0) {
        hooked_roff(_("出現:町 ", "live:town "));
        lore_ptr->old = true;
    } else if (lore_ptr->monrace->r_tkills || lore_ptr->know_everything) {
        if (depth_in_feet) {
            hooked_roff(fmt::format(_("出現:{} フィート ", "depth:{} ft "), lore_ptr->monrace->level * 50));
        } else {
            hooked_roff(fmt::format(_("出現:{}階 ", "depth:{} F "), lore_ptr->monrace->level));
        }
    }
}

/*!
 * @brief どこに出没するかを表示する
 * @param lore_ptr モンスターの思い出構造体への参照ポインタ
 * @return たぬきならFALSE、それ以外はTRUE
 */
bool display_where_to_appear(lore_type *lore_ptr)
{
    lore_ptr->old = false;
    if (lore_ptr->monrace->level == 0) {
        hooked_roff(fmt::format(_("{}は町に住み", "{} lives in the town"), str_upcase_first(Who::who(lore_ptr->msex))));
        lore_ptr->old = true;
    } else if (lore_ptr->monrace->r_tkills || lore_ptr->know_everything) {
        if (depth_in_feet) {
            hooked_roff(fmt::format(_("{}は通常地下 {} フィートで出現し", "{} is normally found at depths of {} feet"), str_upcase_first(Who::who(lore_ptr->msex)), lore_ptr->monrace->level * 50));
        } else {
            hooked_roff(fmt::format(_("{}は通常地下 {} 階で出現し", "{} is normally found on dungeon level {}"), str_upcase_first(Who::who(lore_ptr->msex)), lore_ptr->monrace->level));
        }

        lore_ptr->old = true;
    }

    if (lore_ptr->monrace_id == MonraceId::CHAMELEON) {
        hooked_roff(_("、他のモンスターに化ける。", "and can take the shape of other monster."));
        return false;
    }

    if (lore_ptr->old) {
        hooked_roff(_("、", ", and "));
    } else {
        hooked_roff(fmt::format(_("{}は", "{} "), str_upcase_first(Who::who(lore_ptr->msex))));
        lore_ptr->old = true;
    }

    return true;
}

void display_monster_speed_summary(lore_type *lore_ptr)
{
    const int speed = lore_ptr->speed - STANDARD_SPEED;
    const auto speed_color = lore_ptr->get_speed_color();

    hook_c_roff(speed_color, fmt::format(_("速度:{:+} ", "speed:{:+} "), speed));
}

void display_monster_move(lore_type *lore_ptr)
{
    for (const auto &[text, color] : lore_ptr->build_speed_description()) {
        hook_c_roff(color, text);
    }
}

void display_monster_never_move(lore_type *lore_ptr)
{
    if (lore_ptr->behavior_flags.has_not(MonsterBehaviorType::NEVER_MOVE)) {
        return;
    }

    if (lore_ptr->old) {
        hooked_roff(_("、しかし", ", but "));
    } else {
        hooked_roff(fmt::format(_("{}は", "{} "), str_upcase_first(Who::who(lore_ptr->msex))));
        lore_ptr->old = true;
    }

    hooked_roff(_("侵入者を追跡しない", "does not deign to chase intruders"));
}

void display_monster_exp_summary(lore_type *lore_ptr)
{
    if ((lore_ptr->monrace->r_tkills == 0) && !lore_ptr->know_everything) {
        hooked_roff(_("経験:??? ", "Exp:??? "));
        return;
    }
    hooked_roff(fmt::format(_("経験:{} ", "Exp:{} "), lore_ptr->monrace->mexp));
}

void display_monster_kills_summary(lore_type *lore_ptr)
{
    if (lore_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        if (lore_ptr->monrace->r_pkills == 0) {
            hook_c_roff(TERM_L_GREEN, _("生存 ", "alive "));
            return;
        }

        hook_c_roff(TERM_RED, _("死亡 ", "dead "));
        return;
    }

    hooked_roff(fmt::format(_("殺:{} ", "kill:{} "), lore_ptr->monrace->r_pkills));

    if (!lore_ptr->monrace->population_flags.has(MonsterPopulationType::NAZGUL)) {
        return;
    }
    hooked_roff(fmt::format(_("残:{} ", "remain:{} "), lore_ptr->monrace->max_num));
}

void display_monster_kind_tags(lore_type *lore_ptr)
{
    if (lore_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        hooked_roff(_("[ユニーク]", "[UNIQ]"));
    }

    switch (lore_ptr->msex) {
    case MonsterSex::MALE:
        hooked_roff(_("[男性]", "[male]"));
        break;
    case MonsterSex::FEMALE:
        hooked_roff(_("[女性]", "[female]"));
        break;
    case MonsterSex::NONE:
    case MonsterSex::MAX:
        break;
    }

    if (lore_ptr->misc_flags.has(MonsterMiscType::ELDRITCH_HORROR)) {
        hook_c_roff(TERM_VIOLET, _("[狂気]", "[sanity-blasting]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::ANIMAL)) {
        hook_c_roff(TERM_L_GREEN, _("[自然界]", "[natural]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::EVIL)) {
        hook_c_roff(TERM_L_DARK, _("[邪悪]", "[evil]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::GOOD)) {
        hook_c_roff(TERM_YELLOW, _("[善良]", "[good]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::UNDEAD)) {
        hook_c_roff(TERM_VIOLET, _("[アンデッド]", "[undead]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::AMBERITE)) {
        hook_c_roff(TERM_VIOLET, _("[アンバー]", "[Amberite]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::DRAGON)) {
        hook_c_roff(TERM_ORANGE, _("[ドラゴン]", "[dragon]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::DEMON)) {
        hook_c_roff(TERM_VIOLET, _("[デーモン]", "[demon]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::GIANT)) {
        hook_c_roff(TERM_L_UMBER, _("[巨人]", "[giant]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::TROLL)) {
        hook_c_roff(TERM_BLUE, _("[トロル]", "[troll]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::ORC)) {
        hook_c_roff(TERM_UMBER, _("[オーク]", "[orc]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::HUMAN)) {
        hook_c_roff(TERM_L_WHITE, _("[人間]", "[human]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::QUANTUM)) {
        hook_c_roff(TERM_VIOLET, _("[量子生物]", "[quantum]"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::ANGEL)) {
        hook_c_roff(TERM_YELLOW, _("[天使]", "[angel]"));
    }

    hooked_roff("\n");
}

void display_monster_kind(lore_type *lore_ptr)
{
    if (lore_ptr->kind_flags.has_none_of({ MonsterKindType::DRAGON, MonsterKindType::DEMON, MonsterKindType::GIANT, MonsterKindType::TROLL, MonsterKindType::ORC, MonsterKindType::ANGEL, MonsterKindType::QUANTUM, MonsterKindType::HUMAN })) {
        hooked_roff(_("モンスター", " creature"));
        return;
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::DRAGON)) {
        hook_c_roff(TERM_ORANGE, _("ドラゴン", " dragon"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::DEMON)) {
        hook_c_roff(TERM_VIOLET, _("デーモン", " demon"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::GIANT)) {
        hook_c_roff(TERM_L_UMBER, _("巨人", " giant"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::TROLL)) {
        hook_c_roff(TERM_BLUE, _("トロル", " troll"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::ORC)) {
        hook_c_roff(TERM_UMBER, _("オーク", " orc"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::HUMAN)) {
        hook_c_roff(TERM_L_WHITE, _("人間", " human"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::QUANTUM)) {
        hook_c_roff(TERM_VIOLET, _("量子生物", " quantum creature"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::ANGEL)) {
        hook_c_roff(TERM_YELLOW, _("天使", " angel"));
    }
}

void display_monster_alignment(lore_type *lore_ptr)
{
    if (lore_ptr->misc_flags.has(MonsterMiscType::ELDRITCH_HORROR)) {
        hook_c_roff(TERM_VIOLET, _("狂気を誘う", " sanity-blasting"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::ANIMAL)) {
        hook_c_roff(TERM_L_GREEN, _("自然界の", " natural"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::EVIL)) {
        hook_c_roff(TERM_L_DARK, _("邪悪なる", " evil"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::GOOD)) {
        hook_c_roff(TERM_YELLOW, _("善良な", " good"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::UNDEAD)) {
        hook_c_roff(TERM_VIOLET, _("アンデッドの", " undead"));
    }

    if (lore_ptr->kind_flags.has(MonsterKindType::AMBERITE)) {
        hook_c_roff(TERM_VIOLET, _("アンバーの王族の", " Amberite"));
    }
}

/*!
 * @brief モンスターの経験値の思い出を表示する
 * @param player_ptr プレイヤーの情報へのポインター
 * @param lore_ptr モンスターの思い出の情報へのポインター
 */
void display_monster_exp(PlayerType *player_ptr, lore_type *lore_ptr)
{
#ifdef JP
    hooked_roff("を倒すことは");
#endif

    // 定義域の上限(経験値9999999、レベル127)では3/2倍が符号付き32bit整数に収まらないため、64bitで計算する
    const auto base_exp = static_cast<int64_t>(lore_ptr->monrace->mexp) * lore_ptr->monrace->level * 3 / 2;
    const auto player_factor = player_ptr->max_plv + 2;

    // 割った結果は上限でも約6億で、intに収まる
    const auto exp_integer = static_cast<int>(base_exp / player_factor);
    const auto exp_decimal = static_cast<int>(((base_exp % player_factor * 1000 / player_factor) + 5) / 10);

#ifdef JP
    hooked_roff(fmt::format(" {} レベルのキャラクタにとって 約{}.{:02} ポイントの経験となる。", player_ptr->lev, exp_integer, exp_decimal));
#else
    hooked_roff(fmt::format(" is worth about {}.{:02} point{}", exp_integer, exp_decimal, ((exp_integer == 1) && (exp_decimal == 0)) ? "" : "s"));

    concptr ordinal;
    switch (player_ptr->lev % 10) {
    case 1:
        ordinal = "st";
        break;
    case 2:
        ordinal = "nd";
        break;
    case 3:
        ordinal = "rd";
        break;
    default:
        ordinal = "th";
        break;
    }

    concptr vowel;
    switch (player_ptr->lev) {
    case 8:
    case 11:
    case 18:
        vowel = "n";
        break;
    default:
        vowel = "";
        break;
    }

    hooked_roff(fmt::format(" for a{} {}{} level character.  ", vowel, player_ptr->lev, ordinal));
#endif
}

void set_monster_aura_summary(lore_type *lore_ptr)
{
    auto has_fire_aura = lore_ptr->aura_flags.has(MonsterAuraType::FIRE);
    auto has_cold_aura = lore_ptr->aura_flags.has(MonsterAuraType::COLD);
    auto has_elec_aura = lore_ptr->aura_flags.has(MonsterAuraType::ELEC);

    if (has_fire_aura || has_elec_aura || has_cold_aura) {
        lore_ptr->lore_msgs.emplace_back(_("オーラ:", "aura:"), TERM_WHITE);
    }
    if (has_fire_aura) {
        lore_ptr->lore_msgs.emplace_back(_("炎", "fire"), TERM_RED);
    }
    if (has_cold_aura) {
        lore_ptr->lore_msgs.emplace_back(_("氷", "cold"), TERM_BLUE);
    }
    if (has_elec_aura) {
        lore_ptr->lore_msgs.emplace_back(_("電", "elec"), TERM_L_BLUE);
    }
    if (has_fire_aura || has_elec_aura || has_cold_aura) {
        lore_ptr->lore_msgs.emplace_back(" | ", TERM_WHITE);
    }
}

void display_monster_aura(lore_type *lore_ptr)
{
    auto has_fire_aura = lore_ptr->aura_flags.has(MonsterAuraType::FIRE);
    auto has_elec_aura = lore_ptr->aura_flags.has(MonsterAuraType::ELEC);
    auto has_cold_aura = lore_ptr->aura_flags.has(MonsterAuraType::COLD);
    if (has_fire_aura && has_elec_aura && has_cold_aura) {
        hook_c_roff(
            TERM_VIOLET, fmt::format(_("{}は炎と氷とスパークに包まれている。", "{} is surrounded by flames, ice and electricity.  "), str_upcase_first(Who::who(lore_ptr->msex))));
    } else if (has_fire_aura && has_elec_aura) {
        hook_c_roff(TERM_L_RED, fmt::format(_("{}は炎とスパークに包まれている。", "{} is surrounded by flames and electricity.  "), str_upcase_first(Who::who(lore_ptr->msex))));
    } else if (has_fire_aura && has_cold_aura) {
        hook_c_roff(TERM_BLUE, fmt::format(_("{}は炎と氷に包まれている。", "{} is surrounded by flames and ice.  "), str_upcase_first(Who::who(lore_ptr->msex))));
    } else if (has_cold_aura && has_elec_aura) {
        hook_c_roff(TERM_L_GREEN, fmt::format(_("{}は氷とスパークに包まれている。", "{} is surrounded by ice and electricity.  "), str_upcase_first(Who::who(lore_ptr->msex))));
    } else if (has_fire_aura) {
        hook_c_roff(TERM_RED, fmt::format(_("{}は炎に包まれている。", "{} is surrounded by flames.  "), str_upcase_first(Who::who(lore_ptr->msex))));
    } else if (has_cold_aura) {
        hook_c_roff(TERM_BLUE, fmt::format(_("{}は氷に包まれている。", "{} is surrounded by ice.  "), str_upcase_first(Who::who(lore_ptr->msex))));
    } else if (has_elec_aura) {
        hook_c_roff(TERM_L_BLUE, fmt::format(_("{}はスパークに包まれている。", "{} is surrounded by electricity.  "), str_upcase_first(Who::who(lore_ptr->msex))));
    }
}

void display_lore_this(PlayerType *player_ptr, lore_type *lore_ptr)
{
    if ((lore_ptr->monrace->r_tkills == 0) && !lore_ptr->know_everything) {
        return;
    }

#ifdef JP
    hooked_roff("この");
#else
    if (lore_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        hooked_roff("Killing this");
    } else {
        hooked_roff("A kill of this");
    }
#endif

    display_monster_alignment(lore_ptr);
    display_monster_kind(lore_ptr);
    display_monster_exp(player_ptr, lore_ptr);
}

static void display_monster_escort_contents(lore_type *lore_ptr)
{
    if (!lore_ptr->has_reinforce()) {
        return;
    }

    hooked_roff(_("護衛の構成は", "These escorts"));
    if (lore_ptr->misc_flags.has(MonsterMiscType::ESCORT) || lore_ptr->misc_flags.has(MonsterMiscType::MORE_ESCORT)) {
        hooked_roff(_("少なくとも", " at the least"));
    }

    const auto &reinforces = lore_ptr->monrace->get_reinforces();
#ifdef JP
#else
    hooked_roff(" contain");
    const auto max_idx = reinforces.size() - 1;
    auto idx = 0U;
#endif

    for (const auto &reinforce : reinforces) {
#ifdef JP
#else
        const std::string prefix = (idx == 0) ? " " : (idx == max_idx) ? " and "
                                                                       : ", ";
        ++idx;
#endif
        if (!reinforce.is_valid()) {
            continue;
        }

        const auto &monrace = reinforce.get_monrace();
        if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            hooked_roff(fmt::format("{}{}", _("、", prefix), monrace.name));
            continue;
        }

#ifdef JP
        hooked_roff(fmt::format("、 {} 体の{}", reinforce.get_dice_as_string(), monrace.name));
#else
        const auto is_plural = reinforce.roll_max_dice() > 1;
        const auto &name = is_plural ? pluralize(monrace.name) : monrace.name.string();
        hooked_roff(fmt::format("{}{} {}", prefix, reinforce.get_dice_as_string(), name));
#endif
    }

    hooked_roff(_("で成り立っている。", ".  "));
}

void display_monster_collective(lore_type *lore_ptr)
{
    if (lore_ptr->misc_flags.has(MonsterMiscType::ESCORT) || lore_ptr->misc_flags.has(MonsterMiscType::MORE_ESCORT) || lore_ptr->has_reinforce()) {
        hooked_roff(fmt::format(_("{}は通常護衛を伴って現れる。", "{} usually appears with escorts.  "), str_upcase_first(Who::who(lore_ptr->msex))));
        display_monster_escort_contents(lore_ptr);
    } else if (lore_ptr->misc_flags.has(MonsterMiscType::HAS_FRIENDS)) {
        hooked_roff(fmt::format(_("{}は通常集団で現れる。", "{} usually appears in groups.  "), str_upcase_first(Who::who(lore_ptr->msex))));
    }
}

/*!
 * @brief モンスターの発射に関する情報を表示するルーチン /
 * Display monster launching information
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param lore_ptr モンスターの思い出構造体への参照ポインタ
 * @details
 * This function should only be called when display/dump a recall of
 * a monster.
 * @todo max_blows はゲームの中核的なパラメータの1つなのでどこかのヘッダに定数宣言しておきたい
 */
void display_monster_launching(PlayerType *player_ptr, lore_type *lore_ptr)
{
    if (lore_ptr->ability_flags.has(MonsterAbilityType::ROCKET)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::ROCKET, _("ロケット{}を発射する", "shoot a rocket{}"), TERM_UMBER);
        lore_ptr->rocket = true;
    }

    if (lore_ptr->ability_flags.has_not(MonsterAbilityType::SHOOT)) {
        return;
    }

    std::string msg;
    if (lore_ptr->is_details_known() || lore_ptr->know_everything) {
        msg = fmt::format(_("威力 {} の射撃をする", "fire an arrow (Power:{})"), lore_ptr->monrace->shoot_damage_dice.to_string());
    } else {
        msg = _("射撃をする", "fire an arrow");
    }

    lore_ptr->lore_msgs.emplace_back(msg, TERM_UMBER);
    lore_ptr->shoot = true;
}

void display_monster_sometimes(lore_type *lore_ptr)
{
    if (lore_ptr->lore_msgs.empty()) {
        return;
    }

    hooked_roff(fmt::format(_("{}は", "{}"), str_upcase_first(Who::who(lore_ptr->msex))));
    for (int n = 0; const auto &[msg, color] : lore_ptr->lore_msgs) {
#ifdef JP
        if (n != std::ssize(lore_ptr->lore_msgs) - 1) {
            const auto verb = conjugate_jverb(msg, JVerbConjugationType::OR);
            hook_c_roff(color, verb);
            hook_c_roff(color, "り");
            hooked_roff("、");
        } else {
            hook_c_roff(color, msg);
        }
#else
        if (n == 0) {
            hooked_roff(" may ");
        } else if (n < std::ssize(lore_ptr->lore_msgs) - 1) {
            hooked_roff(", ");
        } else {
            hooked_roff(" or ");
        }

        hook_c_roff(color, msg);
#endif
        n++;
    }

    hooked_roff(_("ことがある。", ".  "));
}

void display_monster_guardian(lore_type *lore_ptr)
{
    bool is_kingpin = lore_ptr->misc_flags.has(MonsterMiscType::QUESTOR);
    is_kingpin &= lore_ptr->record->has_been_seen();
    is_kingpin &= lore_ptr->monrace->max_num > 0;
    is_kingpin &= (lore_ptr->monrace_id == MonraceId::OBERON) || (lore_ptr->monrace_id == MonraceId::SERPENT);
    if (is_kingpin) {
        hook_c_roff(TERM_VIOLET, _("あなたはこのモンスターを殺したいという強い欲望を感じている...", "You feel an intense desire to kill this monster...  "));
    } else if (lore_ptr->misc_flags.has(MonsterMiscType::GUARDIAN)) {
        hook_c_roff(TERM_L_RED, _("このモンスターはダンジョンの主である。", "This monster is the master of a dungeon."));
    }

    hooked_roff("\n");
}
