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
#include "mspell/mspell-result.h"
#include "mspell/mspell-util.h"
#include "player-info/race-info.h"
#include "player/attack-defense-types.h"
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
static void dispel_player(PlayerType *player_ptr)
{
    (void)set_acceleration(player_ptr, 0, true);
    set_lightspeed(player_ptr, 0, true);
    (void)BadStatusSetter(player_ptr).set_deceleration(0, true);
    (void)set_shield(player_ptr, 0, true);
    (void)set_blessed(player_ptr, 0, true);
    (void)set_tsuyoshi(player_ptr, 0, true);
    (void)set_hero(player_ptr, 0, true);
    (void)set_shero(player_ptr, 0, true);
    (void)set_protevil(player_ptr, 0, true);
    (void)set_invuln(player_ptr, 0, true);
    (void)set_wraith_form(player_ptr, 0, true);
    (void)set_pass_wall(player_ptr, 0, true);
    (void)set_tim_res_nether(player_ptr, 0, true);
    (void)set_tim_res_time(player_ptr, 0, true);
    (void)set_tim_reflect(player_ptr, 0, true);
    (void)set_multishadow(player_ptr, 0, true);
    (void)set_dustrobe(player_ptr, 0, true);

    (void)set_tim_invis(player_ptr, 0, true);
    (void)set_tim_infra(player_ptr, 0, true);
    (void)set_tim_esp(player_ptr, 0, true);
    (void)set_tim_regen(player_ptr, 0, true);
    (void)set_tim_stealth(player_ptr, 0, true);
    (void)set_tim_levitation(player_ptr, 0, true);
    (void)set_tim_sh_force(player_ptr, 0, true);
    (void)set_tim_sh_fire(player_ptr, 0, true);
    (void)set_tim_sh_holy(player_ptr, 0, true);
    (void)set_tim_eyeeye(player_ptr, 0, true);
    (void)set_magicdef(player_ptr, 0, true);
    (void)set_resist_magic(player_ptr, 0, true);
    (void)set_oppose_acid(player_ptr, 0, true);
    (void)set_oppose_elec(player_ptr, 0, true);
    (void)set_oppose_fire(player_ptr, 0, true);
    (void)set_oppose_cold(player_ptr, 0, true);
    (void)set_oppose_pois(player_ptr, 0, true);
    (void)set_ultimate_res(player_ptr, 0, true);
    (void)set_mimic(player_ptr, 0, MimicKindType::NONE, true);
    (void)set_ele_attack(player_ptr, 0, 0);
    (void)set_ele_immune(player_ptr, 0, 0);

    if (player_ptr->special_attack & ATTACK_CONFUSE) {
        player_ptr->special_attack &= ~(ATTACK_CONFUSE);
        msg_print(_("手の輝きがなくなった。", "Your hands stop glowing."));
    }

    auto song_interruption = music_singing_any(player_ptr);
    auto spellhex_interruption = SpellHex(player_ptr).is_spelling_any();

    if (song_interruption || spellhex_interruption) {
        if (song_interruption) {
            set_interrupting_song_effect(player_ptr, get_singing_song_effect(player_ptr));
            set_singing_song_effect(player_ptr, MUSIC_NONE);
            msg_print(_("歌が途切れた。", "Your singing is interrupted."));
        }
        if (spellhex_interruption) {
            SpellHex(player_ptr).interrupt_spelling();
            msg_print(_("呪文が途切れた。", "Your casting is interrupted."));
        }

        player_ptr->action = ACTION_NONE;
        player_ptr->update |= (PU_BONUS | PU_HP | PU_MONSTERS);
        player_ptr->redraw |= (PR_MAP | PR_STATUS | PR_STATE);
        player_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);
        player_ptr->energy_need += ENERGY_NEED();
    }
}

/*!
 * @brief RF4_DISPELの処理。魔力消去。 /
 * @param m_idx 呪文を唱えるモンスターID
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF4_DISPEL(MONSTER_IDX m_idx, PlayerType *player_ptr, MONSTER_IDX t_idx, int target_type)
{
    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    mspell_cast_msg_blind msg(_("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."),
        _("%^sが魔力消去の呪文を念じた。", "%^s invokes a dispel magic."), _("%^sが%sに対して魔力消去の呪文を念じた。", "%^s invokes a dispel magic at %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    if (target_type == MONSTER_TO_PLAYER) {
        dispel_player(player_ptr);
        if (player_ptr->riding) {
            dispel_monster_status(player_ptr, player_ptr->riding);
        }

        if (is_echizen(player_ptr)) {
            msg_print(_("やりやがったな！", ""));
        } else if (is_chargeman(player_ptr)) {
            if (randint0(2) == 0) {
                msg_print(_("ジュラル星人め！", ""));
            } else {
                msg_print(_("弱い者いじめは止めるんだ！", ""));
            }
        }

        return res;
    }

    if (target_type == MONSTER_TO_MONSTER) {
        if (t_idx == player_ptr->riding) {
            dispel_player(player_ptr);
        }

        dispel_monster_status(player_ptr, t_idx);
    }

    return res;
}
