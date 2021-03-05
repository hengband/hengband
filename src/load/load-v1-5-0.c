﻿/*!
 * @brief 変愚蛮怒 v1.5.0以前の旧いセーブデータを読み込む処理
 * @date 2020/07/04
 * @author Hourier
 * @details 互換性を最大限に確保するため、基本的に関数分割は行わないものとする.
 */

#include "load/load-v1-5-0.h"
#include "cmd-item/cmd-smith.h"
#include "dungeon/dungeon.h"
#include "game-option/birth-options.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "load/angband-version-comparer.h"
#include "load/item-loader.h"
#include "load/load-util.h"
#include "load/monster-loader.h"
#include "load/old-feature-types.h"
#include "mind/mind-weaponsmith.h"
#include "monster-floor/monster-move.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/old-ego-extra-values.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/trg-types.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object/object-kind-hook.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-lite-types.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "world/world-object.h"
#include "world/world.h"

/* Old hidden trap flag */
static const BIT_FLAGS CAVE_TRAP = 0x8000;

const int OLD_QUEST_WATER_CAVE = 18; // 湖の洞窟.
const int QUEST_OLD_CASTLE = 27; // 古い城.
const int QUEST_ROYAL_CRYPT = 28; // 王家の墓.

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
    o_ptr->tval = static_cast<tval_type>(tmp8u);
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
    rd_byte(&tmp8u);
    o_ptr->xtra2 = tmp8u;

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
        if (has_flag(o_ptr->art_flags, TR_MAGIC_MASTERY)) {
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

static void move_RF3_to_RFR(monster_race *r_ptr, const BIT_FLAGS rf3, const BIT_FLAGS rfr)
{
    if (r_ptr->r_flags3 & rf3) {
        r_ptr->r_flags3 &= ~rf3;
        r_ptr->r_flagsr |= rfr;
    }
}

static void move_RF4_BR_to_RFR(monster_race *r_ptr, const BIT_FLAGS rf4_br, const BIT_FLAGS rfr)
{
    if (r_ptr->r_flags4 & rf4_br)
        r_ptr->r_flagsr |= rfr;
}

/*!
 * @brief モンスターの思い出を読み込む
 * @param r_ptr モンスター種族情報への参照ポインタ
 * @param r_idx モンスター種族ID
 * @details 本来はr_idxからr_ptrを決定可能だが、互換性を優先するため元コードのままとする
 */
void set_old_lore(monster_race *r_ptr, const MONRACE_IDX r_idx)
{
    r_ptr->r_flagsr = 0L;
    move_RF3_to_RFR(r_ptr, RF3_IM_ACID, RFR_IM_ACID);
    move_RF3_to_RFR(r_ptr, RF3_IM_ELEC, RFR_IM_ELEC);
    move_RF3_to_RFR(r_ptr, RF3_IM_FIRE, RFR_IM_FIRE);
    move_RF3_to_RFR(r_ptr, RF3_IM_COLD, RFR_IM_COLD);
    move_RF3_to_RFR(r_ptr, RF3_IM_POIS, RFR_IM_POIS);
    move_RF3_to_RFR(r_ptr, RF3_RES_TELE, RFR_RES_TELE);
    move_RF3_to_RFR(r_ptr, RF3_RES_NETH, RFR_RES_NETH);
    move_RF3_to_RFR(r_ptr, RF3_RES_WATE, RFR_RES_WATE);
    move_RF3_to_RFR(r_ptr, RF3_RES_PLAS, RFR_RES_PLAS);
    move_RF3_to_RFR(r_ptr, RF3_RES_NEXU, RFR_RES_NEXU);
    move_RF3_to_RFR(r_ptr, RF3_RES_DISE, RFR_RES_DISE);
    move_RF3_to_RFR(r_ptr, RF3_RES_ALL, RFR_RES_ALL);

    move_RF4_BR_to_RFR(r_ptr, RF4_BR_LITE, RFR_RES_LITE);
    move_RF4_BR_to_RFR(r_ptr, RF4_BR_DARK, RFR_RES_DARK);
    move_RF4_BR_to_RFR(r_ptr, RF4_BR_SOUN, RFR_RES_SOUN);
    move_RF4_BR_to_RFR(r_ptr, RF4_BR_CHAO, RFR_RES_CHAO);
    move_RF4_BR_to_RFR(r_ptr, RF4_BR_TIME, RFR_RES_TIME);
    move_RF4_BR_to_RFR(r_ptr, RF4_BR_INER, RFR_RES_INER);
    move_RF4_BR_to_RFR(r_ptr, RF4_BR_GRAV, RFR_RES_GRAV);
    move_RF4_BR_to_RFR(r_ptr, RF4_BR_SHAR, RFR_RES_SHAR);
    move_RF4_BR_to_RFR(r_ptr, RF4_BR_WALL, RFR_RES_WALL);

    if (r_ptr->r_flags4 & RF4_BR_CONF)
        r_ptr->r_flags3 |= RF3_NO_CONF;

    if (r_idx == MON_STORMBRINGER)
        r_ptr->r_flagsr |= RFR_RES_CHAO;

    if (r_ptr->r_flags3 & RF3_ORC)
        r_ptr->r_flagsr |= RFR_RES_DARK;
}

/*!
 * @brief ダンジョン情報を読み込む / Read the dungeon (old method)
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * The monsters/objects must be loaded in the same order
 * that they were stored, since the actual indexes matter.
 */
errr rd_dungeon_old(player_type *player_ptr)
{
    s16b tmp16s;
    rd_s16b(&tmp16s);
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    floor_ptr->dun_level = (DEPTH)tmp16s;
    if (z_older_than(10, 3, 8))
        player_ptr->dungeon_idx = DUNGEON_ANGBAND;
    else {
        byte tmp8u;
        rd_byte(&tmp8u);
        player_ptr->dungeon_idx = (IDX)tmp8u;
    }

    floor_ptr->base_level = floor_ptr->dun_level;
    rd_s16b(&tmp16s);
    floor_ptr->base_level = (DEPTH)tmp16s;

    rd_s16b(&tmp16s);
    floor_ptr->num_repro = (MONSTER_NUMBER)tmp16s;
    rd_s16b(&tmp16s);
    player_ptr->y = (POSITION)tmp16s;
    rd_s16b(&tmp16s);
    player_ptr->x = (POSITION)tmp16s;
    if (z_older_than(10, 3, 13) && !floor_ptr->dun_level && !floor_ptr->inside_arena) {
        player_ptr->y = 33;
        player_ptr->x = 131;
    }
    rd_s16b(&tmp16s);
    floor_ptr->height = (POSITION)tmp16s;
    rd_s16b(&tmp16s);
    floor_ptr->width = (POSITION)tmp16s;
    rd_s16b(&tmp16s); /* max_panel_rows */
    rd_s16b(&tmp16s); /* max_panel_cols */

    int ymax = floor_ptr->height;
    int xmax = floor_ptr->width;

    for (int x = 0, y = 0; y < ymax;) {
        u16b info;
        byte count;
        rd_byte(&count);
        if (z_older_than(10, 3, 6)) {
            byte tmp8u;
            rd_byte(&tmp8u);
            info = (u16b)tmp8u;
        } else {
            rd_u16b(&info);
            info &= ~(CAVE_LITE | CAVE_VIEW | CAVE_MNLT | CAVE_MNDK);
        }

        for (int i = count; i > 0; i--) {
            grid_type *g_ptr;
            g_ptr = &floor_ptr->grid_array[y][x];
            g_ptr->info = info;
            if (++x >= xmax) {
                x = 0;
                if (++y >= ymax)
                    break;
            }
        }
    }

    for (int x = 0, y = 0; y < ymax;) {
        byte count;
        rd_byte(&count);
        byte tmp8u;
        rd_byte(&tmp8u);
        for (int i = count; i > 0; i--) {
            grid_type *g_ptr;
            g_ptr = &floor_ptr->grid_array[y][x];
            g_ptr->feat = (s16b)tmp8u;
            if (++x >= xmax) {
                x = 0;
                if (++y >= ymax)
                    break;
            }
        }
    }

    for (int x = 0, y = 0; y < ymax;) {
        byte count;
        rd_byte(&count);
        byte tmp8u;
        rd_byte(&tmp8u);
        for (int i = count; i > 0; i--) {
            grid_type *g_ptr;
            g_ptr = &floor_ptr->grid_array[y][x];
            g_ptr->mimic = (s16b)tmp8u;
            if (++x >= xmax) {
                x = 0;
                if (++y >= ymax)
                    break;
            }
        }
    }

    for (int x = 0, y = 0; y < ymax;) {
        byte count;
        rd_byte(&count);
        rd_s16b(&tmp16s);
        for (int i = count; i > 0; i--) {
            grid_type *g_ptr;
            g_ptr = &floor_ptr->grid_array[y][x];
            g_ptr->special = tmp16s;
            if (++x >= xmax) {
                x = 0;
                if (++y >= ymax)
                    break;
            }
        }
    }

    if (z_older_than(11, 0, 99)) {
        for (int y = 0; y < ymax; y++) {
            for (int x = 0; x < xmax; x++) {
                floor_ptr->grid_array[y][x].info &= ~(CAVE_MASK);
            }
        }
    }

    if (h_older_than(1, 1, 1, 0)) {
        for (int y = 0; y < ymax; y++) {
            for (int x = 0; x < xmax; x++) {
                grid_type *g_ptr;
                g_ptr = &floor_ptr->grid_array[y][x];

                /* Very old */
                if (g_ptr->feat == OLD_FEAT_INVIS) {
                    g_ptr->feat = feat_floor;
                    g_ptr->info |= CAVE_TRAP;
                }

                /* Older than 1.1.1 */
                if (g_ptr->feat == OLD_FEAT_MIRROR) {
                    g_ptr->feat = feat_floor;
                    g_ptr->info |= CAVE_OBJECT;
                }
            }
        }
    }

    if (h_older_than(1, 3, 1, 0)) {
        for (int y = 0; y < ymax; y++) {
            for (int x = 0; x < xmax; x++) {
                grid_type *g_ptr;
                g_ptr = &floor_ptr->grid_array[y][x];

                /* Old CAVE_IN_MIRROR flag */
                if (g_ptr->info & CAVE_OBJECT) {
                    g_ptr->mimic = feat_mirror;
                } else if ((g_ptr->feat == OLD_FEAT_RUNE_EXPLOSION) || (g_ptr->feat == OLD_FEAT_RUNE_PROTECTION)) {
                    g_ptr->info |= CAVE_OBJECT;
                    g_ptr->mimic = g_ptr->feat;
                    g_ptr->feat = feat_floor;
                } else if (g_ptr->info & CAVE_TRAP) {
                    g_ptr->info &= ~CAVE_TRAP;
                    g_ptr->mimic = g_ptr->feat;
                    g_ptr->feat = choose_random_trap(player_ptr);
                } else if (g_ptr->feat == OLD_FEAT_INVIS) {
                    g_ptr->mimic = feat_floor;
                    g_ptr->feat = feat_trap_open;
                }
            }
        }
    }

    /* Quest 18 was removed */
    if (!vanilla_town) {
        for (int y = 0; y < ymax; y++) {
            for (int x = 0; x < xmax; x++) {
                grid_type *g_ptr;
                g_ptr = &floor_ptr->grid_array[y][x];

                if ((g_ptr->special == OLD_QUEST_WATER_CAVE) && !floor_ptr->dun_level) {
                    if (g_ptr->feat == OLD_FEAT_QUEST_ENTER) {
                        g_ptr->feat = feat_tree;
                        g_ptr->special = 0;
                    } else if (g_ptr->feat == OLD_FEAT_BLDG_1) {
                        g_ptr->special = lite_town ? QUEST_OLD_CASTLE : QUEST_ROYAL_CRYPT;
                    }
                } else if ((g_ptr->feat == OLD_FEAT_QUEST_EXIT) && (floor_ptr->inside_quest == OLD_QUEST_WATER_CAVE)) {
                    g_ptr->feat = feat_up_stair;
                    g_ptr->special = 0;
                }
            }
        }
    }

    u16b limit;
    rd_u16b(&limit);
    if (limit > current_world_ptr->max_o_idx) {
        load_note(format(_("アイテムの配列が大きすぎる(%d)！", "Too many (%d) object entries!"), limit));
        return (151);
    }

    for (int i = 1; i < limit; i++) {
        OBJECT_IDX o_idx = o_pop(floor_ptr);
        if (i != o_idx) {
            load_note(format(_("アイテム配置エラー (%d <> %d)", "Object allocation error (%d <> %d)"), i, o_idx));
            return (152);
        }

        object_type *o_ptr;
        o_ptr = &floor_ptr->o_list[o_idx];
        rd_item(player_ptr, o_ptr);
        if (object_is_held_monster(o_ptr)) {
            monster_type *m_ptr;
            m_ptr = &floor_ptr->m_list[o_ptr->held_m_idx];
            o_ptr->next_o_idx = m_ptr->hold_o_idx;
            m_ptr->hold_o_idx = o_idx;
            continue;
        }

        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[o_ptr->iy][o_ptr->ix];
        o_ptr->next_o_idx = g_ptr->o_idx;
        g_ptr->o_idx = o_idx;
    }

    rd_u16b(&limit);
    if (limit > current_world_ptr->max_m_idx) {
        load_note(format(_("モンスターの配列が大きすぎる(%d)！", "Too many (%d) monster entries!"), limit));
        return (161);
    }

    for (int i = 1; i < limit; i++) {
        MONSTER_IDX m_idx;
        monster_type *m_ptr;
        m_idx = m_pop(floor_ptr);
        if (i != m_idx) {
            load_note(format(_("モンスター配置エラー (%d <> %d)", "Monster allocation error (%d <> %d)"), i, m_idx));
            return (162);
        }

        m_ptr = &floor_ptr->m_list[m_idx];
        rd_monster(player_ptr, m_ptr);
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[m_ptr->fy][m_ptr->fx];
        g_ptr->m_idx = m_idx;
        real_r_ptr(m_ptr)->cur_num++;
    }

    if (z_older_than(10, 3, 13) && !floor_ptr->dun_level && !floor_ptr->inside_arena)
        current_world_ptr->character_dungeon = FALSE;
    else
        current_world_ptr->character_dungeon = TRUE;

    return 0;
}
