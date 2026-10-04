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
#include "system/floor/floor-info.h"
#include "timed-effect/timed-effects.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

static bool message_curse(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, std::string_view msg1, std::string_view msg2, std::string_view msg3, int target_type)
{
    const auto m_name = monster_name(player_ptr, m_idx);
    const auto t_name = monster_name(player_ptr, t_idx);

    if (target_type == MONSTER_TO_PLAYER) {
        disturb(player_ptr, true, true);
        if (player_ptr->effects()->blindness().is_active()) {
            msg_print(fmt::runtime(msg1), str_upcase_first(m_name));
        } else {
            msg_print(fmt::runtime(msg2), str_upcase_first(m_name));
        }
    } else if (target_type == MONSTER_TO_MONSTER) {
        if (see_monster(player_ptr, m_idx)) {
            msg_print(fmt::runtime(msg3), str_upcase_first(m_name), t_name);
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
    { MonsterAbilityType::CAUSE_1, { _("{}が何かをつぶやいた。", "{} mumbles."),
                                       _("{}があなたを指さして呪った。", "{} points at you and curses."), _("{}は{}を指さして呪いをかけた。", "{} points at {} and curses."),
                                       AttributeType::CAUSE_1 } },
    { MonsterAbilityType::CAUSE_2, { _("{}が何かをつぶやいた。", "{} mumbles."),
                                       _("{}があなたを指さして恐ろしげに呪った。", "{} points at you and curses horribly."),
                                       _("{}は{}を指さして恐ろしげに呪いをかけた。", "{} points at {} and curses horribly."),
                                       AttributeType::CAUSE_2 } },
    { MonsterAbilityType::CAUSE_3, { _("{}が何かを大声で叫んだ。", "{} mumbles loudly."),
                                       _("{}があなたを指さして恐ろしげに呪文を唱えた！", "{} points at you, incanting terribly!"),
                                       _("{}は{}を指さし、恐ろしげに呪文を唱えた！", "{} points at {}, incanting terribly!"),
                                       AttributeType::CAUSE_3 } },
    { MonsterAbilityType::CAUSE_4, { _("{}が「お前は既に死んでいる」と叫んだ。", "{} screams the word 'DIE!'"),
                                       _("{}があなたの秘孔を突いて「お前は既に死んでいる」と叫んだ。", "{} points at you, screaming the word DIE!"),
                                       _("{}が{}の秘孔を突いて、「お前は既に死んでいる」と叫んだ。", "{} points at {}, screaming the word, 'DIE!'"),
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
