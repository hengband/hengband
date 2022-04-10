#include "mspell/mspell-attack/mspell-ball.h"
#include "effect/attribute-types.h"
#include "effect/effect-processor.h"
#include "main/sound-of-music.h"
#include "mind/drs-types.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "mspell/mspell-attack/abstract-mspell.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-data.h"
#include "mspell/mspell-result.h"
#include "mspell/mspell-util.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

static bool message_fire_ball(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg;
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];

    if (m_ptr->r_idx == MonsterRaceId::ROLENTO) {
        msg.blind = _("%sが何かを投げた。", "%^s throws something.");
        msg.to_player = _("%sは手榴弾を投げた。", "%^s throws a hand grenade.");
        msg.to_mons = _("%^sが%^sに向かって手榴弾を投げた。", "%^s throws a hand grenade.");
    } else {
        msg.blind = _("%^sが何かをつぶやいた。", "%^s mumbles.");
        msg.to_player = _("%^sがファイア・ボールの呪文を唱えた。", "%^s casts a fire ball.");
        msg.to_mons = _("%^sが%sに向かってファイア・ボールの呪文を唱えた。", "%^s casts a fire ball at %s.");
    }

    return monspell_message(player_ptr, m_idx, t_idx, msg, target_type);
}

static bool message_water_ball(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto known = monster_near_player(player_ptr->current_floor_ptr, m_idx, t_idx);
    auto see_either = see_monster(player_ptr, m_idx) || see_monster(player_ptr, t_idx);
    auto mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    auto mon_to_player = (target_type == MONSTER_TO_PLAYER);
    GAME_TEXT t_name[MAX_NLEN];
    monster_name(player_ptr, t_idx, t_name);

    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."), _("%^sが流れるような身振りをした。", "%^s gestures fluidly."),
        _("%^sが%sに対して流れるような身振りをした。", "%^s gestures fluidly at %s."));

    auto result = monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    if (mon_to_player) {
        msg_format(_("あなたは渦巻きに飲み込まれた。", "You are engulfed in a whirlpool."));
    } else if (mon_to_mon && known && see_either && !player_ptr->blind) {
        msg_format(_("%^sは渦巻に飲み込まれた。", "%^s is engulfed in a whirlpool."), t_name);
    }
    return result;
}

const std::unordered_map<MonsterAbilityType, MSpellData> ball_list = {
    { MonsterAbilityType::BA_NUKE, { { _("%^sが何かをつぶやいた。", "%^s mumbles."),
                                         _("%^sが放射能球を放った。", "%^s casts a ball of radiation."),
                                         _("%^sが%sに放射能球を放った。", "%^s casts a ball of radiation at %s.") },
                                       AttributeType::NUKE, DRS_POIS } },
    { MonsterAbilityType::BA_CHAO, { { _("%^sが恐ろしげにつぶやいた。", "%^s mumbles frighteningly."),
                                         _("%^sが純ログルスを放った。", "%^s invokes a raw Logrus."),
                                         _("%^sが%sに純ログルスを放った。", "%^s invokes raw Logrus upon %s.") },
                                       AttributeType::CHAOS, DRS_CHAOS } },
    { MonsterAbilityType::BA_ACID, { { _("%^sが何かをつぶやいた。", "%^s mumbles."),
                                         _("%^sがアシッド・ボールの呪文を唱えた。", "%^s casts an acid ball."),
                                         _("%^sが%sに向かってアシッド・ボールの呪文を唱えた。", "%^s casts an acid ball at %s.") },
                                       AttributeType::ACID, DRS_ACID } },
    { MonsterAbilityType::BA_ELEC, { { _("%^sが何かをつぶやいた。", "%^s mumbles."),
                                         _("%^sがサンダー・ボールの呪文を唱えた。", "%^s casts a lightning ball."),
                                         _("%^sが%sに向かってサンダー・ボールの呪文を唱えた。", "%^s casts a lightning ball at %s.") },
                                       AttributeType::ELEC, DRS_ELEC } },
    { MonsterAbilityType::BA_FIRE, { message_fire_ball, AttributeType::FIRE, DRS_FIRE } },
    { MonsterAbilityType::BA_COLD, { { _("%^sが何かをつぶやいた。", "%^s mumbles."),
                                         _("%^sがアイス・ボールの呪文を唱えた。", "%^s casts a frost ball."),
                                         _("%^sが%sに向かってアイス・ボールの呪文を唱えた。", "%^s casts a frost ball at %s.") },
                                       AttributeType::COLD, DRS_COLD } },
    { MonsterAbilityType::BA_POIS, { { _("%^sが何かをつぶやいた。", "%^s mumbles."),
                                         _("%^sが悪臭雲の呪文を唱えた。", "%^s casts a stinking cloud."),
                                         _("%^sが%sに向かって悪臭雲の呪文を唱えた。", "%^s casts a stinking cloud at %s.") },
                                       AttributeType::POIS, DRS_POIS } },
    { MonsterAbilityType::BA_NETH, { { _("%^sが何かをつぶやいた。", "%^s mumbles."),
                                         _("%^sが地獄球の呪文を唱えた。", "%^s casts a nether ball."),
                                         _("%^sが%sに向かって地獄球の呪文を唱えた。", "%^s casts a nether ball at %s.") },
                                       AttributeType::NETHER, DRS_NETH } },
    { MonsterAbilityType::BA_WATE, { message_water_ball, AttributeType::WATER } },
    { MonsterAbilityType::BA_MANA, { { _("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."),
                                         _("%^sが魔力の嵐の呪文を念じた。", "%^s invokes a mana storm."),
                                         _("%^sが%sに対して魔力の嵐の呪文を念じた。", "%^s invokes a mana storm upon %s.") },
                                       AttributeType::MANA } },
    { MonsterAbilityType::BA_DARK, { { _("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."),
                                         _("%^sが暗黒の嵐の呪文を念じた。", "%^s invokes a darkness storm."),
                                         _("%^sが%sに対して暗黒の嵐の呪文を念じた。", "%^s invokes a darkness storm upon %s.") },
                                       AttributeType::DARK, DRS_DARK } },
    { MonsterAbilityType::BA_LITE, { { _("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."),
                                         _("%^sがスターバーストの呪文を念じた。", "%^s invokes a starburst."),
                                         _("%^sが%sに対してスターバーストの呪文を念じた。", "%^s invokes a starburst upon %s.") },
                                       AttributeType::LITE, DRS_LITE } },
    { MonsterAbilityType::BA_VOID, { { _("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."),
                                         _("%^sが虚無の嵐の呪文を念じた。", "%^s invokes a void storm."),
                                         _("%^sが%sに対して虚無の嵐の呪文を念じた。", "%^s invokes a void storm upon %s.") },
                                       AttributeType::VOID_MAGIC } },
    { MonsterAbilityType::BA_ABYSS, { { _("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."),
                                          _("%^sが深淵の嵐の呪文を念じた。", "%^s invokes a abyss storm."),
                                          _("%^sが%sに対して深淵の嵐の呪文を念じた。", "%^s invokes a void abyss upon %s.") },
                                        AttributeType::ABYSS, DRS_DARK } }
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
