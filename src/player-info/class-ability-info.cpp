﻿#include "player-info/class-ability-info.h"
#include "player-info/self-info-util.h"
#include "realm/realm-names-table.h"
#include "realm/realm-types.h"

void set_class_ability_info(player_type *creature_ptr, self_info_type *self_ptr)
{
    switch (creature_ptr->pclass) {
    case CLASS_WARRIOR:
        if (creature_ptr->lev > 39)
            self_ptr->info[self_ptr->line++]
                = _("あなたはランダムな方向に対して数回攻撃することができる。(75 HP)", "You can attack some random directions simultaneously (cost 75).");

        break;
    case CLASS_HIGH_MAGE:
        if (creature_ptr->realm1 == REALM_HEX)
            break;
        /* Fall through */
    case CLASS_MAGE:
    case CLASS_SORCERER:
        if (creature_ptr->lev > 24)
            self_ptr->info[self_ptr->line++] = _("あなたはアイテムの魔力を吸収することができる。(1 MP)", "You can absorb charges from an item (cost 1).");

        break;
    case CLASS_PRIEST:
        if (is_good_realm(creature_ptr->realm1)) {
            if (creature_ptr->lev > 34)
                self_ptr->info[self_ptr->line++] = _("あなたは武器を祝福することができる。(70 MP)", "You can bless a weapon (cost 70).");

            break;
        }

        if (creature_ptr->lev > 41)
            self_ptr->info[self_ptr->line++]
                = _("あなたは周りのすべてのモンスターを攻撃することができる。(40 MP)", "You can damage all monsters in sight (cost 40).");

        break;
    case CLASS_ROGUE:
        if (creature_ptr->lev > 7)
            self_ptr->info[self_ptr->line++]
                = _("あなたは攻撃して即座に逃げることができる。(12 MP)", "You can hit a monster and teleport away simultaneously (cost 12).");

        break;
    case CLASS_RANGER:
        if (creature_ptr->lev > 14)
            self_ptr->info[self_ptr->line++] = _("あなたは怪物を調査することができる。(20 MP)", "You can probe monsters (cost 20).");

        break;
    case CLASS_PALADIN:
        if (is_good_realm(creature_ptr->realm1)) {
            if (creature_ptr->lev > 29) {
                self_ptr->info[self_ptr->line++] = _("あなたは聖なる槍を放つことができる。(30 MP)", "You can fire a holy spear (cost 30).");
            }

            break;
        }

        if (creature_ptr->lev > 29)
            self_ptr->info[self_ptr->line++]
                = _("あなたは生命力を減少させる槍を放つことができる。(30 MP)", "You can fire a spear which drains vitality (cost 30).");

        break;
    case CLASS_WARRIOR_MAGE:
        if (creature_ptr->lev > 24) {
            self_ptr->info[self_ptr->line++] = _("あなたはＨＰをＭＰに変換することができる。(消費なし)", "You can convert HP to SP (cost 0).");
            self_ptr->info[self_ptr->line++] = _("あなたはＭＰをＨＰに変換することができる。(消費なし)", "You can convert SP to HP (cost 0).");
        }

        break;
    case CLASS_CHAOS_WARRIOR:
        if (creature_ptr->lev > 39) {
            self_ptr->info[self_ptr->line++]
                = _("あなたは周囲に怪物を惑わす光を発生させることができる。(50 MP)", "You can radiate light which confuses nearby monsters (cost 50).");
        }

        break;
    case CLASS_MONK:
        if (creature_ptr->lev > 24)
            self_ptr->info[self_ptr->line++] = _("あなたは構えることができる。(消費なし)", "You can assume a special stance (cost 0).");

        if (creature_ptr->lev > 29)
            self_ptr->info[self_ptr->line++]
                = _("あなたは通常の2倍の攻撃を行うことができる。(30 MP)", "You can perform two attacks at the same time (cost 30).");

        break;
    case CLASS_MINDCRAFTER:
    case CLASS_FORCETRAINER:
        if (creature_ptr->lev > 14)
            self_ptr->info[self_ptr->line++]
                = _("あなたは精神を集中してＭＰを回復させることができる。(消費なし)", "You can concentrate to regenerate your mana (cost 0).");

        break;
    case CLASS_TOURIST:
        self_ptr->info[self_ptr->line++] = _("あなたは写真を撮影することができる。(消費なし)", "You can take a photograph (cost 0).");

        if (creature_ptr->lev > 24)
            self_ptr->info[self_ptr->line++] = _("あなたはアイテムを完全に鑑定することができる。(20 MP)", "You can *identify* items (cost 20).");

        break;
    case CLASS_IMITATOR:
        if (creature_ptr->lev > 29)
            self_ptr->info[self_ptr->line++] = _("あなたは怪物の特殊攻撃をダメージ2倍でまねることができる。(100 MP)",
                "You can imitate monster's special attacks with double damage (cost 100).");

        break;
    case CLASS_BEASTMASTER:
        self_ptr->info[self_ptr->line++]
            = _("あなたは1体の生命のあるモンスターを支配することができる。((レベル+3)/4 MP)", "You can dominate a monster (cost (level+3)/4).");

        if (creature_ptr->lev > 29)
            self_ptr->info[self_ptr->line++] = _("あなたは視界内の生命のあるモンスターを支配することができる。((レベル+20)/2 MP)",
                "You can dominate living monsters in sight (cost (level+20)/2).");

        break;
    case CLASS_MAGIC_EATER:
        self_ptr->info[self_ptr->line++]
            = _("あなたは杖/魔法棒/ロッドの魔力を自分のものにすることができる。(消費なし)", "You can absorb a staff, wand or rod itself (cost 0).");

        if (creature_ptr->lev > 9)
            self_ptr->info[self_ptr->line++] = _("あなたは杖/魔法棒/ロッドの魔力を強力に発動することができる。(10+(レベル-10)/2 HP)",
                "You can powerfully activate a staff, wand or rod (cost 10+(level-10)/2).");

        break;
    case CLASS_RED_MAGE:
        if (creature_ptr->lev > 47)
            self_ptr->info[self_ptr->line++] = _("あなたは1ターンに2回魔法を唱えることができる。(20 MP)", "You can cast two spells simultaneously (cost 20).");

        break;
    case CLASS_SAMURAI:
        self_ptr->info[self_ptr->line++]
            = _("あなたは精神を集中して気合いを溜めることができる。(消費なし)", "You can concentrate to regenerate your mana (cost 0).");

        if (creature_ptr->lev > 24)
            self_ptr->info[self_ptr->line++] = _("あなたは特殊な型で構えることができる。(消費なし)", "You can assume a special stance (cost 0).");

        break;
    case CLASS_BLUE_MAGE:
        self_ptr->info[self_ptr->line++]
            = _("あなたは相手に使われた魔法を学ぶことができる。(消費なし)", "You can study spells which your enemy casts on you (cost 0).");
        break;
    case CLASS_CAVALRY:
        if (creature_ptr->lev > 9)
            self_ptr->info[self_ptr->line++] = _("あなたはモンスターに乗って無理矢理ペットにすることができる。(消費なし)",
                "You can ride on a hostile monster to forcibly turn it into a pet  (cost 0).");

        break;
    case CLASS_BERSERKER:
        if (creature_ptr->lev > 9)
            self_ptr->info[self_ptr->line++]
                = _("あなたは街とダンジョンの間を行き来することができる。(10 HP)", "You can travel between town and the depths (cost 10).");

        break;
    case CLASS_SMITH:
        if (creature_ptr->lev > 4)
            self_ptr->info[self_ptr->line++] = _("あなたは武器や防具を鑑定することができる。(15 HP)", "You can identify weapons or armours (cost 15).");

        break;
    case CLASS_MIRROR_MASTER:
        self_ptr->info[self_ptr->line++] = _("あなたは鏡を作り出すことができる。(2 MP)", "You can create a Mirror (cost 2).");
        self_ptr->info[self_ptr->line++] = _("あなたは鏡を割ることができる。(消費なし)", "You can break distant Mirrors (cost 0).");
        break;
    case CLASS_NINJA:
        if (creature_ptr->lev > 19)
            self_ptr->info[self_ptr->line++] = _("あなたは素早く移動することができる。(消費なし)", "You can walk extremely fast (cost 0).");

        break;
    }
}
