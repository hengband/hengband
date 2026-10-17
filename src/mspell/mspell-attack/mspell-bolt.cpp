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
#include "system/floor/floor-info.h"
#include "system/player-type-definition.h"

static bool message_shoot(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("{}が奇妙な音を発した。", "{} makes a strange noise."),
        _("{}が矢を放った。", "{} fires an arrow."),
        _("{}が{}に矢を放った。", "{} fires an arrow at {}."));

    auto notice = monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    if (notice) {
        sound(SoundKind::SHOOT);
    }
    return notice;
}

const std::unordered_map<MonsterAbilityType, MSpellData> bolt_list = {
    { MonsterAbilityType::SHOOT, { message_shoot, AttributeType::MONSTER_SHOOT } },
    { MonsterAbilityType::BO_ACID, { { _("{}が何かをつぶやいた。", "{} mumbles."),
                                         _("{}がアシッド・ボルトの呪文を唱えた。", "{} casts an acid bolt."),
                                         _("{}が{}に向かってアシッド・ボルトの呪文を唱えた。", "{} casts an acid bolt at {}.") },
                                       AttributeType::ACID, { DRS_REFLECT, DRS_ACID } } },
    { MonsterAbilityType::BO_ELEC, { { _("{}が何かをつぶやいた。", "{} mumbles."),
                                         _("{}がサンダー・ボルトの呪文を唱えた。", "{} casts a lightning bolt."),
                                         _("{}が{}に向かってサンダー・ボルトの呪文を唱えた。", "{} casts a lightning bolt at {}.") },
                                       AttributeType::ELEC, { DRS_REFLECT, DRS_ELEC } } },
    { MonsterAbilityType::BO_FIRE, { { _("{}が何かをつぶやいた。", "{} mumbles."),
                                         _("{}がファイア・ボルトの呪文を唱えた。", "{} casts a fire bolt."),
                                         _("{}が{}に向かってファイア・ボルトの呪文を唱えた。", "{} casts a fire bolt at {}.") },
                                       AttributeType::FIRE, { DRS_REFLECT, DRS_FIRE } } },
    { MonsterAbilityType::BO_COLD, { { _("{}が何かをつぶやいた。", "{} mumbles."),
                                         _("{}がアイス・ボルトの呪文を唱えた。", "{} casts a frost bolt."),
                                         _("{}が{}に向かってアイス・ボルトの呪文を唱えた。", "{} casts a frost bolt at {}.") },
                                       AttributeType::COLD, { DRS_REFLECT, DRS_COLD } } },
    { MonsterAbilityType::BO_NETH, { { _("{}が何かをつぶやいた。", "{} mumbles."),
                                         _("{}が地獄の矢の呪文を唱えた。", "{} casts a nether bolt."),
                                         _("{}が{}に向かって地獄の矢の呪文を唱えた。", "{} casts a nether bolt at {}.") },
                                       AttributeType::NETHER, { DRS_REFLECT, DRS_NETH } } },
    { MonsterAbilityType::BO_WATE, { { _("{}が何かをつぶやいた。", "{} mumbles."),
                                         _("{}がウォーター・ボルトの呪文を唱えた。", "{} casts a water bolt."),
                                         _("{}が{}に向かってウォーター・ボルトの呪文を唱えた。", "{} casts a water bolt at {}.") },
                                       AttributeType::WATER, DRS_REFLECT } },
    { MonsterAbilityType::BO_MANA, { { _("{}が何かをつぶやいた。", "{} mumbles."),
                                         _("{}が魔力の矢の呪文を唱えた。", "{} casts a mana bolt."),
                                         _("{}が{}に向かって魔力の矢の呪文を唱えた。", "{} casts a mana bolt at {}.") },
                                       AttributeType::MANA, DRS_REFLECT } },
    { MonsterAbilityType::BO_PLAS, { { _("{}が何かをつぶやいた。", "{} mumbles."),
                                         _("{}がプラズマ・ボルトの呪文を唱えた。", "{} casts a plasma bolt."),
                                         _("{}が{}に向かってプラズマ・ボルトの呪文を唱えた。", "{} casts a plasma bolt at {}.") },
                                       AttributeType::PLASMA, DRS_REFLECT } },
    { MonsterAbilityType::BO_ICEE, { { _("{}が何かをつぶやいた。", "{} mumbles."),
                                         _("{}が極寒の矢の呪文を唱えた。", "{} casts an ice bolt."),
                                         _("{}が{}に向かって極寒の矢の呪文を唱えた。", "{} casts an ice bolt at {}.") },
                                       AttributeType::ICE, { DRS_REFLECT, DRS_COLD } } },
    { MonsterAbilityType::BO_VOID, { { _("{}が何かをつぶやいた。", "{} mumbles."),
                                         _("{}がヴォイド・ボルトの呪文を唱えた。", "{} casts a void bolt."),
                                         _("{}が{}に向かってヴォイド・ボルトの呪文を唱えた。", "{} casts a void bolt at {}.") },
                                       AttributeType::VOID_MAGIC, DRS_REFLECT } },
    { MonsterAbilityType::BO_ABYSS, { { _("{}が何かをつぶやいた。", "{} mumbles."),
                                          _("{}がアビス・ボルトの呪文を唱えた。", "{} casts a abyss bolt."),
                                          _("{}が{}に向かってアビス・ボルトの呪文を唱えた。", "{} casts a abyss bolt at {}.") },
                                        AttributeType::ABYSS, DRS_REFLECT } },
    { MonsterAbilityType::BO_METEOR, { { _("{}が何かをつぶやいた。", "{} mumbles."),
                                           _("{}がメテオストライクの呪文を唱えた。", "{} casts a meteor strike."),
                                           _("{}が{}に向かってメテオストライクの呪文を唱えた。", "{} casts a meteor strike at {}.") },
                                         AttributeType::METEOR, DRS_REFLECT } },
    { MonsterAbilityType::BO_LITE, { { _("{}が何かをつぶやいた。", "{} mumbles."),
                                         _("{}がスターライトアローを放った。", "{} fires a starlight arrow."),
                                         _("{}が{}に向かってスターライトアローを放った。", "{} fires a starlight arrow at {}.") },
                                       AttributeType::LITE, { DRS_REFLECT, DRS_LITE } } },
    { MonsterAbilityType::MISSILE, { { _("{}が何かをつぶやいた。", "{} mumbles."),
                                         _("{}がマジック・ミサイルの呪文を唱えた。", "{} casts a magic missile."),
                                         _("{}が{}に向かってマジック・ミサイルの呪文を唱えた。", "{} casts a magic missile at {}.") },
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
