/*!
 * @brief モンスターの思い出を表示する処理
 * @date 2020/06/09
 * @author Hourier
 */

#include "view/display-lore.h"
#include "game-option/cheat-options.h"
#include "game-option/text-display-options.h"
#include "locale/english.h"
#include "locale/japanese.h"
#include "lore/lore-calculator.h"
#include "lore/lore-util.h"
#include "lore/monster-lore.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * 英語の複数系記述用マクロ / Pluralizer.  Args(count, singular, plural)
 */
#define plural(c, s, p) (((c) == 1) ? (s) : (p))

/*!
 * @brief モンスター情報のヘッダを記述する
 * Hack -- Display the "name" and "attr/chars" of a monster race
 * @param r_idx モンスターの種族ID
 */
void roff_top(MonsterRaceId r_idx)
{
    auto *r_ptr = &monraces_info[r_idx];
    char c1 = r_ptr->d_char;
    char c2 = r_ptr->x_char;

    TERM_COLOR a1 = r_ptr->d_attr;
    TERM_COLOR a2 = r_ptr->x_attr;

    term_erase(0, 0, 255);
    term_gotoxy(0, 0);

#ifdef JP
#else
    if (r_ptr->kind_flags.has_not(MonsterKindType::UNIQUE)) {
        term_addstr(-1, TERM_WHITE, "The ");
    }
#endif

    if (w_ptr->wizard || cheat_know) {
        term_addstr(-1, TERM_WHITE, "[");
        term_addstr(-1, TERM_L_BLUE, format("%d", r_idx));
        term_addstr(-1, TERM_WHITE, "] ");
    }

    term_addstr(-1, TERM_WHITE, (r_ptr->name.data()));

    term_addstr(-1, TERM_WHITE, " ('");
    term_add_bigch(a1, c1);
    term_addstr(-1, TERM_WHITE, "')");

    term_addstr(-1, TERM_WHITE, "/('");
    term_add_bigch(a2, c2);
    term_addstr(-1, TERM_WHITE, "'):");
}

/*!
 * @brief  モンスター情報の表示と共に画面を一時消去するサブルーチン /
 * Hack -- describe the given monster race at the top of the screen
 * @param r_idx モンスターの種族ID
 * @param mode 表示オプション
 */
void screen_roff(PlayerType *player_ptr, MonsterRaceId r_idx, monster_lore_mode mode)
{
    msg_erase();
    term_erase(0, 1, 255);
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
        term_erase(0, y, 255);
    }

    term_gotoxy(0, 1);
    hook_c_roff = c_roff;
    MonsterRaceId r_idx = player_ptr->monster_race_idx;
    process_monster_lore(player_ptr, r_idx, MONSTER_LORE_NORMAL);
    roff_top(r_idx);
}

/*!
 * @brief モンスター詳細情報を自動スポイラー向けに出力する /
 * Hack -- output description of the given monster race
 * @param r_idx モンスターの種族ID
 * @param roff_func 出力処理を行う関数ポインタ
 * @todo ここのroff_funcの引数にFILE* を追加しないとspoiler_file をローカル関数化することができないと判明した、保留.
 */
void output_monster_spoiler(MonsterRaceId r_idx, hook_c_roff_pf roff_func)
{
    hook_c_roff = roff_func;
    PlayerType dummy;

    dummy.lev = 1;
    dummy.max_plv = 1;
    process_monster_lore(&dummy, r_idx, MONSTER_LORE_DEBUG);
}

static bool display_kill_unique(lore_type *lore_ptr)
{
    if (lore_ptr->kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return false;
    }

    bool dead = (lore_ptr->r_ptr->max_num == 0);
    if (lore_ptr->r_ptr->r_deaths) {
        hooked_roff(format(_("%^sはあなたの先祖を %d 人葬っている", "%^s has slain %d of your ancestors"), Who::who(lore_ptr->msex), lore_ptr->r_ptr->r_deaths));

        if (dead) {
            hooked_roff(
                _(format("が、すでに仇討ちは果たしている！"), format(", but you have avenged %s!  ", plural(lore_ptr->r_ptr->r_deaths, "him", "them"))));
        } else {
            hooked_roff(
                _(format("のに、まだ仇討ちを果たしていない。"), format(", who %s unavenged.  ", plural(lore_ptr->r_ptr->r_deaths, "remains", "remain"))));
        }

        hooked_roff("\n");
    } else {
        if (dead) {
            hooked_roff(_("あなたはこの仇敵をすでに葬り去っている。", "You have slain this foe.  "));
        } else {
            hooked_roff(_("この仇敵はまだ生きている！", "This foe is still alive!  "));
        }

        hooked_roff("\n");
    }

    return true;
}

static void display_killed(lore_type *lore_ptr)
{
    hooked_roff(_(format("このモンスターはあなたの先祖を %d 人葬っている", lore_ptr->r_ptr->r_deaths),
        format("%d of your ancestors %s been killed by this creature, ", lore_ptr->r_ptr->r_deaths, plural(lore_ptr->r_ptr->r_deaths, "has", "have"))));

    if (lore_ptr->r_ptr->r_pkills) {
        hooked_roff(format(_("が、あなたはこのモンスターを少なくとも %d 体は倒している。", "and you have exterminated at least %d of the creatures.  "),
            lore_ptr->r_ptr->r_pkills));
    } else if (lore_ptr->r_ptr->r_tkills) {
        hooked_roff(format(
            _("が、あなたの先祖はこのモンスターを少なくとも %d 体は倒している。", "and your ancestors have exterminated at least %d of the creatures.  "),
            lore_ptr->r_ptr->r_tkills));
    } else {
        hooked_roff(format(_("が、まだ%sを倒したことはない。", "and %s is not ever known to have been defeated.  "), Who::who(lore_ptr->msex)));
    }
}

static void display_no_killed(lore_type *lore_ptr)
{
    if (lore_ptr->r_ptr->r_pkills) {
        hooked_roff(format(
            _("あなたはこのモンスターを少なくとも %d 体は殺している。", "You have killed at least %d of these creatures.  "), lore_ptr->r_ptr->r_pkills));
    } else if (lore_ptr->r_ptr->r_tkills) {
        hooked_roff(format(_("あなたの先祖はこのモンスターを少なくとも %d 体は殺している。", "Your ancestors have killed at least %d of these creatures.  "),
            lore_ptr->r_ptr->r_tkills));
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
    if (lore_ptr->mode != MONSTER_LORE_DEBUG && lore_ptr->r_ptr->r_tkills == 0) {
        return;
    }
    if (!lore_ptr->r_ptr->population_flags.has(MonsterPopulationType::NAZGUL)) {
        return;
    }

    int remain = lore_ptr->r_ptr->max_num;
    int killed = lore_ptr->r_ptr->r_akills;
    if (remain == 0) {
#ifdef JP
        hooked_roff(format("%sはかつて %ld 体存在した。", Who::who(lore_ptr->msex, (killed > 1)), killed));
#else
        hooked_roff(format("You already killed all %ld of %s.  ", killed, Who::whom(lore_ptr->msex, (killed > 1))));
#endif
    } else {
#ifdef JP
        hooked_roff(format("%sはまだ %ld 体生きている。", Who::who(lore_ptr->msex, (remain + killed > 1)), remain));
#else
        concptr be = (remain > 1) ? "are" : "is";
        hooked_roff(format("%ld of %s %s still alive.  ", remain, Who::whom(lore_ptr->msex, (remain + killed > 1)), be));
#endif
    }
}

void display_kill_numbers(lore_type *lore_ptr)
{
    if ((lore_ptr->mode & 0x02) != 0) {
        return;
    }

    if (display_kill_unique(lore_ptr)) {
        return;
    }

    if (lore_ptr->r_ptr->r_deaths == 0) {
        display_no_killed(lore_ptr);
    } else {
        display_killed(lore_ptr);
    }

    display_number_of_nazguls(lore_ptr);

    hooked_roff("\n");
}

/*!
 * @brief どこに出没するかを表示する
 * @param lore_ptr モンスターの思い出構造体への参照ポインタ
 * @return たぬきならFALSE、それ以外はTRUE
 */
bool display_where_to_appear(lore_type *lore_ptr)
{
    lore_ptr->old = false;
    if (lore_ptr->r_ptr->level == 0) {
        hooked_roff(format(_("%^sは町に住み", "%^s lives in the town"), Who::who(lore_ptr->msex)));
        lore_ptr->old = true;
    } else if (lore_ptr->r_ptr->r_tkills || lore_ptr->know_everything) {
        if (depth_in_feet) {
            hooked_roff(format(
                _("%^sは通常地下 %d フィートで出現し", "%^s is normally found at depths of %d feet"), Who::who(lore_ptr->msex), lore_ptr->r_ptr->level * 50));
        } else {
            hooked_roff(format(_("%^sは通常地下 %d 階で出現し", "%^s is normally found on dungeon level %d"), Who::who(lore_ptr->msex), lore_ptr->r_ptr->level));
        }

        lore_ptr->old = true;
    }

    if (lore_ptr->r_idx == MonsterRaceId::CHAMELEON) {
        hooked_roff(_("、他のモンスターに化ける。", "and can take the shape of other monster."));
        return false;
    }

    if (lore_ptr->old) {
        hooked_roff(_("、", ", and "));
    } else {
        hooked_roff(format(_("%^sは", "%^s "), Who::who(lore_ptr->msex)));
        lore_ptr->old = true;
    }

    return true;
}

// @todo モンスターの速度表記はmonster_typeのオブジェクトメソッドにした方がベター
void display_monster_move(lore_type *lore_ptr)
{
#ifdef JP
#else
    hooked_roff("moves");
#endif

    display_random_move(lore_ptr);
    if (lore_ptr->speed > STANDARD_SPEED) {
        if (lore_ptr->speed > 139) {
            hook_c_roff(TERM_RED, _("信じ難いほど", " incredibly"));
        } else if (lore_ptr->speed > 134) {
            hook_c_roff(TERM_ORANGE, _("猛烈に", " extremely"));
        } else if (lore_ptr->speed > 129) {
            hook_c_roff(TERM_ORANGE, _("非常に", " very"));
        } else if (lore_ptr->speed > 124) {
            hook_c_roff(TERM_UMBER, _("かなり", " fairly"));
        } else if (lore_ptr->speed < 120) {
            hook_c_roff(TERM_L_UMBER, _("やや", " somewhat"));
        }
        hook_c_roff(TERM_L_RED, _("素早く", " quickly"));
    } else if (lore_ptr->speed < STANDARD_SPEED) {
        if (lore_ptr->speed < 90) {
            hook_c_roff(TERM_L_GREEN, _("信じ難いほど", " incredibly"));
        } else if (lore_ptr->speed < 95) {
            hook_c_roff(TERM_BLUE, _("非常に", " very"));
        } else if (lore_ptr->speed < 100) {
            hook_c_roff(TERM_BLUE, _("かなり", " fairly"));
        } else if (lore_ptr->speed > 104) {
            hook_c_roff(TERM_GREEN, _("やや", " somewhat"));
        }
        hook_c_roff(TERM_L_BLUE, _("ゆっくりと", " slowly"));
    } else {
        hooked_roff(_("普通の速さで", " at normal speed"));
    }

#ifdef JP
    hooked_roff("動いている");
#endif
}

void display_random_move(lore_type *lore_ptr)
{
    if (lore_ptr->behavior_flags.has_none_of({ MonsterBehaviorType::RAND_MOVE_50, MonsterBehaviorType::RAND_MOVE_25 })) {
        return;
    }

    if (lore_ptr->behavior_flags.has(MonsterBehaviorType::RAND_MOVE_50) && lore_ptr->behavior_flags.has(MonsterBehaviorType::RAND_MOVE_25)) {
        hooked_roff(_("かなり", " extremely"));
    } else if (lore_ptr->behavior_flags.has(MonsterBehaviorType::RAND_MOVE_50)) {
        hooked_roff(_("幾分", " somewhat"));
    } else if (lore_ptr->behavior_flags.has(MonsterBehaviorType::RAND_MOVE_25)) {
        hooked_roff(_("少々", " a bit"));
    }

    hooked_roff(_("不規則に", " erratically"));
    if (lore_ptr->speed != STANDARD_SPEED) {
        hooked_roff(_("、かつ", ", and"));
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
        hooked_roff(format(_("%^sは", "%^s "), Who::who(lore_ptr->msex)));
        lore_ptr->old = true;
    }

    hooked_roff(_("侵入者を追跡しない", "does not deign to chase intruders"));
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
    if (lore_ptr->flags2 & RF2_ELDRITCH_HORROR) {
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

    int64_t base_exp = lore_ptr->r_ptr->mexp * lore_ptr->r_ptr->level * 3 / 2;
    int64_t player_factor = (int64_t)player_ptr->max_plv + 2;

    int64_t exp_integer = base_exp / player_factor;
    int64_t exp_decimal = ((base_exp % player_factor * 1000 / player_factor) + 5) / 10;

#ifdef JP
    hooked_roff(format(" %d レベルのキャラクタにとって 約%Ld.%02Ld ポイントの経験となる。", player_ptr->lev, exp_integer, exp_decimal));
#else
    hooked_roff(format(" is worth about %Ld.%02Ld point%s", exp_integer, exp_decimal, ((exp_integer == 1) && (exp_decimal == 0)) ? "" : "s"));

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

    hooked_roff(format(" for a%s %d%s level character.  ", vowel, player_ptr->lev, ordinal));
#endif
}

void display_monster_aura(lore_type *lore_ptr)
{
    auto has_fire_aura = lore_ptr->aura_flags.has(MonsterAuraType::FIRE);
    auto has_elec_aura = lore_ptr->aura_flags.has(MonsterAuraType::ELEC);
    auto has_cold_aura = lore_ptr->aura_flags.has(MonsterAuraType::COLD);
    if (has_fire_aura && has_elec_aura && has_cold_aura) {
        hook_c_roff(
            TERM_VIOLET, format(_("%^sは炎と氷とスパークに包まれている。", "%^s is surrounded by flames, ice and electricity.  "), Who::who(lore_ptr->msex)));
    } else if (has_fire_aura && has_elec_aura) {
        hook_c_roff(TERM_L_RED, format(_("%^sは炎とスパークに包まれている。", "%^s is surrounded by flames and electricity.  "), Who::who(lore_ptr->msex)));
    } else if (has_fire_aura && has_cold_aura) {
        hook_c_roff(TERM_BLUE, format(_("%^sは炎と氷に包まれている。", "%^s is surrounded by flames and ice.  "), Who::who(lore_ptr->msex)));
    } else if (has_cold_aura && has_elec_aura) {
        hook_c_roff(TERM_L_GREEN, format(_("%^sは氷とスパークに包まれている。", "%^s is surrounded by ice and electricity.  "), Who::who(lore_ptr->msex)));
    } else if (has_fire_aura) {
        hook_c_roff(TERM_RED, format(_("%^sは炎に包まれている。", "%^s is surrounded by flames.  "), Who::who(lore_ptr->msex)));
    } else if (has_cold_aura) {
        hook_c_roff(TERM_BLUE, format(_("%^sは氷に包まれている。", "%^s is surrounded by ice.  "), Who::who(lore_ptr->msex)));
    } else if (has_elec_aura) {
        hook_c_roff(TERM_L_BLUE, format(_("%^sはスパークに包まれている。", "%^s is surrounded by electricity.  "), Who::who(lore_ptr->msex)));
    }
}

void display_lore_this(PlayerType *player_ptr, lore_type *lore_ptr)
{
    if ((lore_ptr->r_ptr->r_tkills == 0) && !lore_ptr->know_everything) {
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
    if (!lore_ptr->reinforce) {
        return;
    }

    hooked_roff(_("護衛の構成は", "These escorts"));
    if ((lore_ptr->flags1 & RF1_ESCORT) || (lore_ptr->flags1 & RF1_ESCORTS)) {
        hooked_roff(_("少なくとも", " at the least"));
    }

#ifdef JP
#else
    hooked_roff(" contain ");
#endif

    for (auto [r_idx, dd, ds] : lore_ptr->r_ptr->reinforces) {
        auto is_reinforced = MonsterRace(r_idx).is_valid();
        is_reinforced &= dd > 0;
        is_reinforced &= ds > 0;
        if (!is_reinforced) {
            continue;
        }

        const auto *rf_ptr = &monraces_info[r_idx];
        if (rf_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
            hooked_roff(format(_("、%s", ", %s"), rf_ptr->name.data()));
            continue;
        }

#ifdef JP
        hooked_roff(format("、 %dd%d 体の%s", dd, ds, rf_ptr->name.data()));
#else
        auto plural = (dd * ds > 1);
        GAME_TEXT name[MAX_NLEN];
        strcpy(name, rf_ptr->name.data());
        if (plural) {
            plural_aux(name);
        }
        hooked_roff(format(",%dd%d %s", dd, ds, name));
#endif
    }

    hooked_roff(_("で成り立っている。", "."));
}

void display_monster_collective(lore_type *lore_ptr)
{
    if ((lore_ptr->flags1 & RF1_ESCORT) || (lore_ptr->flags1 & RF1_ESCORTS) || lore_ptr->reinforce) {
        hooked_roff(format(_("%^sは通常護衛を伴って現れる。", "%^s usually appears with escorts.  "), Who::who(lore_ptr->msex)));
        display_monster_escort_contents(lore_ptr);
    } else if (lore_ptr->flags1 & RF1_FRIENDS) {
        hooked_roff(format(_("%^sは通常集団で現れる。", "%^s usually appears in groups.  "), Who::who(lore_ptr->msex)));
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
        set_damage(player_ptr, lore_ptr, MonsterAbilityType::ROCKET, _("ロケット%sを発射する", "shoot a rocket%s"));
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_UMBER;
        lore_ptr->rocket = true;
    }

    if (lore_ptr->ability_flags.has_not(MonsterAbilityType::SHOOT)) {
        return;
    }

    int p = -1; /* Position of SHOOT */
    int n = 0; /* Number of blows */
    const int max_blows = 4;
    for (int m = 0; m < max_blows; m++) {
        if (lore_ptr->r_ptr->blow[m].method != RaceBlowMethodType::NONE) {
            n++;
        } /* Count blows */

        if (lore_ptr->r_ptr->blow[m].method == RaceBlowMethodType::SHOOT) {
            p = m; /* Remember position */
            break;
        }
    }

    /* When full blows, use a first damage */
    if (n == max_blows) {
        p = 0;
    }

    if (p < 0) {
        return;
    }

    if (know_armour(lore_ptr->r_idx, lore_ptr->know_everything)) {
        strnfmt(lore_ptr->tmp_msg[lore_ptr->vn], sizeof(lore_ptr->tmp_msg[lore_ptr->vn]), _("威力 %dd%d の射撃をする", "fire an arrow (Power:%dd%d)"), lore_ptr->r_ptr->blow[p].d_dice,
            lore_ptr->r_ptr->blow[p].d_side);
    } else {
        angband_strcpy(lore_ptr->tmp_msg[lore_ptr->vn], _("射撃をする", "fire an arrow"), sizeof(lore_ptr->tmp_msg[lore_ptr->vn]));
    }

    lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
    lore_ptr->color[lore_ptr->vn++] = TERM_UMBER;
    lore_ptr->shoot = true;
}

void display_monster_sometimes(lore_type *lore_ptr)
{
    if (lore_ptr->vn <= 0) {
        return;
    }

    hooked_roff(format(_("%^sは", "%^s"), Who::who(lore_ptr->msex)));
    for (int n = 0; n < lore_ptr->vn; n++) {
#ifdef JP
        if (n != lore_ptr->vn - 1) {
            jverb(lore_ptr->vp[n], lore_ptr->jverb_buf, JVERB_OR);
            hook_c_roff(lore_ptr->color[n], lore_ptr->jverb_buf);
            hook_c_roff(lore_ptr->color[n], "り");
            hooked_roff("、");
        } else {
            hook_c_roff(lore_ptr->color[n], lore_ptr->vp[n]);
        }
#else
        if (n == 0) {
            hooked_roff(" may ");
        } else if (n < lore_ptr->vn - 1) {
            hooked_roff(", ");
        } else {
            hooked_roff(" or ");
        }

        hook_c_roff(lore_ptr->color[n], lore_ptr->vp[n]);
#endif
    }

    hooked_roff(_("ことがある。", ".  "));
}

void display_monster_guardian(lore_type *lore_ptr)
{
    bool is_kingpin = (lore_ptr->flags1 & RF1_QUESTOR) != 0;
    is_kingpin &= lore_ptr->r_ptr->r_sights > 0;
    is_kingpin &= lore_ptr->r_ptr->max_num > 0;
    is_kingpin &= (lore_ptr->r_idx == MonsterRaceId::OBERON) || (lore_ptr->r_idx == MonsterRaceId::SERPENT);
    if (is_kingpin) {
        hook_c_roff(TERM_VIOLET, _("あなたはこのモンスターを殺したいという強い欲望を感じている...", "You feel an intense desire to kill this monster...  "));
    } else if (lore_ptr->flags7 & RF7_GUARDIAN) {
        hook_c_roff(TERM_L_RED, _("このモンスターはダンジョンの主である。", "This monster is the master of a dungeon."));
    }

    hooked_roff("\n");
}
