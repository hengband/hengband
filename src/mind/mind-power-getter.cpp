#include "mind/mind-power-getter.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "game-option/text-display-options.h"
#include "inventory/inventory-slot-types.h"
#include "io/command-repeater.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "mind/mind-explanations-table.h"
#include "mind/mind-force-trainer.h"
#include "mind/mind-info.h"
#include "mind/mind-types.h"
#include "player-info/class-info.h"
#include "player-info/equipment-info.h"
#include "player/player-status-table.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "util/enum-converter.h"
#include "util/int-char-converter.h"

MindPowerGetter::MindPowerGetter(PlayerType *player_ptr)
    : player_ptr(player_ptr)
    , menu_line(use_menu ? 1 : 0)
{
}

/*!
 * @brief 使用可能な特殊技能を選択する /
 * Allow user to choose a mindcrafter power.
 * @param sn 選択した特殊技能ID、キャンセルの場合-1、不正な選択の場合-2を返す
 * @param only_browse 一覧を見るだけの場合trueを返す
 * @return 発動可能な魔法を選択した場合true、キャンセル処理か不正な選択が行われた場合falseを返す。
 * @details
 * If a valid spell is chosen, saves it in '*sn' and returns true\n
 * If the user hits escape, returns false, and set '*sn' to -1\n
 * If there are no legal choices, returns false, and sets '*sn' to -2\n
 *\n
 * The "prompt" should be "cast", "recite", or "study"\n
 * The "known" should be true for cast/pray, false for study\n
 *\n
 * nb: This function has a (trivial) display bug which will be obvious\n
 * when you run it. It's probably easy to fix but I haven't tried,\n
 * sorry.\n
 */
bool MindPowerGetter::get_mind_power(SPELL_IDX *sn, bool only_browse)
{
    select_mind_description();
    if (select_spell_index(sn)) {
        return true;
    }

    for (this->index = 0; this->index < MAX_MIND_POWERS; this->index++) {
        if (mind_ptr->info[this->index].min_lev <= this->player_ptr->lev) {
            this->num++;
        }
    }

    char out_val[160];
    if (only_browse) {
        (void)strnfmt(out_val, 78, _("(%^s %c-%c, '*'で一覧, ESC) どの%sについて知りますか？", "(%^ss %c-%c, *=List, ESC=exit) Use which %s? "),
            this->mind_description, I2A(0), I2A(this->num - 1), this->mind_description);
    } else {
        (void)strnfmt(out_val, 78, _("(%^s %c-%c, '*'で一覧, ESC) どの%sを使いますか？", "(%^ss %c-%c, *=List, ESC=exit) Use which %s? "),
            this->mind_description, I2A(0), I2A(this->num - 1), this->mind_description);
    }

    if (use_menu && !only_browse) {
        screen_save();
    }

    this->choice = (always_show_list || use_menu) ? ESCAPE : 1;
    decide_mind_choice(out_val, only_browse);
    if (this->redraw && !only_browse) {
        screen_load();
    }

    this->player_ptr->window_flags |= PW_SPELL;
    handle_stuff(this->player_ptr);
    if (!this->flag) {
        return false;
    }

    *sn = this->index;
    repeat_push((COMMAND_CODE)this->index);
    return true;
}

/*
 * @brief 職業ごとの特殊技能表記を取得する
 */
void MindPowerGetter::select_mind_description()
{
    switch (this->player_ptr->pclass)
    case PlayerClassType::MINDCRAFTER: {
        this->use_mind = MindKindType::MINDCRAFTER;
        this->mind_description = _("超能力", "mindcraft");
        break;
    case PlayerClassType::FORCETRAINER:
        this->use_mind = MindKindType::KI;
        this->mind_description = _("練気術", "Force");
        break;
    case PlayerClassType::BERSERKER:
        this->use_mind = MindKindType::BERSERKER;
        this->mind_description = _("技", "brutal power");
        break;
    case PlayerClassType::MIRROR_MASTER:
        this->use_mind = MindKindType::MIRROR_MASTER;
        this->mind_description = _("鏡魔法", "magic");
        break;
    case PlayerClassType::NINJA:
        this->use_mind = MindKindType::NINJUTSU;
        this->mind_description = _("忍術", "ninjutsu");
        break;
    default:
        this->use_mind = MindKindType::MINDCRAFTER;
        this->mind_description = _("超能力", "mindcraft");
        break;
    }
}

bool MindPowerGetter::select_spell_index(SPELL_IDX *sn)
{
    COMMAND_CODE code;
    this->mind_ptr = &mind_powers[enum2i(this->use_mind)];
    *sn = -1;
    if (!repeat_pull(&code)) {
        return false;
    }

    *sn = (SPELL_IDX)code;
    if (*sn == INVEN_FORCE) {
        (void)repeat_pull(&code);
    }

    *sn = (SPELL_IDX)code;
    return mind_ptr->info[*sn].min_lev <= this->player_ptr->lev;
}

bool MindPowerGetter::decide_mind_choice(char *out_val, const bool only_browse)
{
    while (!this->flag) {
        if (this->choice == ESCAPE) {
            this->choice = ' ';
        } else if (!get_com(out_val, &this->choice, true)) {
            break;
        }

        if (!interpret_mind_key_input(only_browse)) {
            return false;
        }

        if (display_minds_chance(only_browse)) {
            continue;
        }

        make_choice_lower();
        if ((this->index < 0) || (this->index >= this->num)) {
            bell();
            continue;
        }

        this->spell = &mind_ptr->info[this->index];
        this->flag = true;
    }

    return true;
}

bool MindPowerGetter::interpret_mind_key_input(const bool only_browse)
{
    if (!use_menu || this->choice == ' ') {
        return true;
    }

    this->should_redraw_cursor = true;
    switch (this->choice) {
    case '0':
        if (!only_browse) {
            screen_load();
        }

        return false;
    case '8':
    case 'k':
    case 'K':
        this->menu_line += (this->num - 1);
        break;
    case '2':
    case 'j':
    case 'J':
        this->menu_line++;
        break;
    case 'x':
    case 'X':
    case '\r':
    case '\n':
        this->index = this->menu_line - 1;
        this->should_redraw_cursor = false;
        break;
    default:
        break;
    }

    if (this->menu_line > this->num) {
        this->menu_line -= this->num;
    }

    return true;
}

bool MindPowerGetter::display_minds_chance(const bool only_browse)
{
    if ((this->choice != ' ') && (this->choice != '*') && (this->choice != '?') && (!use_menu || !this->should_redraw_cursor)) {
        return false;
    }

    if (!this->redraw || use_menu) {
        this->redraw = true;
        if (!only_browse && !use_menu) {
            screen_save();
        }

        prt("", y, x);
        put_str(_("名前", "Name"), y, x + 5);
        put_str(format(_("Lv   %s   失率 効果", "Lv   %s   Fail Info"),
                    ((this->use_mind == MindKindType::BERSERKER) || (this->use_mind == MindKindType::NINJUTSU)) ? "HP" : "MP"),
            y, x + 35);
        display_each_mind_chance();
        prt("", y + this->index + 1, x);
        return true;
    }

    if (only_browse) {
        return true;
    }

    this->redraw = false;
    screen_load();
    return true;
}

void MindPowerGetter::display_each_mind_chance()
{
    bool has_weapon[2];
    has_weapon[0] = has_melee_weapon(this->player_ptr, INVEN_MAIN_HAND);
    has_weapon[1] = has_melee_weapon(this->player_ptr, INVEN_SUB_HAND);
    for (this->index = 0; this->index < MAX_MIND_POWERS; this->index++) {
        this->spell = &mind_ptr->info[this->index];
        if (this->spell->min_lev > this->player_ptr->lev) {
            break;
        }

        calculate_mind_chance(has_weapon);
        char comment[80];
        mindcraft_info(this->player_ptr, comment, this->use_mind, this->index);
        char psi_desc[80];
        if (use_menu) {
            if (this->index == (this->menu_line - 1)) {
                strcpy(psi_desc, _("  》 ", "  >  "));
            } else {
                strcpy(psi_desc, "     ");
            }
        } else {
            sprintf(psi_desc, "  %c) ", I2A(this->index));
        }

        strcat(psi_desc,
            format("%-30s%2d %4d%s %3d%%%s", this->spell->name, this->spell->min_lev, mana_cost,
                (((this->use_mind == MindKindType::MINDCRAFTER) && (this->index == 13)) ? _("～", "~ ") : "  "), chance, comment));
        prt(psi_desc, y + this->index + 1, x);
    }
}

void MindPowerGetter::calculate_mind_chance(bool *has_weapon)
{
    this->chance = this->spell->fail;
    this->mana_cost = this->spell->mana_cost;
    if (this->chance == 0) {
        return;
    }

    this->chance -= 3 * (this->player_ptr->lev - this->spell->min_lev);
    this->chance -= 3 * (adj_mag_stat[this->player_ptr->stat_index[mp_ptr->spell_stat]] - 1);
    calculate_ki_chance(has_weapon);
    if ((this->use_mind != MindKindType::BERSERKER) && (this->use_mind != MindKindType::NINJUTSU) && (this->mana_cost > this->player_ptr->csp)) {
        this->chance += 5 * (this->mana_cost - this->player_ptr->csp);
    }

    this->chance += this->player_ptr->to_m_chance;
    PERCENTAGE minfail = adj_mag_fail[this->player_ptr->stat_index[mp_ptr->spell_stat]];
    if (this->chance < minfail) {
        this->chance = minfail;
    }

    auto player_stun = this->player_ptr->effects()->stun();
    this->chance += player_stun->get_magic_chance_penalty();
    add_ki_chance();
    if (this->chance > 95) {
        this->chance = 95;
    }
}

void MindPowerGetter::calculate_ki_chance(bool *has_weapon)
{
    if (this->use_mind != MindKindType::KI) {
        return;
    }

    if (heavy_armor(this->player_ptr)) {
        this->chance += 20;
    }

    if (this->player_ptr->is_icky_wield[0]) {
        this->chance += 20;
    } else if (has_weapon[0]) {
        this->chance += 10;
    }

    if (this->player_ptr->is_icky_wield[1]) {
        chance += 20;
    } else if (has_weapon[1]) {
        this->chance += 10;
    }

    if (this->index == 5) {
        for (auto j = 0; j < get_current_ki(this->player_ptr) / 50; j++) {
            this->mana_cost += (j + 1) * 3 / 2;
        }
    }
}

void MindPowerGetter::add_ki_chance()
{
    if (this->use_mind != MindKindType::KI) {
        return;
    }

    if (heavy_armor(this->player_ptr)) {
        this->chance += 5;
    }

    if (this->player_ptr->is_icky_wield[0]) {
        this->chance += 5;
    }

    if (this->player_ptr->is_icky_wield[1]) {
        this->chance += 5;
    }
}

void MindPowerGetter::make_choice_lower()
{
    if (use_menu) {
        return;
    }

    this->index = A2I(this->choice);
}
