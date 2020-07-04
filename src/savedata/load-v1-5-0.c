/*!
 * @brief 変愚蛮怒 v1.5.0以前の旧いセーブデータを読み込む処理
 * @date 2020/07/04
 * @author Hourier
 * @details 互換性を最大限に確保するため、基本的に関数分割は行わないものとする.
 */

#include "savedata/load-v1-5-0.h"
#include "cmd-item/cmd-smith.h"
#include "game-option/birth-options.h"
#include "mind/mind-weaponsmith.h"
#include "monster-floor/monster-move.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-flag-types.h"
#include "object-enchant/artifact.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/old-ego-extra-values.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/trg-types.h"
#include "object-hook/hook-enchant.h"
#include "object/object-kind-hook.h"
#include "savedata/angband-version-comparer.h"
#include "savedata/load-util.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-lite-types.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "world/world.h"

/*!
 * @brief アイテムオブジェクト1件を読み込む / Read an object
 * @param o_ptr アイテムオブジェクト読み取り先ポインタ
 * @return なし
 */
void rd_item_old(player_type *player_ptr, object_type *o_ptr)
{
    rd_s16b(&o_ptr->k_idx);

    byte tmp8u;
    rd_byte(&tmp8u);
    o_ptr->iy = (POSITION)tmp8u;
    rd_byte(&tmp8u);
    o_ptr->ix = (POSITION)tmp8u;

    /* Type/Subtype */
    rd_byte(&tmp8u);
    o_ptr->tval = tmp8u;
    rd_byte(&tmp8u);
    o_ptr->sval = tmp8u;

    if (z_older_than(10, 4, 4)) {
        if (o_ptr->tval == 100)
            o_ptr->tval = TV_GOLD;
        if (o_ptr->tval == 98)
            o_ptr->tval = TV_MUSIC_BOOK;
        if (o_ptr->tval == 110)
            o_ptr->tval = TV_HISSATSU_BOOK;
    }

    rd_s16b(&o_ptr->pval);
    rd_byte(&o_ptr->discount);
    rd_byte(&tmp8u);
    o_ptr->number = (ITEM_NUMBER)tmp8u;

    s16b tmp16s;
    rd_s16b(&tmp16s);
    o_ptr->weight = tmp16s;

    rd_byte(&tmp8u);
    o_ptr->name1 = tmp8u;

    rd_byte(&tmp8u);
    o_ptr->name2 = tmp8u;

    rd_s16b(&o_ptr->timeout);
    rd_s16b(&o_ptr->to_h);
    rd_s16b(&tmp16s);
    o_ptr->to_d = tmp16s;

    rd_s16b(&o_ptr->to_a);
    rd_s16b(&o_ptr->ac);
    rd_byte(&tmp8u);
    o_ptr->dd = tmp8u;

    rd_byte(&tmp8u);
    o_ptr->ds = tmp8u;

    rd_byte(&o_ptr->ident);
    rd_byte(&o_ptr->marked);
    rd_u32b(&o_ptr->art_flags[0]);
    rd_u32b(&o_ptr->art_flags[1]);
    rd_u32b(&o_ptr->art_flags[2]);
    if (h_older_than(1, 3, 0, 0))
        o_ptr->art_flags[3] = 0L;
    else
        rd_u32b(&o_ptr->art_flags[3]);

    if (h_older_than(1, 3, 0, 0)) {
        if (o_ptr->name2 == EGO_TELEPATHY)
            add_flag(o_ptr->art_flags, TR_TELEPATHY);
    }

    if (z_older_than(11, 0, 11)) {
        o_ptr->curse_flags = 0L;
        if (o_ptr->ident & 0x40) {
            o_ptr->curse_flags |= TRC_CURSED;
            if (o_ptr->art_flags[2] & 0x40000000L)
                o_ptr->curse_flags |= TRC_HEAVY_CURSE;
            if (o_ptr->art_flags[2] & 0x80000000L)
                o_ptr->curse_flags |= TRC_PERMA_CURSE;
            if (object_is_fixed_artifact(o_ptr)) {
                artifact_type *a_ptr = &a_info[o_ptr->name1];
                if (a_ptr->gen_flags & (TRG_HEAVY_CURSE))
                    o_ptr->curse_flags |= TRC_HEAVY_CURSE;
                if (a_ptr->gen_flags & (TRG_PERMA_CURSE))
                    o_ptr->curse_flags |= TRC_PERMA_CURSE;
            } else if (object_is_ego(o_ptr)) {
                ego_item_type *e_ptr = &e_info[o_ptr->name2];
                if (e_ptr->gen_flags & (TRG_HEAVY_CURSE))
                    o_ptr->curse_flags |= TRC_HEAVY_CURSE;
                if (e_ptr->gen_flags & (TRG_PERMA_CURSE))
                    o_ptr->curse_flags |= TRC_PERMA_CURSE;
            }
        }
        o_ptr->art_flags[2] &= (0x1FFFFFFFL);
    } else {
        rd_u32b(&o_ptr->curse_flags);
    }

    rd_s16b(&o_ptr->held_m_idx);
    rd_byte(&o_ptr->xtra1);
    rd_byte(&o_ptr->xtra2);

    if (z_older_than(11, 0, 10)) {
        if (o_ptr->xtra1 == EGO_XTRA_SUSTAIN) {
            switch (o_ptr->xtra2 % 6) {
            case 0:
                add_flag(o_ptr->art_flags, TR_SUST_STR);
                break;
            case 1:
                add_flag(o_ptr->art_flags, TR_SUST_INT);
                break;
            case 2:
                add_flag(o_ptr->art_flags, TR_SUST_WIS);
                break;
            case 3:
                add_flag(o_ptr->art_flags, TR_SUST_DEX);
                break;
            case 4:
                add_flag(o_ptr->art_flags, TR_SUST_CON);
                break;
            case 5:
                add_flag(o_ptr->art_flags, TR_SUST_CHR);
                break;
            }
            o_ptr->xtra2 = 0;
        } else if (o_ptr->xtra1 == EGO_XTRA_POWER) {
            switch (o_ptr->xtra2 % 11) {
            case 0:
                add_flag(o_ptr->art_flags, TR_RES_BLIND);
                break;
            case 1:
                add_flag(o_ptr->art_flags, TR_RES_CONF);
                break;
            case 2:
                add_flag(o_ptr->art_flags, TR_RES_SOUND);
                break;
            case 3:
                add_flag(o_ptr->art_flags, TR_RES_SHARDS);
                break;
            case 4:
                add_flag(o_ptr->art_flags, TR_RES_NETHER);
                break;
            case 5:
                add_flag(o_ptr->art_flags, TR_RES_NEXUS);
                break;
            case 6:
                add_flag(o_ptr->art_flags, TR_RES_CHAOS);
                break;
            case 7:
                add_flag(o_ptr->art_flags, TR_RES_DISEN);
                break;
            case 8:
                add_flag(o_ptr->art_flags, TR_RES_POIS);
                break;
            case 9:
                add_flag(o_ptr->art_flags, TR_RES_DARK);
                break;
            case 10:
                add_flag(o_ptr->art_flags, TR_RES_LITE);
                break;
            }
            o_ptr->xtra2 = 0;
        } else if (o_ptr->xtra1 == EGO_XTRA_ABILITY) {
            switch (o_ptr->xtra2 % 8) {
            case 0:
                add_flag(o_ptr->art_flags, TR_LEVITATION);
                break;
            case 1:
                add_flag(o_ptr->art_flags, TR_LITE_1);
                break;
            case 2:
                add_flag(o_ptr->art_flags, TR_SEE_INVIS);
                break;
            case 3:
                add_flag(o_ptr->art_flags, TR_WARNING);
                break;
            case 4:
                add_flag(o_ptr->art_flags, TR_SLOW_DIGEST);
                break;
            case 5:
                add_flag(o_ptr->art_flags, TR_REGEN);
                break;
            case 6:
                add_flag(o_ptr->art_flags, TR_FREE_ACT);
                break;
            case 7:
                add_flag(o_ptr->art_flags, TR_HOLD_EXP);
                break;
            }
            o_ptr->xtra2 = 0;
        }
        o_ptr->xtra1 = 0;
    }

    if (z_older_than(10, 2, 3)) {
        o_ptr->xtra3 = 0;
        o_ptr->xtra4 = 0;
        o_ptr->xtra5 = 0;
        if ((o_ptr->tval == TV_CHEST) || (o_ptr->tval == TV_CAPTURE)) {
            o_ptr->xtra3 = o_ptr->xtra1;
            o_ptr->xtra1 = 0;
        }
        if (o_ptr->tval == TV_CAPTURE) {
            if (r_info[o_ptr->pval].flags1 & RF1_FORCE_MAXHP)
                o_ptr->xtra5 = maxroll(r_info[o_ptr->pval].hdice, r_info[o_ptr->pval].hside);
            else
                o_ptr->xtra5 = damroll(r_info[o_ptr->pval].hdice, r_info[o_ptr->pval].hside);
            if (ironman_nightmare) {
                o_ptr->xtra5 = (s16b)MIN(30000, o_ptr->xtra5 * 2L);
            }
            o_ptr->xtra4 = o_ptr->xtra5;
        }
    } else {
        rd_byte(&o_ptr->xtra3);
        if (h_older_than(1, 3, 0, 1)) {
            if (object_is_smith(player_ptr, o_ptr) && o_ptr->xtra3 >= 1 + 96)
                o_ptr->xtra3 += -96 + MIN_SPECIAL_ESSENCE;
        }

        rd_s16b(&o_ptr->xtra4);
        rd_s16b(&o_ptr->xtra5);
    }

    if (z_older_than(11, 0, 5)
        && (((o_ptr->tval == TV_LITE) && ((o_ptr->sval == SV_LITE_TORCH) || (o_ptr->sval == SV_LITE_LANTERN))) || (o_ptr->tval == TV_FLASK))) {
        o_ptr->xtra4 = o_ptr->pval;
        o_ptr->pval = 0;
    }

    rd_byte(&o_ptr->feeling);

    char buf[128];
    rd_string(buf, sizeof(buf));
    if (buf[0])
        o_ptr->inscription = quark_add(buf);

    rd_string(buf, sizeof(buf));

    /* todo 元々このif文には末尾に";"が付いていた、バグかもしれない */
    if (buf[0])
        o_ptr->art_name = quark_add(buf);
    {
        s32b tmp32s;

        rd_s32b(&tmp32s);
        strip_bytes(tmp32s);
    }

    if ((o_ptr->k_idx >= 445) && (o_ptr->k_idx <= 479))
        return;

    if (z_older_than(10, 4, 10) && (o_ptr->name2 == EGO_YOIYAMI))
        o_ptr->k_idx = lookup_kind(TV_SOFT_ARMOR, SV_YOIYAMI_ROBE);

    if (z_older_than(10, 4, 9)) {
        if (have_flag(o_ptr->art_flags, TR_MAGIC_MASTERY)) {
            remove_flag(o_ptr->art_flags, TR_MAGIC_MASTERY);
            add_flag(o_ptr->art_flags, TR_DEC_MANA);
        }
    }

    if (object_is_fixed_artifact(o_ptr)) {
        artifact_type *a_ptr;
        a_ptr = &a_info[o_ptr->name1];
        if (!a_ptr->name)
            o_ptr->name1 = 0;
    }

    if (object_is_ego(o_ptr)) {
        ego_item_type *e_ptr;
        e_ptr = &e_info[o_ptr->name2];
        if (!e_ptr->name)
            o_ptr->name2 = 0;
    }
}

/*!
 * @brief モンスターを読み込む / Read a monster
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param m_ptr モンスター保存先ポインタ
 * @return なし
 */
void rd_monster_old(player_type *player_ptr, monster_type *m_ptr)
{
    rd_s16b(&m_ptr->r_idx);

    if (z_older_than(11, 0, 12))
        m_ptr->ap_r_idx = m_ptr->r_idx;
    else
        rd_s16b(&m_ptr->ap_r_idx);

    if (z_older_than(11, 0, 14)) {
        monster_race *r_ptr = &r_info[m_ptr->r_idx];

        m_ptr->sub_align = SUB_ALIGN_NEUTRAL;
        if (r_ptr->flags3 & RF3_EVIL)
            m_ptr->sub_align |= SUB_ALIGN_EVIL;
        if (r_ptr->flags3 & RF3_GOOD)
            m_ptr->sub_align |= SUB_ALIGN_GOOD;
    } else
        rd_byte(&m_ptr->sub_align);

    byte tmp8u;
    rd_byte(&tmp8u);
    m_ptr->fy = (POSITION)tmp8u;
    rd_byte(&tmp8u);
    m_ptr->fx = (POSITION)tmp8u;
    m_ptr->current_floor_ptr = player_ptr->current_floor_ptr;

    s16b tmp16s;
    rd_s16b(&tmp16s);
    m_ptr->hp = tmp16s;
    rd_s16b(&tmp16s);
    m_ptr->maxhp = tmp16s;

    if (z_older_than(11, 0, 5)) {
        m_ptr->max_maxhp = m_ptr->maxhp;
    } else {
        rd_s16b(&tmp16s);
        m_ptr->max_maxhp = (HIT_POINT)tmp16s;
    }
    if (h_older_than(2, 1, 2, 1)) {
        m_ptr->dealt_damage = 0;
    } else {
        rd_s32b(&m_ptr->dealt_damage);
    }

    rd_s16b(&m_ptr->mtimed[MTIMED_CSLEEP]);
    rd_byte(&tmp8u);
    m_ptr->mspeed = tmp8u;

    if (z_older_than(10, 4, 2)) {
        rd_byte(&tmp8u);
        m_ptr->energy_need = (s16b)tmp8u;
    } else
        rd_s16b(&m_ptr->energy_need);

    if (z_older_than(11, 0, 13))
        m_ptr->energy_need = 100 - m_ptr->energy_need;

    if (z_older_than(10, 0, 7)) {
        m_ptr->mtimed[MTIMED_FAST] = 0;
        m_ptr->mtimed[MTIMED_SLOW] = 0;
    } else {
        rd_byte(&tmp8u);
        m_ptr->mtimed[MTIMED_FAST] = (s16b)tmp8u;
        rd_byte(&tmp8u);
        m_ptr->mtimed[MTIMED_SLOW] = (s16b)tmp8u;
    }

    rd_byte(&tmp8u);
    m_ptr->mtimed[MTIMED_STUNNED] = (s16b)tmp8u;
    rd_byte(&tmp8u);
    m_ptr->mtimed[MTIMED_CONFUSED] = (s16b)tmp8u;
    rd_byte(&tmp8u);
    m_ptr->mtimed[MTIMED_MONFEAR] = (s16b)tmp8u;

    if (z_older_than(10, 0, 10)) {
        reset_target(m_ptr);
    } else if (z_older_than(10, 0, 11)) {
        rd_s16b(&tmp16s);
        reset_target(m_ptr);
    } else {
        rd_s16b(&tmp16s);
        m_ptr->target_y = (POSITION)tmp16s;
        rd_s16b(&tmp16s);
        m_ptr->target_x = (POSITION)tmp16s;
    }

    rd_byte(&tmp8u);
    m_ptr->mtimed[MTIMED_INVULNER] = (s16b)tmp8u;

    if (!(current_world_ptr->z_major == 2 && current_world_ptr->z_minor == 0 && current_world_ptr->z_patch == 6))
        rd_u32b(&m_ptr->smart);
    else
        m_ptr->smart = 0;

    u32b tmp32u;
    if (z_older_than(10, 4, 5)) {
        m_ptr->exp = 0;
    } else {
        rd_u32b(&tmp32u);
        m_ptr->exp = tmp32u;
    }

    if (z_older_than(10, 2, 2)) {
        if (m_ptr->r_idx < 0) {
            m_ptr->r_idx = (0 - m_ptr->r_idx);
            m_ptr->mflag2 |= MFLAG2_KAGE;
        }
    } else {
        rd_byte(&m_ptr->mflag2);
    }

    if (z_older_than(11, 0, 12)) {
        if (m_ptr->mflag2 & MFLAG2_KAGE)
            m_ptr->ap_r_idx = MON_KAGE;
    }

    if (z_older_than(10, 1, 3)) {
        m_ptr->nickname = 0;
    } else {
        char buf[128];
        rd_string(buf, sizeof(buf));
        if (buf[0])
            m_ptr->nickname = quark_add(buf);
    }

    rd_byte(&tmp8u);
}
