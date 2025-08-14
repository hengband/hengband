#include "system/monrace/monrace-definition.h"
#include "game-option/cheat-options.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-race/race-ability-mask.h"
#include "monster-race/race-resistance-mask.h"
#include "monster-race/race-sex.h"
#include "monster/horror-descriptions.h"
#include "system/enums/grid-flow.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-list.h"
#include "system/monrace/monrace-message.h"
#include "system/system-variables.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "world/world.h"
#include <algorithm>
#ifdef JP
#else
#include "locale/english.h"
#endif

namespace {
constexpr auto MAX_MONSTER_NUM = 100; /*!< 1種類の非ユニークモンスターが1フロアに存在できる最大数 */

template <class T>
static int count_lore_mflag_group(const EnumClassFlagGroup<T> &flags, const EnumClassFlagGroup<T> &r_flags)
{
    auto result_flags = flags;
    auto num = result_flags.reset(r_flags).count();
    return num;
}
}

DropArtifact::DropArtifact(FixedArtifactId fa_id, int chance)
    : fa_id(fa_id)
    , chance(chance)
{
}

MonsterSummon::MonsterSummon(MonraceId id, int probability, int min_num, int max_num, int radius)
    : id(id)
    , probability(probability)
    , min_num(min_num)
    , max_num(max_num)
    , radius(radius)
{
}

Reinforce::Reinforce(MonraceId monrace_id, Dice dice)
    : monrace_id(monrace_id)
    , dice(dice)
{
}

MonraceId Reinforce::get_monrace_id() const
{
    return this->monrace_id;
}

bool Reinforce::is_valid() const
{
    return MonraceList::is_valid(this->monrace_id) && this->dice.is_valid();
}

const MonraceDefinition &Reinforce::get_monrace() const
{
    return MonraceList::get_instance().get_monrace(this->monrace_id);
}

std::string Reinforce::get_dice_as_string() const
{
    return this->dice.to_string();
}

int Reinforce::roll_dice() const
{
    return this->dice.roll();
}

int Reinforce::roll_max_dice() const
{
    return this->dice.maxroll();
}

MonraceDefinition::MonraceDefinition()
    : idx(MonraceId::PLAYER)
{
}

/*!
 * @brief 正当なモンスター (実際に存在するモンスター種族IDである)かどうかを調べる
 * @details モンスター種族IDが MonsterRaceDefinitions に実在するもの(MonraceId::PLAYERは除く)であるかどうかの用途の他、
 * m_list 上の要素などの r_idx にMonraceId::PLAYER を入れることで死亡扱いとして使われるのでその判定に使用する事もある
 * @return 正当なものであれば true、そうでなければ false
 * @todo 将来的に定義側のIDが廃止されたら有効フラグのフィールド変数を代わりに作る.
 */
bool MonraceDefinition::is_valid() const
{
    return this->idx != MonraceId::PLAYER;
}

bool MonraceDefinition::is_male() const
{
    return this->sex == MonsterSex::MALE;
}

bool MonraceDefinition::is_female() const
{
    return this->sex == MonsterSex::FEMALE;
}

/*!
 * @brief モンスターが生命体かどうかを返す
 * @return 生命体ならばtrue
 */
bool MonraceDefinition::has_living_flag() const
{
    return this->kind_flags.has_none_of({ MonsterKindType::DEMON, MonsterKindType::UNDEAD, MonsterKindType::NONLIVING });
}

/*!
 * @brief モンスターが自爆するか否か
 * @return 自爆するならtrue
 */
bool MonraceDefinition::is_explodable() const
{
    return std::any_of(std::begin(this->blows), std::end(this->blows),
        [](const auto &blow) { return blow.method == RaceBlowMethodType::EXPLODE; });
}

/*!
 * @brief モンスターが表面的に天使かどうかを返す
 * @return 天使か否か
 * @details 「Aシンボルだが天使ではないモンスター」もいる。例外リストはMonraceList 側に定義されている
 */
bool MonraceDefinition::is_angel_superficially() const
{
    auto is_angel = this->symbol_char_is_any_of("A");
    is_angel |= this->kind_flags.has(MonsterKindType::ANGEL);
    return is_angel;
}

/*!
 * @brief モンスターのシンボル文字が指定された文字列に含まれるかどうかを返す
 * @param candidate_chars シンボル文字の集合の文字列。"pht" のように複数の文字を指定可能。
 * @return モンスターのシンボル文字が candidate_chars に含まれるならばtrue
 * @note ASCIIのみ対応。マルチバイト文字が指定された場合の動作は未定義。
 */
bool MonraceDefinition::symbol_char_is_any_of(std::string_view candidate_chars) const
{
    return candidate_chars.find(this->symbol_definition.character) != std::string_view::npos;
}

/*!
 * @brief モンスターを撃破した際の述語メッセージを返す
 * @return 撃破されたモンスターの述語
 */
std::string MonraceDefinition::get_died_message() const
{
    const auto is_explodable = this->is_explodable();
    if (this->has_living_flag()) {
        return is_explodable ? _("は爆発して死んだ。", " explodes and dies.") : _("は死んだ。", " dies.");
    }

    return is_explodable ? _("は爆発して粉々になった。", " explodes into tiny shreds.") : _("を倒した。", " is destroyed.");
}

tl::optional<bool> MonraceDefinition::order_level(const MonraceDefinition &other) const
{
    if (this->level > other.level) {
        return true;
    }

    if (this->level < other.level) {
        return false;
    }

    return tl::nullopt;
}

bool MonraceDefinition::order_level_strictly(const MonraceDefinition &other) const
{
    return this->level > other.level;
}

tl::optional<bool> MonraceDefinition::order_pet(const MonraceDefinition &other) const
{
    if (this->kind_flags.has(MonsterKindType::UNIQUE) && other.kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return true;
    }

    if (this->kind_flags.has_not(MonsterKindType::UNIQUE) && other.kind_flags.has(MonsterKindType::UNIQUE)) {
        return false;
    }

    return this->order_level(other);
}

std::string MonraceDefinition::get_pronoun_of_summoned_kin() const
{
    if (this->kind_flags.has(MonsterKindType::UNIQUE)) {
        return _("手下", "minions");
    }
    switch (this->idx) {
    case MonraceId::LAFFEY_II:
        return _("ウサウサストライカー", "Bunbun Strikers");
    default:
        return _("仲間", "kin");
    }
}

/*!
 * @brief 進化先モンスターを返す. 進化しなければプレイヤー (無効値の意)
 * @return 進化先モンスター
 */
const MonraceDefinition &MonraceDefinition::get_next() const
{
    return MonraceList::get_instance().get_monrace(this->next_r_idx);
}

/*!
 * @brief モンスター種族が賞金首の対象かどうかを調べる。日替わり賞金首は対象外。
 * @param unachieved_only true の場合未達成の賞金首のみを対象とする。false の場合達成未達成に関わらずすべての賞金首を対象とする。
 * @return モンスター種族が賞金首の対象ならば true、そうでなければ false
 */
bool MonraceDefinition::is_bounty(bool unachieved_only) const
{
    const auto &world = AngbandWorld::get_instance();
    const auto end = std::end(world.bounties);
    const auto it = std::find_if(std::begin(world.bounties), end,
        [this](const auto &bounty) { return bounty.r_idx == this->idx; });
    if (it == end) {
        return false;
    }

    return !unachieved_only || !it->is_achieved;
}

/*!
 * @brief モンスター種族の総合的な強さを計算する。
 * @details 現在はモンスター闘技場でのモンスターの強さの総合的な評価にのみ使用されている。
 * @return 計算した結果のモンスター種族の総合的な強さの値を返す。
 */
int MonraceDefinition::calc_power() const
{
    auto power = 0;
    const auto num_resistances = EnumClassFlagGroup<MonsterResistanceType>(this->resistance_flags & RFR_EFF_IMMUNE_ELEMENT_MASK).count();
    if (this->misc_flags.has(MonsterMiscType::FORCE_MAXHP)) {
        power = this->hit_dice.maxroll() * 2;
    } else {
        power = this->hit_dice.floored_expected_value_multiplied_by(2);
    }

    power = power * (100 + this->level) / 100;
    if (this->speed > STANDARD_SPEED) {
        power = power * (this->speed * 2 - 110) / 100;
    }

    if (this->speed < STANDARD_SPEED) {
        power = power * (this->speed - 20) / 100;
    }

    if (num_resistances > 2) {
        power = power * (num_resistances * 2 + 5) / 10;
    } else if (this->ability_flags.has(MonsterAbilityType::INVULNER)) {
        power = power * 4 / 3;
    } else if (this->ability_flags.has(MonsterAbilityType::HEAL)) {
        power = power * 4 / 3;
    } else if (this->ability_flags.has(MonsterAbilityType::DRAIN_MANA)) {
        power = power * 11 / 10;
    }

    if (this->behavior_flags.has(MonsterBehaviorType::RAND_MOVE_25)) {
        power = power * 9 / 10;
    }

    if (this->behavior_flags.has(MonsterBehaviorType::RAND_MOVE_50)) {
        power = power * 9 / 10;
    }

    if (this->resistance_flags.has(MonsterResistanceType::RESIST_ALL)) {
        power *= 100000;
    }

    if (this->arena_ratio) {
        power = power * this->arena_ratio / 100;
    }

    return power;
}

int MonraceDefinition::calc_figurine_value() const
{
    const auto figurine_level = this->level;
    if (figurine_level < 20) {
        return figurine_level * 50;
    }

    if (figurine_level < 30) {
        return 1000 + (figurine_level - 20) * 150;
    }

    if (figurine_level < 40) {
        return 2500 + (figurine_level - 30) * 350;
    }

    if (figurine_level < 50) {
        return 6000 + (figurine_level - 40) * 800;
    }

    return 14000 + (figurine_level - 50) * 2000;
}

int MonraceDefinition::calc_capture_value() const
{
    if (!this->is_valid()) {
        return 1000;
    }

    return this->level * 50 + 1000;
}

/*!
 * @brief エルドリッチホラー持ちのモンスターを見た時の反応メッセージを作って返す
 * @param description モンスター表記
 * @return 反応メッセージ
 * @details 実際に見るとは限らない (悪夢モードで宿に泊まった時など)
 */
std::string MonraceDefinition::build_eldritch_horror_message(std::string_view description) const
{
    const auto &horror_message = this->decide_horror_message();
    constexpr auto fmt = _("%s%sの顔を見てしまった！", "You behold the %s visage of %s!");
    return format(fmt, horror_message.data(), description.data());
}

bool MonraceDefinition::has_reinforce() const
{
    const auto end = this->reinforces.end();
    const auto it = std::find_if(this->reinforces.begin(), end,
        [](const auto &reinforce) { return reinforce.is_valid(); });
    return it != end;
}

tl::optional<std::string> MonraceDefinition::get_message(std::string_view monster_name, const MonsterMessageType message_type) const
{
    return MonraceMessageList::get_instance().get_message((int)this->idx, monster_name, message_type);
}

const std::vector<DropArtifact> &MonraceDefinition::get_drop_artifacts() const
{
    return this->drop_artifacts;
}

const std::vector<Reinforce> &MonraceDefinition::get_reinforces() const
{
    return this->reinforces;
}

bool MonraceDefinition::can_generate() const
{
    auto can_generate = this->kind_flags.has_not(MonsterKindType::UNIQUE) && this->population_flags.has_not(MonsterPopulationType::NAZGUL);
    can_generate |= this->cur_num < this->max_num;
    return can_generate;
}

GridFlow MonraceDefinition::get_grid_flow_type() const
{
    return this->feature_flags.has(MonsterFeatureType::CAN_FLY) ? GridFlow::CAN_FLY : GridFlow::NORMAL;
}

bool MonraceDefinition::is_suitable_for_floor() const
{
    return this->feature_flags.has_not(MonsterFeatureType::AQUATIC) || this->feature_flags.has(MonsterFeatureType::CAN_FLY);
}

/*!
 * @brief モンスター種族がランダムクエストの討伐対象に成り得るかをチェックする
 * @return 討伐対象にできるか否か
 */
bool MonraceDefinition::is_suitable_for_random_quest() const
{
    auto is_suitable = this->kind_flags.has(MonsterKindType::UNIQUE);
    is_suitable &= this->misc_flags.has_not(MonsterMiscType::NO_QUEST);
    is_suitable &= this->misc_flags.has_not(MonsterMiscType::QUESTOR);
    is_suitable &= this->rarity <= 100;
    is_suitable &= this->wilderness_flags.has_not(MonsterWildernessType::WILD_ONLY);
    is_suitable &= this->feature_flags.has_not(MonsterFeatureType::AQUATIC);
    is_suitable &= this->misc_flags.has_not(MonsterMiscType::MULTIPLY);
    is_suitable &= this->behavior_flags.has_not(MonsterBehaviorType::FRIENDLY);
    return is_suitable;
}

bool MonraceDefinition::is_suitable_for_shallow_water() const
{
    return this->aura_flags.has_not(MonsterAuraType::FIRE);
}

bool MonraceDefinition::is_suitable_for_deep_water() const
{
    return this->feature_flags.has(MonsterFeatureType::AQUATIC);
}

bool MonraceDefinition::is_suitable_for_lava() const
{
    auto is_suitable = this->resistance_flags.has_any_of(RFR_EFF_IM_FIRE_MASK) || this->feature_flags.has(MonsterFeatureType::CAN_FLY);
    is_suitable &= this->aura_flags.has_not(MonsterAuraType::COLD);
    return is_suitable;
}

bool MonraceDefinition::is_suitable_for_trapped_pit() const
{
    return this->feature_flags.has_none_of({ MonsterFeatureType::PASS_WALL, MonsterFeatureType::KILL_WALL });
}

/*!
 * @brief 特殊な部屋へ配置可能かを判定する (Vault/Pit/Nest/ガラス部屋)
 * @return 配置可不可
 */
bool MonraceDefinition::is_suitable_for_special_room() const
{
    auto is_valid = this->kind_flags.has_not(MonsterKindType::UNIQUE);
    is_valid &= this->population_flags.has_not(MonsterPopulationType::ONLY_ONE);
    is_valid &= this->resistance_flags.has_not(MonsterResistanceType::RESIST_ALL);
    is_valid &= this->feature_flags.has_not(MonsterFeatureType::AQUATIC);
    return is_valid;
}

/*!
 * @brief ガラス部屋に配置可能かを判定する
 *
 * 以下の全条件を満たせばOK
 * - 特殊部屋に出現可能
 * - スターバーストか閃光ブレスを使える
 * - 壁抜けも壁破壊もできない
 * - 分解ブレスは使えない
 * @param 配置可不可
 */
bool MonraceDefinition::is_suitable_for_glass_through() const
{
    auto is_suitable = this->is_suitable_for_special_room();
    is_suitable &= this->ability_flags.has_any_of({ MonsterAbilityType::BR_LITE, MonsterAbilityType::BA_LITE });
    is_suitable &= this->feature_flags.has_none_of({ MonsterFeatureType::PASS_WALL, MonsterFeatureType::KILL_WALL });
    is_suitable &= this->ability_flags.has_not(MonsterAbilityType::BR_DISI);
    return is_suitable;
}

bool MonraceDefinition::is_suitable_for_glass_breaking() const
{
    auto is_suitable = this->is_suitable_for_special_room();
    is_suitable &= this->ability_flags.has(MonsterAbilityType::BR_SHAR);
    return is_suitable;
}

bool MonraceDefinition::is_suitable_for_town() const
{
    return this->wilderness_flags.has_any_of({ MonsterWildernessType::WILD_TOWN, MonsterWildernessType::WILD_ALL });
}

bool MonraceDefinition::is_suitable_for_ocean() const
{
    return this->wilderness_flags.has(MonsterWildernessType::WILD_OCEAN);
}

bool MonraceDefinition::is_suitable_for_shore() const
{
    return this->wilderness_flags.has(MonsterWildernessType::WILD_SHORE);
}

bool MonraceDefinition::is_suitable_for_waste() const
{
    return this->wilderness_flags.has_any_of({ MonsterWildernessType::WILD_WASTE, MonsterWildernessType::WILD_ALL });
}

bool MonraceDefinition::is_suitable_for_grass() const
{
    return this->wilderness_flags.has_any_of({ MonsterWildernessType::WILD_GRASS, MonsterWildernessType::WILD_ALL });
}

bool MonraceDefinition::is_suitable_for_wood() const
{
    return this->wilderness_flags.has_any_of({ MonsterWildernessType::WILD_WOOD, MonsterWildernessType::WILD_ALL });
}

bool MonraceDefinition::is_suitable_for_volcano() const
{
    return this->wilderness_flags.has(MonsterWildernessType::WILD_VOLCANO);
}

bool MonraceDefinition::is_suitable_for_mountain() const
{
    return this->wilderness_flags.has(MonsterWildernessType::WILD_MOUNTAIN);
}

/*!
 * @brief たぬきが変身できるかを判定する
 *
 * 以下の全条件を満たせばOK
 * - ユニークではない
 * - 分裂しない
 * - フレンドリーフラグがない
 * - 水棲生物ではない
 * - カメレオンではない
 * @param 変身可不可
 */
bool MonraceDefinition::is_suitable_for_tanuki() const
{
    auto is_suitable = this->kind_flags.has_not(MonsterKindType::UNIQUE);
    is_suitable &= this->misc_flags.has_not(MonsterMiscType::MULTIPLY);
    is_suitable &= this->behavior_flags.has_not(MonsterBehaviorType::FRIENDLY);
    is_suitable &= this->feature_flags.has_not(MonsterFeatureType::AQUATIC);
    is_suitable &= this->misc_flags.has_not(MonsterMiscType::CHAMELEON);
    is_suitable &= !this->is_explodable();
    return is_suitable;
}

/*!
 * モンスターが人形のベースにできるかを返す
 * @return 人形にできるか否か
 */
bool MonraceDefinition::is_suitable_for_figurine() const
{
    auto is_suitable = this->kind_flags.has_not(MonsterKindType::UNIQUE);
    is_suitable &= this->misc_flags.has_not(MonsterMiscType::KAGE);
    is_suitable &= this->resistance_flags.has_not(MonsterResistanceType::RESIST_ALL);
    is_suitable &= this->population_flags.has_not(MonsterPopulationType::NAZGUL);
    is_suitable &= this->misc_flags.has_not(MonsterMiscType::FORCE_DEPTH);
    is_suitable &= this->population_flags.has_none_of({ MonsterPopulationType::ONLY_ONE, MonsterPopulationType::BUNBUN_STRIKER });
    return is_suitable;
}

bool MonraceDefinition::can_entry_arena() const
{
    if (!this->is_suitable_for_arena()) {
        return false;
    }

    auto can_entry = this->has_blow_with_damage();
    can_entry |= this->ability_flags.has_any_of(RF_ABILITY_BOLT_MASK | RF_ABILITY_BEAM_MASK | RF_ABILITY_BALL_MASK | RF_ABILITY_BREATH_MASK);
    return can_entry;
}

bool MonraceDefinition::is_suitable_for_nightmare(int min_level) const
{
    return this->misc_flags.has(MonsterMiscType::ELDRITCH_HORROR) && (this->level > min_level);
}

bool MonraceDefinition::is_human() const
{
    return this->symbol_char_is_any_of("pht");
}

bool MonraceDefinition::is_eatable_human() const
{
    return this->is_human() && this->kind_flags.has_not(MonsterKindType::UNIQUE);
}

bool MonraceDefinition::is_catchable_for_fishing() const
{
    auto is_catchable = this->feature_flags.has(MonsterFeatureType::AQUATIC);
    is_catchable &= this->kind_flags.has_not(MonsterKindType::UNIQUE);
    is_catchable &= str_find("Jjlw", &this->symbol_definition.character);
    return is_catchable;
}

bool MonraceDefinition::is_suitable_for_orc_pit() const
{
    auto is_suitable = this->is_suitable_for_special_room();
    is_suitable &= this->kind_flags.has(MonsterKindType::ORC);
    is_suitable &= this->kind_flags.has_not(MonsterKindType::UNDEAD);
    return is_suitable;
}

bool MonraceDefinition::is_suitable_for_troll_pit() const
{
    auto is_suitable = this->is_suitable_for_special_room();
    is_suitable &= this->kind_flags.has(MonsterKindType::TROLL);
    is_suitable &= this->kind_flags.has_not(MonsterKindType::UNDEAD);
    return is_suitable;
}

bool MonraceDefinition::is_suitable_for_giant_pit() const
{
    auto is_suitable = this->is_suitable_for_special_room();
    is_suitable &= this->kind_flags.has(MonsterKindType::GIANT);
    is_suitable &= this->kind_flags.has_not(MonsterKindType::GOOD);
    is_suitable &= this->kind_flags.has_not(MonsterKindType::UNDEAD);
    return is_suitable;
}

bool MonraceDefinition::is_suitable_for_demon_pit() const
{
    auto is_suitable = this->is_suitable_for_special_room();
    is_suitable &= this->kind_flags.has(MonsterKindType::DEMON);
    is_suitable &= this->behavior_flags.has_not(MonsterBehaviorType::KILL_BODY) || this->behavior_flags.has(MonsterBehaviorType::NEVER_BLOW);
    return is_suitable;
}

bool MonraceDefinition::is_suitable_for_horror_pit() const
{
    auto is_suitable = this->is_suitable_for_special_room();
    is_suitable &= this->misc_flags.has(MonsterMiscType::ELDRITCH_HORROR);
    is_suitable &= this->behavior_flags.has_not(MonsterBehaviorType::KILL_BODY) || this->behavior_flags.has(MonsterBehaviorType::NEVER_BLOW);
    return is_suitable;
}

bool MonraceDefinition::is_suitable_for_mimic_nest() const
{
    auto is_suitable = this->is_suitable_for_special_room();
    is_suitable &= this->symbol_char_is_any_of("!$&(/=?[\\|][`~>+");
    return is_suitable;
}

bool MonraceDefinition::is_suitable_for_dog_nest() const
{
    auto is_suitable = this->is_suitable_for_special_room();
    is_suitable &= this->symbol_char_is_any_of("CZ");
    return is_suitable;
}

bool MonraceDefinition::is_suitable_for_chapel_nest() const
{
    auto is_suitable = this->is_suitable_for_special_room();
    is_suitable &= this->kind_flags.has_not(MonsterKindType::EVIL);
    return is_suitable;
}

bool MonraceDefinition::is_suitable_for_jelly_nest() const
{
    auto is_suitable = this->is_suitable_for_special_room();
    is_suitable &= this->behavior_flags.has_not(MonsterBehaviorType::KILL_BODY) && this->behavior_flags.has(MonsterBehaviorType::NEVER_BLOW);
    is_suitable &= this->kind_flags.has_not(MonsterKindType::EVIL);
    is_suitable &= this->symbol_char_is_any_of("ijm,");
    return is_suitable;
}

bool MonraceDefinition::is_suitable_for_animal_nest() const
{
    auto is_suitable = this->is_suitable_for_special_room();
    is_suitable &= this->kind_flags.has(MonsterKindType::ANIMAL);
    return is_suitable;
}

bool MonraceDefinition::is_suitable_for_undead_nest() const
{
    auto is_suitable = this->is_suitable_for_special_room();
    is_suitable &= this->kind_flags.has(MonsterKindType::UNDEAD);
    return is_suitable;
}

bool MonraceDefinition::is_suitable_for_dragon_nest(const EnumClassFlagGroup<MonsterAbilityType> &dragon_breaths) const
{
    auto is_suitable = this->is_suitable_for_special_room();
    is_suitable &= this->kind_flags.has(MonsterKindType::DRAGON);
    is_suitable &= this->kind_flags.has_not(MonsterKindType::UNDEAD);
    auto flags = RF_ABILITY_BREATH_MASK;
    flags.reset(dragon_breaths);
    is_suitable &= this->ability_flags.has_none_of(flags) && this->ability_flags.has_all_of(dragon_breaths);
    return is_suitable;
}

bool MonraceDefinition::is_suitable_for_good_nest(char symbol) const
{
    auto is_suitable = this->is_suitable_for_special_room();
    is_suitable &= this->behavior_flags.has_not(MonsterBehaviorType::KILL_BODY) || this->behavior_flags.has(MonsterBehaviorType::NEVER_BLOW);
    is_suitable &= this->kind_flags.has_not(MonsterKindType::EVIL);
    is_suitable &= this->symbol_definition.matches_character(symbol);
    return is_suitable;
}

bool MonraceDefinition::is_suitable_for_evil_nest(char symbol) const
{
    auto is_suitable = this->is_suitable_for_special_room();
    is_suitable &= this->behavior_flags.has_not(MonsterBehaviorType::KILL_BODY) || this->behavior_flags.has(MonsterBehaviorType::NEVER_BLOW);
    is_suitable &= this->kind_flags.has_not(MonsterKindType::GOOD);
    is_suitable &= this->symbol_definition.matches_character(symbol);
    return is_suitable;
}

void MonraceDefinition::init_sex(uint32_t value)
{
    const auto sex_tmp = i2enum<MonsterSex>(value);
    if ((sex_tmp < MonsterSex::NONE) || (sex_tmp >= MonsterSex::MAX)) {
        THROW_EXCEPTION(std::logic_error, "Invalid monrace sex is specified!");
    }

    this->sex = sex_tmp;
}

tl::optional<std::string> MonraceDefinition::probe_lore()
{
    auto n = false;
    if (this->r_wake != MAX_UCHAR) {
        n = true;
    }

    if (this->r_ignore != MAX_UCHAR) {
        n = true;
    }

    this->r_wake = MAX_UCHAR;
    this->r_ignore = MAX_UCHAR;
    for (auto i = 0; i < std::ssize(this->blows); i++) {
        const auto &blow = this->blows[i];
        if ((blow.effect != RaceBlowEffectType::NONE) || (blow.method != RaceBlowMethodType::NONE)) {
            if (this->r_blows[i] != MAX_UCHAR) {
                n = true;
            }

            this->r_blows[i] = MAX_UCHAR;
        }
    }

    using Mdt = MonsterDropType;
    auto num_drops = (this->drop_flags.has(Mdt::DROP_4D2) ? 8 : 0);
    num_drops += (this->drop_flags.has(Mdt::DROP_3D2) ? 6 : 0);
    num_drops += (this->drop_flags.has(Mdt::DROP_2D2) ? 4 : 0);
    num_drops += (this->drop_flags.has(Mdt::DROP_1D2) ? 2 : 0);
    num_drops += (this->drop_flags.has(Mdt::DROP_90) ? 1 : 0);
    num_drops += (this->drop_flags.has(Mdt::DROP_60) ? 1 : 0);
    if (this->drop_flags.has_not(Mdt::ONLY_GOLD)) {
        if (this->r_drop_item != num_drops) {
            n = true;
        }

        this->r_drop_item = num_drops;
    }

    if (this->drop_flags.has_not(Mdt::ONLY_ITEM)) {
        if (this->r_drop_gold != num_drops) {
            n = true;
        }

        this->r_drop_gold = num_drops;
    }

    if (this->r_cast_spell != MAX_UCHAR) {
        n = true;
    }

    this->r_cast_spell = MAX_UCHAR;
    n |= count_lore_mflag_group(this->resistance_flags, this->r_resistance_flags) > 0;
    n |= count_lore_mflag_group(this->ability_flags, this->r_ability_flags) > 0;
    n |= count_lore_mflag_group(this->behavior_flags, this->r_behavior_flags) > 0;
    n |= count_lore_mflag_group(this->drop_flags, this->r_drop_flags) > 0;
    n |= count_lore_mflag_group(this->feature_flags, this->r_feature_flags) > 0;
    n |= count_lore_mflag_group(this->special_flags, this->r_special_flags) > 0;
    n |= count_lore_mflag_group(this->misc_flags, this->r_misc_flags) > 0;

    this->r_resistance_flags = this->resistance_flags;
    this->r_ability_flags = this->ability_flags;
    this->r_behavior_flags = this->behavior_flags;
    this->r_drop_flags = this->drop_flags;
    this->r_feature_flags = this->feature_flags;
    this->r_special_flags = this->special_flags;
    this->r_misc_flags = this->misc_flags;
    if (!this->r_can_evolve) {
        n = true;
    }

    this->r_can_evolve = true;
    if (n == 0) {
        return tl::nullopt;
    }

#ifdef JP
    return format("%sについてさらに詳しくなった気がする。", this->name.data());
#else
    const auto nm = pluralize(this->name);
    return format("You now know more about %s.", nm.data());
#endif
}

void MonraceDefinition::make_lore_treasure(int num_item, int num_gold)
{
    if (this->r_drop_item < num_item) {
        this->r_drop_item = num_item;
    }

    if (this->r_drop_gold < num_gold) {
        this->r_drop_gold = num_gold;
    }

    if (this->drop_flags.has(MonsterDropType::DROP_GOOD)) {
        this->r_drop_flags.set(MonsterDropType::DROP_GOOD);
    }

    if (this->drop_flags.has(MonsterDropType::DROP_GREAT)) {
        this->r_drop_flags.set(MonsterDropType::DROP_GREAT);
    }
}

void MonraceDefinition::emplace_drop_artifact(FixedArtifactId fa_id, int chance)
{
    this->drop_artifacts.emplace_back(fa_id, chance);
}

void MonraceDefinition::emplace_reinforce(MonraceId monrace_id, const Dice &dice)
{
    this->reinforces.emplace_back(monrace_id, dice);
}

/*!
 * @brief 該当モンスター種族が1体以上実体化されているかを返す
 * @return 実体の有無
 */
bool MonraceDefinition::has_entity() const
{
    return this->cur_num > 0;
}

/*!
 * @brief モンスターリストを走査し、生きているか死んでいるユニークだけを抽出する
 * @param is_alive 生きているユニークのリストならばTRUE、撃破したユニークのリストならばFALSE
 * @return is_aliveの条件に見合うユニークがいたらTRUE、それ以外はFALSE
 * @details 闘技場のモンスターとは再戦できないので、生きているなら表示から外す
 */
bool MonraceDefinition::should_display(bool is_alive) const
{
    if (this->kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return false;
    }

    if (!cheat_know && !this->r_sights) {
        return false;
    }

    const auto is_except_arena = is_alive ? (this->rarity > 100) && (this->misc_flags.has_not(MonsterMiscType::QUESTOR)) : false;
    if (is_except_arena) {
        return false;
    }

    if (is_alive) {
        return this->max_num > 0;
    }

    return this->max_num == 0;
}

/*!
 * @brief 詳細情報(HP,AC,スキルダメージの量)を得ることができるかを返す
 * @return モンスターの詳細情報を得る条件が満たされているか否か
 * @details 高レベルモンスターほど撃破数は少なくても詳細を知ることができる
 */
bool MonraceDefinition::is_details_known() const
{
    if ((this->r_cast_spell == MAX_UCHAR) || (this->r_tkills > 304 / (4 + this->level))) {
        return true;
    }

    if (this->kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return false;
    }

    return this->r_tkills > 304 / (38 + (5 * this->level) / 4);
}

/*!
 * @brief モンスターの打撃威力を知ることができるかどうかを返す
 * @param num_blow 確認したい攻撃手番
 * @return 敵のダメージダイスを知る条件が満たされているか否か
 * @details レベルの高いモンスターほど被打撃数が少なくても打撃威力が分かる
 */
bool MonraceDefinition::is_blow_damage_known(int num_blow) const
{
    const auto r_blow = this->r_blows[num_blow];
    auto max_damage = this->blows[num_blow].damage_dice.maxroll();
    if (max_damage >= ((4 + this->level) * MAX_UCHAR) / 80) {
        max_damage = ((4 + this->level) * MAX_UCHAR - 1) / 80;
    }

    if ((4 + this->level) * r_blow > 80 * max_damage) {
        return true;
    }

    if (this->kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return false;
    }

    return (4 + this->level) * (2 * r_blow) > 80 * max_damage;
}

/*!
 * @brief ユニークモンスターの撃破状態を更新する
 * @todo 状態変更はモンスター「定義」ではないので将来的に別クラスへ分離する
 */
void MonraceDefinition::kill_unique()
{
    this->max_num = 0;
}

bool MonraceDefinition::is_dead_unique() const
{
    return this->kind_flags.has(MonsterKindType::UNIQUE) && this->max_num == 0;
}

void MonraceDefinition::reset_current_numbers()
{
    this->cur_num = 0;
}

void MonraceDefinition::increment_current_numbers()
{
    this->cur_num++;
}

void MonraceDefinition::decrement_current_numbers()
{
    this->cur_num--;
}

void MonraceDefinition::reset_max_number()
{
    if (this->kind_flags.has(MonsterKindType::UNIQUE) || this->population_flags.has(MonsterPopulationType::ONLY_ONE)) {
        this->max_num = MAX_UNIQUE_NUM;
        return;
    }

    if (this->population_flags.has(MonsterPopulationType::NAZGUL)) {
        this->max_num = MAX_NAZGUL_NUM;
        return;
    }

    if (this->population_flags.has(MonsterPopulationType::BUNBUN_STRIKER)) {
        this->max_num = MAX_BUNBUN_NUM;
        return;
    }

    this->max_num = MAX_MONSTER_NUM;
}

void MonraceDefinition::increment_akills()
{
    if (this->r_akills < MAX_SHORT) {
        this->r_akills++;
    }
}

void MonraceDefinition::increment_pkills()
{
    if (this->r_pkills < MAX_SHORT) {
        this->r_pkills++;
    }
}

void MonraceDefinition::increment_tkills()
{
    if (this->r_tkills < MAX_SHORT) {
        this->r_tkills++;
    }
}

void MonraceDefinition::emplace_final_summon(MonraceId id, int probability, int min_num, int max_num, int radius)
{
    this->final_summons.emplace_back(MonsterSummon(id, probability, min_num, max_num, radius));
}

const std::vector<MonsterSummon> &MonraceDefinition::get_final_summons() const
{
    return this->final_summons;
}

/*!
 * @brief モンスター闘技場 (ギャンブル)に出場できるかを返す
 * @return 出場可不可
 */
bool MonraceDefinition::is_suitable_for_arena() const
{
    auto is_suitable = this->behavior_flags.has_not(MonsterBehaviorType::NEVER_MOVE);
    is_suitable &= this->misc_flags.has_not(MonsterMiscType::MULTIPLY);
    is_suitable &= this->kind_flags.has_not(MonsterKindType::QUANTUM) || this->kind_flags.has(MonsterKindType::UNIQUE);
    is_suitable &= this->feature_flags.has_not(MonsterFeatureType::AQUATIC);
    is_suitable &= this->misc_flags.has_not(MonsterMiscType::CHAMELEON);
    is_suitable &= !this->is_explodable();
    return is_suitable;
}

bool MonraceDefinition::has_blow_with_damage() const
{
    auto max_blow_damage = 0;
    for (const auto &blow : this->blows) {
        if (blow.effect != RaceBlowEffectType::DR_MANA) {
            max_blow_damage += blow.damage_dice.num;
        }
    }

    return max_blow_damage > 0;
}

/*!
 * @brief エルドリッチホラーの形容詞種別を決める
 * @return エルドリッチホラーの形容詞
 */
const std::string &MonraceDefinition::decide_horror_message() const
{
    const int horror_desc_common_size = horror_desc_common.size();
    auto horror_num = randint0(horror_desc_common_size + horror_desc_evil.size());
    if (horror_num < horror_desc_common_size) {
        return horror_desc_common[horror_num];
    }

    if (this->kind_flags.has(MonsterKindType::EVIL)) {
        return horror_desc_evil[horror_num - horror_desc_common_size];
    }

    return horror_desc_neutral[horror_num - horror_desc_common_size];
}
