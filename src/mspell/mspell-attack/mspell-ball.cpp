#include "mspell/mspell-attack/mspell-ball.h"
#include "effect/attribute-types.h"
#include "effect/effect-processor.h"
#include "main/sound-of-music.h"
#include "mind/drs-types.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-ability-mask.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "mspell/mspell-attack/abstract-mspell.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-data.h"
#include "mspell/mspell-result.h"
#include "mspell/mspell-util.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "timed-effect/timed-effects.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

static bool message_fire_ball(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg;
    const auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];

    if (monster.r_idx == MonraceId::ROLENTO) {
        msg.blind = _("{}が何かを投げた。", "{} throws something.");
        msg.to_player = _("{}は手榴弾を投げた。", "{} throws a hand grenade.");
        msg.to_mons = _("{}が{}に向かって手榴弾を投げた。", "{} throws a hand grenade.");
    } else {
        msg.blind = _("{}が何かをつぶやいた。", "{} mumbles.");
        msg.to_player = _("{}がファイア・ボールの呪文を唱えた。", "{} casts a fire ball.");
        msg.to_mons = _("{}が{}に向かってファイア・ボールの呪文を唱えた。", "{} casts a fire ball at {}.");
    }

    return monspell_message(player_ptr, m_idx, t_idx, msg, target_type);
}

static bool message_water_ball(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto known = monster_near_player(*player_ptr->current_floor_ptr, m_idx, t_idx);
    auto see_either = see_monster(player_ptr, m_idx) || see_monster(player_ptr, t_idx);
    auto mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    auto mon_to_player = (target_type == MONSTER_TO_PLAYER);
    const auto t_name = monster_name(player_ptr, t_idx);

    mspell_cast_msg_blind msg(_("{}が何かをつぶやいた。", "{} mumbles."), _("{}が流れるような身振りをした。", "{} gestures fluidly."),
        _("{}が{}に対して流れるような身振りをした。", "{} gestures fluidly at {}."));

    auto result = monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    if (mon_to_player) {
        msg_print(_("あなたは渦巻きに飲み込まれた。", "You are engulfed in a whirlpool."));
    } else if (mon_to_mon && known && see_either && !player_ptr->effects()->blindness().is_active()) {
        msg_print(_("{}は渦巻に飲み込まれた。", "{} is engulfed in a whirlpool."), str_upcase_first(t_name));
    }
    return result;
}

const std::unordered_map<MonsterAbilityType, MSpellData> ball_list = {
    { MonsterAbilityType::BA_NUKE, { { _("{}が何かをつぶやいた。", "{} mumbles."),
                                         _("{}が放射能球を放った。", "{} casts a ball of radiation."),
                                         _("{}が{}に放射能球を放った。", "{} casts a ball of radiation at {}.") },
                                       AttributeType::NUKE, DRS_POIS } },
    { MonsterAbilityType::BA_CHAO, { { _("{}が恐ろしげにつぶやいた。", "{} mumbles frighteningly."),
                                         _("{}が純ログルスを放った。", "{} invokes a raw Logrus."),
                                         _("{}が{}に純ログルスを放った。", "{} invokes raw Logrus upon {}.") },
                                       AttributeType::CHAOS, DRS_CHAOS } },
    { MonsterAbilityType::BA_ACID, { { _("{}が何かをつぶやいた。", "{} mumbles."),
                                         _("{}がアシッド・ボールの呪文を唱えた。", "{} casts an acid ball."),
                                         _("{}が{}に向かってアシッド・ボールの呪文を唱えた。", "{} casts an acid ball at {}.") },
                                       AttributeType::ACID, DRS_ACID } },
    { MonsterAbilityType::BA_ELEC, { { _("{}が何かをつぶやいた。", "{} mumbles."),
                                         _("{}がサンダー・ボールの呪文を唱えた。", "{} casts a lightning ball."),
                                         _("{}が{}に向かってサンダー・ボールの呪文を唱えた。", "{} casts a lightning ball at {}.") },
                                       AttributeType::ELEC, DRS_ELEC } },
    { MonsterAbilityType::BA_FIRE, { message_fire_ball, AttributeType::FIRE, DRS_FIRE } },
    { MonsterAbilityType::BA_COLD, { { _("{}が何かをつぶやいた。", "{} mumbles."),
                                         _("{}がアイス・ボールの呪文を唱えた。", "{} casts a frost ball."),
                                         _("{}が{}に向かってアイス・ボールの呪文を唱えた。", "{} casts a frost ball at {}.") },
                                       AttributeType::COLD, DRS_COLD } },
    { MonsterAbilityType::BA_POIS, { { _("{}が何かをつぶやいた。", "{} mumbles."),
                                         _("{}が悪臭雲の呪文を唱えた。", "{} casts a stinking cloud."),
                                         _("{}が{}に向かって悪臭雲の呪文を唱えた。", "{} casts a stinking cloud at {}.") },
                                       AttributeType::POIS, DRS_POIS } },
    { MonsterAbilityType::BA_NETH, { { _("{}が何かをつぶやいた。", "{} mumbles."),
                                         _("{}が地獄球の呪文を唱えた。", "{} casts a nether ball."),
                                         _("{}が{}に向かって地獄球の呪文を唱えた。", "{} casts a nether ball at {}.") },
                                       AttributeType::NETHER, DRS_NETH } },
    { MonsterAbilityType::BA_WATE, { message_water_ball, AttributeType::WATER } },
    { MonsterAbilityType::BA_MANA, { { _("{}が何かを力強くつぶやいた。", "{} mumbles powerfully."),
                                         _("{}が魔力の嵐の呪文を念じた。", "{} invokes a mana storm."),
                                         _("{}が{}に対して魔力の嵐の呪文を念じた。", "{} invokes a mana storm upon {}.") },
                                       AttributeType::MANA } },
    { MonsterAbilityType::BA_DARK, { { _("{}が何かを力強くつぶやいた。", "{} mumbles powerfully."),
                                         _("{}が暗黒の嵐の呪文を念じた。", "{} invokes a darkness storm."),
                                         _("{}が{}に対して暗黒の嵐の呪文を念じた。", "{} invokes a darkness storm upon {}.") },
                                       AttributeType::DARK, DRS_DARK } },
    { MonsterAbilityType::BA_LITE, { { _("{}が何かを力強くつぶやいた。", "{} mumbles powerfully."),
                                         _("{}がスターバーストの呪文を念じた。", "{} invokes a starburst."),
                                         _("{}が{}に対してスターバーストの呪文を念じた。", "{} invokes a starburst upon {}.") },
                                       AttributeType::LITE, DRS_LITE } },
    { MonsterAbilityType::BA_VOID, { { _("{}が何かを力強くつぶやいた。", "{} mumbles powerfully."),
                                         _("{}が虚無の嵐の呪文を念じた。", "{} invokes a void storm."),
                                         _("{}が{}に対して虚無の嵐の呪文を念じた。", "{} invokes a void storm upon {}.") },
                                       AttributeType::VOID_MAGIC } },
    { MonsterAbilityType::BA_ABYSS, { { _("{}が何かを力強くつぶやいた。", "{} mumbles powerfully."),
                                          _("{}が深淵の嵐の呪文を念じた。", "{} invokes a abyss storm."),
                                          _("{}が{}に対して深淵の嵐の呪文を念じた。", "{} invokes a void abyss upon {}.") },
                                        AttributeType::ABYSS, DRS_DARK } },
    { MonsterAbilityType::BA_METEOR, { { _("{}が何かを力強くつぶやいた。", "{} mumbles powerfully."),
                                           _("{}がメテオスウォームの呪文を念じた。", "{} invokes a meteor swarm."),
                                           _("{}が{}に対してメテオスウォームの呪文を念じた。", "{} invokes a meteor swarm upon {}.") },
                                         AttributeType::METEOR } }
};

MSpellBall::MSpellBall(PlayerType *player_ptr, MONSTER_IDX m_idx, MonsterAbilityType ability, POSITION rad, int target_type)
    : AbstractMSpellAttack(player_ptr, m_idx, ability, get_mspell_data(ball_list, ability), target_type,
          [=](auto y, auto x, int dam, auto attribute) {
              return ball(player_ptr, y, x, m_idx, attribute, dam, rad, target_type);
          })
{
}

MSpellBall::MSpellBall(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, MonsterAbilityType ability, POSITION rad, int target_type)
    : AbstractMSpellAttack(player_ptr, m_idx, t_idx, ability, get_mspell_data(ball_list, ability), target_type,
          [=](auto y, auto x, int dam, auto attribute) {
              return ball(player_ptr, y, x, m_idx, attribute, dam, rad, target_type);
          })
{
}

/*!
 * @brief モンスターが放つボールの半径を返す
 * @param ability モンスターの能力
 * @param is_powerful 強力なモンスター (POWERFUL) か
 * @return ボールの半径。ボールでない能力 (ロケット・ブレスを含む) なら tl::nullopt
 * @details 強さによらず半径 4 のボールは RF_ABILITY_BIG_BALL_MASK で決める。
 */
tl::optional<POSITION> get_monster_ball_radius(MonsterAbilityType ability, bool is_powerful)
{
    if (RF_ABILITY_BIG_BALL_MASK.has(ability)) {
        return 4;
    }

    switch (ability) {
    case MonsterAbilityType::BA_ACID:
    case MonsterAbilityType::BA_ELEC:
    case MonsterAbilityType::BA_FIRE:
    case MonsterAbilityType::BA_COLD:
        return is_powerful ? 4 : 2;
    case MonsterAbilityType::BA_POIS:
    case MonsterAbilityType::BA_NETH:
    case MonsterAbilityType::BA_NUKE:
        return 2;
    default:
        return tl::nullopt;
    }
}
