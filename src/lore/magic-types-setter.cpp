#include "lore/magic-types-setter.h"
#include "lore/lore-calculator.h"
#include "lore/lore-util.h"
#include "monster-race/race-brightness-mask.h"
#include "player-base/player-class.h"
#include "system/monrace/monrace-definition.h"
#include "system/player-type-definition.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"

void set_breath_types(PlayerType *player_ptr, lore_type *lore_ptr)
{
    lore_ptr->lore_msgs.clear();
    if (lore_ptr->ability_flags.has(MonsterAbilityType::BR_ACID)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BR_ACID, _("酸%s", "acid%s"), TERM_GREEN);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BR_ELEC)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BR_ELEC, _("稲妻%s", "lightning%s"), TERM_BLUE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BR_FIRE)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BR_FIRE, _("火炎%s", "fire%s"), TERM_RED);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BR_COLD)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BR_COLD, _("冷気%s", "frost%s"), TERM_L_WHITE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BR_POIS)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BR_POIS, _("毒%s", "poison%s"), TERM_L_GREEN);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BR_NETH)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BR_NETH, _("地獄%s", "nether%s"), TERM_L_DARK);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BR_LITE)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BR_LITE, _("閃光%s", "light%s"), TERM_YELLOW);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BR_DARK)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BR_DARK, _("暗黒%s", "darkness%s"), TERM_L_DARK);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BR_CONF)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BR_CONF, _("混乱%s", "confusion%s"), TERM_L_UMBER);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BR_SOUN)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BR_SOUN, _("轟音%s", "sound%s"), TERM_ORANGE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BR_CHAO)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BR_CHAO, _("カオス%s", "chaos%s"), TERM_VIOLET);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BR_DISE)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BR_DISE, _("劣化%s", "disenchantment%s"), TERM_VIOLET);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BR_NEXU)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BR_NEXU, _("因果混乱%s", "nexus%s"), TERM_VIOLET);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BR_TIME)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BR_TIME, _("時間逆転%s", "time%s"), TERM_L_BLUE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BR_INER)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BR_INER, _("遅鈍%s", "inertia%s"), TERM_SLATE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BR_GRAV)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BR_GRAV, _("重力%s", "gravity%s"), TERM_SLATE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BR_SHAR)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BR_SHAR, _("破片%s", "shards%s"), TERM_L_UMBER);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BR_PLAS)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BR_PLAS, _("プラズマ%s", "plasma%s"), TERM_L_RED);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BR_FORC)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BR_FORC, _("フォース%s", "force%s"), TERM_UMBER);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BR_MANA)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BR_MANA, _("魔力%s", "mana%s"), TERM_L_BLUE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BR_NUKE)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BR_NUKE, _("放射性廃棄物%s", "toxic waste%s"), TERM_L_GREEN);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BR_DISI)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BR_DISI, _("分解%s", "disintegration%s"), TERM_SLATE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BR_VOID)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BR_VOID, _("虚無%s", "void%s"), TERM_L_DARK);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BR_ABYSS)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BR_ABYSS, _("深淵%s", "abyss%s"), TERM_L_DARK);
    }
}

void set_ball_types(PlayerType *player_ptr, lore_type *lore_ptr)
{
    if (lore_ptr->ability_flags.has(MonsterAbilityType::BA_ACID)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BA_ACID, _("アシッド・ボール%s", "produce acid balls%s"), TERM_GREEN);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BA_ELEC)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BA_ELEC, _("サンダー・ボール%s", "produce lightning balls%s"), TERM_BLUE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BA_FIRE)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BA_FIRE, _("ファイア・ボール%s", "produce fire balls%s"), TERM_RED);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BA_COLD)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BA_COLD, _("アイス・ボール%s", "produce frost balls%s"), TERM_L_WHITE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BA_POIS)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BA_POIS, _("悪臭雲%s", "produce poison balls%s"), TERM_L_GREEN);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BA_NETH)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BA_NETH, _("地獄球%s", "produce nether balls%s"), TERM_L_DARK);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BA_WATE)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BA_WATE, _("ウォーター・ボール%s", "produce water balls%s"), TERM_BLUE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BA_NUKE)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BA_NUKE, _("放射能球%s", "produce balls of radiation%s"), TERM_L_GREEN);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BA_MANA)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BA_MANA, _("魔力の嵐%s", "invoke mana storms%s"), TERM_L_BLUE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BA_DARK)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BA_DARK, _("暗黒の嵐%s", "invoke darkness storms%s"), TERM_L_DARK);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BA_LITE)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BA_LITE, _("スターバースト%s", "invoke starburst%s"), TERM_YELLOW);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BA_CHAO)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BA_CHAO, _("純ログルス%s", "invoke raw Logrus%s"), TERM_VIOLET);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BA_VOID)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BA_VOID, _("虚無の嵐%s", "invoke void storms%s"), TERM_L_DARK);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BA_ABYSS)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BA_ABYSS, _("深淵の嵐%s", "invoke abyss storms%s"), TERM_L_DARK);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BA_METEOR)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BA_METEOR, _("メテオスウォーム%s", "invoke meteor swarm%s"), TERM_UMBER);
    }
}

void set_particular_types(PlayerType *player_ptr, lore_type *lore_ptr)
{
    if (lore_ptr->ability_flags.has(MonsterAbilityType::HAND_DOOM)) {
        lore_ptr->lore_msgs.emplace_back(_("破滅の手(40%-60%)", "invoke the Hand of Doom(40%-60%)"), TERM_VIOLET);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::PSY_SPEAR)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::PSY_SPEAR, _("光の剣%s", "psycho-spear%s"), TERM_YELLOW);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::DRAIN_MANA)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::DRAIN_MANA, _("魔力吸収%s", "drain mana%s"), TERM_SLATE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::MIND_BLAST)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::MIND_BLAST, _("精神攻撃%s", "cause mind blasting%s"), TERM_L_RED);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BRAIN_SMASH)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BRAIN_SMASH, _("脳攻撃%s", "cause brain smashing%s"), TERM_RED);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::CAUSE_1)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::CAUSE_1, _("軽傷＋呪い%s", "cause light wounds and cursing%s"), TERM_L_WHITE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::CAUSE_2)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::CAUSE_2, _("重傷＋呪い%s", "cause serious wounds and cursing%s"), TERM_L_WHITE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::CAUSE_3)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::CAUSE_3, _("致命傷＋呪い%s", "cause critical wounds and cursing%s"), TERM_L_WHITE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::CAUSE_4)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::CAUSE_4, _("秘孔を突く%s", "cause mortal wounds%s"), TERM_L_WHITE);
    }
}

void set_bolt_types(PlayerType *player_ptr, lore_type *lore_ptr)
{
    if (lore_ptr->ability_flags.has(MonsterAbilityType::BO_ACID)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BO_ACID, _("アシッド・ボルト%s", "produce acid bolts%s"), TERM_GREEN);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BO_ELEC)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BO_ELEC, _("サンダー・ボルト%s", "produce lightning bolts%s"), TERM_BLUE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BO_FIRE)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BO_FIRE, _("ファイア・ボルト%s", "produce fire bolts%s"), TERM_RED);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BO_COLD)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BO_COLD, _("アイス・ボルト%s", "produce frost bolts%s"), TERM_L_WHITE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BO_NETH)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BO_NETH, _("地獄の矢%s", "produce nether bolts%s"), TERM_L_DARK);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BO_WATE)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BO_WATE, _("ウォーター・ボルト%s", "produce water bolts%s"), TERM_BLUE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BO_MANA)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BO_MANA, _("魔力の矢%s", "produce mana bolts%s"), TERM_L_BLUE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BO_PLAS)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BO_PLAS, _("プラズマ・ボルト%s", "produce plasma bolts%s"), TERM_L_RED);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BO_ICEE)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BO_ICEE, _("極寒の矢%s", "produce ice bolts%s"), TERM_WHITE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BO_VOID)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BO_VOID, _("ヴォイド・ボルト%s", "produce void bolts%s"), TERM_L_DARK);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BO_ABYSS)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BO_ABYSS, _("アビス・ボルト%s", "produce abyss bolts%s"), TERM_L_DARK);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BO_METEOR)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BO_METEOR, _("メテオストライク%s", "produce meteor strikes%s"), TERM_UMBER);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BO_LITE)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::BO_LITE, _("スターライトアロー%s", "produce starlight arrow%s"), TERM_YELLOW);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::MISSILE)) {
        add_lore_of_damage_skill(player_ptr, lore_ptr, MonsterAbilityType::MISSILE, _("マジックミサイル%s", "produce magic missiles%s"), TERM_SLATE);
    }
}

void set_status_types(lore_type *lore_ptr)
{
    if (lore_ptr->ability_flags.has(MonsterAbilityType::SCARE)) {
        lore_ptr->lore_msgs.emplace_back(_("恐怖", "terrify"), TERM_SLATE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::BLIND)) {
        lore_ptr->lore_msgs.emplace_back(_("目くらまし", "blind"), TERM_L_DARK);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::CONF)) {
        lore_ptr->lore_msgs.emplace_back(_("混乱", "confuse"), TERM_L_UMBER);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::SLOW)) {
        lore_ptr->lore_msgs.emplace_back(_("減速", "slow"), TERM_UMBER);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::HOLD)) {
        lore_ptr->lore_msgs.emplace_back(_("麻痺", "paralyze"), TERM_RED);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::HASTE)) {
        lore_ptr->lore_msgs.emplace_back(_("加速", "haste-self"), TERM_L_GREEN);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::HEAL)) {
        lore_ptr->lore_msgs.emplace_back(_("治癒", "heal-self"), TERM_WHITE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::INVULNER)) {
        lore_ptr->lore_msgs.emplace_back(_("無敵化", "make invulnerable"), TERM_WHITE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::DISPEL)) {
        lore_ptr->lore_msgs.emplace_back(_("魔力消去", "dispel-magic"), TERM_L_WHITE);
    }
}

/*!
 * @details 間にザ・ワールドが入っているが、元々こうなので敢えて修正はしない
 */
void set_teleport_types(lore_type *lore_ptr)
{
    if (lore_ptr->ability_flags.has(MonsterAbilityType::BLINK)) {
        lore_ptr->lore_msgs.emplace_back(_("ショートテレポート", "blink-self"), TERM_UMBER);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::TPORT)) {
        lore_ptr->lore_msgs.emplace_back(_("テレポート", "teleport-self"), TERM_ORANGE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::WORLD)) {
        lore_ptr->lore_msgs.emplace_back(_("時を止める", "stop time"), TERM_L_BLUE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::TELE_TO)) {
        lore_ptr->lore_msgs.emplace_back(_("テレポートバック", "teleport to"), TERM_L_UMBER);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::TELE_AWAY)) {
        lore_ptr->lore_msgs.emplace_back(_("テレポートアウェイ", "teleport away"), TERM_UMBER);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::TELE_LEVEL)) {
        lore_ptr->lore_msgs.emplace_back(_("テレポート・レベル", "teleport level"), TERM_ORANGE);
    }
}

void set_floor_types(PlayerType *player_ptr, lore_type *lore_ptr)
{
    if (lore_ptr->ability_flags.has(MonsterAbilityType::DARKNESS)) {
        if (!PlayerClass(player_ptr).equals(PlayerClassType::NINJA) || lore_ptr->r_ptr->kind_flags.has_not(MonsterKindType::UNDEAD) || lore_ptr->r_ptr->resistance_flags.has(MonsterResistanceType::HURT_LITE) || (lore_ptr->r_ptr->brightness_flags.has_any_of(dark_mask))) {
            lore_ptr->lore_msgs.emplace_back(_("暗闇", "create darkness"), TERM_L_DARK);
        } else {
            lore_ptr->lore_msgs.emplace_back(_("閃光", "create light"), TERM_YELLOW);
        }
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::TRAPS)) {
        lore_ptr->lore_msgs.emplace_back(_("トラップ", "create traps"), TERM_BLUE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::FORGET)) {
        lore_ptr->lore_msgs.emplace_back(_("記憶消去", "cause amnesia"), TERM_BLUE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::RAISE_DEAD)) {
        lore_ptr->lore_msgs.emplace_back(_("死者復活", "raise dead"), TERM_RED);
    }
}

void set_summon_types(lore_type *lore_ptr)
{
    if (lore_ptr->ability_flags.has(MonsterAbilityType::S_MONSTER)) {
        lore_ptr->lore_msgs.emplace_back(_("モンスター一体召喚", "summon a monster"), TERM_SLATE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::S_MONSTERS)) {
        lore_ptr->lore_msgs.emplace_back(_("モンスター複数召喚", "summon monsters"), TERM_L_WHITE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::S_KIN)) {
        lore_ptr->lore_msgs.emplace_back(_("救援召喚", "summon aid"), TERM_ORANGE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::S_ANT)) {
        lore_ptr->lore_msgs.emplace_back(_("アリ召喚", "summon ants"), TERM_RED);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::S_SPIDER)) {
        lore_ptr->lore_msgs.emplace_back(_("クモ召喚", "summon spiders"), TERM_L_DARK);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::S_HOUND)) {
        lore_ptr->lore_msgs.emplace_back(_("ハウンド召喚", "summon hounds"), TERM_L_UMBER);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::S_HYDRA)) {
        lore_ptr->lore_msgs.emplace_back(_("ヒドラ召喚", "summon hydras"), TERM_L_GREEN);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::S_ANGEL)) {
        lore_ptr->lore_msgs.emplace_back(_("天使一体召喚", "summon an angel"), TERM_YELLOW);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::S_DEMON)) {
        lore_ptr->lore_msgs.emplace_back(_("デーモン一体召喚", "summon a demon"), TERM_L_RED);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::S_UNDEAD)) {
        lore_ptr->lore_msgs.emplace_back(_("アンデッド一体召喚", "summon an undead"), TERM_L_DARK);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::S_DRAGON)) {
        lore_ptr->lore_msgs.emplace_back(_("ドラゴン一体召喚", "summon a dragon"), TERM_ORANGE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::S_HI_UNDEAD)) {
        lore_ptr->lore_msgs.emplace_back(_("強力なアンデッド召喚", "summon Greater Undead"), TERM_L_DARK);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::S_HI_DRAGON)) {
        lore_ptr->lore_msgs.emplace_back(_("古代ドラゴン召喚", "summon Ancient Dragons"), TERM_ORANGE);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::S_CYBER)) {
        lore_ptr->lore_msgs.emplace_back(_("サイバーデーモン召喚", "summon Cyberdemons"), TERM_UMBER);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::S_AMBERITES)) {
        lore_ptr->lore_msgs.emplace_back(_("アンバーの王族召喚", "summon Lords of Amber"), TERM_VIOLET);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::S_UNIQUE)) {
        lore_ptr->lore_msgs.emplace_back(_("ユニーク・モンスター召喚", "summon Unique Monsters"), TERM_VIOLET);
    }

    if (lore_ptr->ability_flags.has(MonsterAbilityType::S_DEAD_UNIQUE)) {
        lore_ptr->lore_msgs.emplace_back(_("ユニーク・モンスター口寄せ", "animate Unique Monsters"), TERM_VIOLET);
    }
}
