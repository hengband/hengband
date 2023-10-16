#include "mspell/mspell-attack/mspell-bolt.h"
#include "effect/attribute-types.h"
#include "effect/effect-processor.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/drs-types.h"
#include "monster-race/race-ability-flags.h"
#include "monster/monster-info.h"
#include "monster/monster-update.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-data.h"
#include "mspell/mspell-result.h"
#include "mspell/mspell-util.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"

static bool message_shoot(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%s^が奇妙な音を発した。", "%s^ makes a strange noise."),
        _("%s^が矢を放った。", "%s^ fires an arrow."),
        _("%s^が%sに矢を放った。", "%s^ fires an arrow at %s."));

    auto notice = monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    if (notice) {
        sound(SOUND_SHOOT);
    }
    return notice;
}

const std::unordered_map<MonsterAbilityType, MSpellData> bolt_list = {
    { MonsterAbilityType::SHOOT, { message_shoot, AttributeType::MONSTER_SHOOT } },
    { MonsterAbilityType::BO_ACID, { { _("%s^が何かをつぶやいた。", "%s^ mumbles."),
                                         _("%s^がアシッド・ボルトの呪文を唱えた。", "%s^ casts an acid bolt."),
                                         _("%sが%sに向かってアシッド・ボルトの呪文を唱えた。", "%s^ casts an acid bolt at %s.") },
                                       AttributeType::ACID, { DRS_REFLECT, DRS_ACID } } },
    { MonsterAbilityType::BO_ELEC, { { _("%s^が何かをつぶやいた。", "%s^ mumbles."),
                                         _("%s^がサンダー・ボルトの呪文を唱えた。", "%s^ casts a lightning bolt."),
                                         _("%s^が%sに向かってサンダー・ボルトの呪文を唱えた。", "%s^ casts a lightning bolt at %s.") },
                                       AttributeType::ELEC, { DRS_REFLECT, DRS_ELEC } } },
    { MonsterAbilityType::BO_FIRE, { { _("%s^が何かをつぶやいた。", "%s^ mumbles."),
                                         _("%s^がファイア・ボルトの呪文を唱えた。", "%s^ casts a fire bolt."),
                                         _("%s^が%sに向かってファイア・ボルトの呪文を唱えた。", "%s^ casts a fire bolt at %s.") },
                                       AttributeType::FIRE, { DRS_REFLECT, DRS_FIRE } } },
    { MonsterAbilityType::BO_COLD, { { _("%s^が何かをつぶやいた。", "%s^ mumbles."),
                                         _("%s^がアイス・ボルトの呪文を唱えた。", "%s^ casts a frost bolt."),
                                         _("%s^が%sに向かってアイス・ボルトの呪文を唱えた。", "%s^ casts a frost bolt at %s.") },
                                       AttributeType::COLD, { DRS_REFLECT, DRS_COLD } } },
    { MonsterAbilityType::BO_NETH, { { _("%s^が何かをつぶやいた。", "%s^ mumbles."),
                                         _("%s^が地獄の矢の呪文を唱えた。", "%s^ casts a nether bolt."),
                                         _("%s^が%sに向かって地獄の矢の呪文を唱えた。", "%s^ casts a nether bolt at %s.") },
                                       AttributeType::NETHER, { DRS_REFLECT, DRS_NETH } } },
    { MonsterAbilityType::BO_WATE, { { _("%s^が何かをつぶやいた。", "%s^ mumbles."),
                                         _("%s^がウォーター・ボルトの呪文を唱えた。", "%s^ casts a water bolt."),
                                         _("%s^が%sに向かってウォーター・ボルトの呪文を唱えた。", "%s^ casts a water bolt at %s.") },
                                       AttributeType::WATER, DRS_REFLECT } },
    { MonsterAbilityType::BO_MANA, { { _("%s^が何かをつぶやいた。", "%s^ mumbles."),
                                         _("%s^が魔力の矢の呪文を唱えた。", "%s^ casts a mana bolt."),
                                         _("%s^が%sに向かって魔力の矢の呪文を唱えた。", "%s^ casts a mana bolt at %s.") },
                                       AttributeType::MANA, DRS_REFLECT } },
    { MonsterAbilityType::BO_PLAS, { { _("%s^が何かをつぶやいた。", "%s^ mumbles."),
                                         _("%s^がプラズマ・ボルトの呪文を唱えた。", "%s^ casts a plasma bolt."),
                                         _("%s^が%sに向かってプラズマ・ボルトの呪文を唱えた。", "%s^ casts a plasma bolt at %s.") },
                                       AttributeType::PLASMA, DRS_REFLECT } },
    { MonsterAbilityType::BO_ICEE, { { _("%s^が何かをつぶやいた。", "%s^ mumbles."),
                                         _("%s^が極寒の矢の呪文を唱えた。", "%s^ casts an ice bolt."),
                                         _("%s^が%sに向かって極寒の矢の呪文を唱えた。", "%s^ casts an ice bolt at %s.") },
                                       AttributeType::ICE, { DRS_REFLECT, DRS_NETH } } },
    { MonsterAbilityType::BO_VOID, { { _("%s^が何かをつぶやいた。", "%s^ mumbles."),
                                         _("%s^がヴォイド・ボルトの呪文を唱えた。", "%s^ casts a void bolt."),
                                         _("%s^が%sに向かってヴォイド・ボルトの呪文を唱えた。", "%s^ casts a void bolt at %s.") },
                                       AttributeType::VOID_MAGIC, DRS_REFLECT } },
    { MonsterAbilityType::BO_ABYSS, { { _("%s^が何かをつぶやいた。", "%s^ mumbles."),
                                          _("%s^がアビス・ボルトの呪文を唱えた。", "%s^ casts a abyss bolt."),
                                          _("%s^が%sに向かってアビス・ボルトの呪文を唱えた。", "%s^ casts a abyss bolt at %s.") },
                                        AttributeType::ABYSS, DRS_REFLECT } },
    { MonsterAbilityType::BO_METEOR, { { _("%s^が何かをつぶやいた。", "%s^ mumbles."),
                                           _("%s^がメテオストライクの呪文を唱えた。", "%s^ casts a meteor strike."),
                                           _("%s^が%sに向かってメテオストライクの呪文を唱えた。", "%s^ casts a meteor strike at %s.") },
                                         AttributeType::METEOR, DRS_REFLECT } },
    { MonsterAbilityType::MISSILE, { { _("%s^が何かをつぶやいた。", "%s^ mumbles."),
                                         _("%s^がマジック・ミサイルの呪文を唱えた。", "%s^ casts a magic missile."),
                                         _("%s^が%sに向かってマジック・ミサイルの呪文を唱えた。", "%s^ casts a magic missile at %s.") },
                                       AttributeType::MISSILE, DRS_REFLECT } },
};

MSpellBolt::MSpellBolt(PlayerType *player_ptr, MONSTER_IDX m_idx, MonsterAbilityType ability, int target_type)
    : AbstractMSpellAttack(player_ptr, m_idx, ability, get_mspell_data(bolt_list, ability), target_type,
          [=](auto y, auto x, int dam, auto attribute) {
              return bolt(player_ptr, m_idx, y, x, attribute, dam, target_type);
          })
{
}

MSpellBolt::MSpellBolt(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, MonsterAbilityType ability, int target_type)
    : AbstractMSpellAttack(player_ptr, m_idx, t_idx, ability, get_mspell_data(bolt_list, ability), target_type,
          [=](auto y, auto x, int dam, auto attribute) {
              return bolt(player_ptr, m_idx, y, x, attribute, dam, target_type);
          })
{
}
