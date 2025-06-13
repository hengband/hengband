#include "mspell/mspell-attack/mspell-breath.h"
#include "core/disturbance.h"
#include "effect/attribute-types.h"
#include "effect/effect-processor.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/drs-types.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-speak-flags.h"
#include "monster/monster-info.h"
#include "monster/monster-update.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-data.h"
#include "mspell/mspell-result.h"
#include "mspell/mspell-util.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"

/*!
 * @brief ブレスを吐くときにモンスター固有のセリフを返す
 * @param monrace モンスター種族定義
 * @param GF_TYPE 魔法効果
 * @param m_name モンスター表示名
 * @return 固有のセリフがあるならばその文字列、なければtl::nullopt
 */
static tl::optional<std::string> spell_RF4_BREATH_special_message(const MonraceDefinition &monrace, const AttributeType GF_TYPE, std::string_view m_name)
{
    if (GF_TYPE == AttributeType::SOUND) {
        return monrace.get_message(m_name, MonsterMessageType::MESSAGE_BREATH_SOUND);
    }
    if (GF_TYPE == AttributeType::SHARDS) {
        return monrace.get_message(m_name, MonsterMessageType::MESSAGE_BREATH_SHARDS);
    }
    if (GF_TYPE == AttributeType::FORCE) {
        return monrace.get_message(m_name, MonsterMessageType::MESSAGE_BREATH_FORCE);
    }
    return tl::nullopt;
}

static void message_breath(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type, std::string_view type_s, AttributeType GF_TYPE)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[m_idx];
    auto see_either = see_monster(player_ptr, m_idx) || see_monster(player_ptr, t_idx);
    auto known = monster_near_player(floor, m_idx, t_idx);
    auto mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    auto mon_to_player = (target_type == MONSTER_TO_PLAYER);
    const auto m_name = monster_name(player_ptr, m_idx);
    const auto t_name = monster_name(player_ptr, t_idx);

    const auto message = spell_RF4_BREATH_special_message(monster.get_monrace(), GF_TYPE, m_name);
    if (message) {
        msg_print(*message);
    } else {
        if (player_ptr->effects()->blindness().is_blind()) {
            if (mon_to_player || (mon_to_mon && known && see_either)) {
                msg_format(_("%s^が何かのブレスを吐いた。", "%s^ breathes."), m_name.data());
            }
        } else {
            if (mon_to_player) {
                msg_format(_("%s^が%s^のブレスを吐いた。", "%s^ breathes %s^."), m_name.data(), type_s.data());
            } else if (mon_to_mon && known && see_either) {
                _(msg_format("%s^が%s^に%s^のブレスを吐いた。", m_name.data(), t_name.data(), type_s.data()), msg_format("%s^ breathes %s^ at %s^.", m_name.data(), type_s.data(), t_name.data()));
            }
        }
    }

    if (mon_to_mon && known && !see_either) {
        floor.monster_noise = true;
    }

    if (known || see_either) {
        sound(SoundKind::BREATH);
    }
}

static std::pair<MonsterAbilityType, MSpellData> make_breath_elemental(MonsterAbilityType ms_type, AttributeType GF_TYPE, std::string_view type_s)
{
    return { ms_type, { [type_s, GF_TYPE](auto *player_ptr, auto m_idx, auto t_idx, int target_type) {
                           message_breath(player_ptr, m_idx, t_idx, target_type, type_s, GF_TYPE);
                           return false;
                       },
                          GF_TYPE } };
};

static std::pair<MonsterAbilityType, MSpellData> make_breath_elemental(MonsterAbilityType ms_type, AttributeType GF_TYPE, std::string_view type_s, MSpellDrsData drs)
{
    return { ms_type, { [type_s, GF_TYPE](auto *player_ptr, auto m_idx, auto t_idx, int target_type) {
                           message_breath(player_ptr, m_idx, t_idx, target_type, type_s, GF_TYPE);
                           return false;
                       },
                          GF_TYPE, drs } };
};

const std::unordered_map<MonsterAbilityType, MSpellData> breath_list = {
    make_breath_elemental(MonsterAbilityType::BR_ACID, AttributeType::ACID, _("酸", "acid"), DRS_ACID),
    make_breath_elemental(MonsterAbilityType::BR_ELEC, AttributeType::ELEC, _("稲妻", "lightning"), DRS_ELEC),
    make_breath_elemental(MonsterAbilityType::BR_FIRE, AttributeType::FIRE, _("火炎", "fire"), DRS_FIRE),
    make_breath_elemental(MonsterAbilityType::BR_COLD, AttributeType::COLD, _("冷気", "frost"), DRS_COLD),
    make_breath_elemental(MonsterAbilityType::BR_POIS, AttributeType::POIS, _("ガス", "gas"), DRS_POIS),
    make_breath_elemental(MonsterAbilityType::BR_NETH, AttributeType::NETHER, _("地獄", "nether"), DRS_NETH),
    make_breath_elemental(MonsterAbilityType::BR_LITE, AttributeType::LITE, _("閃光", "light"), DRS_LITE),
    make_breath_elemental(MonsterAbilityType::BR_DARK, AttributeType::DARK, _("暗黒", "darkness"), DRS_DARK),
    make_breath_elemental(MonsterAbilityType::BR_CONF, AttributeType::CONFUSION, _("混乱", "confusion"), DRS_CONF),
    make_breath_elemental(MonsterAbilityType::BR_SOUN, AttributeType::SOUND, _("轟音", "sound"), DRS_SOUND),
    make_breath_elemental(MonsterAbilityType::BR_CHAO, AttributeType::CHAOS, _("カオス", "chaos"), DRS_CHAOS),
    make_breath_elemental(MonsterAbilityType::BR_DISE, AttributeType::DISENCHANT, _("劣化", "disenchantment"), DRS_DISEN),
    make_breath_elemental(MonsterAbilityType::BR_NEXU, AttributeType::NEXUS, _("因果混乱", "nexus"), DRS_NEXUS),
    make_breath_elemental(MonsterAbilityType::BR_TIME, AttributeType::TIME, _("時間逆転", "time")),
    make_breath_elemental(MonsterAbilityType::BR_INER, AttributeType::INERTIAL, _("遅鈍", "inertia")),
    make_breath_elemental(MonsterAbilityType::BR_GRAV, AttributeType::GRAVITY, _("重力", "gravity")),
    make_breath_elemental(MonsterAbilityType::BR_SHAR, AttributeType::SHARDS, _("破片", "shards"), DRS_SHARD),
    make_breath_elemental(MonsterAbilityType::BR_PLAS, AttributeType::PLASMA, _("プラズマ", "plasma")),
    make_breath_elemental(MonsterAbilityType::BR_FORC, AttributeType::FORCE, _("フォース", "force")),
    make_breath_elemental(MonsterAbilityType::BR_MANA, AttributeType::MANA, _("魔力", "mana")),
    make_breath_elemental(MonsterAbilityType::BR_NUKE, AttributeType::NUKE, _("放射性廃棄物", "toxic waste"), DRS_POIS),
    make_breath_elemental(MonsterAbilityType::BR_DISI, AttributeType::DISINTEGRATE, _("分解", "disintegration")),
    make_breath_elemental(MonsterAbilityType::BR_VOID, AttributeType::VOID_MAGIC, _("虚無", "void")),
    make_breath_elemental(MonsterAbilityType::BR_ABYSS, AttributeType::ABYSS, _("深淵", "abyss")),
};

/*!
 * @brief RF4_BR_*の処理。各種ブレス。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param GF_TYPE ブレスの属性
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF4_BREATH(PlayerType *player_ptr, MonsterAbilityType ms_type, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto dam = 0;
    auto mon_to_player = (target_type == MONSTER_TO_PLAYER);
    std::unique_ptr<MSpellData> data;

    if (breath_list.find(ms_type) != breath_list.end()) {
        dam = monspell_damage(player_ptr, ms_type, m_idx, DAM_ROLL);
        data = std::make_unique<MSpellData>(breath_list.at(ms_type));
    } else {
        dam = 0;
        data = std::make_unique<MSpellData>([](auto *player_ptr, auto m_idx, auto t_idx, int target_type) {
            message_breath(player_ptr, m_idx, t_idx, target_type, _("不明", "Unknown"), AttributeType::NONE);
            return false;
        },
            AttributeType::NONE);
    }

    data->msg.output(player_ptr, m_idx, t_idx, target_type);

    const auto proj_res = breath(player_ptr, y, x, m_idx, data->type, dam, 0, target_type);

    if (mon_to_player) {
        data->drs.execute(player_ptr, m_idx);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}
