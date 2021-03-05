﻿#include "object-enchant/vorpal-weapon.h"
#include "artifact/fixed-art-types.h"
#include "inventory/inventory-slot-types.h"
#include "io/files-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "view/display-messages.h"

/*!
 * @brief ヴォーパル武器で攻撃した時のメッセージ表示
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param int 倍率
 * @return なし
 */
static void print_vorpal_message(player_attack_type *pa_ptr, const int magnification)
{
    switch (magnification) {
    case 2:
        msg_format(_("%sを斬った！", "You gouge %s!"), pa_ptr->m_name);
        break;
    case 3:
        msg_format(_("%sをぶった斬った！", "You maim %s!"), pa_ptr->m_name);
        break;
    case 4:
        msg_format(_("%sをメッタ斬りにした！", "You carve %s!"), pa_ptr->m_name);
        break;
    case 5:
        msg_format(_("%sをメッタメタに斬った！", "You cleave %s!"), pa_ptr->m_name);
        break;
    case 6:
        msg_format(_("%sを刺身にした！", "You smite %s!"), pa_ptr->m_name);
        break;
    case 7:
        msg_format(_("%sを斬って斬って斬りまくった！", "You eviscerate %s!"), pa_ptr->m_name);
        break;
    default:
        msg_format(_("%sを細切れにした！", "You shred %s!"), pa_ptr->m_name);
        break;
    }
}

/*!
 * @brief チェンソーのノイズ音を表示する
 * @param o_ptr チェンソーへの参照ポインタ
 * @return なし
 */
static void print_chainsword_noise(object_type *o_ptr)
{
    if ((o_ptr->name1 != ART_CHAINSWORD) || one_in_(2))
        return;

    char chainsword_noise[1024];
    if (!get_rnd_line(_("chainswd_j.txt", "chainswd.txt"), 0, chainsword_noise))
        msg_print(chainsword_noise);
}

/*!
 * @brief ヴォーパル武器利用時の処理メインルーチン
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param vorpal_cut メッタ斬りにできるかどうか
 * @param vorpal_chance ヴォーパル倍率上昇の機会値
 * @return なし
 */
void process_vorpal_attack(player_type *attacker_ptr, player_attack_type *pa_ptr, const bool vorpal_cut, const int vorpal_chance)
{
    if (!vorpal_cut)
        return;

    object_type *o_ptr = &attacker_ptr->inventory_list[INVEN_MAIN_HAND + pa_ptr->hand];
    int vorpal_magnification = 2;
    print_chainsword_noise(o_ptr);
    if (o_ptr->name1 == ART_VORPAL_BLADE)
        msg_print(_("目にも止まらぬヴォーパルブレード、手錬の早業！", "Your Vorpal Blade goes snicker-snack!"));
    else
        msg_format(_("%sをグッサリ切り裂いた！", "Your weapon cuts deep into %s!"), pa_ptr->m_name);

    while (one_in_(vorpal_chance))
        vorpal_magnification++;

    pa_ptr->attack_damage *= (HIT_POINT)vorpal_magnification;
    monster_race *r_ptr = &r_info[pa_ptr->m_ptr->r_idx];
    if (((r_ptr->flagsr & RFR_RES_ALL) ? pa_ptr->attack_damage / 100 : pa_ptr->attack_damage) > pa_ptr->m_ptr->hp)
        msg_format(_("%sを真っ二つにした！", "You cut %s in half!"), pa_ptr->m_name);
    else
        print_vorpal_message(pa_ptr, vorpal_magnification);

    pa_ptr->drain_result = pa_ptr->drain_result * 3 / 2;
}
