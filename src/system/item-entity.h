#pragma once

/*
 * @file item-entity.h
 * @brief アイテム定義の構造体とエンティティ処理定義
 * @author Hourier
 * @date 2021/05/02
 */

#include "object-enchant/object-ego.h"
#include "object-enchant/tr-flags.h"
#include "object-enchant/trc-types.h"
#include "object/object-mark-types.h"
#include "system/angband.h"
#include "system/baseitem/baseitem-key.h"
#include "system/system-variables.h"
#include "util/dice.h"
#include "util/flag-group.h"
#include "util/point-2d.h"
#include <string>
#include <tl/optional.hpp>
#include <vector>

enum class FixedArtifactId : short;
enum class ItemKindType : short;
enum class MonraceId : short;
enum class QuestId : short;
enum class RandomArtifactBias : int;
enum class RandomArtActType : short;
enum class SmithEffectType : short;
class ActivationType;
class ArtifactType;
class BaseitemDefinition;
class DisplaySymbol;
class EgoItemDefinition;
class MonraceDefinition;
class ItemEntity {
public:
    ItemEntity();
    explicit ItemEntity(short bi_id);
    explicit ItemEntity(const BaseitemKey &bi_key);
    ItemEntity(ItemEntity &&) = default;
    ItemEntity &operator=(ItemEntity &&) = default;

    short bi_id{}; /*!< ベースアイテムID (0は、不具合調査用の無効アイテム または 何も装備していない箇所のアイテム であることを示す) */
    POSITION iy{}; /*!< Y-position on map, or zero */
    POSITION ix{}; /*!< X-position on map, or zero */
    IDX stack_idx{}; /*!< このアイテムを含むアイテムリスト内の位置(降順) */
    BaseitemKey bi_key;
    PARAMETER_VALUE pval{}; /*!< Item extra-parameter */
    byte discount{}; /*!< ゲーム中の値引き率 (0～100) / Discount (if any) */
    ITEM_NUMBER number{}; /*!< Number of items */
    WEIGHT weight{}; /*!< Item weight */
    FixedArtifactId fa_id; /*!< 固定アーティファクト番号 (固定アーティファクトでないなら0) */
    EgoType ego_idx{}; /*!< エゴ番号 (エゴでないなら0) */

    RandomArtActType activation_id{}; /*!< エゴ/アーティファクトの発動ID / Extra info activation index */
    byte chest_level = 0; /*!< 箱の中身レベル */
    uint8_t captured_monster_speed = 0; /*!< 捕らえたモンスターの速度 */
    short captured_monster_current_hp = 0; /*!< 捕らえたモンスターの現HP */
    short captured_monster_max_hp = 0; /*!< 捕らえたモンスターの最大HP */
    short fuel = 0; /*!< 光源の残り寿命 / Remaining fuel */

    byte smith_hit = 0; /*!< 鍛冶をした結果上昇した命中値 */
    byte smith_damage = 0; /*!< 鍛冶をした結果上昇したダメージ */
    tl::optional<SmithEffectType> smith_effect; //!< 鍛冶で付与された効果
    tl::optional<RandomArtActType> smith_act_idx; //!< 鍛冶で付与された発動効果のID

    HIT_PROB to_h{}; /*!< Plusses to hit */
    int to_d{}; /*!< Plusses to damage */
    ARMOUR_CLASS to_a{}; /*!< Plusses to AC */
    ARMOUR_CLASS ac{}; /*!< Normal AC */

    Dice damage_dice{}; /*!< Damage dice */
    TIME_EFFECT timeout{}; /*!< Timeout Counter */
    byte ident{}; /*!< Special flags  */
    EnumClassFlagGroup<OmType> marked{}; /*!< Object is marked */
    tl::optional<std::string> inscription{}; /*!< Inscription */
    tl::optional<std::string> randart_name{}; /*!< Artifact name (random artifacts) */
    byte feeling{}; /*!< Game generated inscription number (eg, pseudo-id) */

    TrFlags art_flags{}; /*!< Extra Flags for ego and artifacts */
    EnumClassFlagGroup<CurseTraitType> curse_flags{}; /*!< Flags for curse */
    MONSTER_IDX held_m_idx{}; /*!< アイテムを所持しているモンスターID (いないなら 0) / Monster holding us (if any) */
    RandomArtifactBias artifact_bias{}; /*!< ランダムアーティファクト生成時のバイアスID */

    void wipe();
    ItemEntity clone() const;
    void generate(const BaseitemKey &new_bi_key);
    void generate(short new_bi_id);
    bool is(ItemKindType tval) const;
    bool is_weapon() const;
    bool is_weapon_ammo() const;
    bool is_weapon_armour_ammo() const;
    bool is_melee_weapon() const;
    bool is_melee_ammo() const;
    bool is_wearable() const;
    bool is_equipment() const;
    bool is_orthodox_melee_weapons() const;
    bool is_broken_weapon() const;
    bool is_throwable() const;
    bool is_wieldable_in_etheir_hand() const;
    bool allow_enchant_weapon() const;
    bool allow_enchant_melee_weapon() const;
    bool allow_two_hands_wielding() const;
    bool is_ammo() const;
    bool is_convertible() const;
    bool is_lance() const;
    bool is_protector() const;
    bool can_be_aura_protector() const;
    bool is_rare() const;
    bool is_ego() const;
    bool is_smith() const;
    bool is_fixed_or_random_artifact() const;
    bool is_fixed_artifact() const;
    bool is_random_artifact() const;
    bool is_nameless() const;
    bool is_valid() const;
    bool is_broken() const;
    bool is_cursed() const;
    bool is_held_by_monster() const;
    bool is_known() const;
    bool is_fully_known() const;
    bool is_aware() const;
    bool is_tried() const;
    bool is_potion() const;
    bool is_readable() const;
    bool can_refill_lantern() const;
    bool can_refill_torch() const;
    bool can_recharge() const;
    bool is_offerable() const;
    bool is_activatable() const;
    bool is_fuel() const;
    bool is_spell_book() const;
    bool is_glove_same_temper(const ItemEntity *j_ptr) const;
    bool can_pile(const ItemEntity *j_ptr) const;
    DisplaySymbol get_symbol() const;
    int calc_price() const;
    bool is_specific_artifact(FixedArtifactId id) const;
    bool has_unidentified_name() const;
    ItemKindType get_arrow_kind() const;
    bool is_wand_rod() const;
    bool is_wand_staff() const;
    short get_bow_energy() const;
    int get_arrow_magnification() const;
    bool is_aiming_rod() const;
    bool is_lite_requiring_fuel() const;
    bool is_junk() const;
    bool is_armour() const;
    bool is_cross_bow() const;
    bool is_corpse() const;
    bool is_inscribed() const;
    std::vector<ActivationType>::const_iterator find_activation_info() const;
    bool has_activation() const;
    bool has_bias() const;
    bool is_bounty() const;
    bool is_target_of(QuestId quest_id) const;
    BaseitemDefinition &get_baseitem() const;
    EgoItemDefinition &get_ego() const;
    ArtifactType &get_fixed_artifact();
    const ArtifactType &get_fixed_artifact() const;
    TrFlags get_flags() const;
    TrFlags get_flags_known() const;
    std::string explain_activation() const;
    bool has_monrace() const;
    const MonraceDefinition &get_monrace() const;
    void track_baseitem() const;
    bool is_similar(const ItemEntity &other) const;
    int is_similar_part(const ItemEntity &other) const;
    bool is_similar_for_store(const ItemEntity &other) const;
    int get_baseitem_level() const;
    short get_baseitem_pval() const;
    bool is_worthless() const;
    int get_baseitem_cost() const;
    MonraceId get_monrace_id() const;
    int get_lite_radius() const;
    Pos2D get_position() const;

    void mark_as_known();
    void mark_as_tried() const;

    void set_position(const Pos2D &pos);
    bool try_become_artifact(int dungeon_level);
    void absorb(ItemEntity &other);

private:
    ItemEntity(const ItemEntity &) = default;
    ItemEntity &operator=(const ItemEntity &) = default;

    int get_baseitem_price() const;
    int calc_figurine_value() const;
    int calc_capture_value() const;
    bool should_refuse_enchant() const;
    void modify_ego_lite_flags(TrFlags &flags) const;
    RandomArtActType get_activation_index() const;
    std::string build_activation_description() const;
    std::string build_timeout_description(const ActivationType &act) const;
    std::string build_activation_description(const ActivationType &act) const;
    std::string build_activation_description_dragon_breath() const;
    uint8_t get_color() const;
    char get_character() const;

    std::string build_item_info_for_debug() const;
};
