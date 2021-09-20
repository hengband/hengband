/*!
 * @brief アイテムに影響のある魔法の処理
 * @date 2019/01/22
 * @author deskull
 */

#include "spell/spells-object.h"
#include "avatar/avatar.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "game-option/disturbance-options.h"
#include "inventory/inventory-slot-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "object-enchant/apply-magic.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/trg-types.h"
#include "object-hook/hook-armor.h"
#include "object-hook/hook-weapon.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-kind-hook.h"
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "player-info/class-info.h"
#include "player/player-damage.h"
#include "racial/racial-android.h"
#include "spell-kind/spells-perception.h"
#include "status/bad-status-setter.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
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

          { TV_NONE, 0, 0, 0 } };

/*!
 * @brief 誰得ドロップを行う。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y1 配置したいフロアのY座標
 * @param x1 配置したいフロアのX座標
 * @param num 誰得の処理回数
 * @param known TRUEならばオブジェクトが必ず＊鑑定＊済になる
 */
void amusement(player_type *player_ptr, POSITION y1, POSITION x1, int num, bool known)
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
        i_ptr->wipe();
        k_idx = lookup_kind(amuse_info[i].tval, amuse_info[i].sval);

        /* Paranoia - reroll if nothing */
        if (!k_idx)
            continue;

        /* Search an artifact index if need */
        insta_art = k_info[k_idx].gen_flags.has(TRG::INSTA_ART);
        fixed_art = (amuse_info[i].flag & AMS_FIXED_ART);

        if (insta_art || fixed_art) {
            for (const auto &a_ref : a_info) {
                if (a_ref.idx == 0)
                    continue;
                if (insta_art && !a_ref.gen_flags.has(TRG::INSTA_ART))
                    continue;
                if (a_ref.tval != k_info[k_idx].tval)
                    continue;
                if (a_ref.sval != k_info[k_idx].sval)
                    continue;
                if (a_ref.cur_num > 0)
                    continue;
                break;
            }

            if (a_idx >= max_a_idx)
                continue;
        }

        /* Make an object (if possible) */
        i_ptr->prep(k_idx);
        if (a_idx)
            i_ptr->name1 = a_idx;
        apply_magic_to_object(player_ptr, i_ptr, 1, AM_NO_FIXED_ART);

        if (amuse_info[i].flag & AMS_NO_UNIQUE) {
            if (r_info[i_ptr->pval].flags1 & RF1_UNIQUE)
                continue;
        }

        if (amuse_info[i].flag & AMS_MULTIPLE)
            i_ptr->number = randint1(3);
        if (amuse_info[i].flag & AMS_PILE)
            i_ptr->number = randint1(99);

        if (known) {
            object_aware(player_ptr, i_ptr);
            object_known(i_ptr);
        }

        /* Paranoia - reroll if nothing */
        if (!(i_ptr->k_idx))
            continue;

        (void)drop_near(player_ptr, i_ptr, -1, y1, x1);

        num--;
    }
}

/*!
 * @brief 獲得ドロップを行う。
 * Scatter some "great" objects near the player
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y1 配置したいフロアのY座標
 * @param x1 配置したいフロアのX座標
 * @param num 獲得の処理回数
 * @param great TRUEならば必ず高級品以上を落とす
 * @param special TRUEならば必ず特別品を落とす
 * @param known TRUEならばオブジェクトが必ず＊鑑定＊済になる
 */
void acquirement(player_type *player_ptr, POSITION y1, POSITION x1, int num, bool great, bool special, bool known)
{
    object_type *i_ptr;
    object_type object_type_body;
    BIT_FLAGS mode = AM_GOOD | (great || special ? AM_GREAT : AM_NONE) | (special ? AM_SPECIAL : AM_NONE);

    /* Acquirement */
    while (num--) {
        i_ptr = &object_type_body;
        i_ptr->wipe();

        /* Make a good (or great) object (if possible) */
        if (!make_object(player_ptr, i_ptr, mode))
            continue;

        if (known) {
            object_aware(player_ptr, i_ptr);
            object_known(i_ptr);
        }

        (void)drop_near(player_ptr, i_ptr, -1, y1, x1);
    }
}

/*!
 * @brief 防具呪縛処理 /
 * Curse the players armor
 * @return 何も持っていない場合を除き、常にTRUEを返す
 * @todo 元のreturnは間違っているが、修正後の↓文がどれくらい正しいかは要チェック
 */
bool curse_armor(player_type *player_ptr)
{
    /* Curse the body armor */
    object_type *o_ptr;
    o_ptr = &player_ptr->inventory_list[INVEN_BODY];

    if (!o_ptr->k_idx)
        return false;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, o_ptr, OD_OMIT_PREFIX);

    /* Attempt a saving throw for artifacts */
    if (o_ptr->is_artifact() && (randint0(100) < 50)) {
        /* Cool */
#ifdef JP
        msg_format("%sが%sを包み込もうとしたが、%sはそれを跳ね返した！", "恐怖の暗黒オーラ", "防具", o_name);
#else
        msg_format("A %s tries to %s, but your %s resists the effects!", "terrible black aura", "surround your armor", o_name);
#endif
        return true;
    }

    /* not artifact or failed save... */
    msg_format(_("恐怖の暗黒オーラがあなたの%sを包み込んだ！", "A terrible black aura blasts your %s!"), o_name);
    chg_virtue(player_ptr, V_ENCHANT, -5);

    /* Blast the armor */
    o_ptr->name1 = 0;
    o_ptr->name2 = EGO_BLASTED;
    o_ptr->to_a = 0 - randint1(5) - randint1(5);
    o_ptr->to_h = 0;
    o_ptr->to_d = 0;
    o_ptr->ac = 0;
    o_ptr->dd = 0;
    o_ptr->ds = 0;

    o_ptr->art_flags.clear();

    /* Curse it */
    o_ptr->curse_flags.set(TRC::CURSED);

    /* Break it */
    o_ptr->ident |= (IDENT_BROKEN);
    player_ptr->update |= (PU_BONUS | PU_MANA);
    player_ptr->window_flags |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
    return true;
}

/*!
 * @brief 武器呪縛処理 /
 * Curse the players weapon
 * @param player_ptr 所持者の参照ポインタ
 * @param force 無条件に呪縛を行うならばTRUE
 * @param o_ptr 呪縛する武器のアイテム情報参照ポインタ
 * @return 何も持っていない場合を除き、常にTRUEを返す
 * @todo 元のreturnは間違っているが、修正後の↓文がどれくらい正しいかは要チェック
 */
bool curse_weapon_object(player_type *player_ptr, bool force, object_type *o_ptr)
{
    if (!o_ptr->k_idx)
        return false;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, o_ptr, OD_OMIT_PREFIX);

    /* Attempt a saving throw */
    if (o_ptr->is_artifact() && (randint0(100) < 50) && !force) {
#ifdef JP
        msg_format("%sが%sを包み込もうとしたが、%sはそれを跳ね返した！", "恐怖の暗黒オーラ", "武器", o_name);
#else
        msg_format("A %s tries to %s, but your %s resists the effects!", "terrible black aura", "surround your weapon", o_name);
#endif
        return true;
    }

    /* not artifact or failed save... */
    if (!force)
        msg_format(_("恐怖の暗黒オーラがあなたの%sを包み込んだ！", "A terrible black aura blasts your %s!"), o_name);
    chg_virtue(player_ptr, V_ENCHANT, -5);

    /* Shatter the weapon */
    o_ptr->name1 = 0;
    o_ptr->name2 = EGO_SHATTERED;
    o_ptr->to_h = 0 - randint1(5) - randint1(5);
    o_ptr->to_d = 0 - randint1(5) - randint1(5);
    o_ptr->to_a = 0;
    o_ptr->ac = 0;
    o_ptr->dd = 0;
    o_ptr->ds = 0;

    o_ptr->art_flags.clear();

    /* Curse it */
    o_ptr->curse_flags.set(TRC::CURSED);

    /* Break it */
    o_ptr->ident |= (IDENT_BROKEN);
    player_ptr->update |= (PU_BONUS | PU_MANA);
    player_ptr->window_flags |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
    return true;
}

/*!
 * @brief ボルトのエゴ化処理(火炎エゴのみ) /
 * Enchant some bolts
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void brand_bolts(player_type *player_ptr)
{
    /* Use the first acceptable bolts */
    for (int i = 0; i < INVEN_PACK; i++) {
        object_type *o_ptr = &player_ptr->inventory_list[i];

        /* Skip non-bolts */
        if (o_ptr->tval != TV_BOLT)
            continue;

        /* Skip artifacts and ego-items */
        if (o_ptr->is_artifact() || o_ptr->is_ego())
            continue;

        /* Skip cursed/broken items */
        if (o_ptr->is_cursed() || o_ptr->is_broken())
            continue;

        /* Randomize */
        if (randint0(100) < 75)
            continue;

        msg_print(_("クロスボウの矢が炎のオーラに包まれた！", "Your bolts are covered in a fiery aura!"));

        /* Ego-item */
        o_ptr->name2 = EGO_FLAME;
        enchant_equipment(player_ptr, o_ptr, randint0(3) + 4, ENCH_TOHIT | ENCH_TODAM);
        return;
    }

    if (flush_failure)
        flush();
    msg_print(_("炎で強化するのに失敗した。", "The fiery enchantment failed."));
}

/*!
 * @brief 知識の石の発動を実行する / Do activation of the stone of lore.
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @return 実行したらTRUE、しなかったらFALSE
 * @details
 * 鑑定を実行した後HPを消費する。1/5で混乱し、1/20で追加ダメージ。
 * MPがある場合はさらにMPを20消費する。不足する場合は麻痺及び混乱。
 */
bool perilous_secrets(player_type *player_ptr)
{
    if (!ident_spell(player_ptr, false))
        return false;

    if (player_ptr->msp > 0) {
        if (20 <= player_ptr->csp)
            player_ptr->csp -= 20;
        else {
            int oops = 20 - player_ptr->csp;

            player_ptr->csp = 0;
            player_ptr->csp_frac = 0;

            msg_print(_("石を制御できない！", "You are too weak to control the stone!"));

            (void)set_paralyzed(player_ptr, player_ptr->paralyzed + randint1(5 * oops + 1));
            (void)set_confused(player_ptr, player_ptr->confused + randint1(5 * oops + 1));
        }

        player_ptr->redraw |= (PR_MANA);
    }

    take_hit(player_ptr, DAMAGE_LOSELIFE, damroll(1, 12), _("危険な秘密", "perilous secrets"));

    if (one_in_(5))
        (void)set_confused(player_ptr, player_ptr->confused + randint1(10));

    if (one_in_(20))
        take_hit(player_ptr, DAMAGE_LOSELIFE, damroll(4, 10), _("危険な秘密", "perilous secrets"));

    return true;
}

/*!
 * @brief 呪いの打ち破り処理 /
 * Break the curse of an item
 * @param o_ptr 呪い装備情報の参照ポインタ
 */
static void break_curse(object_type *o_ptr)
{
    BIT_FLAGS is_curse_broken
        = o_ptr->is_cursed() && o_ptr->curse_flags.has_not(TRC::PERMA_CURSE) && o_ptr->curse_flags.has_not(TRC::HEAVY_CURSE) && (randint0(100) < 25);
    if (!is_curse_broken) {
        return;
    }

    msg_print(_("かけられていた呪いが打ち破られた！", "The curse is broken!"));

    o_ptr->curse_flags.clear();
    o_ptr->ident |= (IDENT_SENSE);
    o_ptr->feeling = FEEL_NONE;
}

/*!
 * @brief 装備修正強化処理 /
 * Enchants a plus onto an item. -RAK-
 * @param player_ptr プレイヤーへの参照ポインタ
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
bool enchant_equipment(player_type *player_ptr, object_type *o_ptr, int n, int eflag)
{
    /* Large piles resist enchantment */
    int prob = o_ptr->number * 100;

    /* Missiles are easy to enchant */
    if ((o_ptr->tval == TV_BOLT) || (o_ptr->tval == TV_ARROW) || (o_ptr->tval == TV_SHOT)) {
        prob = prob / 20;
    }

    /* Try "n" times */
    int chance;
    bool res = false;
    bool a = o_ptr->is_artifact();
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
                res = true;

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
                res = true;

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
            res = true;

            /* only when you get it above -1 -CFT */
            if (o_ptr->to_a >= 0)
                break_curse(o_ptr);
        }
    }

    /* Failure */
    if (!res)
        return false;
    set_bits(player_ptr->update, PU_BONUS | PU_COMBINE | PU_REORDER);
    set_bits(player_ptr->window_flags, PW_INVEN | PW_EQUIP | PW_PLAYER | PW_FLOOR_ITEM_LIST);

    /* Success */
    return true;
}

/*!
 * @brief 装備修正強化処理のメインルーチン /
 * Enchant an item (in the inventory or on the floor)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param num_hit 命中修正量
 * @param num_dam ダメージ修正量
 * @param num_ac AC修正量
 * @return 強化に成功した場合TRUEを返す
 * @details
 * Note that "num_ac" requires armour, else weapon
 * Returns TRUE if attempted, FALSE if cancelled
 */
bool enchant_spell(player_type *player_ptr, HIT_PROB num_hit, HIT_POINT num_dam, ARMOUR_CLASS num_ac)
{
    /* Assume enchant weapon */
    FuncItemTester item_tester(&object_type::allow_enchant_weapon);

    /* Enchant armor if requested */
    if (num_ac)
        item_tester = FuncItemTester(&object_type::is_armour);

    concptr q = _("どのアイテムを強化しますか? ", "Enchant which item? ");
    concptr s = _("強化できるアイテムがない。", "You have nothing to enchant.");

    OBJECT_IDX item;
    object_type *o_ptr;
    o_ptr = choose_object(player_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), item_tester);
    if (!o_ptr)
        return false;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
#ifdef JP
    msg_format("%s は明るく輝いた！", o_name);
#else
    msg_format("%s %s glow%s brightly!", ((item >= 0) ? "Your" : "The"), o_name, ((o_ptr->number > 1) ? "" : "s"));
#endif

    /* Enchant */
    bool is_enchant_successful = false;
    if (enchant_equipment(player_ptr, o_ptr, num_hit, ENCH_TOHIT))
        is_enchant_successful = true;
    if (enchant_equipment(player_ptr, o_ptr, num_dam, ENCH_TODAM))
        is_enchant_successful = true;
    if (enchant_equipment(player_ptr, o_ptr, num_ac, ENCH_TOAC))
        is_enchant_successful = true;

    if (!is_enchant_successful) {
        if (flush_failure)
            flush();
        msg_print(_("強化に失敗した。", "The enchantment failed."));
        if (one_in_(3))
            chg_virtue(player_ptr, V_ENCHANT, -1);
    } else
        chg_virtue(player_ptr, V_ENCHANT, 1);

    calc_android_exp(player_ptr);

    /* Something happened */
    return true;
}

/*!
 * @brief 武器へのエゴ付加処理 /
 * Brand the current weapon
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param brand_type エゴ化ID(e_info.txtとは連動していない)
 */
void brand_weapon(player_type *player_ptr, int brand_type)
{
    concptr q = _("どの武器を強化しますか? ", "Enchant which weapon? ");
    concptr s = _("強化できる武器がない。", "You have nothing to enchant.");

    OBJECT_IDX item;
    object_type *o_ptr;
    o_ptr = choose_object(player_ptr, &item, q, s, USE_EQUIP | IGNORE_BOTHHAND_SLOT, FuncItemTester(&object_type::allow_enchant_melee_weapon));
    if (!o_ptr)
        return;

    bool is_special_item = o_ptr->k_idx && !o_ptr->is_artifact() && !o_ptr->is_ego() && !o_ptr->is_cursed()
        && !((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_POISON_NEEDLE)) && !((o_ptr->tval == TV_POLEARM) && (o_ptr->sval == SV_DEATH_SCYTHE))
        && !((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_DIAMOND_EDGE));
    if (!is_special_item) {
        if (flush_failure)
            flush();

        msg_print(_("属性付加に失敗した。", "The branding failed."));
        chg_virtue(player_ptr, V_ENCHANT, -2);
        calc_android_exp(player_ptr);
        return;
    }

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

    concptr act = nullptr;
    switch (brand_type) {
    case 17:
        if (o_ptr->tval == TV_SWORD) {
            act = _("は鋭さを増した！", "becomes very sharp!");

            o_ptr->name2 = EGO_SHARPNESS;
            o_ptr->pval = (PARAMETER_VALUE)m_bonus(5, player_ptr->current_floor_ptr->dun_level) + 1;

            if ((o_ptr->sval == SV_HAYABUSA) && (o_ptr->pval > 2))
                o_ptr->pval = 2;
        } else {
            act = _("は破壊力を増した！", "seems very powerful.");
            o_ptr->name2 = EGO_EARTHQUAKES;
            o_ptr->pval = (PARAMETER_VALUE)m_bonus(3, player_ptr->current_floor_ptr->dun_level);
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
    enchant_equipment(player_ptr, o_ptr, randint0(3) + 4, ENCH_TOHIT | ENCH_TODAM);
    o_ptr->discount = 99;
    chg_virtue(player_ptr, V_ENCHANT, 2);
    calc_android_exp(player_ptr);
}
