#include "mspell/mspell-attack/mspell-curse.h"
#include "core/disturbance.h"
#include "effect/attribute-types.h"
#include "effect/effect-processor.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-data.h"
#include "mspell/mspell-result.h"
#include "mspell/mspell-util.h"
#include "system/floor-type-definition.h"
#include "view/display-messages.h"

static bool message_curse(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, std::string_view msg1, std::string_view msg2, std::string_view msg3, int target_type)
{
    GAME_TEXT m_name[MAX_NLEN], t_name[MAX_NLEN];
    monster_name(player_ptr, m_idx, m_name);
    monster_name(player_ptr, t_idx, t_name);

    if (target_type == MONSTER_TO_PLAYER) {
        disturb(player_ptr, true, true);
        if (player_ptr->blind) {
            msg_format(msg1.data(), m_name);
        } else {
            msg_format(msg2.data(), m_name);
        }
    } else if (target_type == MONSTER_TO_MONSTER) {
        if (see_monster(player_ptr, m_idx)) {
            msg_format(msg3.data(), m_name, t_name);
        } else {
            player_ptr->current_floor_ptr->monster_noise = true;
        }
    }
    return false;
}

CurseData::CurseData(const std::string_view &msg1, const std::string_view &msg2, const std::string_view &msg3, const AttributeType &typ)
    : MSpellData([=](auto *player_ptr, auto m_idx, auto t_idx, int target_type) {
        return message_curse(player_ptr, m_idx, t_idx, msg1, msg2, msg3, target_type);
    },
          typ)
{
}

const std::unordered_map<MonsterAbilityType, CurseData> curse_list = {
    { MonsterAbilityType::CAUSE_1, { _("%^sが何かをつぶやいた。", "%^s mumbles."),
                                       _("%^sがあなたを指さして呪った。", "%^s points at you and curses."), _("%^sは%sを指さして呪いをかけた。", "%^s points at %s and curses."),
                                       AttributeType::CAUSE_1 } },
    { MonsterAbilityType::CAUSE_2, { _("%^sが何かをつぶやいた。", "%^s mumbles."),
                                       _("%^sがあなたを指さして恐ろしげに呪った。", "%^s points at you and curses horribly."),
                                       _("%^sは%sを指さして恐ろしげに呪いをかけた。", "%^s points at %s and curses horribly."),
                                       AttributeType::CAUSE_2 } },
    { MonsterAbilityType::CAUSE_3, { _("%^sが何かを大声で叫んだ。", "%^s mumbles loudly."),
                                       _("%^sがあなたを指さして恐ろしげに呪文を唱えた！", "%^s points at you, incanting terribly!"),
                                       _("%^sは%sを指さし、恐ろしげに呪文を唱えた！", "%^s points at %s, incanting terribly!"),
                                       AttributeType::CAUSE_3 } },
    { MonsterAbilityType::CAUSE_4, { _("%^sが「お前は既に死んでいる」と叫んだ。", "%^s screams the word 'DIE!'"),
                                       _("%^sがあなたの秘孔を突いて「お前は既に死んでいる」と叫んだ。", "%^s points at you, screaming the word DIE!"),
                                       _("%^sが%sの秘孔を突いて、「お前は既に死んでいる」と叫んだ。", "%^s points at %s, screaming the word, 'DIE!'"),
                                       AttributeType::CAUSE_4 } },
};

/*!
 * @brief RF5_CAUSE_* の処理関数
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param ms_type 呪文の番号
 * @param dam 攻撃に使用するダメージ量
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
MonsterSpellResult spell_RF5_CAUSE(PlayerType *player_ptr, MonsterAbilityType ms_type, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    if (curse_list.find(ms_type) == curse_list.end()) {
        return MonsterSpellResult::make_invalid();
    }

    curse_list.at(ms_type).msg.output(player_ptr, m_idx, t_idx, target_type);

    const auto dam = monspell_damage(player_ptr, ms_type, m_idx, DAM_ROLL);

    pointed(player_ptr, y, x, m_idx, curse_list.at(ms_type).type, dam, target_type);

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}
