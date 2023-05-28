/*!
 * @brief スナイパー技能の実装 / Sniping
 * @date 2014/01/18
 * @author
 * 2014 Deskull rearranged comment for Doxygen.\n
 */

#include "mind/mind-sniper.h"
#include "action/action-limited.h"
#include "cmd-action/cmd-shoot.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "floor/geometry.h"
#include "game-option/text-display-options.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "io/command-repeater.h"
#include "io/input-key-requester.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/snipe-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags3.h"
#include "object/tval-types.h"
#include "player-base/player-class.h"
#include "player-info/sniper-data-type.h"
#include "player-status/player-energy.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "view/display-util.h"

#define MAX_SNIPE_POWERS 16

/*! スナイパー技能情報の構造体 */
struct snipe_power {
    PLAYER_LEVEL min_lev;
    MANA_POINT mana_cost;
    concptr name;
};

/*! スナイパー技能の解説メッセージ */
static concptr const snipe_tips[MAX_SNIPE_POWERS] = {
#ifdef JP
    "精神を集中する。射撃の威力、精度が上がり、高度な射撃術が使用できるようになる。",
    "光る矢を放つ。光に弱いモンスターに威力を発揮する。",
    "射撃を行った後、短距離の瞬間移動を行う。",
    "軌道上の罠をすべて無効にする低空飛行の矢を放つ。",
    "火炎属性の矢を放つ。",
    "壁を粉砕する矢を放つ。岩でできたモンスターと無生物のモンスターに威力を発揮する。",
    "冷気属性の矢を放つ。",
    "敵を突き飛ばす矢を放つ。",
    "複数の敵を貫通する矢を放つ。",
    "善良なモンスターに威力を発揮する矢を放つ。",
    "邪悪なモンスターに威力を発揮する矢を放つ。",
    "当たると爆発する矢を放つ。",
    "2回射撃を行う。",
    "電撃属性の矢を放つ。",
    "敵の急所にめがけて矢を放つ。成功すると敵を一撃死させる。失敗すると1ダメージ。",
    "全てのモンスターに高威力を発揮する矢を放つ。反動による副次効果を受ける。",
#else
    "Concentrate your mind for shooting.",
    "Shoot a glowing arrow effective against light-sensitive foes.",
    "Blink after shooting.",
    "Shoot an arrow able to shatter traps.",
    "Deals extra damage of fire.",
    "Shoot an arrow able to shatter rocks.",
    "Deals extra damage of ice.",
    "Shoot an arrow that pushes away the target.",
    "Shoot an arrow that does not always stop at the first target on its path.",
    "Deals more damage to good monsters.",
    "Deals more damage to evil monsters.",
    "Shoot an arrow that explodes when it hits a monster.",
    "Shoot two arrows at once.",
    "Deals extra damage of lightning.",
    "Deals quick death or 1 damage.",
    "Deals great damage to all monsters, and some side effects to you.",
#endif
};

/*! スナイパー技能テーブル */
static snipe_power const snipe_powers[MAX_SNIPE_POWERS] = {
/* Level gained,  cost,  name */
#ifdef JP
    { 1, 0, "精神集中" },
    { 2, 1, "フラッシュアロー" },
    { 3, 1, "シュート＆アウェイ" },
    { 5, 1, "解除の矢" },
    { 8, 2, "火炎の矢" },
    { 10, 2, "岩砕き" },
    { 13, 2, "冷気の矢" },
    { 18, 2, "烈風弾" },
    { 22, 3, "貫通弾" },
    { 25, 4, "邪念弾" },
    { 26, 4, "破魔矢" },
    { 30, 3, "爆発の矢" },
    { 32, 4, "ダブルショット" },
    { 36, 3, "プラズマボルト" },
    { 40, 3, "ニードルショット" },
    { 48, 7, "セイントスターアロー" },
#else
    { 1, 0, "Concentration" },
    { 2, 1, "Flash Arrow" },
    { 3, 1, "Shoot & Away" },
    { 5, 1, "Disarm Shot" },
    { 8, 2, "Fire Shot" },
    { 10, 2, "Shatter Arrow" },
    { 13, 2, "Ice Shot" },
    { 18, 2, "Rushing Arrow" },
    { 22, 3, "Piercing Shot" },
    { 25, 4, "Evil Shot" },
    { 26, 4, "Holy Shot" },
    { 30, 3, "Missile" },
    { 32, 4, "Double Shot" },
    { 36, 3, "Plasma Bolt" },
    { 40, 3, "Needle Shot" },
    { 48, 7, "Saint Stars Arrow" },
#endif
};

static void reset_concentration_flag(sniper_data_type *sniper_data)
{
    sniper_data->reset_concent = false;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const auto flags = {
        StatusRedrawingFlag::BONUS,
        StatusRedrawingFlag::MONSTER_STATUSES,
    };
    rfu.set_flags(flags);
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
}

/*!
 * @brief スナイパーの集中度加算
 * @return 集中度を加算した場合は true、そうでなければ false
 */
static bool snipe_concentrate(PlayerType *player_ptr)
{
    auto sniper_data = PlayerClass(player_ptr).get_specific_data<sniper_data_type>();
    if (!sniper_data) {
        return false;
    }

    if (sniper_data->concent < (2 + (player_ptr->lev + 5) / 10)) {
        sniper_data->concent++;
    }

    msg_format(_("集中した。(集中度 %d)", "You concentrate deeply. (lvl %d)"), sniper_data->concent);
    reset_concentration_flag(sniper_data.get());
    return true;
}

/*!
 * @brief スナイパーの集中度リセット
 * スナイパーではない、もしくは集中度がすでに0であればなにもしない。
 * @param msg TRUEならばメッセージを表示する
 */
void reset_concentration(PlayerType *player_ptr, bool msg)
{
    auto sniper_data = PlayerClass(player_ptr).get_specific_data<sniper_data_type>();
    if (!sniper_data) {
        return;
    }

    if (msg && (sniper_data->concent > 0)) {
        msg_print(_("集中力が途切れてしまった。", "Stop concentrating."));
    }

    sniper_data->concent = 0;
    reset_concentration_flag(sniper_data.get());
}

/*!
 * @brief スナイパーの集中度によるダメージボーナスを加算する
 * @param tdam 算出中のダメージ
 * @return 集中度修正を加えたダメージ
 */
int boost_concentration_damage(PlayerType *player_ptr, int tdam)
{
    auto sniper_data = PlayerClass(player_ptr).get_specific_data<sniper_data_type>();
    const auto sniper_concent = sniper_data ? sniper_data->concent : 0;

    tdam = tdam * (10 + sniper_concent) / 10;

    return tdam;
}

/*!
 * @brief スナイパーの技能リストを表示する
 */
void display_snipe_list(PlayerType *player_ptr)
{
    int i;
    TERM_LEN y = 1;
    TERM_LEN x = 1;
    PLAYER_LEVEL plev = player_ptr->lev;
    snipe_power spell;
    char psi_desc[80];

    /* Display a list of spells */
    prt("", y, x);
    put_str(_("名前", "Name"), y, x + 5);
    put_str(_("Lv   MP", "Lv Mana"), y, x + 35);

    auto sniper_data = PlayerClass(player_ptr).get_specific_data<sniper_data_type>();

    for (i = 0; i < MAX_SNIPE_POWERS; i++) {
        /* Access the available spell */
        spell = snipe_powers[i];
        if (spell.min_lev > plev) {
            continue;
        }

        strnfmt(psi_desc, sizeof(psi_desc), "  %c) %-30s%2d %4d", I2A(i), spell.name, spell.min_lev, spell.mana_cost);

        TERM_COLOR tcol = (spell.mana_cost > sniper_data->concent) ? TERM_SLATE : TERM_WHITE;
        term_putstr(x, y + i + 1, -1, tcol, psi_desc);
    }
}

/*!
 * @brief スナイパー技能を選択する
 * @param sn 選択した特殊技能ID、キャンセルの場合-1、不正な選択の場合-2を返す
 * @param only_browse 一覧を見るだけの場合TRUEを返す
 * @return 発動可能な魔法を選択した場合TRUE、キャンセル処理か不正な選択が行われた場合FALSEを返す。
 * Allow user to choose a mindcrafter power.\n
 *\n
 * If a valid spell is chosen, saves it in '*sn' and returns TRUE\n
 * If the user hits escape, returns FALSE, and set '*sn' to -1\n
 * If there are no legal choices, returns FALSE, and sets '*sn' to -2\n
 *\n
 * The "prompt" should be "cast", "recite", or "study"\n
 * The "known" should be TRUE for cast/pray, FALSE for study\n
 *\n
 * nb: This function has a (trivial) display bug which will be obvious\n
 * when you run it. It's probably easy to fix but I haven't tried,\n
 * sorry.\n
 */
static int get_snipe_power(PlayerType *player_ptr, COMMAND_CODE *sn, bool only_browse)
{
    COMMAND_CODE i;
    int num = 0;
    TERM_LEN y = 1;
    TERM_LEN x = 20;
    PLAYER_LEVEL plev = player_ptr->lev;
    char choice;
    char out_val[160];
    concptr p = _("射撃術", "power");
    snipe_power spell;
    bool flag, redraw;

    repeat_push(*sn);

    /* Assume cancelled */
    *sn = (-1);

    auto sniper_data = PlayerClass(player_ptr).get_specific_data<sniper_data_type>();

    /* Repeat previous command */
    /* Get the spell, if available */
    if (repeat_pull(sn)) {
        /* Verify the spell */
        if ((snipe_powers[*sn].min_lev <= plev) && (snipe_powers[*sn].mana_cost <= sniper_data->concent)) {
            /* Success */
            return true;
        }
    }

    flag = false;
    redraw = false;

    for (i = 0; i < MAX_SNIPE_POWERS; i++) {
        if ((snipe_powers[i].min_lev <= plev) && ((only_browse) || (snipe_powers[i].mana_cost <= sniper_data->concent))) {
            num = i;
        }
    }

    /* Build a prompt (accept all spells) */
    if (only_browse) {
        constexpr auto mes = _("(%s^ %c-%c, '*'で一覧, ESC) どの%sについて知りますか？", "(%s^s %c-%c, *=List, ESC=exit) Use which %s? ");
        (void)strnfmt(out_val, 78, mes, p, I2A(0), I2A(num), p);
    } else {
        constexpr auto mes = _("(%s^ %c-%c, '*'で一覧, ESC) どの%sを使いますか？", "(%s^s %c-%c, *=List, ESC=exit) Use which %s? ");
        (void)strnfmt(out_val, 78, mes, p, I2A(0), I2A(num), p);
    }

    choice = always_show_list ? ESCAPE : 1;
    while (!flag) {
        if (choice == ESCAPE) {
            choice = ' ';
        } else if (!get_com(out_val, &choice, false)) {
            break;
        }

        /* Request redraw */
        if ((choice == ' ') || (choice == '*') || (choice == '?')) {
            /* Show the list */
            if (!redraw) {
                redraw = true;
                if (!only_browse) {
                    screen_save();
                }

                /* Display a list of spells */
                prt("", y, x);
                put_str(_("名前", "Name"), y, x + 5);
                put_str(_("Lv   集中度", "Lv Pow"), y, x + 35);

                /* Dump the spells */
                for (i = 0; i < MAX_SNIPE_POWERS; i++) {
                    term_color_type tcol = TERM_WHITE;
                    term_erase(x, y + i + 1, 255);

                    /* Access the spell */
                    spell = snipe_powers[i];

                    /* Dump the spell --(-- */
                    if (spell.min_lev > plev) {
                        tcol = TERM_SLATE;
                    } else if (spell.mana_cost > sniper_data->concent) {
                        tcol = TERM_L_BLUE;
                    }
                    term_putstr(x, y + i + 1, -1, tcol, (spell.min_lev > plev) ? "   ) " : format("  %c) ", I2A(i)));
                    term_putstr(x + 5, y + i + 1, -1, tcol, format("%-30s%2d %4d", spell.name, spell.min_lev, spell.mana_cost));
                }

                /* Clear the bottom line */
                prt("", y + i + 1, x);
            }

            /* Hide the list */
            else {
                /* Hide list */
                redraw = false;
                if (!only_browse) {
                    screen_load();
                }
            }

            /* Redo asking */
            continue;
        }

        i = A2I(choice);

        /* Totally Illegal */
        if ((i < 0) || (i > num) || (!only_browse && (snipe_powers[i].mana_cost > sniper_data->concent))) {
            bell();
            continue;
        }

        /* Save the spell index */
        spell = snipe_powers[i];

        /* Stop the loop */
        flag = true;
    }
    if (redraw && !only_browse) {
        screen_load();
    }

    player_ptr->window_flags |= (PW_SPELL);
    handle_stuff(player_ptr);

    /* Abort if needed */
    if (!flag) {
        return false;
    }

    /* Save the choice */
    (*sn) = i;

    repeat_push(*sn);

    /* Success */
    return true;
}

/*!
 * @brief スナイバー技能のスレイ倍率計算を行う /
 * Calcurate magnification of snipe technics
 * @param mult スナイバー技能のスレイ効果以前に算出している多要素の倍率(/10倍)
 * @param m_ptr 目標となるモンスターの構造体参照ポインタ
 * @return スレイの倍率(/10倍)
 */
MULTIPLY calc_snipe_damage_with_slay(PlayerType *player_ptr, MULTIPLY mult, MonsterEntity *m_ptr, SPELL_IDX snipe_type)
{
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    bool seen = is_seen(player_ptr, m_ptr);

    auto sniper_data = PlayerClass(player_ptr).get_specific_data<sniper_data_type>();
    const auto sniper_concent = sniper_data ? sniper_data->concent : 0;

    switch (snipe_type) {
    case SP_LITE:
        if (r_ptr->resistance_flags.has(MonsterResistanceType::HURT_LITE)) {
            MULTIPLY n = 20 + sniper_concent;
            if (seen) {
                r_ptr->r_resistance_flags.set(MonsterResistanceType::HURT_LITE);
            }
            if (mult < n) {
                mult = n;
            }
        }
        break;
    case SP_FIRE:
        if (r_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_FIRE)) {
            if (seen) {
                r_ptr->r_resistance_flags.set(MonsterResistanceType::IMMUNE_FIRE);
            }
        } else {
            MULTIPLY n;
            if (r_ptr->resistance_flags.has(MonsterResistanceType::HURT_FIRE)) {
                n = 22 + (sniper_concent * 4);
                r_ptr->r_resistance_flags.set(MonsterResistanceType::HURT_FIRE);
            } else {
                n = 15 + (sniper_concent * 3);
            }

            if (mult < n) {
                mult = n;
            }
        }
        break;
    case SP_COLD:
        if (r_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_COLD)) {
            if (seen) {
                r_ptr->r_resistance_flags.set(MonsterResistanceType::IMMUNE_COLD);
            }
        } else {
            MULTIPLY n;
            if (r_ptr->resistance_flags.has(MonsterResistanceType::HURT_COLD)) {
                n = 22 + (sniper_concent * 4);
                r_ptr->r_resistance_flags.set(MonsterResistanceType::HURT_COLD);
            } else {
                n = 15 + (sniper_concent * 3);
            }

            if (mult < n) {
                mult = n;
            }
        }
        break;
    case SP_ELEC:
        if (r_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_ELEC)) {
            if (seen) {
                r_ptr->r_resistance_flags.set(MonsterResistanceType::IMMUNE_ELEC);
            }
        } else {
            MULTIPLY n = 18 + (sniper_concent * 4);
            if (mult < n) {
                mult = n;
            }
        }
        break;
    case SP_KILL_WALL:
        if (r_ptr->resistance_flags.has(MonsterResistanceType::HURT_ROCK)) {
            MULTIPLY n = 15 + (sniper_concent * 2);
            if (seen) {
                r_ptr->r_resistance_flags.set(MonsterResistanceType::HURT_ROCK);
            }
            if (mult < n) {
                mult = n;
            }
        } else if (r_ptr->kind_flags.has(MonsterKindType::NONLIVING)) {
            MULTIPLY n = 15 + (sniper_concent * 2);
            if (seen) {
                r_ptr->r_kind_flags.set(MonsterKindType::NONLIVING);
            }
            if (mult < n) {
                mult = n;
            }
        }
        break;
    case SP_EVILNESS:
        if (r_ptr->kind_flags.has(MonsterKindType::GOOD)) {
            MULTIPLY n = 15 + (sniper_concent * 4);
            if (seen) {
                r_ptr->r_kind_flags.set(MonsterKindType::GOOD);
            }
            if (mult < n) {
                mult = n;
            }
        }
        break;
    case SP_HOLYNESS:
        if (r_ptr->kind_flags.has(MonsterKindType::EVIL)) {
            MULTIPLY n = 12 + (sniper_concent * 3);
            if (seen) {
                r_ptr->r_kind_flags.set(MonsterKindType::EVIL);
            }
            if (r_ptr->resistance_flags.has(MonsterResistanceType::HURT_LITE)) {
                n += (sniper_concent * 3);
                if (seen) {
                    r_ptr->r_resistance_flags.set(MonsterResistanceType::HURT_LITE);
                }
            }
            if (mult < n) {
                mult = n;
            }
        }
        break;
    case SP_FINAL:
        if (mult < 50) {
            mult = 50;
        }
        break;
    }

    return mult;
}

/*!
 * @brief スナイパー技能の発動 /
 * do_cmd_cast calls this function if the player's class is 'snipe'.
 * @param spell 発動する特殊技能のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
static bool cast_sniper_spell(PlayerType *player_ptr, int spell)
{
    auto *o_ptr = &player_ptr->inventory_list[INVEN_BOW];
    if (o_ptr->bi_key.tval() != ItemKindType::BOW) {
        msg_print(_("弓を装備していない！", "You wield no bow!"));
        return false;
    }

    auto snipe_type = SP_NONE;
    switch (spell) {
    case 0: /* Concentration */
        sound(SOUND_ZAP);
        if (!snipe_concentrate(player_ptr)) {
            return false;
        }
        PlayerEnergy(player_ptr).set_player_turn_energy(100);
        return true;
    case 1:
        snipe_type = SP_LITE;
        break;
    case 2:
        snipe_type = SP_AWAY;
        break;
    case 3:
        snipe_type = SP_KILL_TRAP;
        break;
    case 4:
        snipe_type = SP_FIRE;
        break;
    case 5:
        snipe_type = SP_KILL_WALL;
        break;
    case 6:
        snipe_type = SP_COLD;
        break;
    case 7:
        snipe_type = SP_RUSH;
        break;
    case 8:
        snipe_type = SP_PIERCE;
        break;
    case 9:
        snipe_type = SP_EVILNESS;
        break;
    case 10:
        snipe_type = SP_HOLYNESS;
        break;
    case 11:
        snipe_type = SP_EXPLODE;
        break;
    case 12:
        snipe_type = SP_DOUBLE;
        break;
    case 13:
        snipe_type = SP_ELEC;
        break;
    case 14:
        snipe_type = SP_NEEDLE;
        break;
    case 15:
        snipe_type = SP_FINAL;
        break;
    default:
        msg_print(_("なに？", "Zap?"));
    }

    command_cmd = 'f';
    do_cmd_fire(player_ptr, snipe_type);

    return player_ptr->is_fired;
}

/*!
 * @brief スナイパー技能コマンドのメインルーチン /
 */
void do_cmd_snipe(PlayerType *player_ptr)
{
    if (cmd_limit_confused(player_ptr)) {
        return;
    }

    if (cmd_limit_image(player_ptr)) {
        return;
    }

    if (cmd_limit_stun(player_ptr)) {
        return;
    }

    COMMAND_CODE n = 0;
    if (!get_snipe_power(player_ptr, &n, false)) {
        return;
    }

    if (!cast_sniper_spell(player_ptr, n)) {
        return;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const auto flags_mwrf = {
        MainWindowRedrawingFlag::HP,
        MainWindowRedrawingFlag::MP,
    };
    rfu.set_flags(flags_mwrf);
    player_ptr->window_flags |= (PW_PLAYER);
    player_ptr->window_flags |= (PW_SPELL);
}

/*!
 * @brief スナイパー技能コマンドの表示 /
 */
void do_cmd_snipe_browse(PlayerType *player_ptr)
{
    COMMAND_CODE n = 0;

    screen_save();

    while (true) {
        if (!get_snipe_power(player_ptr, &n, true)) {
            screen_load();
            return;
        }

        /* Clear lines, position cursor  (really should use strlen here) */
        term_erase(12, 22, 255);
        term_erase(12, 21, 255);
        term_erase(12, 20, 255);
        term_erase(12, 19, 255);
        term_erase(12, 18, 255);

        display_wrap_around(snipe_tips[n], 62, 19, 15);
    }
}
