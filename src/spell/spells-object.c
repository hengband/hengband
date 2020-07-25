/*!
 * @brief アイテムに影響のある魔法の処理
 * @date 2019/01/22
 * @author deskull
 */

#include "spell/spells-object.h"
#include "action/action-limited.h"
#include "art-definition/art-weapon-types.h"
#include "autopick/autopick.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "game-option/disturbance-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "inventory/player-inventory.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "object-enchant/apply-magic.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/trg-types.h"
#include "object-hook/hook-armor.h"
#include "object-hook/hook-bow.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object-hook/hook-magic.h"
#include "object-hook/hook-weapon.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-flags.h"
#include "object/object-generator.h"
#include "object/object-kind-hook.h"
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "player/avatar.h"
#include "player/player-class.h"
#include "player/player-damage.h"
#include "player/player-status.h"
#include "racial/racial-android.h"
#include "spell-kind/spells-perception.h"
#include "status/bad-status-setter.h"
#include "sv-definition/sv-food-types.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "sv-definition/sv-staff-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

typedef struct {
    tval_type tval;
    OBJECT_SUBTYPE_VALUE sval;
    PERCENTAGE prob;
    byte flag;
} amuse_type;

/*!
 * @brief 装備強化処理の失敗率定数（千分率） /
 * Used by the "enchant" function (chance of failure)
 * (modified for Zangband, we need better stuff there...) -- TY
 * @return なし
 */
static int enchant_table[16] = { 0, 10, 50, 100, 200, 300, 400, 500, 650, 800, 950, 987, 993, 995, 998, 1000 };

/*
 * Scatter some "amusing" objects near the player
 */

#define AMS_NOTHING 0x00 /* No restriction */
#define AMS_NO_UNIQUE 0x01 /* Don't make the amusing object of uniques */
#define AMS_FIXED_ART 0x02 /* Make a fixed artifact based on the amusing object */
#define AMS_MULTIPLE 0x04 /* Drop 1-3 objects for one type */
#define AMS_PILE 0x08 /* Drop 1-99 pile objects for one type */

static amuse_type amuse_info[]
    = { { TV_BOTTLE, SV_ANY, 5, AMS_NOTHING }, { TV_JUNK, SV_ANY, 3, AMS_MULTIPLE }, { TV_SPIKE, SV_ANY, 10, AMS_PILE }, { TV_STATUE, SV_ANY, 15, AMS_NOTHING },
          { TV_CORPSE, SV_ANY, 15, AMS_NO_UNIQUE }, { TV_SKELETON, SV_ANY, 10, AMS_NO_UNIQUE }, { TV_FIGURINE, SV_ANY, 10, AMS_NO_UNIQUE },
          { TV_PARCHMENT, SV_ANY, 1, AMS_NOTHING }, { TV_POLEARM, SV_TSURIZAO, 3, AMS_NOTHING }, // Fishing Pole of Taikobo
          { TV_SWORD, SV_BROKEN_DAGGER, 3, AMS_FIXED_ART }, // Broken Dagger of Magician
          { TV_SWORD, SV_BROKEN_DAGGER, 10, AMS_NOTHING }, { TV_SWORD, SV_BROKEN_SWORD, 5, AMS_NOTHING }, { TV_SCROLL, SV_SCROLL_AMUSEMENT, 10, AMS_NOTHING },

          { 0, 0, 0, 0 } };

/*!
 * @brief「弾/矢の製造」処理 / do_cmd_cast calls this function if the player's class is 'archer'.
 * Hook to determine if an object is contertible in an arrow/bolt
 * @return 製造を実際に行ったらTRUE、キャンセルしたらFALSEを返す
 */
bool create_ammo(player_type *creature_ptr)
{
    char com[80];
    if (creature_ptr->lev >= 20)
        sprintf(com, _("[S]弾, [A]矢, [B]クロスボウの矢 :", "Create [S]hots, Create [A]rrow or Create [B]olt ?"));
    else if (creature_ptr->lev >= 10)
        sprintf(com, _("[S]弾, [A]矢:", "Create [S]hots or Create [A]rrow ?"));
    else
        sprintf(com, _("[S]弾:", "Create [S]hots ?"));

    if (cmd_limit_confused(creature_ptr))
        return FALSE;
    if (cmd_limit_blind(creature_ptr))
        return FALSE;

    int ext = 0;
    char ch;
    while (TRUE) {
        if (!get_com(com, &ch, TRUE)) {
            return FALSE;
        }

        if (ch == 'S' || ch == 's') {
            ext = 1;
            break;
        }

        if ((ch == 'A' || ch == 'a') && (creature_ptr->lev >= 10)) {
            ext = 2;
            break;
        }

        if ((ch == 'B' || ch == 'b') && (creature_ptr->lev >= 20)) {
            ext = 3;
            break;
        }
    }

    GAME_TEXT o_name[MAX_NLEN];
    object_type forge;
    object_type *q_ptr;
    q_ptr = &forge;

    /**********Create shots*********/
    if (ext == 1) {
        POSITION x, y;
        DIRECTION dir;
        grid_type *g_ptr;

        if (!get_rep_dir(creature_ptr, &dir, FALSE))
            return FALSE;
        y = creature_ptr->y + ddy[dir];
        x = creature_ptr->x + ddx[dir];
        g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];

        if (!have_flag(f_info[get_feat_mimic(g_ptr)].flags, FF_CAN_DIG)) {
            msg_print(_("そこには岩石がない。", "You need a pile of rubble."));
            return FALSE;
        }

        if (!cave_have_flag_grid(g_ptr, FF_CAN_DIG) || !cave_have_flag_grid(g_ptr, FF_HURT_ROCK)) {
            msg_print(_("硬すぎて崩せなかった。", "You failed to make ammo."));
            return TRUE;
        }

        s16b slot;
        q_ptr = &forge;

        /* Hack -- Give the player some small firestones */
        object_prep(creature_ptr, q_ptr, lookup_kind(TV_SHOT, (OBJECT_SUBTYPE_VALUE)m_bonus(1, creature_ptr->lev) + 1));
        q_ptr->number = (byte)rand_range(15, 30);
        object_aware(creature_ptr, q_ptr);
        object_known(q_ptr);
        apply_magic(creature_ptr, q_ptr, creature_ptr->lev, AM_NO_FIXED_ART);
        q_ptr->discount = 99;

        slot = store_item_to_inventory(creature_ptr, q_ptr);

        describe_flavor(creature_ptr, o_name, q_ptr, 0);
        msg_format(_("%sを作った。", "You make some ammo."), o_name);

        /* Auto-inscription */
        if (slot >= 0)
            autopick_alter_item(creature_ptr, slot, FALSE);

        /* Destroy the wall */
        cave_alter_feat(creature_ptr, y, x, FF_HURT_ROCK);

        creature_ptr->update |= (PU_FLOW);
        return TRUE;
    }

    /**********Create arrows*********/
    if (ext == 2) {
        OBJECT_IDX item;
        concptr q, s;
        s16b slot;

        item_tester_hook = item_tester_hook_convertible;

        q = _("どのアイテムから作りますか？ ", "Convert which item? ");
        s = _("材料を持っていない。", "You have no item to convert.");
        q_ptr = choose_object(creature_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), 0);
        if (!q_ptr)
            return FALSE;

        q_ptr = &forge;

        /* Hack -- Give the player some small firestones */
        object_prep(creature_ptr, q_ptr, lookup_kind(TV_ARROW, (OBJECT_SUBTYPE_VALUE)m_bonus(1, creature_ptr->lev) + 1));
        q_ptr->number = (byte)rand_range(5, 10);
        object_aware(creature_ptr, q_ptr);
        object_known(q_ptr);
        apply_magic(creature_ptr, q_ptr, creature_ptr->lev, AM_NO_FIXED_ART);

        q_ptr->discount = 99;

        describe_flavor(creature_ptr, o_name, q_ptr, 0);
        msg_format(_("%sを作った。", "You make some ammo."), o_name);

        vary_item(creature_ptr, item, -1);
        slot = store_item_to_inventory(creature_ptr, q_ptr);

        /* Auto-inscription */
        if (slot >= 0)
            autopick_alter_item(creature_ptr, slot, FALSE);
        return TRUE;
    }

    /**********Create bolts*********/
    if (ext == 3) {
        OBJECT_IDX item;
        concptr q, s;
        s16b slot;

        item_tester_hook = item_tester_hook_convertible;

        q = _("どのアイテムから作りますか？ ", "Convert which item? ");
        s = _("材料を持っていない。", "You have no item to convert.");

        q_ptr = choose_object(creature_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), 0);
        if (!q_ptr)
            return FALSE;

        q_ptr = &forge;

        /* Hack -- Give the player some small firestones */
        object_prep(creature_ptr, q_ptr, lookup_kind(TV_BOLT, (OBJECT_SUBTYPE_VALUE)m_bonus(1, creature_ptr->lev) + 1));
        q_ptr->number = (byte)rand_range(4, 8);
        object_aware(creature_ptr, q_ptr);
        object_known(q_ptr);
        apply_magic(creature_ptr, q_ptr, creature_ptr->lev, AM_NO_FIXED_ART);

        q_ptr->discount = 99;

        describe_flavor(creature_ptr, o_name, q_ptr, 0);
        msg_format(_("%sを作った。", "You make some ammo."), o_name);

        vary_item(creature_ptr, item, -1);

        slot = store_item_to_inventory(creature_ptr, q_ptr);

        /* Auto-inscription */
        if (slot >= 0)
            autopick_alter_item(creature_ptr, slot, FALSE);
    }

    return TRUE;
}

/*!
 * @brief 魔道具術師の魔力取り込み処理
 * @param user_ptr アイテムを取り込むクリーチャー
 * @return 取り込みを実行したらTRUE、キャンセルしたらFALSEを返す
 */
bool import_magic_device(player_type *user_ptr)
{
    /* Only accept legal items */
    item_tester_hook = item_tester_hook_recharge;

    concptr q = _("どのアイテムの魔力を取り込みますか? ", "Gain power of which item? ");
    concptr s = _("魔力を取り込めるアイテムがない。", "You have nothing to gain power.");

    OBJECT_IDX item;
    object_type *o_ptr;
    o_ptr = choose_object(user_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), 0);
    if (!o_ptr)
        return FALSE;

    if (o_ptr->tval == TV_STAFF && o_ptr->sval == SV_STAFF_NOTHING) {
        msg_print(_("この杖には発動の為の能力は何も備わっていないようだ。", "This staff doesn't have any magical ability."));
        return FALSE;
    }

    if (!object_is_known(o_ptr)) {
        msg_print(_("鑑定されていないと取り込めない。", "You need to identify before absorbing."));
        return FALSE;
    }

    if (o_ptr->timeout) {
        msg_print(_("充填中のアイテムは取り込めない。", "This item is still charging."));
        return FALSE;
    }

    PARAMETER_VALUE pval = o_ptr->pval;
    int ext = 0;
    if (o_ptr->tval == TV_ROD)
        ext = 72;
    else if (o_ptr->tval == TV_WAND)
        ext = 36;

    if (o_ptr->tval == TV_ROD) {
        user_ptr->magic_num2[o_ptr->sval + ext] += (MAGIC_NUM2)o_ptr->number;
        if (user_ptr->magic_num2[o_ptr->sval + ext] > 99)
            user_ptr->magic_num2[o_ptr->sval + ext] = 99;
    } else {
        int num;
        for (num = o_ptr->number; num; num--) {
            int gain_num = pval;
            if (o_ptr->tval == TV_WAND)
                gain_num = (pval + num - 1) / num;
            if (user_ptr->magic_num2[o_ptr->sval + ext]) {
                gain_num *= 256;
                gain_num = (gain_num / 3 + randint0(gain_num / 3)) / 256;
                if (gain_num < 1)
                    gain_num = 1;
            }
            user_ptr->magic_num2[o_ptr->sval + ext] += (MAGIC_NUM2)gain_num;
            if (user_ptr->magic_num2[o_ptr->sval + ext] > 99)
                user_ptr->magic_num2[o_ptr->sval + ext] = 99;
            user_ptr->magic_num1[o_ptr->sval + ext] += pval * 0x10000;
            if (user_ptr->magic_num1[o_ptr->sval + ext] > 99 * 0x10000)
                user_ptr->magic_num1[o_ptr->sval + ext] = 99 * 0x10000;
            if (user_ptr->magic_num1[o_ptr->sval + ext] > user_ptr->magic_num2[o_ptr->sval + ext] * 0x10000)
                user_ptr->magic_num1[o_ptr->sval + ext] = user_ptr->magic_num2[o_ptr->sval + ext] * 0x10000;
            if (o_ptr->tval == TV_WAND)
                pval -= (pval + num - 1) / num;
        }
    }

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(user_ptr, o_name, o_ptr, 0);
    msg_format(_("%sの魔力を取り込んだ。", "You absorb magic of %s."), o_name);

    vary_item(user_ptr, item, -999);
    take_turn(user_ptr, 100);
    return TRUE;
}

/*!
 * @brief 誰得ドロップを行う。
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param y1 配置したいフロアのY座標
 * @param x1 配置したいフロアのX座標
 * @param num 誰得の処理回数
 * @param known TRUEならばオブジェクトが必ず＊鑑定＊済になる
 * @return なし
 */
void amusement(player_type *creature_ptr, POSITION y1, POSITION x1, int num, bool known)
{
    int t = 0;
    for (int n = 0; amuse_info[n].tval != 0; n++) {
        t += amuse_info[n].prob;
    }

    /* Acquirement */
    object_type *i_ptr;
    object_type object_type_body;
    while (num) {
        int i;
        KIND_OBJECT_IDX k_idx;
        ARTIFACT_IDX a_idx = 0;
        int r = randint0(t);
        bool insta_art, fixed_art;

        for (i = 0;; i++) {
            r -= amuse_info[i].prob;
            if (r <= 0)
                break;
        }
        i_ptr = &object_type_body;
        object_wipe(i_ptr);
        k_idx = lookup_kind(amuse_info[i].tval, amuse_info[i].sval);

        /* Paranoia - reroll if nothing */
        if (!k_idx)
            continue;

        /* Search an artifact index if need */
        insta_art = (k_info[k_idx].gen_flags & TRG_INSTA_ART);
        fixed_art = (amuse_info[i].flag & AMS_FIXED_ART);

        if (insta_art || fixed_art) {
            for (a_idx = 1; a_idx < max_a_idx; a_idx++) {
                if (insta_art && !(a_info[a_idx].gen_flags & TRG_INSTA_ART))
                    continue;
                if (a_info[a_idx].tval != k_info[k_idx].tval)
                    continue;
                if (a_info[a_idx].sval != k_info[k_idx].sval)
                    continue;
                if (a_info[a_idx].cur_num > 0)
                    continue;
                break;
            }

            if (a_idx >= max_a_idx)
                continue;
        }

        /* Make an object (if possible) */
        object_prep(creature_ptr, i_ptr, k_idx);
        if (a_idx)
            i_ptr->name1 = a_idx;
        apply_magic(creature_ptr, i_ptr, 1, AM_NO_FIXED_ART);

        if (amuse_info[i].flag & AMS_NO_UNIQUE) {
            if (r_info[i_ptr->pval].flags1 & RF1_UNIQUE)
                continue;
        }

        if (amuse_info[i].flag & AMS_MULTIPLE)
            i_ptr->number = randint1(3);
        if (amuse_info[i].flag & AMS_PILE)
            i_ptr->number = randint1(99);

        if (known) {
            object_aware(creature_ptr, i_ptr);
            object_known(i_ptr);
        }

        /* Paranoia - reroll if nothing */
        if (!(i_ptr->k_idx))
            continue;

        (void)drop_near(creature_ptr, i_ptr, -1, y1, x1);

        num--;
    }
}

/*!
 * @brief 獲得ドロップを行う。
 * Scatter some "great" objects near the player
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param y1 配置したいフロアのY座標
 * @param x1 配置したいフロアのX座標
 * @param num 獲得の処理回数
 * @param great TRUEならば必ず高級品以上を落とす
 * @param special TRUEならば必ず特別品を落とす
 * @param known TRUEならばオブジェクトが必ず＊鑑定＊済になる
 * @return なし
 */
void acquirement(player_type *caster_ptr, POSITION y1, POSITION x1, int num, bool great, bool special, bool known)
{
    object_type *i_ptr;
    object_type object_type_body;
    BIT_FLAGS mode = AM_GOOD | (great || special ? AM_GREAT : 0L) | (special ? AM_SPECIAL : 0L);

    /* Acquirement */
    while (num--) {
        i_ptr = &object_type_body;
        object_wipe(i_ptr);

        /* Make a good (or great) object (if possible) */
        if (!make_object(caster_ptr, i_ptr, mode))
            continue;

        if (known) {
            object_aware(caster_ptr, i_ptr);
            object_known(i_ptr);
        }

        (void)drop_near(caster_ptr, i_ptr, -1, y1, x1);
    }
}

void acquire_chaos_weapon(player_type *creature_ptr)
{
    object_type forge;
    object_type *q_ptr = &forge;
    tval_type dummy = TV_SWORD;
    OBJECT_SUBTYPE_VALUE dummy2;
    switch (randint1(creature_ptr->lev)) {
    case 0:
    case 1:
        dummy2 = SV_DAGGER;
        break;
    case 2:
    case 3:
        dummy2 = SV_MAIN_GAUCHE;
        break;
    case 4:
        dummy2 = SV_TANTO;
        break;
    case 5:
    case 6:
        dummy2 = SV_RAPIER;
        break;
    case 7:
    case 8:
        dummy2 = SV_SMALL_SWORD;
        break;
    case 9:
    case 10:
        dummy2 = SV_BASILLARD;
        break;
    case 11:
    case 12:
    case 13:
        dummy2 = SV_SHORT_SWORD;
        break;
    case 14:
    case 15:
        dummy2 = SV_SABRE;
        break;
    case 16:
    case 17:
        dummy2 = SV_CUTLASS;
        break;
    case 18:
        dummy2 = SV_WAKIZASHI;
        break;
    case 19:
        dummy2 = SV_KHOPESH;
        break;
    case 20:
        dummy2 = SV_TULWAR;
        break;
    case 21:
        dummy2 = SV_BROAD_SWORD;
        break;
    case 22:
    case 23:
        dummy2 = SV_LONG_SWORD;
        break;
    case 24:
    case 25:
        dummy2 = SV_SCIMITAR;
        break;
    case 26:
        dummy2 = SV_NINJATO;
        break;
    case 27:
        dummy2 = SV_KATANA;
        break;
    case 28:
    case 29:
        dummy2 = SV_BASTARD_SWORD;
        break;
    case 30:
        dummy2 = SV_GREAT_SCIMITAR;
        break;
    case 31:
        dummy2 = SV_CLAYMORE;
        break;
    case 32:
        dummy2 = SV_ESPADON;
        break;
    case 33:
        dummy2 = SV_TWO_HANDED_SWORD;
        break;
    case 34:
        dummy2 = SV_FLAMBERGE;
        break;
    case 35:
        dummy2 = SV_NO_DACHI;
        break;
    case 36:
        dummy2 = SV_EXECUTIONERS_SWORD;
        break;
    case 37:
        dummy2 = SV_ZWEIHANDER;
        break;
    case 38:
        dummy2 = SV_HAYABUSA;
        break;
    default:
        dummy2 = SV_BLADE_OF_CHAOS;
    }

    object_prep(creature_ptr, q_ptr, lookup_kind(dummy, dummy2));
    q_ptr->to_h = 3 + randint1(creature_ptr->current_floor_ptr->dun_level) % 10;
    q_ptr->to_d = 3 + randint1(creature_ptr->current_floor_ptr->dun_level) % 10;
    one_resistance(q_ptr);
    q_ptr->name2 = EGO_CHAOTIC;
    (void)drop_near(creature_ptr, q_ptr, -1, creature_ptr->y, creature_ptr->x);
}

/*!
 * todo 元のreturnは間違っているが、修正後の↓文がどれくらい正しいかは要チェック
 * @brief 防具呪縛処理 /
 * Curse the players armor
 * @return 何も持っていない場合を除き、常にTRUEを返す
 */
bool curse_armor(player_type *owner_ptr)
{
    /* Curse the body armor */
    object_type *o_ptr;
    o_ptr = &owner_ptr->inventory_list[INVEN_BODY];

    if (!o_ptr->k_idx)
        return FALSE;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(owner_ptr, o_name, o_ptr, OD_OMIT_PREFIX);

    /* Attempt a saving throw for artifacts */
    if (object_is_artifact(o_ptr) && (randint0(100) < 50)) {
        /* Cool */
#ifdef JP
        msg_format("%sが%sを包み込もうとしたが、%sはそれを跳ね返した！", "恐怖の暗黒オーラ", "防具", o_name);
#else
        msg_format("A %s tries to %s, but your %s resists the effects!", "terrible black aura", "surround your armor", o_name);
#endif
        return TRUE;
    }

    /* not artifact or failed save... */
    msg_format(_("恐怖の暗黒オーラがあなたの%sを包み込んだ！", "A terrible black aura blasts your %s!"), o_name);
    chg_virtue(owner_ptr, V_ENCHANT, -5);

    /* Blast the armor */
    o_ptr->name1 = 0;
    o_ptr->name2 = EGO_BLASTED;
    o_ptr->to_a = 0 - randint1(5) - randint1(5);
    o_ptr->to_h = 0;
    o_ptr->to_d = 0;
    o_ptr->ac = 0;
    o_ptr->dd = 0;
    o_ptr->ds = 0;

    for (int i = 0; i < TR_FLAG_SIZE; i++)
        o_ptr->art_flags[i] = 0;

    /* Curse it */
    o_ptr->curse_flags = TRC_CURSED;

    /* Break it */
    o_ptr->ident |= (IDENT_BROKEN);
    owner_ptr->update |= (PU_BONUS | PU_MANA);
    owner_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
    return TRUE;
}

/*!
 * todo 元のreturnは間違っているが、修正後の↓文がどれくらい正しいかは要チェック
 * @brief 武器呪縛処理 /
 * Curse the players weapon
 * @param owner_ptr 所持者の参照ポインタ
 * @param force 無条件に呪縛を行うならばTRUE
 * @param o_ptr 呪縛する武器のアイテム情報参照ポインタ
 * @return 何も持っていない場合を除き、常にTRUEを返す
 */
bool curse_weapon_object(player_type *owner_ptr, bool force, object_type *o_ptr)
{
    if (!o_ptr->k_idx)
        return FALSE;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(owner_ptr, o_name, o_ptr, OD_OMIT_PREFIX);

    /* Attempt a saving throw */
    if (object_is_artifact(o_ptr) && (randint0(100) < 50) && !force) {
#ifdef JP
        msg_format("%sが%sを包み込もうとしたが、%sはそれを跳ね返した！", "恐怖の暗黒オーラ", "武器", o_name);
#else
        msg_format("A %s tries to %s, but your %s resists the effects!", "terrible black aura", "surround your weapon", o_name);
#endif
        return TRUE;
    }

    /* not artifact or failed save... */
    if (!force)
        msg_format(_("恐怖の暗黒オーラがあなたの%sを包み込んだ！", "A terrible black aura blasts your %s!"), o_name);
    chg_virtue(owner_ptr, V_ENCHANT, -5);

    /* Shatter the weapon */
    o_ptr->name1 = 0;
    o_ptr->name2 = EGO_SHATTERED;
    o_ptr->to_h = 0 - randint1(5) - randint1(5);
    o_ptr->to_d = 0 - randint1(5) - randint1(5);
    o_ptr->to_a = 0;
    o_ptr->ac = 0;
    o_ptr->dd = 0;
    o_ptr->ds = 0;

    for (int i = 0; i < TR_FLAG_SIZE; i++)
        o_ptr->art_flags[i] = 0;

    /* Curse it */
    o_ptr->curse_flags = TRC_CURSED;

    /* Break it */
    o_ptr->ident |= (IDENT_BROKEN);
    owner_ptr->update |= (PU_BONUS | PU_MANA);
    owner_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
    return TRUE;
}

/*!
 * @brief 防具の錆止め防止処理
 * @param caster_ptr 錆止め実行者の参照ポインタ
 * @return ターン消費を要する処理を行ったならばTRUEを返す
 */
bool rustproof(player_type *caster_ptr)
{
    /* Select a piece of armour */
    item_tester_hook = object_is_armour;

    concptr q = _("どの防具に錆止めをしますか？", "Rustproof which piece of armour? ");
    concptr s = _("錆止めできるものがありません。", "You have nothing to rustproof.");

    OBJECT_IDX item;
    object_type *o_ptr;
    o_ptr = choose_object(caster_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), 0);
    if (!o_ptr)
        return FALSE;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(caster_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

    add_flag(o_ptr->art_flags, TR_IGNORE_ACID);

    if ((o_ptr->to_a < 0) && !object_is_cursed(o_ptr)) {
#ifdef JP
        msg_format("%sは新品同様になった！", o_name);
#else
        msg_format("%s %s look%s as good as new!", ((item >= 0) ? "Your" : "The"), o_name, ((o_ptr->number > 1) ? "" : "s"));
#endif

        o_ptr->to_a = 0;
    }

#ifdef JP
    msg_format("%sは腐食しなくなった。", o_name);
#else
    msg_format("%s %s %s now protected against corrosion.", ((item >= 0) ? "Your" : "The"), o_name, ((o_ptr->number > 1) ? "are" : "is"));
#endif

    calc_android_exp(caster_ptr);
    return TRUE;
}

/*!
 * @brief ボルトのエゴ化処理(火炎エゴのみ) /
 * Enchant some bolts
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void brand_bolts(player_type *caster_ptr)
{
    /* Use the first acceptable bolts */
    for (int i = 0; i < INVEN_PACK; i++) {
        object_type *o_ptr = &caster_ptr->inventory_list[i];

        /* Skip non-bolts */
        if (o_ptr->tval != TV_BOLT)
            continue;

        /* Skip artifacts and ego-items */
        if (object_is_artifact(o_ptr) || object_is_ego(o_ptr))
            continue;

        /* Skip cursed/broken items */
        if (object_is_cursed(o_ptr) || object_is_broken(o_ptr))
            continue;

        /* Randomize */
        if (randint0(100) < 75)
            continue;

        msg_print(_("クロスボウの矢が炎のオーラに包まれた！", "Your bolts are covered in a fiery aura!"));

        /* Ego-item */
        o_ptr->name2 = EGO_FLAME;
        enchant(caster_ptr, o_ptr, randint0(3) + 4, ENCH_TOHIT | ENCH_TODAM);
        return;
    }

    if (flush_failure)
        flush();
    msg_print(_("炎で強化するのに失敗した。", "The fiery enchantment failed."));
}

bool perilous_secrets(player_type *user_ptr)
{
    if (!ident_spell(user_ptr, FALSE, 0))
        return FALSE;

    if (mp_ptr->spell_book) {
        /* Sufficient mana */
        if (20 <= user_ptr->csp) {
            /* Use some mana */
            user_ptr->csp -= 20;
        }

        /* Over-exert the player */
        else {
            int oops = 20 - user_ptr->csp;

            /* No mana left */
            user_ptr->csp = 0;
            user_ptr->csp_frac = 0;

            msg_print(_("石を制御できない！", "You are too weak to control the stone!"));
            /* Hack -- Bypass free action */
            (void)set_paralyzed(user_ptr, user_ptr->paralyzed + randint1(5 * oops + 1));

            /* Confusing. */
            (void)set_confused(user_ptr, user_ptr->confused + randint1(5 * oops + 1));
        }

        user_ptr->redraw |= (PR_MANA);
    }

    take_hit(user_ptr, DAMAGE_LOSELIFE, damroll(1, 12), _("危険な秘密", "perilous secrets"), -1);
    /* Confusing. */
    if (one_in_(5))
        (void)set_confused(user_ptr, user_ptr->confused + randint1(10));

    /* Exercise a little care... */
    if (one_in_(20))
        take_hit(user_ptr, DAMAGE_LOSELIFE, damroll(4, 10), _("危険な秘密", "perilous secrets"), -1);
    return TRUE;
}

/*!
 * @brief 固定アーティファクト『ブラッディムーン』の特性を変更する。
 * @details スレイ2d2種、及びone_resistance()による耐性1d2種、pval2種を得る。
 * @param o_ptr 対象のオブジェクト構造体（ブラッディムーン）のポインタ
 * @return なし
 */
void get_bloody_moon_flags(object_type *o_ptr)
{
    for (int i = 0; i < TR_FLAG_SIZE; i++)
        o_ptr->art_flags[i] = a_info[ART_BLOOD].flags[i];

    int dummy = randint1(2) + randint1(2);
    for (int i = 0; i < dummy; i++) {
        int flag = randint0(26);
        if (flag >= 20)
            add_flag(o_ptr->art_flags, TR_KILL_UNDEAD + flag - 20);
        else if (flag == 19)
            add_flag(o_ptr->art_flags, TR_KILL_ANIMAL);
        else if (flag == 18)
            add_flag(o_ptr->art_flags, TR_SLAY_HUMAN);
        else
            add_flag(o_ptr->art_flags, TR_CHAOTIC + flag);
    }

    dummy = randint1(2);
    for (int i = 0; i < dummy; i++)
        one_resistance(o_ptr);

    for (int i = 0; i < 2; i++) {
        int tmp = randint0(11);
        if (tmp < A_MAX)
            add_flag(o_ptr->art_flags, TR_STR + tmp);
        else
            add_flag(o_ptr->art_flags, TR_STEALTH + tmp - 6);
    }
}

/*!
 * @brief 寿命つき光源の燃素追加処理 /
 * Charge a lite (torch or latern)
 * @return なし
 */
void phlogiston(player_type *caster_ptr)
{
    GAME_TURN max_flog = 0;
    object_type *o_ptr = &caster_ptr->inventory_list[INVEN_LITE];

    /* It's a lamp */
    if ((o_ptr->tval == TV_LITE) && (o_ptr->sval == SV_LITE_LANTERN)) {
        max_flog = FUEL_LAMP;
    }

    /* It's a torch */
    else if ((o_ptr->tval == TV_LITE) && (o_ptr->sval == SV_LITE_TORCH)) {
        max_flog = FUEL_TORCH;
    }

    /* No torch to refill */
    else {
        msg_print(_("燃素を消費するアイテムを装備していません。", "You are not wielding anything which uses phlogiston."));
        return;
    }

    if (o_ptr->xtra4 >= max_flog) {
        msg_print(_("このアイテムにはこれ以上燃素を補充できません。", "No more phlogiston can be put in this item."));
        return;
    }

    /* Refuel */
    o_ptr->xtra4 += (XTRA16)(max_flog / 2);
    msg_print(_("照明用アイテムに燃素を補充した。", "You add phlogiston to your light item."));

    if (o_ptr->xtra4 >= max_flog) {
        o_ptr->xtra4 = (XTRA16)max_flog;
        msg_print(_("照明用アイテムは満タンになった。", "Your light item is full."));
    }

    caster_ptr->update |= (PU_TORCH);
}

/*!
 * @brief 武器の祝福処理 /
 * Bless a weapon
 * @return ターン消費を要する処理を行ったならばTRUEを返す
 */
bool bless_weapon(player_type *caster_ptr)
{
    /* Bless only weapons */
    item_tester_hook = object_is_weapon;

    concptr q = _("どのアイテムを祝福しますか？", "Bless which weapon? ");
    concptr s = _("祝福できる武器がありません。", "You have weapon to bless.");

    OBJECT_IDX item;
    object_type *o_ptr;
    o_ptr = choose_object(caster_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), 0);
    if (!o_ptr)
        return FALSE;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(caster_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    object_flags(caster_ptr, o_ptr, flgs);

    if (object_is_cursed(o_ptr)) {
        if (((o_ptr->curse_flags & TRC_HEAVY_CURSE) && (randint1(100) < 33)) || have_flag(flgs, TR_ADD_L_CURSE) || have_flag(flgs, TR_ADD_H_CURSE)
            || (o_ptr->curse_flags & TRC_PERMA_CURSE)) {
#ifdef JP
            msg_format("%sを覆う黒いオーラは祝福を跳ね返した！", o_name);
#else
            msg_format("The black aura on %s %s disrupts the blessing!", ((item >= 0) ? "your" : "the"), o_name);
#endif

            return TRUE;
        }

#ifdef JP
        msg_format("%s から邪悪なオーラが消えた。", o_name);
#else
        msg_format("A malignant aura leaves %s %s.", ((item >= 0) ? "your" : "the"), o_name);
#endif

        o_ptr->curse_flags = 0L;

        o_ptr->ident |= (IDENT_SENSE);
        o_ptr->feeling = FEEL_NONE;

        /* Recalculate the bonuses */
        caster_ptr->update |= (PU_BONUS);
        caster_ptr->window |= (PW_EQUIP);
    }

    /*
     * Next, we try to bless it. Artifacts have a 1/3 chance of
     * being blessed, otherwise, the operation simply disenchants
     * them, godly power negating the magic. Ok, the explanation
     * is silly, but otherwise priests would always bless every
     * artifact weapon they find. Ego weapons and normal weapons
     * can be blessed automatically.
     */
    if (have_flag(flgs, TR_BLESSED)) {
#ifdef JP
        msg_format("%s は既に祝福されている。", o_name);
#else
        msg_format("%s %s %s blessed already.", ((item >= 0) ? "Your" : "The"), o_name, ((o_ptr->number > 1) ? "were" : "was"));
#endif

        return TRUE;
    }

    if (!(object_is_artifact(o_ptr) || object_is_ego(o_ptr)) || one_in_(3)) {
#ifdef JP
        msg_format("%sは輝いた！", o_name);
#else
        msg_format("%s %s shine%s!", ((item >= 0) ? "Your" : "The"), o_name, ((o_ptr->number > 1) ? "" : "s"));
#endif

        add_flag(o_ptr->art_flags, TR_BLESSED);
        o_ptr->discount = 99;
    } else {
        bool dis_happened = FALSE;
        msg_print(_("その武器は祝福を嫌っている！", "The weapon resists your blessing!"));

        /* Disenchant tohit */
        if (o_ptr->to_h > 0) {
            o_ptr->to_h--;
            dis_happened = TRUE;
        }

        if ((o_ptr->to_h > 5) && (randint0(100) < 33))
            o_ptr->to_h--;

        /* Disenchant todam */
        if (o_ptr->to_d > 0) {
            o_ptr->to_d--;
            dis_happened = TRUE;
        }

        if ((o_ptr->to_d > 5) && (randint0(100) < 33))
            o_ptr->to_d--;

        /* Disenchant toac */
        if (o_ptr->to_a > 0) {
            o_ptr->to_a--;
            dis_happened = TRUE;
        }

        if ((o_ptr->to_a > 5) && (randint0(100) < 33))
            o_ptr->to_a--;

        if (dis_happened) {
            msg_print(_("周囲が凡庸な雰囲気で満ちた...", "There is a static feeling in the air..."));

#ifdef JP
            msg_format("%s は劣化した！", o_name);
#else
            msg_format("%s %s %s disenchanted!", ((item >= 0) ? "Your" : "The"), o_name, ((o_ptr->number > 1) ? "were" : "was"));
#endif
        }
    }

    caster_ptr->update |= (PU_BONUS);
    caster_ptr->window |= (PW_EQUIP | PW_PLAYER);
    calc_android_exp(caster_ptr);

    return TRUE;
}

/*!
 * @brief 盾磨き処理 /
 * pulish shield
 * @return ターン消費を要する処理を行ったならばTRUEを返す
 */
bool pulish_shield(player_type *caster_ptr)
{
    concptr q = _("どの盾を磨きますか？", "Pulish which weapon? ");
    concptr s = _("磨く盾がありません。", "You have weapon to pulish.");

    OBJECT_IDX item;
    object_type *o_ptr;
    o_ptr = choose_object(caster_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), TV_SHIELD);
    if (!o_ptr)
        return FALSE;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(caster_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    object_flags(caster_ptr, o_ptr, flgs);

    bool is_pulish_successful = o_ptr->k_idx && !object_is_artifact(o_ptr) && !object_is_ego(o_ptr);
    is_pulish_successful &= !object_is_cursed(o_ptr);
    is_pulish_successful &= (o_ptr->sval != SV_MIRROR_SHIELD);
    if (is_pulish_successful) {
#ifdef JP
        msg_format("%sは輝いた！", o_name);
#else
        msg_format("%s %s shine%s!", ((item >= 0) ? "Your" : "The"), o_name, ((o_ptr->number > 1) ? "" : "s"));
#endif
        o_ptr->name2 = EGO_REFLECTION;
        enchant(caster_ptr, o_ptr, randint0(3) + 4, ENCH_TOAC);

        o_ptr->discount = 99;
        chg_virtue(caster_ptr, V_ENCHANT, 2);

        return TRUE;
    }

    if (flush_failure)
        flush();

    msg_print(_("失敗した。", "Failed."));
    chg_virtue(caster_ptr, V_ENCHANT, -2);
    calc_android_exp(caster_ptr);
    return FALSE;
}

/*!
 * @brief 呪いの打ち破り処理 /
 * Break the curse of an item
 * @param o_ptr 呪い装備情報の参照ポインタ
 * @return なし
 */
static void break_curse(object_type *o_ptr)
{
    BIT_FLAGS is_curse_broken
        = object_is_cursed(o_ptr) && !(o_ptr->curse_flags & TRC_PERMA_CURSE) && !(o_ptr->curse_flags & TRC_HEAVY_CURSE) && (randint0(100) < 25);
    if (!is_curse_broken) {
        return;
    }

    msg_print(_("かけられていた呪いが打ち破られた！", "The curse is broken!"));

    o_ptr->curse_flags = 0L;
    o_ptr->ident |= (IDENT_SENSE);
    o_ptr->feeling = FEEL_NONE;
}

/*!
 * @brief 装備修正強化処理 /
 * Enchants a plus onto an item. -RAK-
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 強化するアイテムの参照ポインタ
 * @param n 強化基本量
 * @param eflag 強化オプション(命中/ダメージ/AC)
 * @return 強化に成功した場合TRUEを返す
 * @details
 * <pre>
 * Revamped!  Now takes item pointer, number of times to try enchanting,
 * and a flag of what to try enchanting.  Artifacts resist enchantment
 * some of the time, and successful enchantment to at least +0 might
 * break a curse on the item. -CFT-
 *
 * Note that an item can technically be enchanted all the way to +15 if
 * you wait a very, very, long time.  Going from +9 to +10 only works
 * about 5% of the time, and from +10 to +11 only about 1% of the time.
 *
 * Note that this function can now be used on "piles" of items, and
 * the larger the pile, the lower the chance of success.
 * </pre>
 */
bool enchant(player_type *caster_ptr, object_type *o_ptr, int n, int eflag)
{
    /* Large piles resist enchantment */
    int prob = o_ptr->number * 100;

    /* Missiles are easy to enchant */
    if ((o_ptr->tval == TV_BOLT) || (o_ptr->tval == TV_ARROW) || (o_ptr->tval == TV_SHOT)) {
        prob = prob / 20;
    }

    /* Try "n" times */
    int chance;
    bool res = FALSE;
    bool a = object_is_artifact(o_ptr);
    bool force = (eflag & ENCH_FORCE);
    for (int i = 0; i < n; i++) {
        /* Hack -- Roll for pile resistance */
        if (!force && randint0(prob) >= 100)
            continue;

        /* Enchant to hit */
        if (eflag & ENCH_TOHIT) {
            if (o_ptr->to_h < 0)
                chance = 0;
            else if (o_ptr->to_h > 15)
                chance = 1000;
            else
                chance = enchant_table[o_ptr->to_h];

            if (force || ((randint1(1000) > chance) && (!a || (randint0(100) < 50)))) {
                o_ptr->to_h++;
                res = TRUE;

                /* only when you get it above -1 -CFT */
                if (o_ptr->to_h >= 0)
                    break_curse(o_ptr);
            }
        }

        /* Enchant to damage */
        if (eflag & ENCH_TODAM) {
            if (o_ptr->to_d < 0)
                chance = 0;
            else if (o_ptr->to_d > 15)
                chance = 1000;
            else
                chance = enchant_table[o_ptr->to_d];

            if (force || ((randint1(1000) > chance) && (!a || (randint0(100) < 50)))) {
                o_ptr->to_d++;
                res = TRUE;

                /* only when you get it above -1 -CFT */
                if (o_ptr->to_d >= 0)
                    break_curse(o_ptr);
            }
        }

        /* Enchant to armor class */
        if (!(eflag & ENCH_TOAC)) {
            continue;
        }

        if (o_ptr->to_a < 0)
            chance = 0;
        else if (o_ptr->to_a > 15)
            chance = 1000;
        else
            chance = enchant_table[o_ptr->to_a];

        if (force || ((randint1(1000) > chance) && (!a || (randint0(100) < 50)))) {
            o_ptr->to_a++;
            res = TRUE;

            /* only when you get it above -1 -CFT */
            if (o_ptr->to_a >= 0)
                break_curse(o_ptr);
        }
    }

    /* Failure */
    if (!res)
        return FALSE;
    caster_ptr->update |= (PU_BONUS | PU_COMBINE | PU_REORDER);
    caster_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

    calc_android_exp(caster_ptr);

    /* Success */
    return TRUE;
}

/*!
 * @brief 装備修正強化処理のメインルーチン /
 * Enchant an item (in the inventory or on the floor)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param num_hit 命中修正量
 * @param num_dam ダメージ修正量
 * @param num_ac AC修正量
 * @return 強化に成功した場合TRUEを返す
 * @details
 * Note that "num_ac" requires armour, else weapon
 * Returns TRUE if attempted, FALSE if cancelled
 */
bool enchant_spell(player_type *caster_ptr, HIT_PROB num_hit, HIT_POINT num_dam, ARMOUR_CLASS num_ac)
{
    /* Assume enchant weapon */
    item_tester_hook = object_allow_enchant_weapon;

    /* Enchant armor if requested */
    if (num_ac)
        item_tester_hook = object_is_armour;

    concptr q = _("どのアイテムを強化しますか? ", "Enchant which item? ");
    concptr s = _("強化できるアイテムがない。", "You have nothing to enchant.");

    OBJECT_IDX item;
    object_type *o_ptr;
    o_ptr = choose_object(caster_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), 0);
    if (!o_ptr)
        return FALSE;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(caster_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
#ifdef JP
    msg_format("%s は明るく輝いた！", o_name);
#else
    msg_format("%s %s glow%s brightly!", ((item >= 0) ? "Your" : "The"), o_name, ((o_ptr->number > 1) ? "" : "s"));
#endif

    /* Enchant */
    bool is_enchant_successful = FALSE;
    if (enchant(caster_ptr, o_ptr, num_hit, ENCH_TOHIT))
        is_enchant_successful = TRUE;
    if (enchant(caster_ptr, o_ptr, num_dam, ENCH_TODAM))
        is_enchant_successful = TRUE;
    if (enchant(caster_ptr, o_ptr, num_ac, ENCH_TOAC))
        is_enchant_successful = TRUE;

    if (!is_enchant_successful) {
        if (flush_failure)
            flush();
        msg_print(_("強化に失敗した。", "The enchantment failed."));
        if (one_in_(3))
            chg_virtue(caster_ptr, V_ENCHANT, -1);
    } else
        chg_virtue(caster_ptr, V_ENCHANT, 1);

    calc_android_exp(caster_ptr);

    /* Something happened */
    return TRUE;
}

/*!
 * @brief 武器へのエゴ付加処理 /
 * Brand the current weapon
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param brand_type エゴ化ID(e_info.txtとは連動していない)
 * @return なし
 */
void brand_weapon(player_type *caster_ptr, int brand_type)
{
    /* Assume enchant weapon */
    item_tester_hook = object_allow_enchant_melee_weapon;

    concptr q = _("どの武器を強化しますか? ", "Enchant which weapon? ");
    concptr s = _("強化できる武器がない。", "You have nothing to enchant.");

    OBJECT_IDX item;
    object_type *o_ptr;
    o_ptr = choose_object(caster_ptr, &item, q, s, (USE_EQUIP | IGNORE_BOTHHAND_SLOT), 0);
    if (!o_ptr)
        return;

    bool is_special_item = o_ptr->k_idx && !object_is_artifact(o_ptr) && !object_is_ego(o_ptr) && !object_is_cursed(o_ptr)
        && !((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_POISON_NEEDLE)) && !((o_ptr->tval == TV_POLEARM) && (o_ptr->sval == SV_DEATH_SCYTHE))
        && !((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_DIAMOND_EDGE));
    if (!is_special_item) {
        if (flush_failure)
            flush();

        msg_print(_("属性付加に失敗した。", "The branding failed."));
        chg_virtue(caster_ptr, V_ENCHANT, -2);
        calc_android_exp(caster_ptr);
        return;
    }

    /* Let's get the name before it is changed... */
    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(caster_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

    concptr act = NULL;
    switch (brand_type) {
    case 17:
        if (o_ptr->tval == TV_SWORD) {
            act = _("は鋭さを増した！", "becomes very sharp!");

            o_ptr->name2 = EGO_SHARPNESS;
            o_ptr->pval = (PARAMETER_VALUE)m_bonus(5, caster_ptr->current_floor_ptr->dun_level) + 1;

            if ((o_ptr->sval == SV_HAYABUSA) && (o_ptr->pval > 2))
                o_ptr->pval = 2;
        } else {
            act = _("は破壊力を増した！", "seems very powerful.");
            o_ptr->name2 = EGO_EARTHQUAKES;
            o_ptr->pval = (PARAMETER_VALUE)m_bonus(3, caster_ptr->current_floor_ptr->dun_level);
        }

        break;
    case 16:
        act = _("は人間の血を求めている！", "seems to be looking for humans!");
        o_ptr->name2 = EGO_KILL_HUMAN;
        break;
    case 15:
        act = _("は電撃に覆われた！", "covered with lightning!");
        o_ptr->name2 = EGO_BRAND_ELEC;
        break;
    case 14:
        act = _("は酸に覆われた！", "coated with acid!");
        o_ptr->name2 = EGO_BRAND_ACID;
        break;
    case 13:
        act = _("は邪悪なる怪物を求めている！", "seems to be looking for evil monsters!");
        o_ptr->name2 = EGO_KILL_EVIL;
        break;
    case 12:
        act = _("は異世界の住人の肉体を求めている！", "seems to be looking for demons!");
        o_ptr->name2 = EGO_KILL_DEMON;
        break;
    case 11:
        act = _("は屍を求めている！", "seems to be looking for undead!");
        o_ptr->name2 = EGO_KILL_UNDEAD;
        break;
    case 10:
        act = _("は動物の血を求めている！", "seems to be looking for animals!");
        o_ptr->name2 = EGO_KILL_ANIMAL;
        break;
    case 9:
        act = _("はドラゴンの血を求めている！", "seems to be looking for dragons!");
        o_ptr->name2 = EGO_KILL_DRAGON;
        break;
    case 8:
        act = _("はトロルの血を求めている！", "seems to be looking for troll!s");
        o_ptr->name2 = EGO_KILL_TROLL;
        break;
    case 7:
        act = _("はオークの血を求めている！", "seems to be looking for orcs!");
        o_ptr->name2 = EGO_KILL_ORC;
        break;
    case 6:
        act = _("は巨人の血を求めている！", "seems to be looking for giants!");
        o_ptr->name2 = EGO_KILL_GIANT;
        break;
    case 5:
        act = _("は非常に不安定になったようだ。", "seems very unstable now.");
        o_ptr->name2 = EGO_TRUMP;
        o_ptr->pval = randint1(2);
        break;
    case 4:
        act = _("は血を求めている！", "thirsts for blood!");
        o_ptr->name2 = EGO_VAMPIRIC;
        break;
    case 3:
        act = _("は毒に覆われた。", "is coated with poison.");
        o_ptr->name2 = EGO_BRAND_POIS;
        break;
    case 2:
        act = _("は純ログルスに飲み込まれた。", "is engulfed in raw Logrus!");
        o_ptr->name2 = EGO_CHAOTIC;
        break;
    case 1:
        act = _("は炎のシールドに覆われた！", "is covered in a fiery shield!");
        o_ptr->name2 = EGO_BRAND_FIRE;
        break;
    default:
        act = _("は深く冷たいブルーに輝いた！", "glows deep, icy blue!");
        o_ptr->name2 = EGO_BRAND_COLD;
        break;
    }

    msg_format(_("あなたの%s%s", "Your %s %s"), o_name, act);
    enchant(caster_ptr, o_ptr, randint0(3) + 4, ENCH_TOHIT | ENCH_TODAM);

    o_ptr->discount = 99;
    chg_virtue(caster_ptr, V_ENCHANT, 2);
    calc_android_exp(caster_ptr);
}

bool create_ration(player_type *creature_ptr)
{
    object_type *q_ptr;
    object_type forge;
    q_ptr = &forge;
    object_prep(creature_ptr, q_ptr, lookup_kind(TV_FOOD, SV_FOOD_RATION));
    (void)drop_near(creature_ptr, q_ptr, -1, creature_ptr->y, creature_ptr->x);
    msg_print(_("食事を料理して作った。", "You cook some food."));
    return TRUE;
}
