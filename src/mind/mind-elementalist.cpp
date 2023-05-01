/*!
 * @brief 元素使いの魔法系統
 */

#include "mind/mind-elementalist.h"
#include "action/action-limited.h"
#include "avatar/avatar.h"
#include "cmd-action/cmd-mind.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-io/cmd-gameoption.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-monster-util.h"
#include "effect/effect-processor.h"
#include "effect/spells-effect-util.h"
#include "floor/cave.h"
#include "floor/floor-util.h"
#include "floor/geometry.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "game-option/text-display-options.h"
#include "grid/feature-flag-types.h"
#include "grid/grid.h"
#include "hpmp/hp-mp-processor.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-explanations-table.h"
#include "mind/mind-mindcrafter.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-brightness-flags.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-describer.h"
#include "player-base/player-class.h"
#include "player-info/equipment-info.h"
#include "player-status/player-energy.h"
#include "player-status/player-status-base.h"
#include "player/player-status-table.h"
#include "racial/racial-util.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/magic-item-recharger.h"
#include "spell-kind/spells-beam.h"
#include "spell-kind/spells-charm.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-genocide.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "system/floor-type-definition.h"
#include "system/game-option-types.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "target/grid-selector.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "view/display-util.h"
#include <array>
#include <string>
#include <unordered_map>

/*!
 * @brief 元素魔法呪文のID定義
 */
enum class ElementSpells {
    BOLT_1ST = 0,
    MON_DETECT = 1,
    PERCEPT = 2,
    CURE = 3,
    BOLT_2ND = 4,
    MAG_DETECT = 5,
    BALL_3RD = 6,
    BALL_1ST = 7,
    BREATH_2ND = 8,
    ANNIHILATE = 9,
    BOLT_3RD = 10,
    WAVE_1ST = 11,
    BALL_2ND = 12,
    BURST_1ST = 13,
    STORM_2ND = 14,
    BREATH_1ST = 15,
    STORM_3ND = 16,
    MAX
};

/*!
 * @brief 元素魔法タイプ構造体
 */
struct element_type {
    std::string_view title; //!< 領域名
    std::array<AttributeType, 3> type; //!< 属性タイプリスト
    std::array<std::string_view, 3> name; //!< 属性名リスト
    std::unordered_map<AttributeType, AttributeType> extra; //!< 追加属性タイプ
};

/*!
 * @brief 元素魔法難易度構造体
 */
struct element_power {
    int elem; //!< 使用属性番号
    mind_type info; //!< 難易度構造体
};

using element_type_list = const std::unordered_map<ElementRealmType, element_type>;
using element_power_list = const std::unordered_map<ElementSpells, element_power>;
using element_tip_list = const std::unordered_map<ElementSpells, std::string_view>;
using element_text_list = const std::unordered_map<ElementRealmType, std::string_view>;

// clang-format off
/*!
 * @brief 元素魔法タイプ定義
 */
static element_type_list element_types = {
    {
        ElementRealmType::FIRE, {
            _("炎", "Fire"),
            { { AttributeType::FIRE, AttributeType::HELL_FIRE, AttributeType::PLASMA } },
            { { _("火炎", "Fire"), _("業火", "Hell Fire"), _("プラズマ", "Plasma") } },
            { },
        }
    },
    {
        ElementRealmType::ICE, {
            _("氷", "Ice"),
            { { AttributeType::COLD, AttributeType::INERTIAL, AttributeType::TIME } },
            { { _("冷気", "Ice"), _("遅鈍", "Inertia"), _("時間逆転", "Time Stream") } },
            { { AttributeType::COLD, AttributeType::ICE} },
        }
    },
    {
        ElementRealmType::SKY, {
            _("空", "Sky"),
            { { AttributeType::ELEC, AttributeType::LITE, AttributeType::MANA } },
            { { _("電撃", "Lightning"), _("光", "Light"), _("魔力", "Mana") } },
            { },
        }
    },
    {
        ElementRealmType::SEA, {
            _("海", "Sea"),
            { { AttributeType::ACID, AttributeType::WATER, AttributeType::DISINTEGRATE } },
            { { _("酸", "Acid"), _("水", "Water"), _("分解", "Disintegration") } },
            { },
        }
    },
    {
        ElementRealmType::DARKNESS, {
            _("闇", "Darkness"),
            { { AttributeType::DARK, AttributeType::NETHER, AttributeType::VOID_MAGIC } },
            { { _("暗黒", "Darkness"), _("地獄", "Nether"), _("虚無", "void") } },
            { { AttributeType::DARK, AttributeType::ABYSS } },
        }
    },
    {
        ElementRealmType::CHAOS, {
            _("混沌", "Chaos"),
            { { AttributeType::CONFUSION, AttributeType::CHAOS, AttributeType::NEXUS } },
            { { _("混乱", "Confusion"), _("カオス", "Chaos"), _("因果混乱", "Nexus") } },
            { },
        }
    },
    {
        ElementRealmType::EARTH, {
            _("地", "Earth"),
            { { AttributeType::SHARDS, AttributeType::FORCE, AttributeType::METEOR } },
            { { _("破片", "Shards"), _("フォース", "Force"), _("隕石", "Meteor") } },
            { },
        }
    },
    {
        ElementRealmType::DEATH, {
            _("瘴気", "Death"),
            { { AttributeType::POIS, AttributeType::HYPODYNAMIA, AttributeType::DISENCHANT } },
            { { _("毒", "Poison"), _("吸血", "Drain Life"), _("劣化", "Disenchantment") } },
            { },
        }
    },
};

/*!
 * @brief 元素魔法呪文定義
 */
static element_power_list element_powers = {
    { ElementSpells::BOLT_1ST,   { 0, {  1,  1,  15, _("%sの矢", "%s Bolt") }}},
    { ElementSpells::MON_DETECT, { 0, {  2,  2,  20, _("モンスター感知", "Detect Monsters") }}},
    { ElementSpells::PERCEPT,    { 0, {  5,  5,  50, _("擬似鑑定", "Psychometry") }}},
    { ElementSpells::CURE,       { 0, {  6,  5,  35, _("傷の治癒", "Cure Wounds") }}},
    { ElementSpells::BOLT_2ND,   { 1, {  8,  6,  35, _("%sの矢", "%s Bolt") }}},
    { ElementSpells::MAG_DETECT, { 0, { 10,  8,  60, _("魔法感知", "Detect Magical Objs") }}},
    { ElementSpells::BALL_3RD,   { 2, { 15, 10,  55, _("%s放射", "%s Spout") }}},
    { ElementSpells::BALL_1ST,   { 0, { 18, 13,  65, _("%sの球", "%s Ball") }}},
    { ElementSpells::BREATH_2ND, { 1, { 21, 20,  70, _("%sのブレス", "Breath of %s") }}},
    { ElementSpells::ANNIHILATE, { 0, { 24, 20,  75, _("モンスター消滅", "Annihilation") }}},
    { ElementSpells::BOLT_3RD,   { 2, { 25, 15,  60, _("%sの矢", "%s Bolt") }}},
    { ElementSpells::WAVE_1ST,   { 0, { 28, 30,  75, _("元素の波動", "Elemental Wave") }}},
    { ElementSpells::BALL_2ND,   { 1, { 28, 22,  75, _("%sの球", "%s Ball") }}},
    { ElementSpells::BURST_1ST,  { 0, { 33, 35,  75, _("精気乱射", "%s Blast") }}},
    { ElementSpells::STORM_2ND,  { 1, { 35, 30,  75, _("%sの嵐", "%s Storm") }}},
    { ElementSpells::BREATH_1ST, { 0, { 42, 48,  75, _("%sのブレス", "Breath of %s") }}},
    { ElementSpells::STORM_3ND,  { 2, { 45, 60,  80, _("%sの嵐", "%s Storm") }}},
};

/*!
 * @brief 元素魔法呪文説明文定義
 */
static element_tip_list element_tips = {
    { ElementSpells::BOLT_1ST,
    _("弱い%sの矢を放つ。", "Fire a weak bolt of %s.") },
    { ElementSpells::MON_DETECT,
    _("近くの全てのモンスターを感知する。", "Detects monsters.") },
    { ElementSpells::PERCEPT,
    _("アイテムの雰囲気を知る。", "Gives feeling of an item.") },
    { ElementSpells::CURE,
    _("怪我と体力を少し回復させる。", "Heals HP and wounds a bit.") },
    { ElementSpells::BOLT_2ND,
    _("%sの矢を放つ。", "Fire a bolt of %s.") },
    { ElementSpells::MAG_DETECT,
    _("近くの魔法のアイテムを感知する。", "Detects magic devices.") },
    { ElementSpells::BALL_3RD,
    _("高威力で射程が短い%sの球を放つ。", "Fire a strong, short-range, ball of %s.") },
    { ElementSpells::BALL_1ST,
    _("%sの球を放つ。",  "Fire a ball of %s.") },
    { ElementSpells::BREATH_2ND,
    _("%sのブレスを吐く。", "Fire a breath of %s.") },
    { ElementSpells::ANNIHILATE,
    _("%s耐性のないモンスターを1体抹殺する。", "Erase a monster unless it resists %s.") },
    { ElementSpells::BOLT_3RD,
    _("%sの矢を放つ。", "Fire a bolt of %s.") },
    { ElementSpells::WAVE_1ST,
    _("視界内の全ての敵に%sによるダメージを与える。", "Inflict %s damage on all monsters in sight.") },
    { ElementSpells::BALL_2ND,
    _("%sの球を放つ。",  "Fire a ball of %s.") },
    { ElementSpells::BURST_1ST,
    _("ランダムな方向に%sの矢を放つ。", "Fire some bolts of %s in random direction.") },
    { ElementSpells::STORM_2ND,
    _("%sの巨大な球を放つ。", "Fire a large ball of %s.") },
    { ElementSpells::BREATH_1ST,
    _("%sのブレスを吐く。", "Fire a breath of %s.") },
    { ElementSpells::STORM_3ND,
    _("%sの巨大な球を放つ。", "Fire a large ball of %s.") },
};

/*!
 * @brief 元素魔法選択時説明文定義
 */
static element_text_list element_texts = {
    { ElementRealmType::FIRE,
    _("炎系統は巨大なエネルギーで灼熱を生み出し、全ての敵を燃やし尽くそうとします。",
        "Great energy of Fire system will be able to burn out all of your enemies.")},
    { ElementRealmType::ICE,
    _("氷系統の魔法はその冷たさで敵の動きを奪い尽くし、魂すらも止めてしまうでしょう。",
        "Ice system will freeze your enemies, even their souls.")},
    { ElementRealmType::SKY,
    _("空系統は大いなる天空のエネルギーを駆使して敵の全てを撃滅できます。",
        "Sky system can terminate all of your enemies powerfully with the energy of the great sky.")},
    { ElementRealmType::SEA,
    _("海系統はその敵の全てを溶かし、大いなる海へと返してしまいます。",
        "Sea system melts all of your enemies and returns them to the great ocean.")},
    { ElementRealmType::DARKNESS,
    _("闇系統は恐るべき力を常闇から引き出し、敵を地獄へと叩き落とすでしょう。",
        "Dark system draws terrifying power from the darkness and knocks your enemies into hell.")},
    { ElementRealmType::CHAOS,
    _("混沌系統は敵の意識も条理も捻じ曲げ、その存在をあの世に送ってしまいます。",
        "Chaos system twists and wraps your enemies, even their souls, and scatters them as dust in the wind.")},
    { ElementRealmType::EARTH,
    _("地系統は偉大なる大地の力を呼び出して、数多の敵のことごとくを粉砕しようとします。",
        "Earth system smashes all of your enemies massively using its huge powers.")},
    { ElementRealmType::DEATH,
    _("瘴気系統は全ての生ける者にとって途轍もない毒です。",
        "Death system is a tremendous poison for all living enemies.")},
};

// clang-format on

/*!
 * @brief 元素魔法の領域名を返す
 * @param realm_idx 領域番号
 * @return 領域名
 */
concptr get_element_title(int realm_idx)
{
    auto realm = i2enum<ElementRealmType>(realm_idx);
    return element_types.at(realm).title.data();
}

/*!
 * @brief 元素魔法領域の属性リストを返す
 * @param realm_idx 領域番号
 * @return 領域で使用できる属性リスト
 */
static std::array<AttributeType, 3> get_element_types(int realm_idx)
{
    auto realm = i2enum<ElementRealmType>(realm_idx);
    return element_types.at(realm).type;
}

/*!
 * @brief 元素魔法領域のn番目の属性を返す
 * @param realm_idx 領域番号
 * @param n 属性の何番目か
 * @return 属性タイプ
 */
AttributeType get_element_type(int realm_idx, int n)
{
    return get_element_types(realm_idx)[n];
}

/*!
 * @brief 元素魔法領域のn番目の呪文用の属性を返す
 * @param realm_idx 領域番号
 * @param n 属性の何番目か
 * @return 属性タイプ
 */
static AttributeType get_element_spells_type(PlayerType *player_ptr, int n)
{
    auto realm = element_types.at(i2enum<ElementRealmType>(player_ptr->element));
    auto t = realm.type.at(n);
    if (realm.extra.find(t) != realm.extra.end()) {
        if (randint0(100) < player_ptr->lev * 2) {
            return realm.extra.at(t);
        }
    }
    return t;
}

/*!
 * @brief 元素魔法領域の属性名リストを返す
 * @param realm_idx 領域番号
 * @return 領域で使用できる属性の名称リスト
 */
static std::array<std::string_view, 3> get_element_names(int realm_idx)
{
    auto realm = i2enum<ElementRealmType>(realm_idx);
    return element_types.at(realm).name;
}

/*!
 * @brief 元素魔法領域のn番目の属性名を返す
 * @param realm_idx 領域番号
 * @param n 属性の何番目か
 * @return 属性名
 */
concptr get_element_name(int realm_idx, int n)
{
    return get_element_names(realm_idx)[n].data();
}

/*!
 * @brief 元素魔法の説明文を取得
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param spell_idx 呪文番号
 * @return 説明文
 */
static std::string get_element_tip(PlayerType *player_ptr, int spell_idx)
{
    auto realm = i2enum<ElementRealmType>(player_ptr->element);
    auto spell = i2enum<ElementSpells>(spell_idx);
    auto elem = element_powers.at(spell).elem;
    return format(element_tips.at(spell).data(), element_types.at(realm).name[elem].data());
}

/*!
 * @brief 元素魔法の説明文を取得
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param spell_idx 呪文番号
 * @return 説明文
 */
static int get_elemental_elem(PlayerType *player_ptr, int spell_idx)
{
    (void)player_ptr;
    auto spell = i2enum<ElementSpells>(spell_idx);
    return element_powers.at(spell).elem;
}

/*!
 * @brief 元素魔法呪文の難易度データを取得
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param spell_idx 呪文番号
 * @return 説明文
 */
static mind_type get_elemental_info(PlayerType *player_ptr, int spell_idx)
{
    (void)player_ptr;
    auto spell = i2enum<ElementSpells>(spell_idx);
    return element_powers.at(spell).info;
}

/*!
 * @brief 元素魔法呪文の効果表示文字列を取得
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param spell_idx 呪文番号
 * @return std::string 魔法の効果を表す文字列
 */
static std::string get_element_effect_info(PlayerType *player_ptr, int spell_idx)
{
    PLAYER_LEVEL plev = player_ptr->lev;
    auto spell = i2enum<ElementSpells>(spell_idx);
    int dam = 0;

    switch (spell) {
    case ElementSpells::BOLT_1ST:
        return format(" %s%dd%d", KWD_DAM, 3 + ((plev - 1) / 5), 4);
    case ElementSpells::CURE:
        return format(" %s%dd%d", KWD_HEAL, 2, 8);
    case ElementSpells::BOLT_2ND:
        return format(" %s%dd%d", KWD_DAM, 8 + ((plev - 5) / 4), 8);
    case ElementSpells::BALL_3RD:
        return format(" %s%d", KWD_DAM, (50 + plev * 2));
    case ElementSpells::BALL_1ST:
        return format(" %s%d", KWD_DAM, 55 + plev);
    case ElementSpells::BREATH_2ND:
        dam = p_ptr->chp / 2;
        return format(" %s%d", KWD_DAM, (dam > 150) ? 150 : dam);
    case ElementSpells::ANNIHILATE:
        return format(" %s%d", _("効力:", "pow "), 50 + plev);
    case ElementSpells::BOLT_3RD:
        return format(" %s%dd%d", KWD_DAM, 12 + ((plev - 5) / 4), 8);
    case ElementSpells::WAVE_1ST:
        return format(" %s50+d%d", KWD_DAM, plev * 3);
    case ElementSpells::BALL_2ND:
        return format(" %s%d", KWD_DAM, 75 + plev * 3 / 2);
    case ElementSpells::BURST_1ST:
        return format(" %s%dd%d", KWD_DAM, 6 + plev / 8, 7);
    case ElementSpells::STORM_2ND:
        return format(" %s%d", KWD_DAM, 115 + plev * 5 / 2);
    case ElementSpells::BREATH_1ST:
        return format(" %s%d", KWD_DAM, p_ptr->chp * 2 / 3);
    case ElementSpells::STORM_3ND:
        return format(" %s%d", KWD_DAM, 300 + plev * 5);
    default:
        return std::string();
    }
}

/*!
 * @brief 元素魔法呪文を実行する
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param spell_idx 呪文番号
 * @return 実行したらTRUE、キャンセルならFALSE
 */
static bool cast_element_spell(PlayerType *player_ptr, SPELL_IDX spell_idx)
{
    auto spell = i2enum<ElementSpells>(spell_idx);
    auto power = element_powers.at(spell);
    AttributeType typ;
    DIRECTION dir;
    PLAYER_LEVEL plev = player_ptr->lev;
    int dam;
    POSITION y, x;
    int num;

    switch (spell) {
    case ElementSpells::BOLT_1ST:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }
        dam = damroll(3 + ((plev - 1) / 5), 4);
        typ = get_element_spells_type(player_ptr, power.elem);
        (void)fire_bolt(player_ptr, typ, dir, dam);
        break;
    case ElementSpells::MON_DETECT:
        (void)detect_monsters_normal(player_ptr, DETECT_RAD_DEFAULT);
        (void)detect_monsters_invis(player_ptr, DETECT_RAD_DEFAULT);
        break;
    case ElementSpells::PERCEPT:
        return psychometry(player_ptr);
    case ElementSpells::CURE:
        (void)hp_player(player_ptr, damroll(2, 8));
        (void)BadStatusSetter(player_ptr).mod_cut(-10);
        break;
    case ElementSpells::BOLT_2ND:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }
        dam = damroll(8 + ((plev - 5) / 4), 8);
        typ = get_element_spells_type(player_ptr, power.elem);
        if (fire_bolt_or_beam(player_ptr, plev, typ, dir, dam)) {
            if (typ == AttributeType::HYPODYNAMIA) {
                (void)hp_player(player_ptr, dam / 2);
            }
        }
        break;
    case ElementSpells::MAG_DETECT:
        (void)detect_objects_magic(player_ptr, DETECT_RAD_DEFAULT);
        break;
    case ElementSpells::BALL_3RD:
        project_length = 4;
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }
        typ = get_element_spells_type(player_ptr, power.elem);
        dam = 50 + plev * 2;
        (void)fire_ball(player_ptr, typ, dir, dam, 1);
        project_length = 0;
        break;
    case ElementSpells::BALL_1ST:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }
        dam = 55 + plev;
        typ = get_element_spells_type(player_ptr, power.elem);
        (void)fire_ball(player_ptr, typ, dir, dam, 2);
        break;
    case ElementSpells::BREATH_2ND:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }
        dam = std::min(150, player_ptr->chp / 2);
        typ = get_element_spells_type(player_ptr, power.elem);
        if (fire_breath(player_ptr, typ, dir, dam, 3)) {
            if (typ == AttributeType::HYPODYNAMIA) {
                (void)hp_player(player_ptr, dam / 2);
            }
        }
        break;
    case ElementSpells::ANNIHILATE:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }
        fire_ball_hide(player_ptr, AttributeType::E_GENOCIDE, dir, plev + 50, 0);
        break;
    case ElementSpells::BOLT_3RD:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }
        dam = damroll(12 + ((plev - 5) / 4), 8);
        typ = get_element_spells_type(player_ptr, power.elem);
        fire_bolt_or_beam(player_ptr, plev, typ, dir, dam);
        break;
    case ElementSpells::WAVE_1ST:
        dam = 50 + randint1(plev * 3);
        typ = get_element_spells_type(player_ptr, power.elem);
        project_all_los(player_ptr, typ, dam);
        break;
    case ElementSpells::BALL_2ND:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }
        dam = 75 + plev * 3 / 2;
        typ = get_element_spells_type(player_ptr, power.elem);
        if (fire_ball(player_ptr, typ, dir, dam, 3)) {
            if (typ == AttributeType::HYPODYNAMIA) {
                (void)hp_player(player_ptr, dam / 2);
            }
        }
        break;
    case ElementSpells::BURST_1ST:
        y = player_ptr->y;
        x = player_ptr->x;
        num = damroll(4, 3);
        typ = get_element_spells_type(player_ptr, power.elem);
        for (int k = 0; k < num; k++) {
            int attempts = 1000;
            while (attempts--) {
                scatter(player_ptr, &y, &x, player_ptr->y, player_ptr->x, 4, PROJECT_NONE);
                if (!cave_has_flag_bold(player_ptr->current_floor_ptr, y, x, TerrainCharacteristics::PROJECT)) {
                    continue;
                }
                if (!player_bold(player_ptr, y, x)) {
                    break;
                }
            }
            project(player_ptr, 0, 0, y, x, damroll(6 + plev / 8, 7), typ, (PROJECT_BEAM | PROJECT_THRU | PROJECT_GRID | PROJECT_KILL));
        }
        break;
    case ElementSpells::STORM_2ND:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }
        dam = 115 + plev * 5 / 2;
        typ = get_element_spells_type(player_ptr, power.elem);
        if (fire_ball(player_ptr, typ, dir, dam, 4)) {
            if (typ == AttributeType::HYPODYNAMIA) {
                (void)hp_player(player_ptr, dam / 2);
            }
        }
        break;
    case ElementSpells::BREATH_1ST:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }
        dam = player_ptr->chp * 2 / 3;
        typ = get_element_spells_type(player_ptr, power.elem);
        (void)fire_breath(player_ptr, typ, dir, dam, 3);
        break;
    case ElementSpells::STORM_3ND:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }
        dam = 300 + plev * 5;
        typ = get_element_spells_type(player_ptr, power.elem);
        (void)fire_ball(player_ptr, typ, dir, dam, 5);
        break;
    default:
        return false;
    }

    return true;
}

/*!
 * @brief 元素魔法呪文の失敗率を計算
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param spell_idx 呪文番号
 * @return 失敗率
 */
static PERCENTAGE decide_element_chance(PlayerType *player_ptr, mind_type spell)
{
    PERCENTAGE chance = spell.fail;

    chance -= 3 * (player_ptr->lev - spell.min_lev);
    chance += player_ptr->to_m_chance;
    chance -= 3 * (adj_mag_stat[player_ptr->stat_index[A_WIS]] - 1);

    PERCENTAGE minfail = adj_mag_fail[player_ptr->stat_index[A_WIS]];
    if (chance < minfail) {
        chance = minfail;
    }

    auto player_stun = player_ptr->effects()->stun();
    chance += player_stun->get_magic_chance_penalty();
    if (heavy_armor(player_ptr)) {
        chance += 5;
    }

    if (player_ptr->is_icky_wield[0]) {
        chance += 5;
    }

    if (player_ptr->is_icky_wield[1]) {
        chance += 5;
    }

    if (chance > 95) {
        chance = 95;
    }

    return chance;
}

/*!
 * @brief 元素魔法呪文の消費MPを計算
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param spell_idx 呪文番号
 * @return 消費MP
 */
static MANA_POINT decide_element_mana_cost(PlayerType *player_ptr, mind_type spell)
{
    (void)player_ptr;
    return spell.mana_cost;
}

/*!
 * @brief 元素魔法呪文を選択して取得
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param sn 呪文番号
 * @param only_browse 閲覧モードかどうか
 * @return 選んだらTRUE、選ばなかったらFALSE
 */
static bool get_element_power(PlayerType *player_ptr, SPELL_IDX *sn, bool only_browse)
{
    SPELL_IDX i;
    int num = 0;
    TERM_LEN y = 1;
    TERM_LEN x = 10;
    PLAYER_LEVEL plev = player_ptr->lev;
    char choice;
    char out_val[160];
    COMMAND_CODE code;
    bool flag, redraw;
    int menu_line = (use_menu ? 1 : 0);

    *sn = -1;
    if (repeat_pull(&code)) {
        *sn = (SPELL_IDX)code;
        if (get_elemental_info(player_ptr, *sn).min_lev <= plev) {
            return true;
        }
    }

    concptr p = _("元素魔法", "power");
    flag = false;
    redraw = false;

    for (i = 0; i < static_cast<SPELL_IDX>(ElementSpells::MAX); i++) {
        if (get_elemental_info(player_ptr, i).min_lev <= plev) {
            num++;
        }
    }

    if (only_browse) {
        (void)strnfmt(out_val, 78, _("(%s^ %c-%c, '*'で一覧, ESC) どの%sについて知りますか？", "(%s^s %c-%c, *=List, ESC=exit) Use which %s? "), p, I2A(0),
            I2A(num - 1), p);
    } else {
        (void)strnfmt(
            out_val, 78, _("(%s^ %c-%c, '*'で一覧, ESC) どの%sを使いますか？", "(%s^s %c-%c, *=List, ESC=exit) Use which %s? "), p, I2A(0), I2A(num - 1), p);
    }

    if (use_menu && !only_browse) {
        screen_save();
    }

    int elem;
    mind_type spell;
    choice = (always_show_list || use_menu) ? ESCAPE : 1;
    while (!flag) {
        if (choice == ESCAPE) {
            choice = ' ';
        } else if (!get_com(out_val, &choice, true)) {
            break;
        }

        auto should_redraw_cursor = true;
        if (use_menu && choice != ' ') {
            switch (choice) {
            case '0':
                if (!only_browse) {
                    screen_load();
                }
                return false;
            case '8':
            case 'k':
            case 'K':
                menu_line += (num - 1);
                break;
            case '2':
            case 'j':
            case 'J':
                menu_line++;
                break;
            case 'x':
            case 'X':
            case '\r':
            case '\n':
                i = menu_line - 1;
                should_redraw_cursor = false;
                break;
            }

            if (menu_line > num) {
                menu_line -= num;
            }
        }

        int spell_max = enum2i(ElementSpells::MAX);
        if ((choice == ' ') || (choice == '*') || (choice == '?') || (use_menu && should_redraw_cursor)) {
            if (!redraw || use_menu) {
                redraw = true;
                if (!only_browse && !use_menu) {
                    screen_save();
                }

                prt("", y, x);
                put_str(_("名前", "Name"), y, x + 5);
                put_str(_("Lv   MP   失率 効果", "Lv   MP Fail Info"), y, x + 35);
                for (i = 0; i < spell_max; i++) {
                    elem = get_elemental_elem(player_ptr, i);
                    spell = get_elemental_info(player_ptr, i);

                    if (spell.min_lev > plev) {
                        break;
                    }

                    PERCENTAGE chance = decide_element_chance(player_ptr, spell);
                    int mana_cost = decide_element_mana_cost(player_ptr, spell);
                    const auto comment = get_element_effect_info(player_ptr, i);

                    std::string desc;
                    if (use_menu) {
                        if (i == (menu_line - 1)) {
                            desc = _("  》 ", "  >  ");
                        } else {
                            desc = "     ";
                        }
                    } else {
                        desc = format("  %c) ", I2A(i));
                    }

                    concptr s = get_element_name(player_ptr->element, elem);
                    std::string name = format(spell.name, s);
                    desc.append(format("%-30s%2d %4d %3d%%%s", name.data(), spell.min_lev, mana_cost, chance, comment.data()));
                    prt(desc, y + i + 1, x);
                }

                prt("", y + i + 1, x);
            } else if (!only_browse) {
                redraw = false;
                screen_load();
            }

            continue;
        }

        if (!use_menu) {
            i = A2I(choice);
        }

        if ((i < 0) || (i >= num)) {
            bell();
            continue;
        }

        flag = true;
    }

    if (redraw && !only_browse) {
        screen_load();
    }

    set_bits(player_ptr->window_flags, PW_SPELL);
    handle_stuff(player_ptr);
    if (!flag) {
        return false;
    }

    *sn = i;
    repeat_push((COMMAND_CODE)i);
    return true;
}

/*!
 * @brief 元素魔法呪文をMPがなくても挑戦するか確認する
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param mana_cost 消費MP
 * @return 詠唱するならTRUE、しないならFALSE
 */
static bool check_element_mp_sufficiency(PlayerType *player_ptr, int mana_cost)
{
    if (mana_cost <= player_ptr->csp) {
        return true;
    }

    msg_print(_("ＭＰが足りません。", "You do not have enough mana to use this power."));
    if (!over_exert) {
        return false;
    }

    return get_check(_("それでも挑戦しますか? ", "Attempt it anyway? "));
}

/*!
 * @brief 元素魔法呪文の詠唱を試み、成功なら詠唱し、失敗ならファンブルする
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param spell_idx 呪文番号
 * @param chance 失敗率
 * @return 詠唱して実行したらTRUE、されなかったらFALSE
 */
static bool try_cast_element_spell(PlayerType *player_ptr, SPELL_IDX spell_idx, PERCENTAGE chance)
{
    if (randint0(100) >= chance) {
        sound(SOUND_ZAP);
        return cast_element_spell(player_ptr, spell_idx);
    }

    if (flush_failure) {
        flush();
    }

    msg_format(_("魔力の集中に失敗した！", "You failed to focus the elemental power!"));
    sound(SOUND_FAIL);

    if (randint1(100) < chance / 2) {
        int plev = player_ptr->lev;
        msg_print(_("元素の力が制御できない氾流となって解放された！", "The elemental power surges from you in an uncontrollable torrent!"));
        project(player_ptr, PROJECT_WHO_UNCTRL_POWER, 2 + plev / 10, player_ptr->y, player_ptr->x, plev * 2, get_element_types(player_ptr->element)[0],
            PROJECT_JUMP | PROJECT_KILL | PROJECT_GRID | PROJECT_ITEM);
        player_ptr->csp = std::max(0, player_ptr->csp - player_ptr->msp * 10 / (20 + randint1(10)));

        PlayerEnergy(player_ptr).set_player_turn_energy(100);
        set_bits(player_ptr->redraw, PR_MP);
        set_bits(player_ptr->window_flags, PW_PLAYER | PW_SPELL);

        return false;
    }

    return true;
}

/*!
 * @brief 元素魔法コマンドのメインルーチン
 * @param player_ptr プレイヤー情報への参照ポインタ
 */
void do_cmd_element(PlayerType *player_ptr)
{
    SPELL_IDX i;
    if (cmd_limit_confused(player_ptr) || !get_element_power(player_ptr, &i, false)) {
        return;
    }

    mind_type spell = get_elemental_info(player_ptr, i);
    PERCENTAGE chance = decide_element_chance(player_ptr, spell);
    int mana_cost = decide_element_mana_cost(player_ptr, spell);

    if (!check_element_mp_sufficiency(player_ptr, mana_cost)) {
        return;
    }

    if (!try_cast_element_spell(player_ptr, i, chance)) {
        return;
    }

    if (mana_cost <= player_ptr->csp) {
        player_ptr->csp -= mana_cost;
    } else {
        int oops = mana_cost;
        player_ptr->csp = 0;
        player_ptr->csp_frac = 0;
        msg_print(_("精神を集中しすぎて気を失ってしまった！", "You faint from the effort!"));
        (void)BadStatusSetter(player_ptr).mod_paralysis(randint1(5 * oops + 1));
        chg_virtue(player_ptr, Virtue::KNOWLEDGE, -10);
        if (randint0(100) < 50) {
            bool perm = (randint0(100) < 25);
            msg_print(_("体を悪くしてしまった！", "You have damaged your health!"));
            (void)dec_stat(player_ptr, A_CON, 15 + randint1(10), perm);
        }
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    set_bits(player_ptr->redraw, PR_MP);
    set_bits(player_ptr->window_flags, PW_PLAYER | PW_SPELL);
}

/*!
 * @brief 現在プレイヤーが使用可能な元素魔法の一覧表示
 * @param player_ptr プレイヤー情報への参照ポインタ
 */
void do_cmd_element_browse(PlayerType *player_ptr)
{
    SPELL_IDX n = 0;

    screen_save();
    while (true) {
        if (!get_element_power(player_ptr, &n, true)) {
            screen_load();
            return;
        }

        term_erase(12, 21, 255);
        term_erase(12, 20, 255);
        term_erase(12, 19, 255);
        term_erase(12, 18, 255);
        term_erase(12, 17, 255);
        term_erase(12, 16, 255);
        display_wrap_around(get_element_tip(player_ptr, n), 62, 17, 15);

        prt(_("何かキーを押して下さい。", "Hit any key."), 0, 0);
        (void)inkey();
    }
}

/*!
 * @brief 元素魔法の単体抹殺が有効か確認する
 * @param r_ptr モンスター種族への参照ポインタ
 * @param type 魔法攻撃属性
 * @return 効果があるならTRUE、なければFALSE
 */
bool is_elemental_genocide_effective(MonsterRaceInfo *r_ptr, AttributeType type)
{
    switch (type) {
    case AttributeType::FIRE:
        if (r_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_FIRE)) {
            return false;
        }
        break;
    case AttributeType::COLD:
        if (r_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_COLD)) {
            return false;
        }
        break;
    case AttributeType::ELEC:
        if (r_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_ELEC)) {
            return false;
        }
        break;
    case AttributeType::ACID:
        if (r_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_ACID)) {
            return false;
        }
        break;
    case AttributeType::DARK:
        if (r_ptr->resistance_flags.has(MonsterResistanceType::RESIST_DARK) || r_ptr->r_resistance_flags.has(MonsterResistanceType::HURT_LITE)) {
            return false;
        }
        break;
    case AttributeType::CONFUSION:
        if (any_bits(r_ptr->flags3, RF3_NO_CONF)) {
            return false;
        }
        break;
    case AttributeType::SHARDS:
        if (r_ptr->resistance_flags.has(MonsterResistanceType::RESIST_SHARDS)) {
            return false;
        }
        break;
    case AttributeType::POIS:
        if (r_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_POISON)) {
            return false;
        }
        break;
    default:
        return false;
    }

    return true;
}

/*!
 * @brief 元素魔法の単体抹殺の効果を発動する
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param em_ptr 魔法効果情報への参照ポインタ
 * @return 効果処理を続けるかどうか
 */
ProcessResult effect_monster_elemental_genocide(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
    auto type = get_element_type(player_ptr->element, 0);
    auto name = get_element_name(player_ptr->element, 0);
    bool b = is_elemental_genocide_effective(em_ptr->r_ptr, type);

    if (em_ptr->seen_msg) {
        msg_format(_("%sが%sを包み込んだ。", "The %s surrounds %s."), name, em_ptr->m_name);
    }

    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (!b) {
        if (em_ptr->seen_msg) {
            msg_format(_("%sには効果がなかった。", "%s^ is unaffected."), em_ptr->m_name);
        }
        em_ptr->dam = 0;
        return ProcessResult::PROCESS_TRUE;
    }

    if (genocide_aux(player_ptr, em_ptr->g_ptr->m_idx, em_ptr->dam, !em_ptr->who, (em_ptr->r_ptr->level + 1) / 2, _("モンスター消滅", "Genocide One"))) {
        if (em_ptr->seen_msg) {
            msg_format(_("%sは消滅した！", "%s^ disappeared!"), em_ptr->m_name);
        }
        em_ptr->dam = 0;
        chg_virtue(player_ptr, Virtue::VITALITY, -1);
        return ProcessResult::PROCESS_TRUE;
    }

    em_ptr->skipped = true;
    return ProcessResult::PROCESS_CONTINUE;
}

/*!
 * @brief 元素領域とレベルの条件に見合うかチェックする
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param realm 領域
 * @param lev プレイヤーレベル
 * @return 見合うならTRUE、そうでなければFALSE
 * @details
 * レベルに応じて取得する耐性などの判定に使用する
 */
bool has_element_resist(PlayerType *player_ptr, ElementRealmType realm, PLAYER_LEVEL lev)
{
    if (!PlayerClass(player_ptr).equals(PlayerClassType::ELEMENTALIST)) {
        return false;
    }

    auto prealm = i2enum<ElementRealmType>(player_ptr->element);
    return (prealm == realm) && (player_ptr->lev >= lev);
}

/*!
 * @brief 領域選択時のカーソル表示(シンボル＋領域名)
 * @param i 位置
 * @param n 最後尾の位置
 * @param color 表示色
 */
static void display_realm_cursor(int i, int n, term_color_type color)
{
    char sym;
    concptr name;
    if (i == n) {
        sym = '*';
        name = _("ランダム", "Random");
    } else {
        sym = I2A(i);
        name = element_types.at(i2enum<ElementRealmType>(i + 1)).title.data();
    }

    c_put_str(color, format("%c) %s", sym, name), 12 + (i / 5), 2 + 15 * (i % 5));
}

/*!
 * @brief 領域選択時の移動キー処理
 * @param cs 現在位置
 * @param n 最後尾の位置
 * @param c 入力キー
 * @return 新しい位置
 */
static int interpret_realm_select_key(int cs, int n, char c)
{
    if (c == 'Q') {
        quit(nullptr);
    }

    if (c == '8') {
        if (cs >= 5) {
            return cs - 5;
        }
    }

    if (c == '4') {
        if (cs > 0) {
            return cs - 1;
        }
    }

    if (c == '6') {
        if (cs < n) {
            return cs + 1;
        }
    }

    if (c == '2') {
        if (cs + 5 <= n) {
            return cs + 5;
        }
    }

    return cs;
}

/*!
 * @brief 領域選択ループ処理
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param n 最後尾の位置
 * @return 領域番号
 */
static int get_element_realm(PlayerType *player_ptr, int is, int n)
{
    int cs = std::max(0, is);
    int os = cs;
    int k;

    std::string buf = format(_("領域を選んで下さい(%c-%c) ('='初期オプション設定): ", "Choose a realm (%c-%c) ('=' for options): "), I2A(0), I2A(n - 1));

    while (true) {
        display_realm_cursor(os, n, TERM_WHITE);
        display_realm_cursor(cs, n, TERM_YELLOW);
        put_str(buf, 10, 10);
        os = cs;

        char c = inkey();
        cs = interpret_realm_select_key(cs, n, c);

        if (c == 'S') {
            return 255;
        }

        if (c == ' ' || c == '\r' || c == '\n') {
            if (cs == n) {
                display_realm_cursor(cs, n, TERM_WHITE);
                cs = randint0(n - 1);
            }
            break;
        }

        if (c == '*') {
            display_realm_cursor(cs, n, TERM_WHITE);
            cs = randint0(n - 1);
            break;
        }

        k = islower(c) ? A2I(c) : -1;
        if (k >= 0 && k < n) {
            display_realm_cursor(cs, n, TERM_WHITE);
            cs = k;
            break;
        }

        k = isupper(c) ? (26 + c - 'A') : -1;
        if (k >= 26 && k < n) {
            display_realm_cursor(cs, n, TERM_WHITE);
            cs = k;
            break;
        }

        if (c == '=') {
            screen_save();
            do_cmd_options_aux(player_ptr, OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Options ((*)) affect score"));
            screen_load();
        } else if (c != '2' && c != '4' && c != '6' && c != '8') {
            bell();
        }
    }

    display_realm_cursor(cs, n, TERM_YELLOW);
    return cs + 1;
}

/*!
 * @brief 領域選択
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @return 領域番号
 */
byte select_element_realm(PlayerType *player_ptr)
{
    clear_from(10);

    int realm_max = enum2i(ElementRealmType::MAX);
    int realm_idx = 1;
    int row = 16;
    while (1) {
        put_str(
            _("注意：元素系統の選択によりあなたが習得する呪文のタイプが決まります。", "Note: The system of element will determine which spells you can learn."),
            23, 5);

        for (int i = 0; i < realm_max; i++) {
            display_realm_cursor(i, realm_max - 1, TERM_WHITE);
        }

        realm_idx = get_element_realm(player_ptr, realm_idx - 1, realm_max - 1);
        if (realm_idx == 255) {
            break;
        }

        auto realm = i2enum<ElementRealmType>(realm_idx);
        display_wrap_around(element_texts.at(realm), 74, row, 3);

        if (get_check_strict(player_ptr, _("よろしいですか？", "Are you sure? "), CHECK_DEFAULT_Y)) {
            break;
        }

        clear_from(row);
    }

    clear_from(10);
    return (byte)realm_idx;
}

/*!
 * @brief クラスパワー情報を追加
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param rc_ptr レイシャルパワー情報への参照ポインタ
 */
void switch_element_racial(PlayerType *player_ptr, rc_type *rc_ptr)
{
    auto plev = player_ptr->lev;
    auto realm = i2enum<ElementRealmType>(player_ptr->element);
    rpi_type rpi;
    switch (realm) {
    case ElementRealmType::FIRE:
        rpi = rpi_type(_("ライト・エリア", "Light area"));
        rpi.text = _("光源が照らしている範囲か部屋全体を永久に明るくする。", "Lights up nearby area and the inside of a room permanently.");
        rpi.min_level = 3;
        rpi.cost = 5;
        rpi.stat = A_WIS;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_1);
        break;
    case ElementRealmType::ICE:
        rpi = rpi_type(_("周辺フリーズ", "Sleep monsters"));
        rpi.info = format("%s%d", KWD_POWER, 20 + plev * 3 / 2);
        rpi.text = _("視界内の全てのモンスターを眠らせる。抵抗されると無効。", "Attempts to put all monsters in sight to sleep.");
        rpi.min_level = 10;
        rpi.cost = 15;
        rpi.stat = A_WIS;
        rpi.fail = 25;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_1);
        break;
    case ElementRealmType::SKY:
        rpi = rpi_type(_("魔力充填", "Recharging"));
        rpi.info = format("%s%d", KWD_POWER, 120);
        rpi.text = _("杖/魔法棒の充填回数を増やすか、充填中のロッドの充填時間を減らす。", "Recharges staffs, wands or rods.");
        rpi.min_level = 20;
        rpi.cost = 15;
        rpi.stat = A_WIS;
        rpi.fail = 25;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_1);
        break;
    case ElementRealmType::SEA:
        rpi = rpi_type(_("岩石溶解", "Stone to mud"));
        rpi.text = _("壁を溶かして床にする。", "Turns one rock square to mud.");
        rpi.min_level = 5;
        rpi.cost = 5;
        rpi.stat = A_WIS;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_1);
        break;
    case ElementRealmType::DARKNESS:
        rpi = rpi_type(_("闇の扉", "Door to darkness"));
        rpi.info = format("%s%d", KWD_SPHERE, 15 + plev / 2);
        rpi.min_level = 5;
        rpi.cost = 5 + plev / 7;
        rpi.stat = A_WIS;
        rpi.fail = 20;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_1);
        break;
    case ElementRealmType::CHAOS:
        rpi = rpi_type(_("現実変容", "Alter reality"));
        rpi.text = _("現在の階を再構成する。", "Recreates current dungeon level.");
        rpi.min_level = 35;
        rpi.cost = 30;
        rpi.stat = A_WIS;
        rpi.fail = 40;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_1);
        break;
    case ElementRealmType::EARTH:
        rpi = rpi_type(_("地震", "Earthquake"));
        rpi.info = format("%s%d", KWD_SPHERE, 10);
        rpi.text = _("周囲のダンジョンを揺らし、壁と床をランダムに入れ変える。", "Shakes dungeon structure, and results in random swapping of floors and walls.");
        rpi.min_level = 25;
        rpi.cost = 15;
        rpi.stat = A_WIS;
        rpi.fail = 20;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_1);
        break;
    case ElementRealmType::DEATH:
        rpi = rpi_type(_("増殖阻止", "Sterilization"));
        rpi.text = _("この階の増殖するモンスターが増殖できなくなる。", "Prevents any breeders on current level from breeding.");
        rpi.min_level = 5;
        rpi.cost = 5;
        rpi.stat = A_WIS;
        rpi.fail = 20;
        rc_ptr->add_power(rpi, RC_IDX_CLASS_1);
        break;
    default:
        break;
    }
}

/*!
 * @todo 宣言だけ。後日適切な場所に移動
 */
static bool door_to_darkness(PlayerType *player_ptr, POSITION dist);

/*!
 * @brief クラスパワーを実行
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @return 実行したらTRUE、しなかったらFALSE
 */
bool switch_element_execution(PlayerType *player_ptr)
{
    auto realm = i2enum<ElementRealmType>(player_ptr->element);
    PLAYER_LEVEL plev = player_ptr->lev;
    DIRECTION dir;

    switch (realm) {
    case ElementRealmType::FIRE:
        (void)lite_area(player_ptr, damroll(2, plev / 2), plev / 10);
        break;
    case ElementRealmType::ICE:
        (void)project(player_ptr, 0, 5, player_ptr->y, player_ptr->x, 1, AttributeType::COLD, PROJECT_ITEM);
        (void)project_all_los(player_ptr, AttributeType::OLD_SLEEP, 20 + plev * 3 / 2);
        break;
    case ElementRealmType::SKY:
        (void)recharge(player_ptr, 120);
        break;
    case ElementRealmType::SEA:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }
        (void)wall_to_mud(player_ptr, dir, plev * 3 / 2);
        break;
    case ElementRealmType::DARKNESS:
        return door_to_darkness(player_ptr, 15 + plev / 2);
        break;
    case ElementRealmType::CHAOS:
        reserve_alter_reality(player_ptr, randint0(21) + 15);
        break;
    case ElementRealmType::EARTH:
        (void)earthquake(player_ptr, player_ptr->y, player_ptr->x, 10, 0);
        break;
    case ElementRealmType::DEATH:
        if (player_ptr->current_floor_ptr->num_repro <= MAX_REPRODUCTION) {
            player_ptr->current_floor_ptr->num_repro += MAX_REPRODUCTION;
        }
        break;
    default:
        return false;
    }

    return true;
}

/*!
 * @brief 指定したマスが暗いかどうか
 * @param f_ptr 階の情報への参照ポインタ
 * @param y 指定のy座標
 * @param x 指定のx座標
 * @return 暗いならTRUE、そうでないならFALSE
 */
static bool is_target_grid_dark(FloorType *f_ptr, POSITION y, POSITION x)
{
    if (any_bits(f_ptr->grid_array[y][x].info, CAVE_MNLT)) {
        return false;
    }

    bool is_dark = false;
    bool is_lite = any_bits(f_ptr->grid_array[y][x].info, CAVE_GLOW | CAVE_LITE);

    for (int dx = x - 2; dx <= x + 2; dx++) {
        for (int dy = y - 2; dy <= y + 2; dy++) {
            if (dx == x && dy == y) {
                continue;
            }
            if (!in_bounds(f_ptr, dy, dx)) {
                continue;
            }

            MONSTER_IDX m_idx = f_ptr->grid_array[dy][dx].m_idx;
            if (!m_idx) {
                continue;
            }

            POSITION d = distance(dy, dx, y, x);
            auto *r_ptr = &monraces_info[f_ptr->m_list[m_idx].r_idx];
            if (d <= 1 && r_ptr->brightness_flags.has_any_of({ MonsterBrightnessType::HAS_LITE_1, MonsterBrightnessType::SELF_LITE_1 })) {
                return false;
            }
            if (d <= 2 && r_ptr->brightness_flags.has_any_of({ MonsterBrightnessType::HAS_LITE_2, MonsterBrightnessType::SELF_LITE_2 })) {
                return false;
            }
            if (d <= 1 && r_ptr->brightness_flags.has_any_of({ MonsterBrightnessType::HAS_DARK_1, MonsterBrightnessType::SELF_DARK_1 })) {
                is_dark = true;
            }
            if (d <= 2 && r_ptr->brightness_flags.has_any_of({ MonsterBrightnessType::HAS_DARK_2, MonsterBrightnessType::SELF_DARK_2 })) {
                is_dark = true;
            }
        }
    }

    return !is_lite || is_dark;
}

/*!
 * @breif 暗いところ限定での次元の扉
 * @param player_ptr プレイヤー情報への参照ポインタ
 */
static bool door_to_darkness(PlayerType *player_ptr, POSITION dist)
{
    POSITION y = player_ptr->y;
    POSITION x = player_ptr->x;
    FloorType *f_ptr;

    for (int i = 0; i < 3; i++) {
        if (!tgt_pt(player_ptr, &x, &y)) {
            return false;
        }

        f_ptr = player_ptr->current_floor_ptr;

        if (distance(y, x, player_ptr->y, player_ptr->x) > dist) {
            msg_print(_("遠すぎる！", "That is too far!"));
            continue;
        }

        if (!is_cave_empty_bold(player_ptr, y, x) || f_ptr->grid_array[y][x].is_icky()) {
            msg_print(_("そこには移動できない。", "Can not teleport to there."));
            continue;
        }

        break;
    }

    bool flag = cave_player_teleportable_bold(player_ptr, y, x, TELEPORT_SPONTANEOUS) && is_target_grid_dark(f_ptr, y, x);
    if (flag) {
        teleport_player_to(player_ptr, y, x, TELEPORT_SPONTANEOUS);
    } else {
        msg_print(_("闇の扉は開かなかった！", "The door to darkness does not open!"));
    }
    return true;
}
