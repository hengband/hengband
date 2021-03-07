#include "mspell/mspell-breath.h"
#include "core/disturbance.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/drs-types.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-info.h"
#include "monster/monster-update.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-util.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief RF4_BR_*の処理。各種ブレス。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param GF_TYPE ブレスの属性
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF4_BREATH(player_type *target_ptr, int GF_TYPE, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam, ms_type, drs_type = 0;
    concptr type_s;
    bool smart_learn_aux = TRUE;
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    bool known = monster_near_player(floor_ptr, m_idx, t_idx);
    bool see_either = see_monster(target_ptr, m_idx) || see_monster(target_ptr, t_idx);
    bool mon_to_mon = (TARGET_TYPE == MONSTER_TO_MONSTER);
    bool mon_to_player = (TARGET_TYPE == MONSTER_TO_PLAYER);
    GAME_TEXT m_name[MAX_NLEN], t_name[MAX_NLEN];
    monster_name(target_ptr, m_idx, m_name);
    monster_name(target_ptr, t_idx, t_name);

    switch (GF_TYPE) {
    case GF_ACID:
        dam = monspell_damage(target_ptr, (MS_BR_ACID), m_idx, DAM_ROLL);
        type_s = _("酸", "acid");
        ms_type = MS_BR_ACID;
        drs_type = DRS_ACID;
        break;
    case GF_ELEC:
        dam = monspell_damage(target_ptr, (MS_BR_ELEC), m_idx, DAM_ROLL);
        type_s = _("稲妻", "lightning");
        ms_type = MS_BR_ELEC;
        drs_type = DRS_ELEC;
        break;
    case GF_FIRE:
        dam = monspell_damage(target_ptr, (MS_BR_FIRE), m_idx, DAM_ROLL);
        type_s = _("火炎", "fire");
        ms_type = MS_BR_FIRE;
        drs_type = DRS_FIRE;
        break;
    case GF_COLD:
        dam = monspell_damage(target_ptr, (MS_BR_COLD), m_idx, DAM_ROLL);
        type_s = _("冷気", "frost");
        ms_type = MS_BR_COLD;
        drs_type = DRS_COLD;
        break;
    case GF_POIS:
        dam = monspell_damage(target_ptr, (MS_BR_POIS), m_idx, DAM_ROLL);
        type_s = _("ガス", "gas");
        ms_type = MS_BR_POIS;
        drs_type = DRS_POIS;
        break;
    case GF_NETHER:
        dam = monspell_damage(target_ptr, (MS_BR_NETHER), m_idx, DAM_ROLL);
        type_s = _("地獄", "nether");
        ms_type = MS_BR_NETHER;
        drs_type = DRS_NETH;
        break;
    case GF_LITE:
        dam = monspell_damage(target_ptr, (MS_BR_LITE), m_idx, DAM_ROLL);
        type_s = _("閃光", "light");
        ms_type = MS_BR_LITE;
        drs_type = DRS_LITE;
        break;
    case GF_DARK:
        dam = monspell_damage(target_ptr, (MS_BR_DARK), m_idx, DAM_ROLL);
        type_s = _("暗黒", "darkness");
        ms_type = MS_BR_DARK;
        drs_type = DRS_DARK;
        break;
    case GF_CONFUSION:
        dam = monspell_damage(target_ptr, (MS_BR_CONF), m_idx, DAM_ROLL);
        type_s = _("混乱", "confusion");
        ms_type = MS_BR_CONF;
        drs_type = DRS_CONF;
        break;
    case GF_SOUND:
        dam = monspell_damage(target_ptr, (MS_BR_SOUND), m_idx, DAM_ROLL);
        type_s = _("轟音", "sound");
        ms_type = MS_BR_SOUND;
        drs_type = DRS_SOUND;
        break;
    case GF_CHAOS:
        dam = monspell_damage(target_ptr, (MS_BR_CHAOS), m_idx, DAM_ROLL);
        type_s = _("カオス", "chaos");
        ms_type = MS_BR_CHAOS;
        drs_type = DRS_CHAOS;
        break;
    case GF_DISENCHANT:
        dam = monspell_damage(target_ptr, (MS_BR_DISEN), m_idx, DAM_ROLL);
        type_s = _("劣化", "disenchantment");
        ms_type = MS_BR_DISEN;
        drs_type = DRS_DISEN;
        break;
    case GF_NEXUS:
        dam = monspell_damage(target_ptr, (MS_BR_NEXUS), m_idx, DAM_ROLL);
        type_s = _("因果混乱", "nexus");
        ms_type = MS_BR_NEXUS;
        drs_type = DRS_NEXUS;
        break;
    case GF_TIME:
        dam = monspell_damage(target_ptr, (MS_BR_TIME), m_idx, DAM_ROLL);
        type_s = _("時間逆転", "time");
        ms_type = MS_BR_TIME;
        smart_learn_aux = FALSE;
        break;
    case GF_INERTIAL:
        dam = monspell_damage(target_ptr, (MS_BR_INERTIA), m_idx, DAM_ROLL);
        type_s = _("遅鈍", "inertia");
        ms_type = MS_BR_INERTIA;
        smart_learn_aux = FALSE;
        break;
    case GF_GRAVITY:
        dam = monspell_damage(target_ptr, (MS_BR_GRAVITY), m_idx, DAM_ROLL);
        type_s = _("重力", "gravity");
        ms_type = MS_BR_GRAVITY;
        smart_learn_aux = FALSE;
        break;
    case GF_SHARDS:
        dam = monspell_damage(target_ptr, (MS_BR_SHARDS), m_idx, DAM_ROLL);
        type_s = _("破片", "shards");
        ms_type = MS_BR_SHARDS;
        drs_type = DRS_SHARD;
        break;
    case GF_PLASMA:
        dam = monspell_damage(target_ptr, (MS_BR_PLASMA), m_idx, DAM_ROLL);
        type_s = _("プラズマ", "plasma");
        ms_type = MS_BR_PLASMA;
        smart_learn_aux = FALSE;
        break;
    case GF_FORCE:
        dam = monspell_damage(target_ptr, (MS_BR_FORCE), m_idx, DAM_ROLL);
        type_s = _("フォース", "force");
        ms_type = MS_BR_FORCE;
        smart_learn_aux = FALSE;
        break;
    case GF_MANA:
        dam = monspell_damage(target_ptr, (MS_BR_MANA), m_idx, DAM_ROLL);
        type_s = _("魔力", "mana");
        ms_type = MS_BR_MANA;
        smart_learn_aux = FALSE;
        break;
    case GF_NUKE:
        dam = monspell_damage(target_ptr, (MS_BR_NUKE), m_idx, DAM_ROLL);
        type_s = _("放射性廃棄物", "toxic waste");
        ms_type = MS_BR_NUKE;
        drs_type = DRS_POIS;
        break;
    case GF_DISINTEGRATE:
        dam = monspell_damage(target_ptr, (MS_BR_DISI), m_idx, DAM_ROLL);
        type_s = _("分解", "disintegration");
        ms_type = MS_BR_DISI;
        smart_learn_aux = FALSE;
        break;
    default:
        /* Do not reach here */
        dam = 0;
        type_s = _("不明", "Unknown");
        ms_type = MS_BR_ACID;
        smart_learn_aux = FALSE;
        break;
    }

    if (mon_to_player || (mon_to_mon && known && see_either))
        disturb(target_ptr, TRUE, TRUE);

    if (m_ptr->r_idx == MON_JAIAN && GF_TYPE == GF_SOUND) {
        msg_format(_("「ボォエ～～～～～～」", "'Booooeeeeee'"));
    } else if (m_ptr->r_idx == MON_BOTEI && GF_TYPE == GF_SHARDS) {
        msg_format(_("「ボ帝ビルカッター！！！」", "'Boty-Build cutter!!!'"));
    } else if (target_ptr->blind) {
        if (mon_to_player || (mon_to_mon && known && see_either))
            msg_format(_("%^sが何かのブレスを吐いた。", "%^s breathes."), m_name);
    } else {
        if (mon_to_player) {
            msg_format(_("%^sが%^sのブレスを吐いた。", "%^s breathes %^s."), m_name, type_s);
        } else if (mon_to_mon && known && see_either) {
            _(msg_format("%^sが%^sに%^sのブレスを吐いた。", m_name, t_name, type_s), msg_format("%^s breathes %^s at %^s.", m_name, type_s, t_name));
        }
    }

    if (mon_to_mon && known && !see_either)
        floor_ptr->monster_noise = TRUE;

    sound(SOUND_BREATH);
    breath(target_ptr, y, x, m_idx, GF_TYPE, dam, 0, TRUE, ms_type, TARGET_TYPE);
    if (smart_learn_aux && mon_to_player)
        update_smart_learn(target_ptr, m_idx, drs_type);

    return dam;
}
