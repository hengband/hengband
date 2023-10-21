/*!
 * @brief アイテムに影響のある魔法の処理
 * @date 2019/01/22
 * @author deskull
 */

#include "spell/spells-object.h"
#include "artifact/fixed-art-types.h"
#include "avatar/avatar.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "game-option/disturbance-options.h"
#include "inventory/inventory-slot-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/item-magic-applier.h"
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
#include "perception/object-perception.h"
#include "player-info/class-info.h"
#include "racial/racial-android.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem-info.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "util/probability-table.h"
#include "view/display-messages.h"

/*!
 * @brief 装備強化処理の失敗率定数 (千分率)
 * @details 強化値が負値から0までは必ず成功する
 * 正値は+15までしか鍛えることができず、+16以上への強化を試みると確実に失敗する
 */
static constexpr std::array<int, 16> enchant_table = { { 0, 10, 50, 100, 200, 300, 400, 500, 650, 800, 950, 987, 993, 995, 998, 1000 } };

/*
 * Scatter some "amusing" objects near the player
 */
enum class AmusementFlagType : byte {
    NOTHING, /* No restriction */
    NO_UNIQUE, /* Don't make the amusing object of uniques */
    FIXED_ART, /* Make a fixed artifact based on the amusing object */
    MULTIPLE, /* Drop 1-3 objects for one type */
    PILE, /* Drop 1-99 pile objects for one type */
};

class AmuseDefinition {
public:
    AmuseDefinition(const BaseitemKey &key, PERCENTAGE prob, AmusementFlagType flag)
        : key(key)
        , prob(prob)
        , flag(flag)
    {
    }

    BaseitemKey key;
    PERCENTAGE prob;
    AmusementFlagType flag;
};

static const std::array<AmuseDefinition, 13> amuse_info = { {
    { { ItemKindType::BOTTLE }, 5, AmusementFlagType::NOTHING },
    { { ItemKindType::JUNK }, 3, AmusementFlagType::MULTIPLE },
    { { ItemKindType::SPIKE }, 10, AmusementFlagType::PILE },
    { { ItemKindType::STATUE }, 15, AmusementFlagType::NOTHING },
    { { ItemKindType::CORPSE }, 15, AmusementFlagType::NO_UNIQUE },
    { { ItemKindType::SKELETON }, 10, AmusementFlagType::NO_UNIQUE },
    { { ItemKindType::FIGURINE }, 10, AmusementFlagType::NO_UNIQUE },
    { { ItemKindType::PARCHMENT }, 1, AmusementFlagType::NOTHING },
    { { ItemKindType::POLEARM, SV_TSURIZAO }, 3, AmusementFlagType::NOTHING }, // Fishing Pole of Taikobo
    { { ItemKindType::SWORD, SV_BROKEN_DAGGER }, 3, AmusementFlagType::FIXED_ART }, // Broken Dagger of Magician
    { { ItemKindType::SWORD, SV_BROKEN_DAGGER }, 10, AmusementFlagType::NOTHING },
    { { ItemKindType::SWORD, SV_BROKEN_SWORD }, 5, AmusementFlagType::NOTHING },
    { { ItemKindType::SCROLL, SV_SCROLL_AMUSEMENT }, 10, AmusementFlagType::NOTHING },
} };

static std::optional<FixedArtifactId> sweep_amusement_artifact(const bool insta_art, const short bi_id)
{
    for (const auto &[a_idx, artifact] : artifacts_info) {
        if (a_idx == FixedArtifactId::NONE) {
            continue;
        }

        if (insta_art && !artifact.gen_flags.has(ItemGenerationTraitType::INSTA_ART)) {
            continue;
        }

        if (artifact.bi_key != baseitems_info[bi_id].bi_key) {
            continue;
        }

        if (artifact.is_generated) {
            continue;
        }

        return a_idx;
    }

    return std::nullopt;
}

/*!
 * @brief 誰得ドロップを行う。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param num 誰得の処理回数
 * @param known TRUEならばオブジェクトが必ず＊鑑定＊済になる
 */
void generate_amusement(PlayerType *player_ptr, int num, bool known)
{
    ProbabilityTable<const AmuseDefinition *> pt;
    for (const auto &am_ref : amuse_info) {
        pt.entry_item(&am_ref, am_ref.prob);
    }

    for (auto i = 0; i < num; i++) {
        auto am_ptr = pt.pick_one_at_random();
        const auto bi_id = lookup_baseitem_id(am_ptr->key);
        if (bi_id == 0) {
            continue;
        }

        const auto insta_art = baseitems_info[bi_id].gen_flags.has(ItemGenerationTraitType::INSTA_ART);
        const auto flag = am_ptr->flag;
        const auto fixed_art = flag == AmusementFlagType::FIXED_ART;
        std::optional<FixedArtifactId> opt_a_idx(std::nullopt);
        if (insta_art || fixed_art) {
            opt_a_idx = sweep_amusement_artifact(insta_art, bi_id);
            if (!opt_a_idx.has_value()) {
                continue;
            }
        }

        ItemEntity item;
        item.prep(bi_id);
        if (opt_a_idx.has_value()) {
            item.fixed_artifact_idx = opt_a_idx.value();
        }

        ItemMagicApplier(player_ptr, &item, 1, AM_NO_FIXED_ART).execute();
        if (flag == AmusementFlagType::NO_UNIQUE) {
            if (monraces_info[i2enum<MonsterRaceId>(item.pval)].kind_flags.has(MonsterKindType::UNIQUE)) {
                continue;
            }
        }

        if (flag == AmusementFlagType::MULTIPLE) {
            item.number = randint1(3);
        }

        if (flag == AmusementFlagType::PILE) {
            item.number = randint1(99);
        }

        if (known) {
            object_aware(player_ptr, &item);
            item.mark_as_known();
        }

        (void)drop_near(player_ptr, &item, -1, player_ptr->y, player_ptr->x);
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
 */
void acquirement(PlayerType *player_ptr, POSITION y1, POSITION x1, int num, bool great)
{
    auto mode = AM_GOOD | (great ? AM_GREAT : AM_NONE);
    for (auto i = 0; i < num; i++) {
        ItemEntity item;
        if (!make_object(player_ptr, &item, mode)) {
            continue;
        }

        (void)drop_near(player_ptr, &item, -1, y1, x1);
    }
}

/*!
 * @brief 防具呪縛処理 /
 * Curse the players armor
 * @return 何も持っていない場合を除き、常にTRUEを返す
 * @todo 元のreturnは間違っているが、修正後の↓文がどれくらい正しいかは要チェック
 */
bool curse_armor(PlayerType *player_ptr)
{
    /* Curse the body armor */
    auto *o_ptr = &player_ptr->inventory_list[INVEN_BODY];
    if (!o_ptr->is_valid()) {
        return false;
    }

    const auto item_name = describe_flavor(player_ptr, o_ptr, OD_OMIT_PREFIX);

    if (o_ptr->is_fixed_or_random_artifact() && one_in_(2)) {
#ifdef JP
        msg_format("%sが%sを包み込もうとしたが、%sはそれを跳ね返した！", "恐怖の暗黒オーラ", "防具", item_name.data());
#else
        msg_format("A %s tries to %s, but your %s resists the effects!", "terrible black aura", "surround your armor", item_name.data());
#endif
        return true;
    }

    msg_format(_("恐怖の暗黒オーラがあなたの%sを包み込んだ！", "A terrible black aura blasts your %s!"), item_name.data());
    chg_virtue(player_ptr, Virtue::ENCHANT, -5);
    o_ptr->fixed_artifact_idx = FixedArtifactId::NONE;
    o_ptr->ego_idx = EgoType::BLASTED;
    o_ptr->to_a = 0 - randint1(5) - randint1(5);
    o_ptr->to_h = 0;
    o_ptr->to_d = 0;
    o_ptr->ac = 0;
    o_ptr->dd = 0;
    o_ptr->ds = 0;
    o_ptr->art_flags.clear();
    o_ptr->curse_flags.set(CurseTraitType::CURSED);
    o_ptr->ident |= IDENT_BROKEN;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::MP,
    };
    rfu.set_flags(flags_srf);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::PLAYER,
    };
    rfu.set_flags(flags_swrf);
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
bool curse_weapon_object(PlayerType *player_ptr, bool force, ItemEntity *o_ptr)
{
    if (!o_ptr->is_valid()) {
        return false;
    }

    const auto item_name = describe_flavor(player_ptr, o_ptr, OD_OMIT_PREFIX);
    if (o_ptr->is_fixed_or_random_artifact() && one_in_(2) && !force) {
#ifdef JP
        msg_format("%sが%sを包み込もうとしたが、%sはそれを跳ね返した！", "恐怖の暗黒オーラ", "武器", item_name.data());
#else
        msg_format("A %s tries to %s, but your %s resists the effects!", "terrible black aura", "surround your weapon", item_name.data());
#endif
        return true;
    }

    if (!force) {
        msg_format(_("恐怖の暗黒オーラがあなたの%sを包み込んだ！", "A terrible black aura blasts your %s!"), item_name.data());
    }

    chg_virtue(player_ptr, Virtue::ENCHANT, -5);
    o_ptr->fixed_artifact_idx = FixedArtifactId::NONE;
    o_ptr->ego_idx = EgoType::SHATTERED;
    o_ptr->to_h = 0 - randint1(5) - randint1(5);
    o_ptr->to_d = 0 - randint1(5) - randint1(5);
    o_ptr->to_a = 0;
    o_ptr->ac = 0;
    o_ptr->dd = 0;
    o_ptr->ds = 0;
    o_ptr->art_flags.clear();
    o_ptr->curse_flags.set(CurseTraitType::CURSED);
    o_ptr->ident |= IDENT_BROKEN;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::MP,
    };
    rfu.set_flags(flags_srf);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::PLAYER,
    };
    rfu.set_flags(flags_swrf);
    return true;
}

/*!
 * @brief ボルトのエゴ化処理(火炎エゴのみ) /
 * Enchant some bolts
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void brand_bolts(PlayerType *player_ptr)
{
    for (auto i = 0; i < INVEN_PACK; i++) {
        auto *o_ptr = &player_ptr->inventory_list[i];
        if (o_ptr->bi_key.tval() != ItemKindType::BOLT) {
            continue;
        }

        if (o_ptr->is_fixed_or_random_artifact() || o_ptr->is_ego()) {
            continue;
        }

        if (o_ptr->is_cursed() || o_ptr->is_broken()) {
            continue;
        }

        if (randint0(100) < 75) {
            continue;
        }

        msg_print(_("クロスボウの矢が炎のオーラに包まれた！", "Your bolts are covered in a fiery aura!"));
        o_ptr->ego_idx = EgoType::FLAME;
        enchant_equipment(o_ptr, randint0(3) + 4, ENCH_TOHIT | ENCH_TODAM);
        return;
    }

    if (flush_failure) {
        flush();
    }

    msg_print(_("炎で強化するのに失敗した。", "The fiery enchantment failed."));
}

/*!
 * @brief 呪いの打ち破り処理 /
 * Break the curse of an item
 * @param o_ptr 呪い装備情報の参照ポインタ
 */
static void break_curse(ItemEntity *o_ptr)
{
    const auto has_heavier_curse = o_ptr->curse_flags.has_any_of({ CurseTraitType::PERMA_CURSE, CurseTraitType::HEAVY_CURSE });
    const auto is_curse_broken = o_ptr->is_cursed() && !has_heavier_curse && one_in_(4);
    if (!is_curse_broken) {
        return;
    }

    msg_print(_("かけられていた呪いが打ち破られた！", "The curse is broken!"));
    o_ptr->curse_flags.clear();
    o_ptr->ident |= IDENT_SENSE;
    o_ptr->feeling = FEEL_NONE;
}

/*!
 * @brief 装備修正強化処理
 * @param o_ptr 強化するアイテムの参照ポインタ
 * @param n 強化基本量
 * @param eflag 強化オプション(命中/ダメージ/AC)
 * @return 強化に成功した場合TRUEを返す
 */
bool enchant_equipment(ItemEntity *o_ptr, int n, int eflag)
{
    auto prob = o_ptr->number * 100;
    if (o_ptr->is_ammo()) {
        prob = prob / 20;
    }

    int chance;
    auto res = false;
    auto a = o_ptr->is_fixed_or_random_artifact();
    auto force = (eflag & ENCH_FORCE);
    for (int i = 0; i < n; i++) {
        if (!force && randint0(prob) >= 100) {
            continue;
        }

        if (eflag & ENCH_TOHIT) {
            if (o_ptr->to_h < 0) {
                chance = 0;
            } else if (o_ptr->to_h > 15) {
                chance = 1000;
            } else {
                chance = enchant_table[o_ptr->to_h];
            }

            if (force || ((randint1(1000) > chance) && (!a || (randint0(100) < 50)))) {
                o_ptr->to_h++;
                res = true;
                if (o_ptr->to_h >= 0) {
                    break_curse(o_ptr);
                }
            }
        }

        if (eflag & ENCH_TODAM) {
            if (o_ptr->to_d < 0) {
                chance = 0;
            } else if (o_ptr->to_d > 15) {
                chance = 1000;
            } else {
                chance = enchant_table[o_ptr->to_d];
            }

            if (force || ((randint1(1000) > chance) && (!a || (randint0(100) < 50)))) {
                o_ptr->to_d++;
                res = true;
                if (o_ptr->to_d >= 0) {
                    break_curse(o_ptr);
                }
            }
        }

        if (!(eflag & ENCH_TOAC)) {
            continue;
        }

        if (o_ptr->to_a < 0) {
            chance = 0;
        } else if (o_ptr->to_a > 15) {
            chance = 1000;
        } else {
            chance = enchant_table[o_ptr->to_a];
        }

        if (force || ((randint1(1000) > chance) && (!a || (randint0(100) < 50)))) {
            o_ptr->to_a++;
            res = true;
            if (o_ptr->to_a >= 0) {
                break_curse(o_ptr);
            }
        }
    }

    if (!res) {
        return false;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::COMBINATION,
        StatusRecalculatingFlag::REORDER,
    };
    rfu.set_flags(flags_srf);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::PLAYER,
        SubWindowRedrawingFlag::FLOOR_ITEMS,
        SubWindowRedrawingFlag::FOUND_ITEMS,
    };
    rfu.set_flags(flags_swrf);
    return true;
}

/*!
 * @brief 装備修正強化処理のメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param num_hit 命中修正量
 * @param num_dam ダメージ修正量
 * @param num_ac AC修正量
 * @return 強化に成功した場合TRUEを返す
 */
bool enchant_spell(PlayerType *player_ptr, HIT_PROB num_hit, int num_dam, ARMOUR_CLASS num_ac)
{
    FuncItemTester item_tester(&ItemEntity::allow_enchant_weapon);
    if (num_ac) {
        item_tester = FuncItemTester(&ItemEntity::is_protector);
    }

    constexpr auto q = _("どのアイテムを強化しますか? ", "Enchant which item? ");
    constexpr auto s = _("強化できるアイテムがない。", "You have nothing to enchant.");
    short i_idx;
    const auto options = USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT;
    auto *o_ptr = choose_object(player_ptr, &i_idx, q, s, options, item_tester);
    if (!o_ptr) {
        return false;
    }

    const auto item_name = describe_flavor(player_ptr, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
#ifdef JP
    msg_format("%s は明るく輝いた！", item_name.data());
#else
    msg_format("%s %s glow%s brightly!", ((i_idx >= 0) ? "Your" : "The"), item_name.data(), ((o_ptr->number > 1) ? "" : "s"));
#endif

    auto is_enchant_successful = false;
    if (enchant_equipment(o_ptr, num_hit, ENCH_TOHIT)) {
        is_enchant_successful = true;
    }

    if (enchant_equipment(o_ptr, num_dam, ENCH_TODAM)) {
        is_enchant_successful = true;
    }

    if (enchant_equipment(o_ptr, num_ac, ENCH_TOAC)) {
        is_enchant_successful = true;
    }

    if (!is_enchant_successful) {
        if (flush_failure) {
            flush();
        }
        msg_print(_("強化に失敗した。", "The enchantment failed."));
        if (one_in_(3)) {
            chg_virtue(player_ptr, Virtue::ENCHANT, -1);
        }
    } else {
        chg_virtue(player_ptr, Virtue::ENCHANT, 1);
    }

    calc_android_exp(player_ptr);
    return true;
}

/*!
 * @brief 武器へのエゴ付加処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param brand_type エゴ化ID(EgoDefinitionsとは連動していない)
 */
void brand_weapon(PlayerType *player_ptr, int brand_type)
{
    constexpr auto q = _("どの武器を強化しますか? ", "Enchant which weapon? ");
    constexpr auto s = _("強化できる武器がない。", "You have nothing to enchant.");
    short i_idx;
    const auto options = USE_EQUIP | IGNORE_BOTHHAND_SLOT;
    auto *o_ptr = choose_object(player_ptr, &i_idx, q, s, options, FuncItemTester(&ItemEntity::allow_enchant_melee_weapon));
    if (o_ptr == nullptr) {
        return;
    }

    const auto &bi_key = o_ptr->bi_key;
    auto special_weapon = bi_key == BaseitemKey(ItemKindType::SWORD, SV_POISON_NEEDLE);
    special_weapon |= bi_key == BaseitemKey(ItemKindType::POLEARM, SV_DEATH_SCYTHE);
    special_weapon |= bi_key == BaseitemKey(ItemKindType::SWORD, SV_DIAMOND_EDGE);
    const auto is_normal_item = o_ptr->is_valid() && !o_ptr->is_fixed_or_random_artifact() && !o_ptr->is_ego() && !o_ptr->is_cursed() && !special_weapon;
    if (!is_normal_item) {
        if (flush_failure) {
            flush();
        }

        msg_print(_("属性付加に失敗した。", "The branding failed."));
        chg_virtue(player_ptr, Virtue::ENCHANT, -2);
        calc_android_exp(player_ptr);
        return;
    }

    const auto item_name = describe_flavor(player_ptr, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    concptr act = nullptr;
    switch (brand_type) {
    case 17:
        if (o_ptr->bi_key.tval() == ItemKindType::SWORD) {
            act = _("は鋭さを増した！", "becomes very sharp!");
            o_ptr->ego_idx = EgoType::SHARPNESS;
            o_ptr->pval = (PARAMETER_VALUE)m_bonus(5, player_ptr->current_floor_ptr->dun_level) + 1;
            if ((o_ptr->bi_key.sval() == SV_HAYABUSA) && (o_ptr->pval > 2)) {
                o_ptr->pval = 2;
            }
        } else {
            act = _("は破壊力を増した！", "seems very powerful.");
            o_ptr->ego_idx = EgoType::EARTHQUAKES;
            o_ptr->pval = (PARAMETER_VALUE)m_bonus(3, player_ptr->current_floor_ptr->dun_level);
        }

        break;
    case 16:
        act = _("は人間の血を求めている！", "seems to be looking for humans!");
        o_ptr->ego_idx = EgoType::KILL_HUMAN;
        break;
    case 15:
        act = _("は電撃に覆われた！", "covered with lightning!");
        o_ptr->ego_idx = EgoType::BRAND_ELEC;
        break;
    case 14:
        act = _("は酸に覆われた！", "coated with acid!");
        o_ptr->ego_idx = EgoType::BRAND_ACID;
        break;
    case 13:
        act = _("は邪悪なる怪物を求めている！", "seems to be looking for evil monsters!");
        o_ptr->ego_idx = EgoType::KILL_EVIL;
        break;
    case 12:
        act = _("は異世界の住人の肉体を求めている！", "seems to be looking for demons!");
        o_ptr->ego_idx = EgoType::KILL_DEMON;
        break;
    case 11:
        act = _("は屍を求めている！", "seems to be looking for undead!");
        o_ptr->ego_idx = EgoType::KILL_UNDEAD;
        break;
    case 10:
        act = _("は動物の血を求めている！", "seems to be looking for animals!");
        o_ptr->ego_idx = EgoType::KILL_ANIMAL;
        break;
    case 9:
        act = _("はドラゴンの血を求めている！", "seems to be looking for dragons!");
        o_ptr->ego_idx = EgoType::KILL_DRAGON;
        break;
    case 8:
        act = _("はトロルの血を求めている！", "seems to be looking for troll!s");
        o_ptr->ego_idx = EgoType::KILL_TROLL;
        break;
    case 7:
        act = _("はオークの血を求めている！", "seems to be looking for orcs!");
        o_ptr->ego_idx = EgoType::KILL_ORC;
        break;
    case 6:
        act = _("は巨人の血を求めている！", "seems to be looking for giants!");
        o_ptr->ego_idx = EgoType::KILL_GIANT;
        break;
    case 5:
        act = _("は非常に不安定になったようだ。", "seems very unstable now.");
        o_ptr->ego_idx = EgoType::TRUMP;
        o_ptr->pval = randint1(2);
        break;
    case 4:
        act = _("は血を求めている！", "thirsts for blood!");
        o_ptr->ego_idx = EgoType::VAMPIRIC;
        break;
    case 3:
        act = _("は毒に覆われた。", "is coated with poison.");
        o_ptr->ego_idx = EgoType::BRAND_POIS;
        break;
    case 2:
        act = _("は純ログルスに飲み込まれた。", "is engulfed in raw Logrus!");
        o_ptr->ego_idx = EgoType::CHAOTIC;
        break;
    case 1:
        act = _("は炎のシールドに覆われた！", "is covered in a fiery shield!");
        o_ptr->ego_idx = EgoType::BRAND_FIRE;
        break;
    default:
        act = _("は深く冷たいブルーに輝いた！", "glows deep, icy blue!");
        o_ptr->ego_idx = EgoType::BRAND_COLD;
        break;
    }

    msg_format(_("あなたの%s%s", "Your %s %s"), item_name.data(), act);
    enchant_equipment(o_ptr, randint0(3) + 4, ENCH_TOHIT | ENCH_TODAM);
    o_ptr->discount = 99;
    chg_virtue(player_ptr, Virtue::ENCHANT, 2);
    calc_android_exp(player_ptr);
}
