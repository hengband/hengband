/*!
 * @brief モンスターの思い出を表示する処理
 * @date 2020/06/09
 * @author Hourier
 */

#include "view/display-lore.h"
#include "game-option/text-display-options.h"
#include "lore/lore-calculator.h"
#include "lore/monster-lore.h"
#include "monster-attack/monster-attack-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags4.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "mspell/mspell-type.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "view/display-messages.h"
#include "world/world.h"
#ifdef JP
#include "locale/japanese.h"
#else
#include "locale/english.h"
#endif

/*!
 * 英語の複数系記述用マクロ / Pluralizer.  Args(count, singular, plural)
 */
#define plural(c, s, p) (((c) == 1) ? (s) : (p))

/*!
 * @brief モンスター情報のヘッダを記述する
 * Hack -- Display the "name" and "attr/chars" of a monster race
 * @param r_idx モンスターの種族ID
 * @return なし
 */
void roff_top(MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    char c1 = r_ptr->d_char;
    char c2 = r_ptr->x_char;

    TERM_COLOR a1 = r_ptr->d_attr;
    TERM_COLOR a2 = r_ptr->x_attr;

    term_erase(0, 0, 255);
    term_gotoxy(0, 0);

#ifdef JP
#else
    if (!(r_ptr->flags1 & RF1_UNIQUE)) {
        term_addstr(-1, TERM_WHITE, "The ");
    }
#endif

    term_addstr(-1, TERM_WHITE, (r_name + r_ptr->name));

    term_addstr(-1, TERM_WHITE, " ('");
    term_add_bigch(a1, c1);
    term_addstr(-1, TERM_WHITE, "')");

    term_addstr(-1, TERM_WHITE, "/('");
    term_add_bigch(a2, c2);
    term_addstr(-1, TERM_WHITE, "'):");

    if (!current_world_ptr->wizard)
        return;

    char buf[16];
    sprintf(buf, "%d", r_idx);
    term_addstr(-1, TERM_WHITE, " (");
    term_addstr(-1, TERM_L_BLUE, buf);
    term_addch(TERM_WHITE, ')');
}

/*!
 * @brief  モンスター情報の表示と共に画面を一時消去するサブルーチン /
 * Hack -- describe the given monster race at the top of the screen
 * @param r_idx モンスターの種族ID
 * @param mode 表示オプション
 * @return なし
 */
void screen_roff(player_type *player_ptr, MONRACE_IDX r_idx, BIT_FLAGS mode)
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
 * @return なし
 */
void display_roff(player_type *player_ptr)
{
    for (int y = 0; y < Term->hgt; y++) {
        term_erase(0, y, 255);
    }

    term_gotoxy(0, 1);
    hook_c_roff = c_roff;
    MONRACE_IDX r_idx = player_ptr->monster_race_idx;
    process_monster_lore(player_ptr, r_idx, 0);
    roff_top(r_idx);
}

/*!
 * todo ここのroff_funcの引数にFILE* を追加しないとspoiler_file をローカル関数化することができないと判明した、保留.
 * @brief モンスター詳細情報を自動スポイラー向けに出力する /
 * Hack -- output description of the given monster race
 * @param r_idx モンスターの種族ID
 * @param roff_func 出力処理を行う関数ポインタ
 * @return なし
 */
void output_monster_spoiler(player_type *player_ptr, MONRACE_IDX r_idx, void (*roff_func)(TERM_COLOR attr, concptr str))
{
    hook_c_roff = roff_func;
    process_monster_lore(player_ptr, r_idx, 0x03);
}

static bool display_kill_unique(lore_type *lore_ptr)
{
    if ((lore_ptr->flags1 & RF1_UNIQUE) == 0)
        return FALSE;

    bool dead = (lore_ptr->r_ptr->max_num == 0);
    if (lore_ptr->r_ptr->r_deaths) {
        hooked_roff(format(_("%^sはあなたの先祖を %d 人葬っている", "%^s has slain %d of your ancestors"), wd_he[lore_ptr->msex], lore_ptr->r_ptr->r_deaths));

        if (dead) {
            hooked_roff(
                _(format("が、すでに仇討ちは果たしている！"), format(", but you have avenged %s!  ", plural(lore_ptr->r_ptr->r_deaths, "him", "them"))));
        } else {
            hooked_roff(
                _(format("のに、まだ仇討ちを果たしていない。"), format(", who %s unavenged.  ", plural(lore_ptr->r_ptr->r_deaths, "remains", "remain"))));
        }

        hooked_roff("\n");
    } else if (dead) {
        hooked_roff(_("あなたはこの仇敵をすでに葬り去っている。", "You have slain this foe.  "));
        hooked_roff("\n");
    }

    return TRUE;
}

static bool display_killed(lore_type *lore_ptr)
{
    if (lore_ptr->r_ptr->r_deaths == 0)
        return FALSE;

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
        hooked_roff(format(_("が、まだ%sを倒したことはない。", "and %s is not ever known to have been defeated.  "), wd_he[lore_ptr->msex]));
    }

    hooked_roff("\n");
    return TRUE;
}

void display_kill_numbers(lore_type *lore_ptr)
{
    if ((lore_ptr->mode & 0x02) != 0)
        return;

    if (display_kill_unique(lore_ptr))
        return;

    if (display_killed(lore_ptr))
        return;

    if (lore_ptr->r_ptr->r_pkills) {
        hooked_roff(format(
            _("あなたはこのモンスターを少なくとも %d 体は殺している。", "You have killed at least %d of these creatures.  "), lore_ptr->r_ptr->r_pkills));
    } else if (lore_ptr->r_ptr->r_tkills) {
        hooked_roff(format(_("あなたの先祖はこのモンスターを少なくとも %d 体は殺している。", "Your ancestors have killed at least %d of these creatures.  "),
            lore_ptr->r_ptr->r_tkills));
    } else {
        hooked_roff(_("このモンスターを倒したことはない。", "No battles to the death are recalled.  "));
    }

    hooked_roff("\n");
}

/*!
 * @brief どこに出没するかを表示する
 * @param lore_ptr モンスターの思い出構造体への参照ポインタ
 * @return たぬきならFALSE、それ以外はTRUE
 */
bool display_where_to_appear(lore_type *lore_ptr)
{
    lore_ptr->old = FALSE;
    if (lore_ptr->r_ptr->level == 0) {
        hooked_roff(format(_("%^sは町に住み", "%^s lives in the town"), wd_he[lore_ptr->msex]));
        lore_ptr->old = TRUE;
    } else if (lore_ptr->r_ptr->r_tkills || lore_ptr->know_everything) {
        if (depth_in_feet) {
            hooked_roff(format(
                _("%^sは通常地下 %d フィートで出現し", "%^s is normally found at depths of %d feet"), wd_he[lore_ptr->msex], lore_ptr->r_ptr->level * 50));
        } else {
            hooked_roff(format(_("%^sは通常地下 %d 階で出現し", "%^s is normally found on dungeon level %d"), wd_he[lore_ptr->msex], lore_ptr->r_ptr->level));
        }

        lore_ptr->old = TRUE;
    }

    if (lore_ptr->r_idx == MON_CHAMELEON) {
        hooked_roff(_("、他のモンスターに化ける。", "and can take the shape of other monster."));
        return FALSE;
    }

    if (lore_ptr->old) {
        hooked_roff(_("、", ", and "));
    } else {
        hooked_roff(format(_("%^sは", "%^s "), wd_he[lore_ptr->msex]));
        lore_ptr->old = TRUE;
    }

    return TRUE;
}

void display_monster_move(lore_type *lore_ptr)
{
#ifdef JP
#else
    hooked_roff("moves");
#endif

    display_random_move(lore_ptr);
    if (lore_ptr->speed > 110) {
        if (lore_ptr->speed > 139)
            hook_c_roff(TERM_RED, _("信じ難いほど", " incredibly"));
        else if (lore_ptr->speed > 134)
            hook_c_roff(TERM_ORANGE, _("猛烈に", " extremely"));
        else if (lore_ptr->speed > 129)
            hook_c_roff(TERM_ORANGE, _("非常に", " very"));
        else if (lore_ptr->speed > 124)
            hook_c_roff(TERM_UMBER, _("かなり", " fairly"));
        else if (lore_ptr->speed < 120)
            hook_c_roff(TERM_L_UMBER, _("やや", " somewhat"));
        hook_c_roff(TERM_L_RED, _("素早く", " quickly"));
    } else if (lore_ptr->speed < 110) {
        if (lore_ptr->speed < 90)
            hook_c_roff(TERM_L_GREEN, _("信じ難いほど", " incredibly"));
        else if (lore_ptr->speed < 95)
            hook_c_roff(TERM_BLUE, _("非常に", " very"));
        else if (lore_ptr->speed < 100)
            hook_c_roff(TERM_BLUE, _("かなり", " fairly"));
        else if (lore_ptr->speed > 104)
            hook_c_roff(TERM_GREEN, _("やや", " somewhat"));
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
    if (((lore_ptr->flags1 & RF1_RAND_50) == 0) && ((lore_ptr->flags1 & RF1_RAND_25) == 0))
        return;

    if ((lore_ptr->flags1 & RF1_RAND_50) && (lore_ptr->flags1 & RF1_RAND_25)) {
        hooked_roff(_("かなり", " extremely"));
    } else if (lore_ptr->flags1 & RF1_RAND_50) {
        hooked_roff(_("幾分", " somewhat"));
    } else if (lore_ptr->flags1 & RF1_RAND_25) {
        hooked_roff(_("少々", " a bit"));
    }

    hooked_roff(_("不規則に", " erratically"));
    if (lore_ptr->speed != 110)
        hooked_roff(_("、かつ", ", and"));
}

void display_monster_never_move(lore_type *lore_ptr)
{
    if ((lore_ptr->flags1 & RF1_NEVER_MOVE) == 0)
        return;

    if (lore_ptr->old) {
        hooked_roff(_("、しかし", ", but "));
    } else {
        hooked_roff(format(_("%^sは", "%^s "), wd_he[lore_ptr->msex]));
        lore_ptr->old = TRUE;
    }

    hooked_roff(_("侵入者を追跡しない", "does not deign to chase intruders"));
}

void display_monster_kind(lore_type *lore_ptr)
{
    if (((lore_ptr->flags3 & (RF3_DRAGON | RF3_DEMON | RF3_GIANT | RF3_TROLL | RF3_ORC | RF3_ANGEL)) == 0)
        && ((lore_ptr->flags2 & (RF2_QUANTUM | RF2_HUMAN)) == 0)) {
        hooked_roff(_("モンスター", " creature"));
        return;
    }

    if (lore_ptr->flags3 & RF3_DRAGON)
        hook_c_roff(TERM_ORANGE, _("ドラゴン", " dragon"));

    if (lore_ptr->flags3 & RF3_DEMON)
        hook_c_roff(TERM_VIOLET, _("デーモン", " demon"));

    if (lore_ptr->flags3 & RF3_GIANT)
        hook_c_roff(TERM_L_UMBER, _("巨人", " giant"));

    if (lore_ptr->flags3 & RF3_TROLL)
        hook_c_roff(TERM_BLUE, _("トロル", " troll"));

    if (lore_ptr->flags3 & RF3_ORC)
        hook_c_roff(TERM_UMBER, _("オーク", " orc"));

    if (lore_ptr->flags2 & RF2_HUMAN)
        hook_c_roff(TERM_L_WHITE, _("人間", " human"));

    if (lore_ptr->flags2 & RF2_QUANTUM)
        hook_c_roff(TERM_VIOLET, _("量子生物", " quantum creature"));

    if (lore_ptr->flags3 & RF3_ANGEL)
        hook_c_roff(TERM_YELLOW, _("天使", " angel"));
}

void display_monster_alignment(lore_type *lore_ptr)
{
    if (lore_ptr->flags2 & RF2_ELDRITCH_HORROR)
        hook_c_roff(TERM_VIOLET, _("狂気を誘う", " sanity-blasting"));

    if (lore_ptr->flags3 & RF3_ANIMAL)
        hook_c_roff(TERM_L_GREEN, _("自然界の", " natural"));

    if (lore_ptr->flags3 & RF3_EVIL)
        hook_c_roff(TERM_L_DARK, _("邪悪なる", " evil"));

    if (lore_ptr->flags3 & RF3_GOOD)
        hook_c_roff(TERM_YELLOW, _("善良な", " good"));

    if (lore_ptr->flags3 & RF3_UNDEAD)
        hook_c_roff(TERM_VIOLET, _("アンデッドの", " undead"));

    if (lore_ptr->flags3 & RF3_AMBERITE)
        hook_c_roff(TERM_VIOLET, _("アンバーの王族の", " Amberite"));
}

void display_monster_exp(player_type *player_ptr, lore_type *lore_ptr)
{
#ifdef JP
    hooked_roff("を倒すことは");
#endif
    long exp_integer = (long)lore_ptr->r_ptr->mexp * lore_ptr->r_ptr->level / (player_ptr->max_plv + 2) * 3 / 2;
    long exp_decimal
        = ((((long)lore_ptr->r_ptr->mexp * lore_ptr->r_ptr->level % (player_ptr->max_plv + 2) * 3 / 2) * (long)1000 / (player_ptr->max_plv + 2) + 5) / 10);

#ifdef JP
    hooked_roff(format(" %d レベルのキャラクタにとって 約%ld.%02ld ポイントの経験となる。", player_ptr->lev, (long)exp_integer, (long)exp_decimal));
#else
    hooked_roff(format(" is worth about %ld.%02ld point%s", (long)exp_integer, (long)exp_decimal, ((exp_integer == 1) && (exp_decimal == 0)) ? "" : "s"));

    char *ordinal;
    ordinal = "th";
    exp_integer = player_ptr->lev % 10;
    if ((player_ptr->lev / 10) != 1) {
        if (exp_integer == 1)
            ordinal = "st";
        else if (exp_integer == 2)
            ordinal = "nd";
        else if (exp_integer == 3)
            ordinal = "rd";
    }

    char *vowel;
    vowel = "";
    exp_integer = player_ptr->lev;
    if ((exp_integer == 8) || (exp_integer == 11) || (exp_integer == 18))
        vowel = "n";

    hooked_roff(format(" for a%s %lu%s level character.  ", vowel, (long)exp_integer, ordinal));
#endif
}

void display_monster_aura(lore_type *lore_ptr)
{
    if ((lore_ptr->flags2 & RF2_AURA_FIRE) && (lore_ptr->flags2 & RF2_AURA_ELEC) && (lore_ptr->flags3 & RF3_AURA_COLD))
        hook_c_roff(
            TERM_VIOLET, format(_("%^sは炎と氷とスパークに包まれている。", "%^s is surrounded by flames, ice and electricity.  "), wd_he[lore_ptr->msex]));
    else if ((lore_ptr->flags2 & RF2_AURA_FIRE) && (lore_ptr->flags2 & RF2_AURA_ELEC))
        hook_c_roff(TERM_L_RED, format(_("%^sは炎とスパークに包まれている。", "%^s is surrounded by flames and electricity.  "), wd_he[lore_ptr->msex]));
    else if ((lore_ptr->flags2 & RF2_AURA_FIRE) && (lore_ptr->flags3 & RF3_AURA_COLD))
        hook_c_roff(TERM_BLUE, format(_("%^sは炎と氷に包まれている。", "%^s is surrounded by flames and ice.  "), wd_he[lore_ptr->msex]));
    else if ((lore_ptr->flags3 & RF3_AURA_COLD) && (lore_ptr->flags2 & RF2_AURA_ELEC))
        hook_c_roff(TERM_L_GREEN, format(_("%^sは氷とスパークに包まれている。", "%^s is surrounded by ice and electricity.  "), wd_he[lore_ptr->msex]));
    else if (lore_ptr->flags2 & RF2_AURA_FIRE)
        hook_c_roff(TERM_RED, format(_("%^sは炎に包まれている。", "%^s is surrounded by flames.  "), wd_he[lore_ptr->msex]));
    else if (lore_ptr->flags3 & RF3_AURA_COLD)
        hook_c_roff(TERM_BLUE, format(_("%^sは氷に包まれている。", "%^s is surrounded by ice.  "), wd_he[lore_ptr->msex]));
    else if (lore_ptr->flags2 & RF2_AURA_ELEC)
        hook_c_roff(TERM_L_BLUE, format(_("%^sはスパークに包まれている。", "%^s is surrounded by electricity.  "), wd_he[lore_ptr->msex]));
}

void display_lore_this(player_type *player_ptr, lore_type *lore_ptr)
{
    if ((lore_ptr->r_ptr->r_tkills == 0) && !lore_ptr->know_everything)
        return;

#ifdef JP
    hooked_roff("この");
#else
    if (lore_ptr->flags1 & RF1_UNIQUE) {
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
    if (!lore_ptr->reinforce)
        return;

    hooked_roff(_("護衛の構成は", "These escorts"));
    if ((lore_ptr->flags1 & RF1_ESCORT) || (lore_ptr->flags1 & RF1_ESCORTS)) {
        hooked_roff(_("少なくとも", " at the least"));
    }

#ifdef JP
#else
    hooked_roff(" contain ");
#endif

    for (int n = 0; n < A_MAX; n++) {
        bool is_reinforced = lore_ptr->r_ptr->reinforce_id[n] > 0;
        is_reinforced &= lore_ptr->r_ptr->reinforce_dd[n] > 0;
        is_reinforced &= lore_ptr->r_ptr->reinforce_ds[n] > 0;
        if (!is_reinforced)
            continue;

        monster_race *rf_ptr = &r_info[lore_ptr->r_ptr->reinforce_id[n]];
        if (rf_ptr->flags1 & RF1_UNIQUE) {
            hooked_roff(format(_("、%s", ", %s"), r_name + rf_ptr->name));
            continue;
        }

#ifdef JP
        hooked_roff(format("、 %dd%d 体の%s", lore_ptr->r_ptr->reinforce_dd[n], lore_ptr->r_ptr->reinforce_ds[n], r_name + rf_ptr->name));
#else
        bool plural = (lore_ptr->r_ptr->reinforce_dd[n] * lore_ptr->r_ptr->reinforce_ds[n] > 1);
        GAME_TEXT name[MAX_NLEN];
        strcpy(name, r_name + rf_ptr->name);
        if (plural)
            plural_aux(name);
        hooked_roff(format(",%dd%d %s", lore_ptr->r_ptr->reinforce_dd[n], lore_ptr->r_ptr->reinforce_ds[n], name));
#endif
    }

    hooked_roff(_("で成り立っている。", "."));
}

void display_monster_collective(lore_type *lore_ptr)
{
    if ((lore_ptr->flags1 & RF1_ESCORT) || (lore_ptr->flags1 & RF1_ESCORTS) || lore_ptr->reinforce) {
        hooked_roff(format(_("%^sは通常護衛を伴って現れる。", "%^s usually appears with escorts.  "), wd_he[lore_ptr->msex]));
        display_monster_escort_contents(lore_ptr);
    } else if (lore_ptr->flags1 & RF1_FRIENDS) {
        hooked_roff(format(_("%^sは通常集団で現れる。", "%^s usually appears in groups.  "), wd_he[lore_ptr->msex]));
    }
}

/*!
 * todo max_blows はゲームの中核的なパラメータの1つなのでどこかのヘッダに定数宣言しておきたい
 * @brief モンスターの発射に関する情報を表示するルーチン /
 * Display monster launching information
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param lore_ptr モンスターの思い出構造体への参照ポインタ
 * @return なし
 * @details
 * This function should only be called when display/dump a recall of
 * a monster.
 */
void display_monster_launching(player_type *player_ptr, lore_type *lore_ptr)
{
    if (lore_ptr->flags4 & RF4_ROCKET) {
        set_damage(player_ptr, lore_ptr->r_idx, (MS_ROCKET), _("ロケット%sを発射する", "shoot a rocket%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_UMBER;
    }

    if ((lore_ptr->flags4 & RF4_SHOOT) == 0)
        return;

    int p = -1; /* Position of SHOOT */
    int n = 0; /* Number of blows */
    const int max_blows = 4;
    for (int m = 0; m < max_blows; m++) {
        if (lore_ptr->r_ptr->blow[m].method != RBM_NONE)
            n++; /* Count blows */

        if (lore_ptr->r_ptr->blow[m].method == RBM_SHOOT) {
            p = m; /* Remember position */
            break;
        }
    }

    /* When full blows, use a first damage */
    if (n == max_blows)
        p = 0;

    if (p < 0)
        return;

    if (know_armour(lore_ptr->r_idx))
        sprintf(lore_ptr->tmp_msg[lore_ptr->vn], _("威力 %dd%d の射撃をする", "fire an arrow (Power:%dd%d)"), lore_ptr->r_ptr->blow[p].d_side,
            lore_ptr->r_ptr->blow[p].d_dice);
    else
        sprintf(lore_ptr->tmp_msg[lore_ptr->vn], _("射撃をする", "fire an arrow"));

    lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
    lore_ptr->color[lore_ptr->vn++] = TERM_UMBER;
}

void display_monster_sometimes(lore_type *lore_ptr)
{
    if (lore_ptr->vn <= 0)
        return;

    hooked_roff(format(_("%^sは", "%^s"), wd_he[lore_ptr->msex]));
    for (int n = 0; n < lore_ptr->vn; n++) {
#ifdef JP
        if (n != lore_ptr->vn - 1) {
            jverb(lore_ptr->vp[n], lore_ptr->jverb_buf, JVERB_OR);
            hook_c_roff(lore_ptr->color[n], lore_ptr->jverb_buf);
            hook_c_roff(lore_ptr->color[n], "り");
            hooked_roff("、");
        } else
            hook_c_roff(lore_ptr->color[n], lore_ptr->vp[n]);
#else
        if (n == 0)
            hooked_roff(" may ");
        else if (n < lore_ptr->vn - 1)
            hooked_roff(", ");
        else
            hooked_roff(" or ");

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
    is_kingpin &= (lore_ptr->r_idx == MON_OBERON) || (lore_ptr->r_idx == MON_SERPENT);
    if (is_kingpin) {
        hook_c_roff(TERM_VIOLET, _("あなたはこのモンスターを殺したいという強い欲望を感じている...", "You feel an intense desire to kill this monster...  "));
    } else if (lore_ptr->flags7 & RF7_GUARDIAN) {
        hook_c_roff(TERM_L_RED, _("このモンスターはダンジョンの主である。", "This monster is the master of a dungeon."));
    }

    hooked_roff("\n");
}
