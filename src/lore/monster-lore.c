/*!
 * @brief モンスターの思い出を表示する処理
 * @date 2020/06/09
 * @author Hourier
 */

#include "lore/monster-lore.h"
#include "locale/english.h"
#include "locale/japanese.h"
#include "lore/lore-util.h"
#include "monster-race/race-flags-ability1.h"
#include "monster-race/race-flags-ability2.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags4.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "mspell/monster-spell.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-type.h"
#include "term/term-color-types.h"
#include "util/util.h"
#include "view/display-monster-lore.h"

/*!
 * 英語の複数系記述用マクロ / Pluralizer.  Args(count, singular, plural)
 */
#define plural(c, s, p) (((c) == 1) ? (s) : (p))

/*!
 * @brief ダイス目を文字列に変換する
 * @param base_damage 固定値
 * @param dice_num ダイス数
 * @param dice_side ダイス面
 * @param dice_mult ダイス倍率
 * @param dice_div ダイス除数
 * @param msg 文字列を格納するポインタ
 * @return なし
 */
void dice_to_string(int base_damage, int dice_num, int dice_side, int dice_mult, int dice_div, char *msg)
{
    char base[80] = "", dice[80] = "", mult[80] = "";

    if (dice_num == 0) {
        sprintf(msg, "%d", base_damage);
        return;
    }

    if (base_damage != 0)
        sprintf(base, "%d+", base_damage);

    if (dice_num == 1)
        sprintf(dice, "d%d", dice_side);
    else
        sprintf(dice, "%dd%d", dice_num, dice_side);

    if (dice_mult != 1 || dice_div != 1) {
        if (dice_div == 1)
            sprintf(mult, "*%d", dice_mult);
        else
            sprintf(mult, "*(%d/%d)", dice_mult, dice_div);
    }

    sprintf(msg, "%s%s%s", base, dice, mult);
}

/*!
 * @brief モンスターのAC情報を得ることができるかを返す / Determine if the "armor" is known
 * @param r_idx モンスターの種族ID
 * @return 敵のACを知る条件が満たされているならTRUEを返す
 * @details
 * The higher the level, the fewer kills needed.
 */
static bool know_armour(MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    DEPTH level = r_ptr->level;
    MONSTER_NUMBER kills = r_ptr->r_tkills;

    bool known = (r_ptr->r_cast_spell == MAX_UCHAR) ? TRUE : FALSE;

    if (cheat_know || known)
        return TRUE;
    if (kills > 304 / (4 + level))
        return TRUE;
    if (!(r_ptr->flags1 & RF1_UNIQUE))
        return FALSE;
    if (kills > 304 / (38 + (5 * level) / 4))
        return TRUE;
    return FALSE;
}

/*!
 * @brief モンスターの打撃威力を知ることができるかどうかを返す
 * Determine if the "damage" of the given attack is known
 * @param r_idx モンスターの種族ID
 * @param i 確認したい攻撃手番
 * @return 敵のダメージダイスを知る条件が満たされているならTRUEを返す
 * @details
 * <pre>
 * the higher the level of the monster, the fewer the attacks you need,
 * the more damage an attack does, the more attacks you need
 * </pre>
 */
static bool know_damage(MONRACE_IDX r_idx, int i)
{
    monster_race *r_ptr = &r_info[r_idx];
    DEPTH level = r_ptr->level;
    s32b a = r_ptr->r_blows[i];

    s32b d1 = r_ptr->blow[i].d_dice;
    s32b d2 = r_ptr->blow[i].d_side;
    s32b d = d1 * d2;

    if (d >= ((4 + level) * MAX_UCHAR) / 80)
        d = ((4 + level) * MAX_UCHAR - 1) / 80;
    if ((4 + level) * a > 80 * d)
        return TRUE;
    if (!(r_ptr->flags1 & RF1_UNIQUE))
        return FALSE;
    if ((4 + level) * (2 * a) > 80 * d)
        return TRUE;

    return FALSE;
}

/*!
 * @brief 文字列にモンスターの攻撃力を加える
 * @param r_idx モンスターの種族ID
 * @param SPELL_NUM 呪文番号
 * @param msg 表示する文字列
 * @param tmp 返すメッセージを格納する配列
 * @return なし
 */
static void set_damage(player_type *player_ptr, MONRACE_IDX r_idx, monster_spell_type ms_type, char *msg, char *tmp)
{
    int base_damage = monspell_race_damage(player_ptr, ms_type, r_idx, BASE_DAM);
    int dice_num = monspell_race_damage(player_ptr, ms_type, r_idx, DICE_NUM);
    int dice_side = monspell_race_damage(player_ptr, ms_type, r_idx, DICE_SIDE);
    int dice_mult = monspell_race_damage(player_ptr, ms_type, r_idx, DICE_MULT);
    int dice_div = monspell_race_damage(player_ptr, ms_type, r_idx, DICE_DIV);
    char dmg_str[80], dice_str[80];
    dice_to_string(base_damage, dice_num, dice_side, dice_mult, dice_div, dmg_str);
    sprintf(dice_str, "(%s)", dmg_str);

    if (know_armour(r_idx))
        sprintf(tmp, msg, dice_str);
    else
        sprintf(tmp, msg, "");
}

static void set_drop_flags(lore_type *lore_ptr)
{
    if (!lore_ptr->know_everything)
        return;

    lore_ptr->drop_gold = lore_ptr->drop_item = (((lore_ptr->r_ptr->flags1 & RF1_DROP_4D2) ? 8 : 0) + ((lore_ptr->r_ptr->flags1 & RF1_DROP_3D2) ? 6 : 0)
        + ((lore_ptr->r_ptr->flags1 & RF1_DROP_2D2) ? 4 : 0) + ((lore_ptr->r_ptr->flags1 & RF1_DROP_1D2) ? 2 : 0)
        + ((lore_ptr->r_ptr->flags1 & RF1_DROP_90) ? 1 : 0) + ((lore_ptr->r_ptr->flags1 & RF1_DROP_60) ? 1 : 0));

    if (lore_ptr->r_ptr->flags1 & RF1_ONLY_GOLD)
        lore_ptr->drop_item = 0;

    if (lore_ptr->r_ptr->flags1 & RF1_ONLY_ITEM)
        lore_ptr->drop_gold = 0;

    lore_ptr->flags1 = lore_ptr->r_ptr->flags1;
    lore_ptr->flags2 = lore_ptr->r_ptr->flags2;
    lore_ptr->flags3 = lore_ptr->r_ptr->flags3;
    lore_ptr->flags4 = lore_ptr->r_ptr->flags4;
    lore_ptr->a_ability_flags1 = lore_ptr->r_ptr->a_ability_flags1;
    lore_ptr->a_ability_flags2 = lore_ptr->r_ptr->a_ability_flags2;
    lore_ptr->flagsr = lore_ptr->r_ptr->flagsr;
}

static void set_msex_flags(lore_type *lore_ptr)
{
    lore_ptr->msex = MSEX_NONE;
    if (lore_ptr->r_ptr->flags1 & RF1_FEMALE) {
        lore_ptr->msex = MSEX_FEMALE;
        return;
    }

    if (lore_ptr->r_ptr->flags1 & RF1_MALE)
        lore_ptr->msex = MSEX_MALE;
}

static void set_flags1(lore_type *lore_ptr)
{
    if (lore_ptr->r_ptr->flags1 & RF1_UNIQUE)
        lore_ptr->flags1 |= (RF1_UNIQUE);

    if (lore_ptr->r_ptr->flags1 & RF1_QUESTOR)
        lore_ptr->flags1 |= (RF1_QUESTOR);

    if (lore_ptr->r_ptr->flags1 & RF1_MALE)
        lore_ptr->flags1 |= (RF1_MALE);

    if (lore_ptr->r_ptr->flags1 & RF1_FEMALE)
        lore_ptr->flags1 |= (RF1_FEMALE);

    if (lore_ptr->r_ptr->flags1 & RF1_FRIENDS)
        lore_ptr->flags1 |= (RF1_FRIENDS);

    if (lore_ptr->r_ptr->flags1 & RF1_ESCORT)
        lore_ptr->flags1 |= (RF1_ESCORT);

    if (lore_ptr->r_ptr->flags1 & RF1_ESCORTS)
        lore_ptr->flags1 |= (RF1_ESCORTS);
}

static void set_race_flags(lore_type *lore_ptr)
{
    if (!lore_ptr->r_ptr->r_tkills && !lore_ptr->know_everything)
        return;

    if (lore_ptr->r_ptr->flags3 & RF3_ORC)
        lore_ptr->flags3 |= (RF3_ORC);

    if (lore_ptr->r_ptr->flags3 & RF3_TROLL)
        lore_ptr->flags3 |= (RF3_TROLL);

    if (lore_ptr->r_ptr->flags3 & RF3_GIANT)
        lore_ptr->flags3 |= (RF3_GIANT);

    if (lore_ptr->r_ptr->flags3 & RF3_DRAGON)
        lore_ptr->flags3 |= (RF3_DRAGON);

    if (lore_ptr->r_ptr->flags3 & RF3_DEMON)
        lore_ptr->flags3 |= (RF3_DEMON);

    if (lore_ptr->r_ptr->flags3 & RF3_UNDEAD)
        lore_ptr->flags3 |= (RF3_UNDEAD);

    if (lore_ptr->r_ptr->flags3 & RF3_EVIL)
        lore_ptr->flags3 |= (RF3_EVIL);

    if (lore_ptr->r_ptr->flags3 & RF3_GOOD)
        lore_ptr->flags3 |= (RF3_GOOD);

    if (lore_ptr->r_ptr->flags3 & RF3_ANIMAL)
        lore_ptr->flags3 |= (RF3_ANIMAL);

    if (lore_ptr->r_ptr->flags3 & RF3_AMBERITE)
        lore_ptr->flags3 |= (RF3_AMBERITE);

    if (lore_ptr->r_ptr->flags2 & RF2_HUMAN)
        lore_ptr->flags2 |= (RF2_HUMAN);

    if (lore_ptr->r_ptr->flags2 & RF2_QUANTUM)
        lore_ptr->flags2 |= (RF2_QUANTUM);

    if (lore_ptr->r_ptr->flags1 & RF1_FORCE_DEPTH)
        lore_ptr->flags1 |= (RF1_FORCE_DEPTH);

    if (lore_ptr->r_ptr->flags1 & RF1_FORCE_MAXHP)
        lore_ptr->flags1 |= (RF1_FORCE_MAXHP);
}

/*!
 * @brief モンスターの思い出情報を表示するメインルーチン
 * Hack -- display monster information using "hooked_roff()"
 * @param r_idx モンスターの種族ID
 * @param mode 表示オプション
 * @return なし
 * @details
 * This function should only be called with the cursor placed at the
 * left edge of the screen, on a cleared line, in which the recall is
 * to take place.  One extra blank line is left after the recall.
 */
void process_monster_lore(player_type *player_ptr, MONRACE_IDX r_idx, BIT_FLAGS mode)
{
    lore_type tmp_lore;
    lore_type *lore_ptr = initialize_lore_type(&tmp_lore, r_idx, mode);
    for (int n = 0; n < A_MAX; n++) {
        if (lore_ptr->r_ptr->reinforce_id[n] > 0)
            lore_ptr->reinforce = TRUE;
    }

    if (cheat_know || (mode & 0x01))
        lore_ptr->know_everything = TRUE;

    set_drop_flags(lore_ptr);
    set_msex_flags(lore_ptr);
    set_flags1(lore_ptr);
    set_race_flags(lore_ptr);
    display_kill_numbers(lore_ptr);
    concptr tmp = r_text + lore_ptr->r_ptr->text;
    if (tmp[0]) {
        hooked_roff(tmp);
        hooked_roff("\n");
    }

    if (r_idx == MON_KAGE) {
        hooked_roff("\n");
        return;
    }

    if (!display_where_to_appear(lore_ptr))
        return;

    display_monster_move(lore_ptr);
    if (lore_ptr->flags1 & RF1_NEVER_MOVE) {
        if (lore_ptr->old) {
            hooked_roff(_("、しかし", ", but "));
        } else {
            hooked_roff(format(_("%^sは", "%^s "), wd_he[lore_ptr->msex]));
            lore_ptr->old = TRUE;
        }

        hooked_roff(_("侵入者を追跡しない", "does not deign to chase intruders"));
    }

    if (lore_ptr->old) {
        hooked_roff(_("。", ".  "));
        lore_ptr->old = FALSE;
    }

    if (lore_ptr->r_ptr->r_tkills || lore_ptr->know_everything) {
#ifdef JP
        hooked_roff("この");
#else
        if (lore_ptr->flags1 & RF1_UNIQUE) {
            hooked_roff("Killing this");
        } else {
            hooked_roff("A kill of this");
        }
#endif

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

        if ((lore_ptr->flags3 & (RF3_DRAGON | RF3_DEMON | RF3_GIANT | RF3_TROLL | RF3_ORC | RF3_ANGEL)) || (lore_ptr->flags2 & (RF2_QUANTUM | RF2_HUMAN))) {
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
        } else {
            hooked_roff(_("モンスター", " creature"));
        }

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

    if ((lore_ptr->flags2 & RF2_AURA_FIRE) && (lore_ptr->flags2 & RF2_AURA_ELEC) && (lore_ptr->flags3 & RF3_AURA_COLD)) {
        hook_c_roff(
            TERM_VIOLET, format(_("%^sは炎と氷とスパークに包まれている。", "%^s is surrounded by flames, ice and electricity.  "), wd_he[lore_ptr->msex]));
    } else if ((lore_ptr->flags2 & RF2_AURA_FIRE) && (lore_ptr->flags2 & RF2_AURA_ELEC)) {
        hook_c_roff(TERM_L_RED, format(_("%^sは炎とスパークに包まれている。", "%^s is surrounded by flames and electricity.  "), wd_he[lore_ptr->msex]));
    } else if ((lore_ptr->flags2 & RF2_AURA_FIRE) && (lore_ptr->flags3 & RF3_AURA_COLD)) {
        hook_c_roff(TERM_BLUE, format(_("%^sは炎と氷に包まれている。", "%^s is surrounded by flames and ice.  "), wd_he[lore_ptr->msex]));
    } else if ((lore_ptr->flags3 & RF3_AURA_COLD) && (lore_ptr->flags2 & RF2_AURA_ELEC)) {
        hook_c_roff(TERM_L_GREEN, format(_("%^sは氷とスパークに包まれている。", "%^s is surrounded by ice and electricity.  "), wd_he[lore_ptr->msex]));
    } else if (lore_ptr->flags2 & RF2_AURA_FIRE) {
        hook_c_roff(TERM_RED, format(_("%^sは炎に包まれている。", "%^s is surrounded by flames.  "), wd_he[lore_ptr->msex]));
    } else if (lore_ptr->flags3 & RF3_AURA_COLD) {
        hook_c_roff(TERM_BLUE, format(_("%^sは氷に包まれている。", "%^s is surrounded by ice.  "), wd_he[lore_ptr->msex]));
    } else if (lore_ptr->flags2 & RF2_AURA_ELEC) {
        hook_c_roff(TERM_L_BLUE, format(_("%^sはスパークに包まれている。", "%^s is surrounded by electricity.  "), wd_he[lore_ptr->msex]));
    }

    if (lore_ptr->flags2 & RF2_REFLECTING)
        hooked_roff(format(_("%^sは矢の呪文を跳ね返す。", "%^s reflects bolt spells.  "), wd_he[lore_ptr->msex]));

    if ((lore_ptr->flags1 & RF1_ESCORT) || (lore_ptr->flags1 & RF1_ESCORTS) || lore_ptr->reinforce) {
        hooked_roff(format(_("%^sは通常護衛を伴って現れる。", "%^s usually appears with escorts.  "), wd_he[lore_ptr->msex]));

        if (lore_ptr->reinforce) {
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
    }

    else if (lore_ptr->flags1 & RF1_FRIENDS) {
        hooked_roff(format(_("%^sは通常集団で現れる。", "%^s usually appears in groups.  "), wd_he[lore_ptr->msex]));
    }

    int vn = 0;
    byte color[96];
    concptr vp[96];
    char tmp_msg[96][96];
    if (lore_ptr->flags4 & RF4_SHRIEK) {
        vp[vn] = _("悲鳴で助けを求める", "shriek for help");
        color[vn++] = TERM_L_WHITE;
    }

    if (lore_ptr->flags4 & RF4_ROCKET) {
        set_damage(player_ptr, r_idx, (MS_ROCKET), _("ロケット%sを発射する", "shoot a rocket%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_UMBER;
    }

    if (lore_ptr->flags4 & RF4_SHOOT) {
        for (int m = 0; m < 4; m++) {
            if (lore_ptr->r_ptr->blow[m].method != RBM_SHOOT)
                continue;

            if (know_armour(r_idx))
                sprintf(
                    tmp_msg[vn], _("威力 %dd%d の射撃をする", "fire an arrow (Power:%dd%d)"), lore_ptr->r_ptr->blow[m].d_side, lore_ptr->r_ptr->blow[m].d_dice);
            else
                sprintf(tmp_msg[vn], _("射撃をする", "fire an arrow"));
            vp[vn] = tmp_msg[vn];
            color[vn++] = TERM_UMBER;
            break;
        }
    }

    if (lore_ptr->a_ability_flags2 & (RF6_SPECIAL)) {
        vp[vn] = _("特別な行動をする", "do something");
        color[vn++] = TERM_VIOLET;
    }

    if (vn > 0) {
        hooked_roff(format(_("%^sは", "%^s"), wd_he[lore_ptr->msex]));
        for (int n = 0; n < vn; n++) {
#ifdef JP
            if (n != vn - 1) {
                jverb(vp[n], lore_ptr->jverb_buf, JVERB_OR);
                hook_c_roff(color[n], lore_ptr->jverb_buf);
                hook_c_roff(color[n], "り");
                hooked_roff("、");
            } else
                hook_c_roff(color[n], vp[n]);
#else
            if (n == 0)
                hooked_roff(" may ");
            else if (n < vn - 1)
                hooked_roff(", ");
            else
                hooked_roff(" or ");

            hook_c_roff(color[n], vp[n]);
#endif
        }

        hooked_roff(_("ことがある。", ".  "));
    }

    vn = 0;
    if (lore_ptr->flags4 & (RF4_BR_ACID)) {
        set_damage(player_ptr, r_idx, (MS_BR_ACID), _("酸%s", "acid%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_GREEN;
    }

    if (lore_ptr->flags4 & (RF4_BR_ELEC)) {
        set_damage(player_ptr, r_idx, (MS_BR_ELEC), _("稲妻%s", "lightning%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_BLUE;
    }

    if (lore_ptr->flags4 & (RF4_BR_FIRE)) {
        set_damage(player_ptr, r_idx, (MS_BR_FIRE), _("火炎%s", "fire%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_RED;
    }

    if (lore_ptr->flags4 & (RF4_BR_COLD)) {
        set_damage(player_ptr, r_idx, (MS_BR_COLD), _("冷気%s", "frost%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_WHITE;
    }

    if (lore_ptr->flags4 & (RF4_BR_POIS)) {
        set_damage(player_ptr, r_idx, (MS_BR_POIS), _("毒%s", "poison%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_GREEN;
    }

    if (lore_ptr->flags4 & (RF4_BR_NETH)) {
        set_damage(player_ptr, r_idx, (MS_BR_NETHER), _("地獄%s", "nether%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_DARK;
    }

    if (lore_ptr->flags4 & (RF4_BR_LITE)) {
        set_damage(player_ptr, r_idx, (MS_BR_LITE), _("閃光%s", "light%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_YELLOW;
    }

    if (lore_ptr->flags4 & (RF4_BR_DARK)) {
        set_damage(player_ptr, r_idx, (MS_BR_DARK), _("暗黒%s", "darkness%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_DARK;
    }

    if (lore_ptr->flags4 & (RF4_BR_CONF)) {
        set_damage(player_ptr, r_idx, (MS_BR_CONF), _("混乱%s", "confusion%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_UMBER;
    }

    if (lore_ptr->flags4 & (RF4_BR_SOUN)) {
        set_damage(player_ptr, r_idx, (MS_BR_SOUND), _("轟音%s", "sound%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_ORANGE;
    }

    if (lore_ptr->flags4 & (RF4_BR_CHAO)) {
        set_damage(player_ptr, r_idx, (MS_BR_CHAOS), _("カオス%s", "chaos%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_VIOLET;
    }

    if (lore_ptr->flags4 & (RF4_BR_DISE)) {
        set_damage(player_ptr, r_idx, (MS_BR_DISEN), _("劣化%s", "disenchantment%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_VIOLET;
    }

    if (lore_ptr->flags4 & (RF4_BR_NEXU)) {
        set_damage(player_ptr, r_idx, (MS_BR_NEXUS), _("因果混乱%s", "nexus%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_VIOLET;
    }

    if (lore_ptr->flags4 & (RF4_BR_TIME)) {
        set_damage(player_ptr, r_idx, (MS_BR_TIME), _("時間逆転%s", "time%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_BLUE;
    }

    if (lore_ptr->flags4 & (RF4_BR_INER)) {
        set_damage(player_ptr, r_idx, (MS_BR_INERTIA), _("遅鈍%s", "inertia%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_SLATE;
    }

    if (lore_ptr->flags4 & (RF4_BR_GRAV)) {
        set_damage(player_ptr, r_idx, (MS_BR_GRAVITY), _("重力%s", "gravity%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_SLATE;
    }

    if (lore_ptr->flags4 & (RF4_BR_SHAR)) {
        set_damage(player_ptr, r_idx, (MS_BR_SHARDS), _("破片%s", "shards%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_UMBER;
    }

    if (lore_ptr->flags4 & (RF4_BR_PLAS)) {
        set_damage(player_ptr, r_idx, (MS_BR_PLASMA), _("プラズマ%s", "plasma%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_RED;
    }

    if (lore_ptr->flags4 & (RF4_BR_WALL)) {
        set_damage(player_ptr, r_idx, (MS_BR_FORCE), _("フォース%s", "force%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_UMBER;
    }

    if (lore_ptr->flags4 & (RF4_BR_MANA)) {
        set_damage(player_ptr, r_idx, (MS_BR_MANA), _("魔力%s", "mana%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_BLUE;
    }

    if (lore_ptr->flags4 & (RF4_BR_NUKE)) {
        set_damage(player_ptr, r_idx, (MS_BR_NUKE), _("放射性廃棄物%s", "toxic waste%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_GREEN;
    }

    if (lore_ptr->flags4 & (RF4_BR_DISI)) {
        set_damage(player_ptr, r_idx, (MS_BR_DISI), _("分解%s", "disintegration%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_SLATE;
    }

    bool breath = FALSE;
    if (vn > 0) {
        breath = TRUE;
        hooked_roff(format(_("%^sは", "%^s"), wd_he[lore_ptr->msex]));
        for (int n = 0; n < vn; n++) {
#ifdef JP
            if (n != 0)
                hooked_roff("や");
#else
            if (n == 0)
                hooked_roff(" may breathe ");
            else if (n < vn - 1)
                hooked_roff(", ");
            else
                hooked_roff(" or ");
#endif
            hook_c_roff(color[n], vp[n]);
        }

#ifdef JP
        hooked_roff("のブレスを吐くことがある");
#endif
    }

    vn = 0;
    if (lore_ptr->a_ability_flags1 & (RF5_BA_ACID)) {
        set_damage(player_ptr, r_idx, (MS_BALL_ACID), _("アシッド・ボール%s", "produce acid balls%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_GREEN;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BA_ELEC)) {
        set_damage(player_ptr, r_idx, (MS_BALL_ELEC), _("サンダー・ボール%s", "produce lightning balls%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_BLUE;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BA_FIRE)) {
        set_damage(player_ptr, r_idx, (MS_BALL_FIRE), _("ファイア・ボール%s", "produce fire balls%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_RED;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BA_COLD)) {
        set_damage(player_ptr, r_idx, (MS_BALL_COLD), _("アイス・ボール%s", "produce frost balls%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_WHITE;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BA_POIS)) {
        set_damage(player_ptr, r_idx, (MS_BALL_POIS), _("悪臭雲%s", "produce poison balls%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_GREEN;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BA_NETH)) {
        set_damage(player_ptr, r_idx, (MS_BALL_NETHER), _("地獄球%s", "produce nether balls%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_DARK;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BA_WATE)) {
        set_damage(player_ptr, r_idx, (MS_BALL_WATER), _("ウォーター・ボール%s", "produce water balls%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_BLUE;
    }

    if (lore_ptr->flags4 & (RF4_BA_NUKE)) {
        set_damage(player_ptr, r_idx, (MS_BALL_NUKE), _("放射能球%s", "produce balls of radiation%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_GREEN;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BA_MANA)) {
        set_damage(player_ptr, r_idx, (MS_BALL_MANA), _("魔力の嵐%s", "invoke mana storms%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_BLUE;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BA_DARK)) {
        set_damage(player_ptr, r_idx, (MS_BALL_DARK), _("暗黒の嵐%s", "invoke darkness storms%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_DARK;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BA_LITE)) {
        set_damage(player_ptr, r_idx, (MS_STARBURST), _("スターバースト%s", "invoke starburst%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_YELLOW;
    }

    if (lore_ptr->flags4 & (RF4_BA_CHAO)) {
        set_damage(player_ptr, r_idx, (MS_BALL_CHAOS), _("純ログルス%s", "invoke raw Logrus%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_VIOLET;
    }

    if (lore_ptr->a_ability_flags2 & (RF6_HAND_DOOM)) {
        vp[vn] = _("破滅の手(40%-60%)", "invoke the Hand of Doom(40%-60%)");
        color[vn++] = TERM_VIOLET;
    }

    if (lore_ptr->a_ability_flags2 & (RF6_PSY_SPEAR)) {
        set_damage(player_ptr, r_idx, (MS_PSY_SPEAR), _("光の剣%s", "psycho-spear%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_YELLOW;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_DRAIN_MANA)) {
        set_damage(player_ptr, r_idx, (MS_DRAIN_MANA), _("魔力吸収%s", "drain mana%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_SLATE;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_MIND_BLAST)) {
        set_damage(player_ptr, r_idx, (MS_MIND_BLAST), _("精神攻撃%s", "cause mind blasting%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_RED;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BRAIN_SMASH)) {
        set_damage(player_ptr, r_idx, (MS_BRAIN_SMASH), _("脳攻撃%s", "cause brain smashing%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_RED;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_CAUSE_1)) {
        set_damage(player_ptr, r_idx, (MS_CAUSE_1), _("軽傷＋呪い%s", "cause light wounds and cursing%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_WHITE;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_CAUSE_2)) {
        set_damage(player_ptr, r_idx, (MS_CAUSE_2), _("重傷＋呪い%s", "cause serious wounds and cursing%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_WHITE;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_CAUSE_3)) {
        set_damage(player_ptr, r_idx, (MS_CAUSE_3), _("致命傷＋呪い%s", "cause critical wounds and cursing%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_WHITE;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_CAUSE_4)) {
        set_damage(player_ptr, r_idx, (MS_CAUSE_4), _("秘孔を突く%s", "cause mortal wounds%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_WHITE;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BO_ACID)) {
        set_damage(player_ptr, r_idx, (MS_BOLT_ACID), _("アシッド・ボルト%s", "produce acid bolts%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_GREEN;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BO_ELEC)) {
        set_damage(player_ptr, r_idx, (MS_BOLT_ELEC), _("サンダー・ボルト%s", "produce lightning bolts%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_BLUE;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BO_FIRE)) {
        set_damage(player_ptr, r_idx, (MS_BOLT_FIRE), _("ファイア・ボルト%s", "produce fire bolts%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_RED;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BO_COLD)) {
        set_damage(player_ptr, r_idx, (MS_BOLT_COLD), _("アイス・ボルト%s", "produce frost bolts%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_WHITE;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BO_NETH)) {
        set_damage(player_ptr, r_idx, (MS_BOLT_NETHER), _("地獄の矢%s", "produce nether bolts%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_DARK;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BO_WATE)) {
        set_damage(player_ptr, r_idx, (MS_BOLT_WATER), _("ウォーター・ボルト%s", "produce water bolts%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_BLUE;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BO_MANA)) {
        set_damage(player_ptr, r_idx, (MS_BOLT_MANA), _("魔力の矢%s", "produce mana bolts%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_BLUE;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BO_PLAS)) {
        set_damage(player_ptr, r_idx, (MS_BOLT_PLASMA), _("プラズマ・ボルト%s", "produce plasma bolts%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_L_RED;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BO_ICEE)) {
        set_damage(player_ptr, r_idx, (MS_BOLT_ICE), _("極寒の矢%s", "produce ice bolts%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_WHITE;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_MISSILE)) {
        set_damage(player_ptr, r_idx, (MS_MAGIC_MISSILE), _("マジックミサイル%s", "produce magic missiles%s"), tmp_msg[vn]);
        vp[vn] = tmp_msg[vn];
        color[vn++] = TERM_SLATE;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_SCARE)) {
        vp[vn] = _("恐怖", "terrify");
        color[vn++] = TERM_SLATE;
    }
    if (lore_ptr->a_ability_flags1 & (RF5_BLIND)) {
        vp[vn] = _("目くらまし", "blind");
        color[vn++] = TERM_L_DARK;
    }
    if (lore_ptr->a_ability_flags1 & (RF5_CONF)) {
        vp[vn] = _("混乱", "confuse");
        color[vn++] = TERM_L_UMBER;
    }
    if (lore_ptr->a_ability_flags1 & (RF5_SLOW)) {
        vp[vn] = _("減速", "slow");
        color[vn++] = TERM_UMBER;
    }
    if (lore_ptr->a_ability_flags1 & (RF5_HOLD)) {
        vp[vn] = _("麻痺", "paralyze");
        color[vn++] = TERM_RED;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_HASTE)) {
        vp[vn] = _("加速", "haste-self");
        color[vn++] = TERM_L_GREEN;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_HEAL)) {
        vp[vn] = _("治癒", "heal-self");
        color[vn++] = TERM_WHITE;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_INVULNER)) {
        vp[vn] = _("無敵化", "make invulnerable");
        color[vn++] = TERM_WHITE;
    }
    if (lore_ptr->flags4 & RF4_DISPEL) {
        vp[vn] = _("魔力消去", "dispel-magic");
        color[vn++] = TERM_L_WHITE;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_BLINK)) {
        vp[vn] = _("ショートテレポート", "blink-self");
        color[vn++] = TERM_UMBER;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_TPORT)) {
        vp[vn] = _("テレポート", "teleport-self");
        color[vn++] = TERM_ORANGE;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_WORLD)) {
        vp[vn] = _("時を止める", "stop the time");
        color[vn++] = TERM_L_BLUE;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_TELE_TO)) {
        vp[vn] = _("テレポートバック", "teleport to");
        color[vn++] = TERM_L_UMBER;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_TELE_AWAY)) {
        vp[vn] = _("テレポートアウェイ", "teleport away");
        color[vn++] = TERM_UMBER;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_TELE_LEVEL)) {
        vp[vn] = _("テレポート・レベル", "teleport level");
        color[vn++] = TERM_ORANGE;
    }

    if (lore_ptr->a_ability_flags2 & (RF6_DARKNESS)) {
        if ((player_ptr->pclass != CLASS_NINJA) || (lore_ptr->r_ptr->flags3 & (RF3_UNDEAD | RF3_HURT_LITE)) || (lore_ptr->r_ptr->flags7 & RF7_DARK_MASK)) {
            vp[vn] = _("暗闇", "create darkness");
            color[vn++] = TERM_L_DARK;
        } else {
            vp[vn] = _("閃光", "create light");
            color[vn++] = TERM_YELLOW;
        }
    }

    if (lore_ptr->a_ability_flags2 & (RF6_TRAPS)) {
        vp[vn] = _("トラップ", "create traps");
        color[vn++] = TERM_BLUE;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_FORGET)) {
        vp[vn] = _("記憶消去", "cause amnesia");
        color[vn++] = TERM_BLUE;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_RAISE_DEAD)) {
        vp[vn] = _("死者復活", "raise dead");
        color[vn++] = TERM_RED;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_S_MONSTER)) {
        vp[vn] = _("モンスター一体召喚", "summon a monster");
        color[vn++] = TERM_SLATE;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_S_MONSTERS)) {
        vp[vn] = _("モンスター複数召喚", "summon monsters");
        color[vn++] = TERM_L_WHITE;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_S_KIN)) {
        vp[vn] = _("救援召喚", "summon aid");
        color[vn++] = TERM_ORANGE;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_S_ANT)) {
        vp[vn] = _("アリ召喚", "summon ants");
        color[vn++] = TERM_RED;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_S_SPIDER)) {
        vp[vn] = _("クモ召喚", "summon spiders");
        color[vn++] = TERM_L_DARK;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_S_HOUND)) {
        vp[vn] = _("ハウンド召喚", "summon hounds");
        color[vn++] = TERM_L_UMBER;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_S_HYDRA)) {
        vp[vn] = _("ヒドラ召喚", "summon hydras");
        color[vn++] = TERM_L_GREEN;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_S_ANGEL)) {
        vp[vn] = _("天使一体召喚", "summon an angel");
        color[vn++] = TERM_YELLOW;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_S_DEMON)) {
        vp[vn] = _("デーモン一体召喚", "summon a demon");
        color[vn++] = TERM_L_RED;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_S_UNDEAD)) {
        vp[vn] = _("アンデッド一体召喚", "summon an undead");
        color[vn++] = TERM_L_DARK;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_S_DRAGON)) {
        vp[vn] = _("ドラゴン一体召喚", "summon a dragon");
        color[vn++] = TERM_ORANGE;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_S_HI_UNDEAD)) {
        vp[vn] = _("強力なアンデッド召喚", "summon Greater Undead");
        color[vn++] = TERM_L_DARK;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_S_HI_DRAGON)) {
        vp[vn] = _("古代ドラゴン召喚", "summon Ancient Dragons");
        color[vn++] = TERM_ORANGE;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_S_CYBER)) {
        vp[vn] = _("サイバーデーモン召喚", "summon Cyberdemons");
        color[vn++] = TERM_UMBER;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_S_AMBERITES)) {
        vp[vn] = _("アンバーの王族召喚", "summon Lords of Amber");
        color[vn++] = TERM_VIOLET;
    }
    if (lore_ptr->a_ability_flags2 & (RF6_S_UNIQUE)) {
        vp[vn] = _("ユニーク・モンスター召喚", "summon Unique Monsters");
        color[vn++] = TERM_VIOLET;
    }

    bool magic = FALSE;
    if (vn) {
        magic = TRUE;
        if (breath) {
            hooked_roff(_("、なおかつ", ", and is also"));
        } else {
            hooked_roff(format(_("%^sは", "%^s is"), wd_he[lore_ptr->msex]));
        }

#ifdef JP
        if (lore_ptr->flags2 & (RF2_SMART))
            hook_c_roff(TERM_YELLOW, "的確に");
        hooked_roff("魔法を使うことができ、");
#else
        hooked_roff(" magical, casting spells");
        if (lore_ptr->flags2 & RF2_SMART)
            hook_c_roff(TERM_YELLOW, " intelligently");
#endif

        for (int n = 0; n < vn; n++) {
#ifdef JP
            if (n != 0)
                hooked_roff("、");
#else
            if (n == 0)
                hooked_roff(" which ");
            else if (n < vn - 1)
                hooked_roff(", ");
            else
                hooked_roff(" or ");
#endif
            hook_c_roff(color[n], vp[n]);
        }

#ifdef JP
        hooked_roff("の呪文を唱えることがある");
#endif
    }

    if (breath || magic) {
        int m = lore_ptr->r_ptr->r_cast_spell;
        int n = lore_ptr->r_ptr->freq_spell;
        if (m > 100 || lore_ptr->know_everything) {
            hooked_roff(format(_("(確率:1/%d)", "; 1 time in %d"), 100 / n));
        } else if (m) {
            n = ((n + 9) / 10) * 10;
            hooked_roff(format(_("(確率:約1/%d)", "; about 1 time in %d"), 100 / n));
        }

        hooked_roff(_("。", ".  "));
    }

    if (lore_ptr->know_everything || know_armour(r_idx)) {
        hooked_roff(format(_("%^sは AC%d の防御力と", "%^s has an armor rating of %d"), wd_he[lore_ptr->msex], lore_ptr->r_ptr->ac));

        if ((lore_ptr->flags1 & RF1_FORCE_MAXHP) || (lore_ptr->r_ptr->hside == 1)) {
            u32b hp = lore_ptr->r_ptr->hdice * (lore_ptr->nightmare ? 2 : 1) * lore_ptr->r_ptr->hside;
            hooked_roff(format(_(" %d の体力がある。", " and a life rating of %d.  "), (s16b)MIN(30000, hp)));
        } else {
            hooked_roff(format(
                _(" %dd%d の体力がある。", " and a life rating of %dd%d.  "), lore_ptr->r_ptr->hdice * (lore_ptr->nightmare ? 2 : 1), lore_ptr->r_ptr->hside));
        }
    }

    vn = 0;
    if (lore_ptr->flags7 & (RF7_HAS_LITE_1 | RF7_HAS_LITE_2)) {
        vp[vn] = _("ダンジョンを照らす", "illuminate the dungeon");
        color[vn++] = TERM_WHITE;
    }
    if (lore_ptr->flags7 & (RF7_HAS_DARK_1 | RF7_HAS_DARK_2)) {
        vp[vn] = _("ダンジョンを暗くする", "darken the dungeon");
        color[vn++] = TERM_L_DARK;
    }
    if (lore_ptr->flags2 & RF2_OPEN_DOOR) {
        vp[vn] = _("ドアを開ける", "open doors");
        color[vn++] = TERM_WHITE;
    }
    if (lore_ptr->flags2 & RF2_BASH_DOOR) {
        vp[vn] = _("ドアを打ち破る", "bash down doors");
        color[vn++] = TERM_WHITE;
    }
    if (lore_ptr->flags7 & RF7_CAN_FLY) {
        vp[vn] = _("空を飛ぶ", "fly");
        color[vn++] = TERM_WHITE;
    }
    if (lore_ptr->flags7 & RF7_CAN_SWIM) {
        vp[vn] = _("水を渡る", "swim");
        color[vn++] = TERM_WHITE;
    }
    if (lore_ptr->flags2 & RF2_PASS_WALL) {
        vp[vn] = _("壁をすり抜ける", "pass through walls");
        color[vn++] = TERM_WHITE;
    }
    if (lore_ptr->flags2 & RF2_KILL_WALL) {
        vp[vn] = _("壁を掘り進む", "bore through walls");
        color[vn++] = TERM_WHITE;
    }
    if (lore_ptr->flags2 & RF2_MOVE_BODY) {
        vp[vn] = _("弱いモンスターを押しのける", "push past weaker monsters");
        color[vn++] = TERM_WHITE;
    }
    if (lore_ptr->flags2 & RF2_KILL_BODY) {
        vp[vn] = _("弱いモンスターを倒す", "destroy weaker monsters");
        color[vn++] = TERM_WHITE;
    }
    if (lore_ptr->flags2 & RF2_TAKE_ITEM) {
        vp[vn] = _("アイテムを拾う", "pick up objects");
        color[vn++] = TERM_WHITE;
    }
    if (lore_ptr->flags2 & RF2_KILL_ITEM) {
        vp[vn] = _("アイテムを壊す", "destroy objects");
        color[vn++] = TERM_WHITE;
    }

    if (vn > 0) {
        hooked_roff(format(_("%^sは", "%^s"), wd_he[lore_ptr->msex]));
        for (int n = 0; n < vn; n++) {
#ifdef JP
            if (n != vn - 1) {
                jverb(vp[n], lore_ptr->jverb_buf, JVERB_AND);
                hook_c_roff(color[n], lore_ptr->jverb_buf);
                hooked_roff("、");
            } else {
                hook_c_roff(color[n], vp[n]);
            }
#else
            if (n == 0)
                hooked_roff(" can ");
            else if (n < vn - 1)
                hooked_roff(", ");
            else
                hooked_roff(" and ");

            hook_c_roff(color[n], vp[n]);
#endif
        }

        hooked_roff(_("ことができる。", ".  "));
    }

    if (lore_ptr->flags7 & RF7_AQUATIC) {
        hooked_roff(format(_("%^sは水中に棲んでいる。", "%^s lives in water.  "), wd_he[lore_ptr->msex]));
    }

    if (lore_ptr->flags7 & (RF7_SELF_LITE_1 | RF7_SELF_LITE_2)) {
        hooked_roff(format(_("%^sは光っている。", "%^s is shining.  "), wd_he[lore_ptr->msex]));
    }

    if (lore_ptr->flags7 & (RF7_SELF_DARK_1 | RF7_SELF_DARK_2)) {
        hook_c_roff(TERM_L_DARK, format(_("%^sは暗黒に包まれている。", "%^s is surrounded by darkness.  "), wd_he[lore_ptr->msex]));
    }

    if (lore_ptr->flags2 & RF2_INVISIBLE) {
        hooked_roff(format(_("%^sは透明で目に見えない。", "%^s is invisible.  "), wd_he[lore_ptr->msex]));
    }

    if (lore_ptr->flags2 & RF2_COLD_BLOOD) {
        hooked_roff(format(_("%^sは冷血動物である。", "%^s is cold blooded.  "), wd_he[lore_ptr->msex]));
    }

    if (lore_ptr->flags2 & RF2_EMPTY_MIND) {
        hooked_roff(format(_("%^sはテレパシーでは感知できない。", "%^s is not detected by telepathy.  "), wd_he[lore_ptr->msex]));
    } else if (lore_ptr->flags2 & RF2_WEIRD_MIND) {
        hooked_roff(format(_("%^sはまれにテレパシーで感知できる。", "%^s is rarely detected by telepathy.  "), wd_he[lore_ptr->msex]));
    }

    if (lore_ptr->flags2 & RF2_MULTIPLY) {
        hook_c_roff(TERM_L_UMBER, format(_("%^sは爆発的に増殖する。", "%^s breeds explosively.  "), wd_he[lore_ptr->msex]));
    }

    if (lore_ptr->flags2 & RF2_REGENERATE) {
        hook_c_roff(TERM_L_WHITE, format(_("%^sは素早く体力を回復する。", "%^s regenerates quickly.  "), wd_he[lore_ptr->msex]));
    }

    if (lore_ptr->flags7 & RF7_RIDING) {
        hook_c_roff(TERM_SLATE, format(_("%^sに乗ることができる。", "%^s is suitable for riding.  "), wd_he[lore_ptr->msex]));
    }

    vn = 0;
    if (lore_ptr->flags3 & RF3_HURT_ROCK) {
        vp[vn] = _("岩を除去するもの", "rock remover");
        color[vn++] = TERM_UMBER;
    }
    if (lore_ptr->flags3 & RF3_HURT_LITE) {
        vp[vn] = _("明るい光", "bright light");
        color[vn++] = TERM_YELLOW;
    }
    if (lore_ptr->flags3 & RF3_HURT_FIRE) {
        vp[vn] = _("炎", "fire");
        color[vn++] = TERM_RED;
    }
    if (lore_ptr->flags3 & RF3_HURT_COLD) {
        vp[vn] = _("冷気", "cold");
        color[vn++] = TERM_L_WHITE;
    }

    if (vn > 0) {
        hooked_roff(format(_("%^sには", "%^s"), wd_he[lore_ptr->msex]));

        for (int n = 0; n < vn; n++) {
#ifdef JP
            if (n != 0)
                hooked_roff("や");
#else
            if (n == 0)
                hooked_roff(" is hurt by ");
            else if (n < vn - 1)
                hooked_roff(", ");
            else
                hooked_roff(" and ");
#endif
            hook_c_roff(color[n], vp[n]);
        }

        hooked_roff(_("でダメージを与えられる。", ".  "));
    }

    vn = 0;
    if (lore_ptr->flagsr & RFR_IM_ACID) {
        vp[vn] = _("酸", "acid");
        color[vn++] = TERM_GREEN;
    }
    if (lore_ptr->flagsr & RFR_IM_ELEC) {
        vp[vn] = _("稲妻", "lightning");
        color[vn++] = TERM_BLUE;
    }
    if (lore_ptr->flagsr & RFR_IM_FIRE) {
        vp[vn] = _("炎", "fire");
        color[vn++] = TERM_RED;
    }
    if (lore_ptr->flagsr & RFR_IM_COLD) {
        vp[vn] = _("冷気", "cold");
        color[vn++] = TERM_L_WHITE;
    }
    if (lore_ptr->flagsr & RFR_IM_POIS) {
        vp[vn] = _("毒", "poison");
        color[vn++] = TERM_L_GREEN;
    }

    if (lore_ptr->flagsr & RFR_RES_LITE) {
        vp[vn] = _("閃光", "light");
        color[vn++] = TERM_YELLOW;
    }
    if (lore_ptr->flagsr & RFR_RES_DARK) {
        vp[vn] = _("暗黒", "dark");
        color[vn++] = TERM_L_DARK;
    }
    if (lore_ptr->flagsr & RFR_RES_NETH) {
        vp[vn] = _("地獄", "nether");
        color[vn++] = TERM_L_DARK;
    }
    if (lore_ptr->flagsr & RFR_RES_WATE) {
        vp[vn] = _("水", "water");
        color[vn++] = TERM_BLUE;
    }
    if (lore_ptr->flagsr & RFR_RES_PLAS) {
        vp[vn] = _("プラズマ", "plasma");
        color[vn++] = TERM_L_RED;
    }
    if (lore_ptr->flagsr & RFR_RES_SHAR) {
        vp[vn] = _("破片", "shards");
        color[vn++] = TERM_L_UMBER;
    }
    if (lore_ptr->flagsr & RFR_RES_SOUN) {
        vp[vn] = _("轟音", "sound");
        color[vn++] = TERM_ORANGE;
    }
    if (lore_ptr->flagsr & RFR_RES_CHAO) {
        vp[vn] = _("カオス", "chaos");
        color[vn++] = TERM_VIOLET;
    }
    if (lore_ptr->flagsr & RFR_RES_NEXU) {
        vp[vn] = _("因果混乱", "nexus");
        color[vn++] = TERM_VIOLET;
    }
    if (lore_ptr->flagsr & RFR_RES_DISE) {
        vp[vn] = _("劣化", "disenchantment");
        color[vn++] = TERM_VIOLET;
    }
    if (lore_ptr->flagsr & RFR_RES_WALL) {
        vp[vn] = _("フォース", "force");
        color[vn++] = TERM_UMBER;
    }
    if (lore_ptr->flagsr & RFR_RES_INER) {
        vp[vn] = _("遅鈍", "inertia");
        color[vn++] = TERM_SLATE;
    }
    if (lore_ptr->flagsr & RFR_RES_TIME) {
        vp[vn] = _("時間逆転", "time");
        color[vn++] = TERM_L_BLUE;
    }
    if (lore_ptr->flagsr & RFR_RES_GRAV) {
        vp[vn] = _("重力", "gravity");
        color[vn++] = TERM_SLATE;
    }
    if (lore_ptr->flagsr & RFR_RES_ALL) {
        vp[vn] = _("あらゆる攻撃", "all");
        color[vn++] = TERM_YELLOW;
    }
    if ((lore_ptr->flagsr & RFR_RES_TELE) && !(lore_ptr->r_ptr->flags1 & RF1_UNIQUE)) {
        vp[vn] = _("テレポート", "teleportation");
        color[vn++] = TERM_ORANGE;
    }

    if (vn > 0) {
        hooked_roff(format(_("%^sは", "%^s"), wd_he[lore_ptr->msex]));
        for (int n = 0; n < vn; n++) {
#ifdef JP
            if (n != 0)
                hooked_roff("と");
#else
            if (n == 0)
                hooked_roff(" resists ");
            else if (n < vn - 1)
                hooked_roff(", ");
            else
                hooked_roff(" and ");
#endif
            hook_c_roff(color[n], vp[n]);
        }

        hooked_roff(_("の耐性を持っている。", ".  "));
    }

    if ((lore_ptr->r_ptr->r_xtra1 & MR1_SINKA) || lore_ptr->know_everything) {
        if (lore_ptr->r_ptr->next_r_idx) {
            hooked_roff(format(_("%^sは経験を積むと、", "%^s will evolve into "), wd_he[lore_ptr->msex]));
            hook_c_roff(TERM_YELLOW, format("%s", r_name + r_info[lore_ptr->r_ptr->next_r_idx].name));

            hooked_roff(_(format("に進化する。"), format(" when %s gets enough experience.  ", wd_he[lore_ptr->msex])));
        } else if (!(lore_ptr->r_ptr->flags1 & RF1_UNIQUE)) {
            hooked_roff(format(_("%sは進化しない。", "%s won't evolve.  "), wd_he[lore_ptr->msex]));
        }
    }

    vn = 0;
    if (lore_ptr->flags3 & RF3_NO_STUN) {
        vp[vn] = _("朦朧としない", "stunned");
        color[vn++] = TERM_ORANGE;
    }
    if (lore_ptr->flags3 & RF3_NO_FEAR) {
        vp[vn] = _("恐怖を感じない", "frightened");
        color[vn++] = TERM_SLATE;
    }
    if (lore_ptr->flags3 & RF3_NO_CONF) {
        vp[vn] = _("混乱しない", "confused");
        color[vn++] = TERM_L_UMBER;
    }
    if (lore_ptr->flags3 & RF3_NO_SLEEP) {
        vp[vn] = _("眠らされない", "slept");
        color[vn++] = TERM_BLUE;
    }
    if ((lore_ptr->flagsr & RFR_RES_TELE) && (lore_ptr->r_ptr->flags1 & RF1_UNIQUE)) {
        vp[vn] = _("テレポートされない", "teleported");
        color[vn++] = TERM_ORANGE;
    }

    if (vn > 0) {
        hooked_roff(format(_("%^sは", "%^s"), wd_he[lore_ptr->msex]));
        for (int n = 0; n < vn; n++) {
#ifdef JP
            if (n != 0)
                hooked_roff("し、");
#else
            if (n == 0)
                hooked_roff(" cannot be ");
            else if (n < vn - 1)
                hooked_roff(", ");
            else
                hooked_roff(" or ");
#endif
            hook_c_roff(color[n], vp[n]);
        }

        hooked_roff(_("。", ".  "));
    }

    if ((((int)lore_ptr->r_ptr->r_wake * (int)lore_ptr->r_ptr->r_wake) > lore_ptr->r_ptr->sleep) || (lore_ptr->r_ptr->r_ignore == MAX_UCHAR)
        || (lore_ptr->r_ptr->sleep == 0 && lore_ptr->r_ptr->r_tkills >= 10) || lore_ptr->know_everything) {
        concptr act;
        if (lore_ptr->r_ptr->sleep > 200) {
            act = _("を無視しがちであるが", "prefers to ignore");
        } else if (lore_ptr->r_ptr->sleep > 95) {
            act = _("に対してほとんど注意を払わないが", "pays very little attention to");
        } else if (lore_ptr->r_ptr->sleep > 75) {
            act = _("に対してあまり注意を払わないが", "pays little attention to");
        } else if (lore_ptr->r_ptr->sleep > 45) {
            act = _("を見過ごしがちであるが", "tends to overlook");
        } else if (lore_ptr->r_ptr->sleep > 25) {
            act = _("をほんの少しは見ており", "takes quite a while to see");
        } else if (lore_ptr->r_ptr->sleep > 10) {
            act = _("をしばらくは見ており", "takes a while to see");
        } else if (lore_ptr->r_ptr->sleep > 5) {
            act = _("を幾分注意深く見ており", "is fairly observant of");
        } else if (lore_ptr->r_ptr->sleep > 3) {
            act = _("を注意深く見ており", "is observant of");
        } else if (lore_ptr->r_ptr->sleep > 1) {
            act = _("をかなり注意深く見ており", "is very observant of");
        } else if (lore_ptr->r_ptr->sleep > 0) {
            act = _("を警戒しており", "is vigilant for");
        } else {
            act = _("をかなり警戒しており", "is ever vigilant for");
        }

        hooked_roff(_(format("%^sは侵入者%s、 %d フィート先から侵入者に気付くことがある。", wd_he[lore_ptr->msex], act, 10 * lore_ptr->r_ptr->aaf),
            format("%^s %s intruders, which %s may notice from %d feet.  ", wd_he[lore_ptr->msex], act, wd_he[lore_ptr->msex], 10 * lore_ptr->r_ptr->aaf)));
    }

    if (lore_ptr->drop_gold || lore_ptr->drop_item) {
        hooked_roff(format(_("%^sは", "%^s may carry"), wd_he[lore_ptr->msex]));
#ifdef JP
#else
        lore_ptr->sin = FALSE;
#endif

        int n = MAX(lore_ptr->drop_gold, lore_ptr->drop_item);
        if (n == 1) {
            hooked_roff(_("一つの", " a"));
#ifdef JP
#else
            lore_ptr->sin = TRUE;
#endif
        } else if (n == 2) {
            hooked_roff(_("一つか二つの", " one or two"));
        } else {
            hooked_roff(format(_(" %d 個までの", " up to %d"), n));
        }

        concptr p;
        if (lore_ptr->flags1 & RF1_DROP_GREAT) {
            p = _("特別な", " exceptional");
        } else if (lore_ptr->flags1 & RF1_DROP_GOOD) {
            p = _("上質な", " good");
#ifdef JP
#else
            lore_ptr->sin = FALSE;
#endif
        } else {
            p = NULL;
        }

        if (lore_ptr->drop_item) {
#ifdef JP
#else
            if (lore_ptr->sin)
                hooked_roff("n");

            lore_ptr->sin = FALSE;
#endif
            if (p)
                hooked_roff(p);
            hooked_roff(_("アイテム", " object"));
#ifdef JP
#else
            if (n != 1)
                hooked_roff("s");
#endif
            p = _("や", " or");
        }

        if (lore_ptr->drop_gold) {
#ifdef JP
#else
            if (!p)
                lore_ptr->sin = FALSE;

            if (lore_ptr->sin)
                hooked_roff("n");

            lore_ptr->sin = FALSE;
#endif
            if (p)
                hooked_roff(p);
            hooked_roff(_("財宝", " treasure"));
#ifdef JP
#else
            if (n != 1)
                hooked_roff("s");
#endif
        }

        hooked_roff(_("を持っていることがある。", ".  "));
    }

    const int max_attack_numbers = 4;
    int count = 0;
    for (int m = 0; m < max_attack_numbers; m++) {
        if (!lore_ptr->r_ptr->blow[m].method)
            continue;
        if (lore_ptr->r_ptr->blow[m].method == RBM_SHOOT)
            continue;

        if (lore_ptr->r_ptr->r_blows[m] || lore_ptr->know_everything)
            count++;
    }

    int attack_numbers = 0;
    for (int m = 0; m < max_attack_numbers; m++) {
        if (!lore_ptr->r_ptr->blow[m].method)
            continue;
        if (lore_ptr->r_ptr->blow[m].method == RBM_SHOOT)
            continue;
        if (!lore_ptr->r_ptr->r_blows[m] && !lore_ptr->know_everything)
            continue;

        rbm_type method = lore_ptr->r_ptr->blow[m].method;
        int effect = lore_ptr->r_ptr->blow[m].effect;
        int d1 = lore_ptr->r_ptr->blow[m].d_dice;
        int d2 = lore_ptr->r_ptr->blow[m].d_side;

        concptr p = NULL;
        switch (method) {
        case RBM_HIT:
            p = _("殴る", "hit");
            break;
        case RBM_TOUCH:
            p = _("触る", "touch");
            break;
        case RBM_PUNCH:
            p = _("パンチする", "punch");
            break;
        case RBM_KICK:
            p = _("蹴る", "kick");
            break;
        case RBM_CLAW:
            p = _("ひっかく", "claw");
            break;
        case RBM_BITE:
            p = _("噛む", "bite");
            break;
        case RBM_STING:
            p = _("刺す", "sting");
            break;
        case RBM_SLASH:
            p = _("斬る", "slash");
            break;
        case RBM_BUTT:
            p = _("角で突く", "butt");
            break;
        case RBM_CRUSH:
            p = _("体当たりする", "crush");
            break;
        case RBM_ENGULF:
            p = _("飲み込む", "engulf");
            break;
        case RBM_CHARGE:
            p = _("請求書をよこす", "charge");
            break;
        case RBM_CRAWL:
            p = _("体の上を這い回る", "crawl on you");
            break;
        case RBM_DROOL:
            p = _("よだれをたらす", "drool on you");
            break;
        case RBM_SPIT:
            p = _("つばを吐く", "spit");
            break;
        case RBM_EXPLODE:
            p = _("爆発する", "explode");
            break;
        case RBM_GAZE:
            p = _("にらむ", "gaze");
            break;
        case RBM_WAIL:
            p = _("泣き叫ぶ", "wail");
            break;
        case RBM_SPORE:
            p = _("胞子を飛ばす", "release spores");
            break;
        case RBM_XXX4:
            break;
        case RBM_BEG:
            p = _("金をせがむ", "beg");
            break;
        case RBM_INSULT:
            p = _("侮辱する", "insult");
            break;
        case RBM_MOAN:
            p = _("うめく", "moan");
            break;
        case RBM_SHOW:
            p = _("歌う", "sing");
            break;
        }

        concptr q = NULL;
        switch (effect) {
        case RBE_SUPERHURT:
            q = _("強力に攻撃する", "slaughter");
            break;
        case RBE_HURT:
            q = _("攻撃する", "attack");
            break;
        case RBE_POISON:
            q = _("毒をくらわす", "poison");
            break;
        case RBE_UN_BONUS:
            q = _("劣化させる", "disenchant");
            break;
        case RBE_UN_POWER:
            q = _("充填魔力を吸収する", "drain charges");
            break;
        case RBE_EAT_GOLD:
            q = _("金を盗む", "steal gold");
            break;
        case RBE_EAT_ITEM:
            q = _("アイテムを盗む", "steal items");
            break;
        case RBE_EAT_FOOD:
            q = _("あなたの食料を食べる", "eat your food");
            break;
        case RBE_EAT_LITE:
            q = _("明かりを吸収する", "absorb light");
            break;
        case RBE_ACID:
            q = _("酸を飛ばす", "shoot acid");
            break;
        case RBE_ELEC:
            q = _("感電させる", "electrocute");
            break;
        case RBE_FIRE:
            q = _("燃やす", "burn");
            break;
        case RBE_COLD:
            q = _("凍らせる", "freeze");
            break;
        case RBE_BLIND:
            q = _("盲目にする", "blind");
            break;
        case RBE_CONFUSE:
            q = _("混乱させる", "confuse");
            break;
        case RBE_TERRIFY:
            q = _("恐怖させる", "terrify");
            break;
        case RBE_PARALYZE:
            q = _("麻痺させる", "paralyze");
            break;
        case RBE_LOSE_STR:
            q = _("腕力を減少させる", "reduce strength");
            break;
        case RBE_LOSE_INT:
            q = _("知能を減少させる", "reduce intelligence");
            break;
        case RBE_LOSE_WIS:
            q = _("賢さを減少させる", "reduce wisdom");
            break;
        case RBE_LOSE_DEX:
            q = _("器用さを減少させる", "reduce dexterity");
            break;
        case RBE_LOSE_CON:
            q = _("耐久力を減少させる", "reduce constitution");
            break;
        case RBE_LOSE_CHR:
            q = _("魅力を減少させる", "reduce charisma");
            break;
        case RBE_LOSE_ALL:
            q = _("全ステータスを減少させる", "reduce all stats");
            break;
        case RBE_SHATTER:
            q = _("粉砕する", "shatter");
            break;
        case RBE_EXP_10:
            q = _("経験値を減少(10d6+)させる", "lower experience (by 10d6+)");
            break;
        case RBE_EXP_20:
            q = _("経験値を減少(20d6+)させる", "lower experience (by 20d6+)");
            break;
        case RBE_EXP_40:
            q = _("経験値を減少(40d6+)させる", "lower experience (by 40d6+)");
            break;
        case RBE_EXP_80:
            q = _("経験値を減少(80d6+)させる", "lower experience (by 80d6+)");
            break;
        case RBE_DISEASE:
            q = _("病気にする", "disease");
            break;
        case RBE_TIME:
            q = _("時間を逆戻りさせる", "time");
            break;
        case RBE_DR_LIFE:
            q = _("生命力を吸収する", "drain life");
            break;
        case RBE_DR_MANA:
            q = _("魔力を奪う", "drain mana force");
            break;
        case RBE_INERTIA:
            q = _("減速させる", "slow");
            break;
        case RBE_STUN:
            q = _("朦朧とさせる", "stun");
            break;
        }

#ifdef JP
        if (attack_numbers == 0) {
            hooked_roff(format("%^sは", wd_he[lore_ptr->msex]));
        }

        if (d1 && d2 && (lore_ptr->know_everything || know_damage(r_idx, m))) {
            hooked_roff(format(" %dd%d ", d1, d2));
            hooked_roff("のダメージで");
        }

        if (!p)
            p = "何か奇妙なことをする";

        /* XXしてYYし/XXしてYYする/XXし/XXする */
        if (q != NULL)
            jverb(p, lore_ptr->jverb_buf, JVERB_TO);
        else if (attack_numbers != count - 1)
            jverb(p, lore_ptr->jverb_buf, JVERB_AND);
        else
            strcpy(lore_ptr->jverb_buf, p);

        hooked_roff(lore_ptr->jverb_buf);
        if (q) {
            if (attack_numbers != count - 1)
                jverb(q, lore_ptr->jverb_buf, JVERB_AND);
            else
                strcpy(lore_ptr->jverb_buf, q);
            hooked_roff(lore_ptr->jverb_buf);
        }

        if (attack_numbers != count - 1)
            hooked_roff("、");
#else
        if (attack_numbers == 0) {
            hooked_roff(format("%^s can ", wd_he[lore_ptr->msex]));
        } else if (attack_numbers < count - 1) {
            hooked_roff(", ");
        } else {
            hooked_roff(", and ");
        }

        if (!p)
            p = "do something weird";
        hooked_roff(p);
        if (q) {
            hooked_roff(" to ");
            hooked_roff(q);
            if (d1 && d2 && (lore_ptr->know_everything || know_damage(r_idx, m))) {
                hooked_roff(" with damage");
                hooked_roff(format(" %dd%d", d1, d2));
            }
        }
#endif

        attack_numbers++;
    }

    if (attack_numbers > 0) {
        hooked_roff(_("。", ".  "));
    } else if (lore_ptr->flags1 & RF1_NEVER_BLOW) {
        hooked_roff(format(_("%^sは物理的な攻撃方法を持たない。", "%^s has no physical attacks.  "), wd_he[lore_ptr->msex]));
    } else {
        hooked_roff(format(_("%s攻撃については何も知らない。", "Nothing is known about %s attack.  "), wd_his[lore_ptr->msex]));
    }

    bool is_kingpin = (lore_ptr->flags1 & RF1_QUESTOR) != 0;
    is_kingpin &= lore_ptr->r_ptr->r_sights > 0;
    is_kingpin &= lore_ptr->r_ptr->max_num > 0;
    is_kingpin &= (r_idx == MON_OBERON) || (r_idx == MON_SERPENT);
    if (is_kingpin) {
        hook_c_roff(TERM_VIOLET, _("あなたはこのモンスターを殺したいという強い欲望を感じている...", "You feel an intense desire to kill this monster...  "));
    } else if (lore_ptr->flags7 & RF7_GUARDIAN) {
        hook_c_roff(TERM_L_RED, _("このモンスターはダンジョンの主である。", "This monster is the master of a dungeon."));
    }

    hooked_roff("\n");
}
