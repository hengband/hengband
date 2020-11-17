#include "specific-object/ring-of-power.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-sight.h"
#include "spell/spell-types.h"
#include "status/base-status.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

/*!
 * @brief 『一つの指輪』の効果処理 /
 * Hack -- activate the ring of power
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 発動の方向ID
 * @return なし
 */
static void exe_ring_of_power(player_type *caster_ptr, DIRECTION dir)
{
    switch (randint1(10)) {
    case 1:
    case 2:
        msg_print(_("あなたは悪性のオーラに包み込まれた。", "You are surrounded by a malignant aura."));
        sound(SOUND_EVIL);
        (void)dec_stat(caster_ptr, A_STR, 50, TRUE);
        (void)dec_stat(caster_ptr, A_INT, 50, TRUE);
        (void)dec_stat(caster_ptr, A_WIS, 50, TRUE);
        (void)dec_stat(caster_ptr, A_DEX, 50, TRUE);
        (void)dec_stat(caster_ptr, A_CON, 50, TRUE);
        (void)dec_stat(caster_ptr, A_CHR, 50, TRUE);
        caster_ptr->exp -= (caster_ptr->exp / 4);
        caster_ptr->max_exp -= (caster_ptr->exp / 4);
        check_experience(caster_ptr);
        break;
    case 3:
        msg_print(_("あなたは強力なオーラに包み込まれた。", "You are surrounded by a powerful aura."));
        dispel_monsters(caster_ptr, 1000);
        break;
    case 4:
    case 5:
    case 6:
        fire_ball(caster_ptr, GF_MANA, dir, 600, 3);
        break;
    case 7:
    case 8:
    case 9:
    case 10:
        fire_bolt(caster_ptr, GF_MANA, dir, 500);
        break;
    default:
        break;
    }
}

bool activate_ring_of_power(player_type *user_ptr, concptr name)
{
    DIRECTION dir;
    msg_format(_("%sは漆黒に輝いた...", "The %s glows intensely black..."), name);
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    exe_ring_of_power(user_ptr, dir);
    return TRUE;
}
