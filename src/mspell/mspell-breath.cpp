﻿#include "mspell/mspell-breath.h"
#include "core/disturbance.h"
#include "effect/effect-processor.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/drs-types.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-info.h"
#include "monster/monster-update.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-util.h"
#include "mspell/mspell.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief ブレスを吐くときにモンスター固有のセリフを表示する
 * @param r_idx モンスター種族番号
 * @param GF_TYPE 魔法効果
 * @return 表示したらTRUE、しなかったらFALSE
 */
static bool spell_RF4_BREATH_special_message(MONSTER_IDX r_idx, int GF_TYPE, concptr m_name)
{
    if (r_idx == MON_JAIAN && GF_TYPE == GF_SOUND) {
        msg_format(_("%^s「ボォエ～～～～～～」", "%^s sings, 'Booooeeeeee'"), m_name);
        return true;
    }
    if (r_idx == MON_BOTEI && GF_TYPE == GF_SHARDS) {
        msg_format(_("%^s「ボ帝ビルカッター！！！」", "%^s shouts, 'Boty-Build cutter!!!'"), m_name);
        return true;
    }
    if (r_idx == MON_RAOU &&GF_TYPE == GF_FORCE) {
        if (one_in_(2))
            msg_format(_("%^s「北斗剛掌波！！」", "%^s says, 'Hokuto Goh-Sho-Ha!!'"), m_name);
        else
            msg_format(_("%^s「受けてみい！！天将奔烈！！！」", "%^s says, 'Tensho-Honretsu!!'"), m_name);
        return true;
    }
    return false;
}

/*!
 * @brief RF4_BR_*の処理。各種ブレス。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param GF_TYPE ブレスの属性
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF4_BREATH(PlayerType *player_ptr, int GF_TYPE, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam, drs_type = 0;
    concptr type_s;
    bool smart_learn_aux = true;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    bool known = monster_near_player(floor_ptr, m_idx, t_idx);
    bool see_either = see_monster(player_ptr, m_idx) || see_monster(player_ptr, t_idx);
    bool mon_to_mon = (TARGET_TYPE == MONSTER_TO_MONSTER);
    bool mon_to_player = (TARGET_TYPE == MONSTER_TO_PLAYER);
    GAME_TEXT m_name[MAX_NLEN], t_name[MAX_NLEN];
    monster_name(player_ptr, m_idx, m_name);
    monster_name(player_ptr, t_idx, t_name);

    switch (GF_TYPE) {
    case GF_ACID:
        dam = monspell_damage(player_ptr, RF_ABILITY::BR_ACID, m_idx, DAM_ROLL);
        type_s = _("酸", "acid");
        drs_type = DRS_ACID;
        break;
    case GF_ELEC:
        dam = monspell_damage(player_ptr, RF_ABILITY::BR_ELEC, m_idx, DAM_ROLL);
        type_s = _("稲妻", "lightning");
        drs_type = DRS_ELEC;
        break;
    case GF_FIRE:
        dam = monspell_damage(player_ptr, RF_ABILITY::BR_FIRE, m_idx, DAM_ROLL);
        type_s = _("火炎", "fire");
        drs_type = DRS_FIRE;
        break;
    case GF_COLD:
        dam = monspell_damage(player_ptr, RF_ABILITY::BR_COLD, m_idx, DAM_ROLL);
        type_s = _("冷気", "frost");
        drs_type = DRS_COLD;
        break;
    case GF_POIS:
        dam = monspell_damage(player_ptr, RF_ABILITY::BR_POIS, m_idx, DAM_ROLL);
        type_s = _("ガス", "gas");
        drs_type = DRS_POIS;
        break;
    case GF_NETHER:
        dam = monspell_damage(player_ptr, RF_ABILITY::BR_NETH, m_idx, DAM_ROLL);
        type_s = _("地獄", "nether");
        drs_type = DRS_NETH;
        break;
    case GF_LITE:
        dam = monspell_damage(player_ptr, RF_ABILITY::BR_LITE, m_idx, DAM_ROLL);
        type_s = _("閃光", "light");
        drs_type = DRS_LITE;
        break;
    case GF_DARK:
        dam = monspell_damage(player_ptr, RF_ABILITY::BR_DARK, m_idx, DAM_ROLL);
        type_s = _("暗黒", "darkness");
        drs_type = DRS_DARK;
        break;
    case GF_CONFUSION:
        dam = monspell_damage(player_ptr, RF_ABILITY::BR_CONF, m_idx, DAM_ROLL);
        type_s = _("混乱", "confusion");
        drs_type = DRS_CONF;
        break;
    case GF_SOUND:
        dam = monspell_damage(player_ptr, RF_ABILITY::BR_SOUN, m_idx, DAM_ROLL);
        type_s = _("轟音", "sound");
        drs_type = DRS_SOUND;
        break;
    case GF_CHAOS:
        dam = monspell_damage(player_ptr, RF_ABILITY::BR_CHAO, m_idx, DAM_ROLL);
        type_s = _("カオス", "chaos");
        drs_type = DRS_CHAOS;
        break;
    case GF_DISENCHANT:
        dam = monspell_damage(player_ptr, RF_ABILITY::BR_DISE, m_idx, DAM_ROLL);
        type_s = _("劣化", "disenchantment");
        drs_type = DRS_DISEN;
        break;
    case GF_NEXUS:
        dam = monspell_damage(player_ptr, RF_ABILITY::BR_NEXU, m_idx, DAM_ROLL);
        type_s = _("因果混乱", "nexus");
        drs_type = DRS_NEXUS;
        break;
    case GF_TIME:
        dam = monspell_damage(player_ptr, RF_ABILITY::BR_TIME, m_idx, DAM_ROLL);
        type_s = _("時間逆転", "time");
        smart_learn_aux = false;
        break;
    case GF_INERTIAL:
        dam = monspell_damage(player_ptr, RF_ABILITY::BR_INER, m_idx, DAM_ROLL);
        type_s = _("遅鈍", "inertia");
        smart_learn_aux = false;
        break;
    case GF_GRAVITY:
        dam = monspell_damage(player_ptr, RF_ABILITY::BR_GRAV, m_idx, DAM_ROLL);
        type_s = _("重力", "gravity");
        smart_learn_aux = false;
        break;
    case GF_SHARDS:
        dam = monspell_damage(player_ptr, RF_ABILITY::BR_SHAR, m_idx, DAM_ROLL);
        type_s = _("破片", "shards");
        drs_type = DRS_SHARD;
        break;
    case GF_PLASMA:
        dam = monspell_damage(player_ptr, RF_ABILITY::BR_PLAS, m_idx, DAM_ROLL);
        type_s = _("プラズマ", "plasma");
        smart_learn_aux = false;
        break;
    case GF_FORCE:
        dam = monspell_damage(player_ptr, RF_ABILITY::BR_FORC, m_idx, DAM_ROLL);
        type_s = _("フォース", "force");
        smart_learn_aux = false;
        break;
    case GF_MANA:
        dam = monspell_damage(player_ptr, RF_ABILITY::BR_MANA, m_idx, DAM_ROLL);
        type_s = _("魔力", "mana");
        smart_learn_aux = false;
        break;
    case GF_NUKE:
        dam = monspell_damage(player_ptr, RF_ABILITY::BR_NUKE, m_idx, DAM_ROLL);
        type_s = _("放射性廃棄物", "toxic waste");
        drs_type = DRS_POIS;
        break;
    case GF_DISINTEGRATE:
        dam = monspell_damage(player_ptr, RF_ABILITY::BR_DISI, m_idx, DAM_ROLL);
        type_s = _("分解", "disintegration");
        smart_learn_aux = false;
        break;
    default:
        /* Do not reach here */
        dam = 0;
        type_s = _("不明", "Unknown");
        smart_learn_aux = false;
        break;
    }

    if (mon_to_player || (mon_to_mon && known && see_either))
        disturb(player_ptr, true, true);

    if (!spell_RF4_BREATH_special_message(m_ptr->r_idx, GF_TYPE, m_name)) {
        if (player_ptr->blind) {
            if (mon_to_player || (mon_to_mon && known && see_either))
                msg_format(_("%^sが何かのブレスを吐いた。", "%^s breathes."), m_name);
        } else {
            if (mon_to_player) {
                msg_format(_("%^sが%^sのブレスを吐いた。", "%^s breathes %^s."), m_name, type_s);
            } else if (mon_to_mon && known && see_either) {
                _(msg_format("%^sが%^sに%^sのブレスを吐いた。", m_name, t_name, type_s), msg_format("%^s breathes %^s at %^s.", m_name, type_s, t_name));
            }
        }
    }

    if (mon_to_mon && known && !see_either)
        floor_ptr->monster_noise = true;

    if (known || see_either)
        sound(SOUND_BREATH);

    const auto proj_res = breath(player_ptr, y, x, m_idx, GF_TYPE, dam, 0, true, TARGET_TYPE);
    if (smart_learn_aux && mon_to_player)
        update_smart_learn(player_ptr, m_idx, drs_type);

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}
