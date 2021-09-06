#include "mspell/mspell-dispel.h"
#include "blue-magic/blue-magic-checker.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/speed-table.h"
#include "core/window-redrawer.h"
#include "mind/mind-force-trainer.h"
#include "mind/mind-magic-resistance.h"
#include "mind/mind-mirror-master.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "mspell/mspell-util.h"
#include "mspell/mspell.h"
#include "player/attack-defense-types.h"
#include "player/player-race.h"
#include "player/player-status.h"
#include "realm/realm-song-numbers.h"
#include "spell-realm/spells-craft.h"
#include "spell-realm/spells-crusade.h"
#include "spell-realm/spells-demon.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "status/bad-status-setter.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/shape-changer.h"
#include "status/sight-setter.h"
#include "status/temporary-resistance.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief プレイヤーに魔力消去効果を与える。
 */
static void dispel_player(player_type *creature_ptr)
{
    (void)set_fast(creature_ptr, 0, true);
    set_lightspeed(creature_ptr, 0, true);
    (void)set_slow(creature_ptr, 0, true);
    (void)set_shield(creature_ptr, 0, true);
    (void)set_blessed(creature_ptr, 0, true);
    (void)set_tsuyoshi(creature_ptr, 0, true);
    (void)set_hero(creature_ptr, 0, true);
    (void)set_shero(creature_ptr, 0, true);
    (void)set_protevil(creature_ptr, 0, true);
    (void)set_invuln(creature_ptr, 0, true);
    (void)set_wraith_form(creature_ptr, 0, true);
    (void)set_pass_wall(creature_ptr, 0, true);
    (void)set_tim_res_nether(creature_ptr, 0, true);
    (void)set_tim_res_time(creature_ptr, 0, true);
    (void)set_tim_reflect(creature_ptr, 0, true);
    (void)set_multishadow(creature_ptr, 0, true);
    (void)set_dustrobe(creature_ptr, 0, true);

    (void)set_tim_invis(creature_ptr, 0, true);
    (void)set_tim_infra(creature_ptr, 0, true);
    (void)set_tim_esp(creature_ptr, 0, true);
    (void)set_tim_regen(creature_ptr, 0, true);
    (void)set_tim_stealth(creature_ptr, 0, true);
    (void)set_tim_levitation(creature_ptr, 0, true);
    (void)set_tim_sh_force(creature_ptr, 0, true);
    (void)set_tim_sh_fire(creature_ptr, 0, true);
    (void)set_tim_sh_holy(creature_ptr, 0, true);
    (void)set_tim_eyeeye(creature_ptr, 0, true);
    (void)set_magicdef(creature_ptr, 0, true);
    (void)set_resist_magic(creature_ptr, 0, true);
    (void)set_oppose_acid(creature_ptr, 0, true);
    (void)set_oppose_elec(creature_ptr, 0, true);
    (void)set_oppose_fire(creature_ptr, 0, true);
    (void)set_oppose_cold(creature_ptr, 0, true);
    (void)set_oppose_pois(creature_ptr, 0, true);
    (void)set_ultimate_res(creature_ptr, 0, true);
    (void)set_mimic(creature_ptr, 0, 0, true);
    (void)set_ele_attack(creature_ptr, 0, 0);
    (void)set_ele_immune(creature_ptr, 0, 0);

    if (creature_ptr->special_attack & ATTACK_CONFUSE) {
        creature_ptr->special_attack &= ~(ATTACK_CONFUSE);
        msg_print(_("手の輝きがなくなった。", "Your hands stop glowing."));
    }

    if (music_singing_any(creature_ptr) || RealmHex(creature_ptr).is_spelling_any()) {
        concptr str = (music_singing_any(creature_ptr)) ? _("歌", "singing") : _("呪文", "casting");
        set_interrupting_song_effect(creature_ptr, get_singing_song_effect(creature_ptr));
        set_singing_song_effect(creature_ptr, MUSIC_NONE);
        msg_format(_("%sが途切れた。", "Your %s is interrupted."), str);

        creature_ptr->action = ACTION_NONE;
        creature_ptr->update |= (PU_BONUS | PU_HP | PU_MONSTERS);
        creature_ptr->redraw |= (PR_MAP | PR_STATUS | PR_STATE);
        creature_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);
        creature_ptr->energy_need += ENERGY_NEED();
    }
}

/*!
 * @brief RF4_DISPELの処理。魔力消去。 /
 * @param m_idx 呪文を唱えるモンスターID
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF4_DISPEL(MONSTER_IDX m_idx, player_type *target_ptr, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    auto res = MonsterSpellResult::make_valid();
    res.learnable = TARGET_TYPE == MONSTER_TO_PLAYER;

    GAME_TEXT m_name[MAX_NLEN], t_name[MAX_NLEN];
    monster_name(target_ptr, m_idx, m_name);
    monster_name(target_ptr, t_idx, t_name);

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."),
        _("%^sが魔力消去の呪文を念じた。", "%^s invokes a dispel magic."), _("%^sが%sに対して魔力消去の呪文を念じた。", "%^s invokes a dispel magic at %s."),
        TARGET_TYPE);

    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        dispel_player(target_ptr);
        if (target_ptr->riding)
            dispel_monster_status(target_ptr, target_ptr->riding);

        if (is_echizen(target_ptr))
            msg_print(_("やりやがったな！", ""));
        else if (is_chargeman(target_ptr)) {
            if (randint0(2) == 0)
                msg_print(_("ジュラル星人め！", ""));
            else
                msg_print(_("弱い者いじめは止めるんだ！", ""));
        }

        return res;
    }

    if (TARGET_TYPE == MONSTER_TO_MONSTER) {
        if (t_idx == target_ptr->riding)
            dispel_player(target_ptr);

        dispel_monster_status(target_ptr, t_idx);
    }

    return res;
}
