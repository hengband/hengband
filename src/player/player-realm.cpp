﻿#include "player/player-realm.h"
#include "object/tval-types.h"

/*!
 * 職業毎に選択可能な第一領域魔法テーブル
 */
const u32b realm_choices1[MAX_CLASS] = {
    (CH_NONE), /* Warrior */
    (CH_LIFE | CH_SORCERY | CH_NATURE | CH_CHAOS | CH_DEATH | CH_TRUMP | CH_ARCANE | CH_ENCHANT | CH_DAEMON | CH_CRUSADE), /* Mage */
    (CH_LIFE | CH_DEATH | CH_DAEMON | CH_CRUSADE), /* Priest */
    (CH_SORCERY | CH_DEATH | CH_TRUMP | CH_ARCANE | CH_ENCHANT), /* Rogue */
    (CH_NATURE), /* Ranger */
    (CH_CRUSADE | CH_DEATH), /* Paladin */
    (CH_ARCANE), /* Warrior-Mage */
    (CH_CHAOS | CH_DAEMON), /* Chaos-Warrior */
    (CH_LIFE | CH_NATURE | CH_DEATH | CH_ENCHANT), /* Monk */
    (CH_NONE), /* Mindcrafter */
    (CH_LIFE | CH_SORCERY | CH_NATURE | CH_CHAOS | CH_DEATH | CH_TRUMP | CH_ARCANE | CH_ENCHANT | CH_DAEMON | CH_CRUSADE | CH_HEX), /* High-Mage */
    (CH_ARCANE), /* Tourist */
    (CH_NONE), /* Imitator */
    (CH_TRUMP), /* Beastmaster */
    (CH_NONE), /* Sorcerer */
    (CH_NONE), /* Archer */
    (CH_NONE), /* Magic eater */
    (CH_MUSIC), /* Bard */
    (CH_NONE), /* Red Mage */
    (CH_HISSATSU), /* Samurai */
    (CH_LIFE | CH_NATURE | CH_DEATH | CH_ENCHANT | CH_CRUSADE), /* ForceTrainer */
    (CH_NONE), /* Blue Mage */
    (CH_NONE), /* Cavalry */
    (CH_NONE), /* Berserker */
    (CH_NONE), /* Weaponsmith */
    (CH_NONE), /* Mirror-master */
    (CH_NONE), /* Ninja */
    (CH_NONE), /* Sniper */
};

/*!
 * 職業毎に選択可能な第二領域魔法テーブル
 */
const u32b realm_choices2[MAX_CLASS] = {
    (CH_NONE), /* Warrior */
    (CH_LIFE | CH_SORCERY | CH_NATURE | CH_CHAOS | CH_DEATH | CH_TRUMP | CH_ARCANE | CH_ENCHANT | CH_DAEMON | CH_CRUSADE), /* Mage */
    (CH_LIFE | CH_SORCERY | CH_NATURE | CH_CHAOS | CH_DEATH | CH_TRUMP | CH_ARCANE | CH_ENCHANT | CH_DAEMON | CH_CRUSADE), /* Priest */
    (CH_NONE), /* Rogue */
    (CH_SORCERY | CH_CHAOS | CH_DEATH | CH_TRUMP | CH_ARCANE | CH_DAEMON), /* Ranger */
    (CH_NONE), /* Paladin */
    (CH_LIFE | CH_NATURE | CH_CHAOS | CH_DEATH | CH_TRUMP | CH_ARCANE | CH_SORCERY | CH_ENCHANT | CH_DAEMON | CH_CRUSADE), /* Warrior-Mage */
    (CH_NONE), /* Chaos-Warrior */
    (CH_NONE), /* Monk */
    (CH_NONE), /* Mindcrafter */
    (CH_NONE), /* High-Mage */
    (CH_NONE), /* Tourist */
    (CH_NONE), /* Imitator */
    (CH_NONE), /* Beastmanster */
    (CH_NONE), /* Sorcerer */
    (CH_NONE), /* Archer */
    (CH_NONE), /* Magic eater */
    (CH_NONE), /* Bard */
    (CH_NONE), /* Red Mage */
    (CH_NONE), /* Samurai */
    (CH_NONE), /* ForceTrainer */
    (CH_NONE), /* Blue Mage */
    (CH_NONE), /* Cavalry */
    (CH_NONE), /* Berserker */
    (CH_NONE), /* Weaponsmith */
    (CH_NONE), /* Mirror-master */
    (CH_NONE), /* Ninja */
    (CH_NONE), /* Sniper */
};

REALM_IDX get_realm1_book(player_type *player_ptr) { return player_ptr->realm1 + TV_LIFE_BOOK - 1; }

REALM_IDX get_realm2_book(player_type *player_ptr) { return player_ptr->realm2 + TV_LIFE_BOOK - 1; }

bool is_wizard_class(player_type *player_ptr)
{
    return (player_ptr->pclass == CLASS_MAGE || player_ptr->pclass == CLASS_HIGH_MAGE || player_ptr->pclass == CLASS_SORCERER || player_ptr->pclass == CLASS_MAGIC_EATER
        || player_ptr->pclass == CLASS_BLUE_MAGE);
}
