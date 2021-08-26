/*!
 * @brief ベースアイテムを強化する処理
 * @date 2020/06/03
 * @author Hourier
 * @todo 少し長い。要分割
 */

#include "object-enchant/apply-magic.h"
#include "artifact/fixed-art-generator.h"
#include "artifact/fixed-art-types.h"
#include "dungeon/dungeon.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/apply-magic-amulet.h"
#include "object-enchant/apply-magic-armor.h"
#include "object-enchant/apply-magic-boots.h"
#include "object-enchant/apply-magic-cloak.h"
#include "object-enchant/apply-magic-crown.h"
#include "object-enchant/apply-magic-gloves.h"
#include "object-enchant/apply-magic-helm.h"
#include "object-enchant/apply-magic-others.h"
#include "object-enchant/apply-magic-ring.h"
#include "object-enchant/apply-magic-shield.h"
#include "object-enchant/apply-magic-weapon.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/object-curse.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/trg-types.h"
#include "object-hook/hook-enchant.h"
#include "object/object-kind.h"
#include "player/player-status-flags.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "world/world.h"

/*!
 * @brief 生成されたベースアイテムに魔法的な強化を与えるメインルーチン
 * Complete the "creation" of an object by applying "magic" to the item
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param lev 生成基準階
 * @param mode 生成オプション
 * @details
 * エゴ＆アーティファクトの生成、呪い、pval強化
 */
void apply_magic_to_object(player_type *owner_ptr, object_type *o_ptr, DEPTH lev, BIT_FLAGS mode)
{
    if (owner_ptr->pseikaku == PERSONALITY_MUNCHKIN)
        lev += randint0(owner_ptr->lev / 2 + 10);
    if (lev > MAX_DEPTH - 1)
        lev = MAX_DEPTH - 1;

    int f1 = lev + 10;
    if (f1 > d_info[owner_ptr->dungeon_idx].obj_good)
        f1 = d_info[owner_ptr->dungeon_idx].obj_good;

    int f2 = f1 * 2 / 3;
    if ((owner_ptr->pseikaku != PERSONALITY_MUNCHKIN) && (f2 > d_info[owner_ptr->dungeon_idx].obj_great))
        f2 = d_info[owner_ptr->dungeon_idx].obj_great;

    if (has_good_luck(owner_ptr)) {
        f1 += 5;
        f2 += 2;
    } else if (owner_ptr->muta.has(MUTA::BAD_LUCK)) {
        f1 -= 5;
        f2 -= 2;
    }

    int power = 0;
    if ((mode & AM_GOOD) || magik(f1)) {
        power = 1;
        if ((mode & AM_GREAT) || magik(f2)) {
            power = 2;
            if (mode & AM_SPECIAL)
                power = 3;
        }
    } else if (magik(f1)) {
        power = -1;
        if (magik(f2))
            power = -2;
    }
    if (mode & AM_CURSED) {
        if (power > 0) {
            power = 0 - power;
        } else {
            power--;
        }
    }

    int rolls = 0;
    if (power >= 2)
        rolls = 1;

    if (mode & (AM_GREAT | AM_SPECIAL))
        rolls = 4;
    if ((mode & AM_NO_FIXED_ART) || o_ptr->name1)
        rolls = 0;

    for (int i = 0; i < rolls; i++) {
        if (make_artifact(owner_ptr, o_ptr))
            break;
        if (has_good_luck(owner_ptr) && one_in_(77)) {
            if (make_artifact(owner_ptr, o_ptr))
                break;
        }
    }

    if (object_is_fixed_artifact(o_ptr)) {
        artifact_type *a_ptr = apply_artifact(owner_ptr, o_ptr);
        a_ptr->cur_num = 1;
        if (current_world_ptr->character_dungeon)
            a_ptr->floor_id = owner_ptr->floor_id;
        return;
    }

    switch (o_ptr->tval) {
    case TV_DIGGING:
    case TV_HAFTED:
    case TV_BOW:
    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT:
        if (power != 0) {
            apply_magic_weapon(owner_ptr, o_ptr, lev, power);
        }

        break;
    case TV_POLEARM:
        if ((power != 0) && (o_ptr->sval != SV_DEATH_SCYTHE)) {
            apply_magic_weapon(owner_ptr, o_ptr, lev, power);
        }

        break;
    case TV_SWORD:
        if ((power != 0) && (o_ptr->sval != SV_POISON_NEEDLE)) {
            apply_magic_weapon(owner_ptr, o_ptr, lev, power);
        }

        break;
    case TV_SHIELD:
        ShieldEnchanter(owner_ptr, o_ptr, lev, power).apply_magic();
        break;
    case TV_CLOAK:
        CloakEnchanter(owner_ptr, o_ptr, lev, power).apply_magic();
        break;
    case TV_HELM:
        HelmEnchanter(owner_ptr, o_ptr, lev, power).apply_magic();
        break;
    case TV_CROWN:
        CrownEnchanter(owner_ptr, o_ptr, lev, power).apply_magic();
        break;
    case TV_BOOTS:
        BootsEnchanter(owner_ptr, o_ptr, lev, power).apply_magic();
        break;
    case TV_DRAG_ARMOR:
    case TV_HARD_ARMOR:
    case TV_SOFT_ARMOR:
        ArmorEnchanter(owner_ptr, o_ptr, lev, power).apply_magic();
        break;
    case TV_GLOVES:
        GlovesEnchanter(owner_ptr, o_ptr, lev, power).apply_magic();
        break;
    case TV_RING:
        RingEnchanter(owner_ptr, o_ptr, lev, power).apply_magic();
        break;
    case TV_AMULET:
        AmuletEnchanter(owner_ptr, o_ptr, lev, power).apply_magic();
        break;
    default:
        apply_magic_others(owner_ptr, o_ptr, power);
        break;
    }

    if ((o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_ABUNAI_MIZUGI) && (owner_ptr->pseikaku == PERSONALITY_SEXY)) {
        o_ptr->pval = 3;
        add_flag(o_ptr->art_flags, TR_STR);
        add_flag(o_ptr->art_flags, TR_INT);
        add_flag(o_ptr->art_flags, TR_WIS);
        add_flag(o_ptr->art_flags, TR_DEX);
        add_flag(o_ptr->art_flags, TR_CON);
        add_flag(o_ptr->art_flags, TR_CHR);
    }

    if (object_is_ego(o_ptr)) {
        apply_ego(o_ptr, lev);
        return;
    }

    if (o_ptr->k_idx) {
        object_kind *k_ptr = &k_info[o_ptr->k_idx];
        if (!k_info[o_ptr->k_idx].cost)
            o_ptr->ident |= (IDENT_BROKEN);

        if (k_ptr->gen_flags.has(TRG::CURSED))
            o_ptr->curse_flags.set(TRC::CURSED);
        if (k_ptr->gen_flags.has(TRG::HEAVY_CURSE))
            o_ptr->curse_flags.set(TRC::HEAVY_CURSE);
        if (k_ptr->gen_flags.has(TRG::PERMA_CURSE))
            o_ptr->curse_flags.set(TRC::PERMA_CURSE);
        if (k_ptr->gen_flags.has(TRG::RANDOM_CURSE0))
            o_ptr->curse_flags.set(get_curse(0, o_ptr));
        if (k_ptr->gen_flags.has(TRG::RANDOM_CURSE1))
            o_ptr->curse_flags.set(get_curse(1, o_ptr));
        if (k_ptr->gen_flags.has(TRG::RANDOM_CURSE2))
            o_ptr->curse_flags.set(get_curse(2, o_ptr));
    }
}
