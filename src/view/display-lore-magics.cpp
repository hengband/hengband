#include "view/display-lore-magics.h"
#include "lore/lore-calculator.h"
#include "lore/lore-util.h"
#include "monster-race/race-ability-flags.h"
#include "system/monrace/monrace-definition.h"
#include "term/term-color-types.h"
#include "util/string-processor.h"
#include <fmt/format.h>

void display_monster_breath(lore_type *lore_ptr)
{
    lore_ptr->breath = false;
    if (lore_ptr->lore_msgs.empty()) {
        return;
    }

    lore_ptr->breath = true;
    hooked_roff(fmt::format(_("{}は", "{}"), str_upcase_first(Who::who(lore_ptr->msex))));
    for (int n = 0; const auto &[msg, color] : lore_ptr->lore_msgs) {
#ifdef JP
        if (n != 0) {
            hooked_roff("や");
        }
#else
        if (n == 0) {
            hooked_roff(" may breathe ");
        } else if (n < std::ssize(lore_ptr->lore_msgs) - 1) {
            hooked_roff(", ");
        } else {
            hooked_roff(" or ");
        }
#endif
        hook_c_roff(color, msg);
        n++;
    }

#ifdef JP
    hooked_roff("のブレスを吐くことがある");
#endif
}

void display_monster_magic_types(lore_type *lore_ptr)
{
    lore_ptr->magic = false;
    if (lore_ptr->lore_msgs.empty()) {
        return;
    }

    lore_ptr->magic = true;
    if (lore_ptr->breath) {
        hooked_roff(_("、なおかつ", ", and is also"));
    } else {
        hooked_roff(fmt::format(_("{}は", "{} is"), str_upcase_first(Who::who(lore_ptr->msex))));
    }

#ifdef JP
    if (lore_ptr->behavior_flags.has(MonsterBehaviorType::SMART)) {
        hook_c_roff(TERM_YELLOW, "的確に");
    }

    hooked_roff("魔法を使うことができ、");
#else
    hooked_roff(" magical, casting spells");
    if (lore_ptr->behavior_flags.has(MonsterBehaviorType::SMART)) {
        hook_c_roff(TERM_YELLOW, " intelligently");
    }
#endif

    for (int n = 0; const auto &[msg, color] : lore_ptr->lore_msgs) {
#ifdef JP
        if (n != 0) {
            hooked_roff("、");
        }
#else
        if (n == 0) {
            hooked_roff(" which ");
        } else if (n < std::ssize(lore_ptr->lore_msgs) - 1) {
            hooked_roff(", ");
        } else {
            hooked_roff(" or ");
        }
#endif
        hook_c_roff(color, msg);
        n++;
    }

#ifdef JP
    hooked_roff("の呪文を唱えることがある");
#endif
}

void display_mosnter_magic_possibility(lore_type *lore_ptr)
{
    if (!lore_ptr->breath && !lore_ptr->magic && !lore_ptr->shoot && !lore_ptr->rocket) {
        return;
    }

    int m = lore_ptr->monrace->r_cast_spell;
    int n = lore_ptr->monrace->freq_spell;
    if (m > 100 || lore_ptr->know_everything) {
        hooked_roff(fmt::format(_("(確率:1/{})", "; 1 time in {}"), 100 / n));
    } else if (m) {
        n = ((n + 9) / 10) * 10;
        hooked_roff(fmt::format(_("(確率:約1/{})", "; about 1 time in {}"), 100 / n));
    }

    hooked_roff(_("。", ".  "));
}

namespace {
struct AbilityCellDef {
    MonsterAbilityType flag;
    const char *jp;
    const char *en;
    byte color;
};

static bool has_percent_s(std::string_view s)
{
    return s.find("%s") != std::string_view::npos;
}

static std::string build_magic_label(PlayerType *player_ptr, lore_type *lore_ptr, MonsterAbilityType type, const char *msg)
{
    if (!has_percent_s(msg)) {
        return msg;
    }

    if (!lore_ptr->is_details_known() && !lore_ptr->know_everything) {
        return format(msg, "");
    }

    return format(msg, get_skill_damage(player_ptr, lore_ptr, type).data());
}

enum class FlagState {
    HAVE,
    ABSENT,
    UNKNOWN
};

static FlagState get_ability_state(lore_type *lore_ptr, MonsterAbilityType flag)
{
    const bool fully_known = lore_ptr->know_everything || lore_ptr->is_details_known();
    if (fully_known) {
        return lore_ptr->monrace->ability_flags.has(flag) ? FlagState::HAVE : FlagState::ABSENT;
    }
    return lore_ptr->monrace->r_ability_flags.has(flag) ? FlagState::HAVE : FlagState::UNKNOWN;
}

static std::string mind_tag(const lore_type *lore_ptr)
{
    const bool fully_known = lore_ptr->know_everything || lore_ptr->is_details_known();
    const auto &flags = fully_known ? lore_ptr->monrace->behavior_flags : lore_ptr->monrace->r_behavior_flags;

    const bool has_smart = flags.has(MonsterBehaviorType::SMART);
    const bool has_stupid = flags.has(MonsterBehaviorType::STUPID);

    if (has_smart && has_stupid) {
        return _("[聡明][愚昧]", "[SMART][STUPID]");
    }
    if (has_smart) {
        return _("[聡明]", "[SMART]");
    }
    if (has_stupid) {
        return _("[愚昧]", "[STUPID]");
    }

    return fully_known ? "---" : "???";
}

static std::string rate_tag(const lore_type *lore_ptr)
{
    const int m = lore_ptr->monrace->r_cast_spell;
    int n = lore_ptr->monrace->freq_spell;
    if (n <= 0) {
        const bool fully_known = lore_ptr->know_everything || lore_ptr->is_details_known();
        return fully_known ? "---" : "???";
    }

    if (m > 100 || lore_ptr->know_everything) {
        return fmt::format("1/{}", 100 / n);
    }
    if (m) {
        n = ((n + 9) / 10) * 10;
        return fmt::format(_("約1/{}", "about 1/{}"), 100 / n);
    }
    return "???";
}

static void print_table(PlayerType *player_ptr, lore_type *lore_ptr, concptr header, const std::vector<AbilityCellDef> &defs)
{
    auto COL_NUM = 6;
    int col = 0;
    auto any = false;
    for (const auto &d : defs) {
        const auto st = get_ability_state(lore_ptr, d.flag);
        if (st == FlagState::HAVE) {
            auto label = build_magic_label(player_ptr, lore_ptr, d.flag, _(d.jp, d.en));
            lore_ptr->lore_msgs.emplace_back(label + " ", d.color);
            col++;
            any = true;
            if (col >= COL_NUM) {
                lore_ptr->lore_msgs.emplace_back("\n", d.color);
                col = 0;
            }
        }
    }
    if (any) {
        hooked_roff(header);
    }

    for (const auto &[msg, color] : lore_ptr->lore_msgs) {
        hook_c_roff(color, msg);
    }

    if (col != 0) {
        hooked_roff("\n");
    }
    lore_ptr->lore_msgs.clear();
}

static const std::vector<AbilityCellDef> &launch_defs()
{
    static const std::vector<AbilityCellDef> defs = {
        { MonsterAbilityType::ROCKET, "ロケット%s", "rocket%s", TERM_UMBER },
        { MonsterAbilityType::SHOOT, "射撃%s", "shoot%s", TERM_UMBER },
    };
    return defs;
}

static const std::vector<AbilityCellDef> &breath_defs()
{
    static const std::vector<AbilityCellDef> defs = {
        { MonsterAbilityType::BR_ACID, "酸%s", "acid%s", TERM_GREEN },
        { MonsterAbilityType::BR_ELEC, "稲妻%s", "lightning%s", TERM_BLUE },
        { MonsterAbilityType::BR_FIRE, "火炎%s", "fire%s", TERM_RED },
        { MonsterAbilityType::BR_COLD, "冷気%s", "frost%s", TERM_L_WHITE },
        { MonsterAbilityType::BR_POIS, "毒%s", "poison%s", TERM_L_GREEN },
        { MonsterAbilityType::BR_NETH, "地獄%s", "nether%s", TERM_L_DARK },
        { MonsterAbilityType::BR_LITE, "閃光%s", "light%s", TERM_YELLOW },
        { MonsterAbilityType::BR_DARK, "暗黒%s", "darkness%s", TERM_L_DARK },
        { MonsterAbilityType::BR_CONF, "混乱%s", "confusion%s", TERM_L_UMBER },
        { MonsterAbilityType::BR_SOUN, "轟音%s", "sound%s", TERM_ORANGE },
        { MonsterAbilityType::BR_CHAO, "カオス%s", "chaos%s", TERM_VIOLET },
        { MonsterAbilityType::BR_DISE, "劣化%s", "disenchantment%s", TERM_VIOLET },
        { MonsterAbilityType::BR_NEXU, "因果混乱%s", "nexus%s", TERM_VIOLET },
        { MonsterAbilityType::BR_TIME, "時間逆転%s", "time%s", TERM_L_BLUE },
        { MonsterAbilityType::BR_INER, "遅鈍%s", "inertia%s", TERM_SLATE },
        { MonsterAbilityType::BR_GRAV, "重力%s", "gravity%s", TERM_SLATE },
        { MonsterAbilityType::BR_SHAR, "破片%s", "shards%s", TERM_L_UMBER },
        { MonsterAbilityType::BR_PLAS, "プラズマ%s", "plasma%s", TERM_L_RED },
        { MonsterAbilityType::BR_FORC, "フォース%s", "force%s", TERM_UMBER },
        { MonsterAbilityType::BR_MANA, "魔力%s", "mana%s", TERM_L_BLUE },
        { MonsterAbilityType::BR_NUKE, "放射性廃棄物%s", "nuke%s", TERM_L_GREEN },
        { MonsterAbilityType::BR_DISI, "分解%s", "disintegration%s", TERM_SLATE },
        { MonsterAbilityType::BR_VOID, "虚無%s", "void%s", TERM_L_DARK },
        { MonsterAbilityType::BR_ABYSS, "深淵%s", "abyss%s", TERM_L_DARK },
    };
    return defs;
}

static const std::vector<AbilityCellDef> &attack_magic_defs()
{
    static const std::vector<AbilityCellDef> defs = {
        // set_ball_types()
        { MonsterAbilityType::BA_ACID, "酸球%s", "acid ball%s", TERM_GREEN },
        { MonsterAbilityType::BA_ELEC, "雷球%s", "lightning ball%s", TERM_BLUE },
        { MonsterAbilityType::BA_FIRE, "炎球%s", "fire ball%s", TERM_RED },
        { MonsterAbilityType::BA_COLD, "冷球%s", "frost ball%s", TERM_L_WHITE },
        { MonsterAbilityType::BA_POIS, "悪臭雲%s", "poison ball%s", TERM_L_GREEN },
        { MonsterAbilityType::BA_NETH, "地獄球%s", "nether ball%s", TERM_L_DARK },
        { MonsterAbilityType::BA_WATE, "水球%s", "water ball%s", TERM_BLUE },
        { MonsterAbilityType::BA_NUKE, "放射能球%s", "balls of radiation%s", TERM_L_GREEN },
        { MonsterAbilityType::BA_MANA, "魔力嵐%s", "mana storm%s", TERM_L_BLUE },
        { MonsterAbilityType::BA_DARK, "暗黒嵐%s", "darkness storm%s", TERM_L_DARK },
        { MonsterAbilityType::BA_LITE, "スターバースト%s", "star burst%s", TERM_YELLOW },
        { MonsterAbilityType::BA_CHAO, "純ログルス%s", "logrus%s", TERM_VIOLET },
        { MonsterAbilityType::BA_VOID, "虚無嵐%s", "void storm%s", TERM_L_DARK },
        { MonsterAbilityType::BA_ABYSS, "深淵嵐%s", "abyss storm%s", TERM_L_DARK },
        { MonsterAbilityType::BA_METEOR, "メテオスウォーム%s", "meteor swarm%s", TERM_UMBER },

        // set_particular_types()
        { MonsterAbilityType::HAND_DOOM, "破滅の手(40%-60%)", "the Hand of Doom(40%-60%)", TERM_VIOLET },
        { MonsterAbilityType::PSY_SPEAR, "光の剣%s", "psycho-spear%s", TERM_YELLOW },
        { MonsterAbilityType::DRAIN_MANA, "魔力吸収%s", "drain mana%s", TERM_SLATE },
        { MonsterAbilityType::MIND_BLAST, "精神攻撃%s", "mind blasting%s", TERM_L_RED },
        { MonsterAbilityType::BRAIN_SMASH, "脳攻撃%s", "brain smashing%s", TERM_RED },
        { MonsterAbilityType::CAUSE_1, "軽傷%s", "light wounds%s", TERM_L_WHITE },
        { MonsterAbilityType::CAUSE_2, "重傷%s", "serious wounds%s", TERM_L_WHITE },
        { MonsterAbilityType::CAUSE_3, "致命傷%s", "critical wounds%s", TERM_L_WHITE },
        { MonsterAbilityType::CAUSE_4, "秘孔%s", "mortal wounds%s", TERM_L_WHITE },

        // set_bolt_types()
        { MonsterAbilityType::BO_ACID, "酸矢%s", "acid bolt%s", TERM_GREEN },
        { MonsterAbilityType::BO_ELEC, "雷矢%s", "lightning bolt%s", TERM_BLUE },
        { MonsterAbilityType::BO_FIRE, "炎矢%s", "fire bolt%s", TERM_RED },
        { MonsterAbilityType::BO_COLD, "冷矢%s", "frost bolt%s", TERM_L_WHITE },
        { MonsterAbilityType::BO_NETH, "地獄矢%s", "nether bolt%s", TERM_L_DARK },
        { MonsterAbilityType::BO_WATE, "水矢%s", "water bolt%s", TERM_BLUE },
        { MonsterAbilityType::BO_MANA, "魔力矢%s", "mana bolt%s", TERM_L_BLUE },
        { MonsterAbilityType::BO_PLAS, "プラズマ矢%s", "plasma bolt%s", TERM_L_RED },
        { MonsterAbilityType::BO_ICEE, "極寒矢%s", "ice bolt%s", TERM_L_WHITE },
        { MonsterAbilityType::BO_VOID, "虚無矢%s", "void bolt%s", TERM_L_DARK },
        { MonsterAbilityType::BO_ABYSS, "深淵矢%s", "abyss bolt%s", TERM_L_DARK },
        { MonsterAbilityType::BO_METEOR, "メテオストライク%s", "meteor strike%s", TERM_UMBER },
        { MonsterAbilityType::BO_LITE, "スターライトアロー%s", "starlight arrow%s", TERM_YELLOW },
        { MonsterAbilityType::MISSILE, "マジックミサイル%s", "magic missile%s", TERM_L_WHITE },
    };
    return defs;
}

static const std::vector<AbilityCellDef> &summon_magic_defs()
{
    static const std::vector<AbilityCellDef> defs = {
        // set_summon_types()
        { MonsterAbilityType::S_MONSTER, "モンスター一体", "a monster", TERM_SLATE },
        { MonsterAbilityType::S_MONSTERS, "モンスター複数", "monsters", TERM_L_WHITE },
        { MonsterAbilityType::S_KIN, "救援", "aid", TERM_ORANGE },
        { MonsterAbilityType::S_ANT, "アリ", "ants", TERM_RED },
        { MonsterAbilityType::S_SPIDER, "クモ", "spiders", TERM_L_DARK },
        { MonsterAbilityType::S_HOUND, "ハウンド", "hounds", TERM_L_UMBER },
        { MonsterAbilityType::S_HYDRA, "ヒドラ", "hydras", TERM_L_GREEN },
        { MonsterAbilityType::S_ANGEL, "天使", "an angel", TERM_YELLOW },
        { MonsterAbilityType::S_DEMON, "デーモン", "a demon", TERM_L_RED },
        { MonsterAbilityType::S_UNDEAD, "アンデッド", "an undead", TERM_L_DARK },
        { MonsterAbilityType::S_DRAGON, "ドラゴン", "a dragon", TERM_ORANGE },
        { MonsterAbilityType::S_HI_UNDEAD, "強力なアンデッド", "Greater Undead", TERM_L_DARK },
        { MonsterAbilityType::S_HI_DRAGON, "古代ドラゴン", "Ancient Dragons", TERM_ORANGE },
        { MonsterAbilityType::S_CYBER, "サイバーデーモン", "Cyberdemons", TERM_UMBER },
        { MonsterAbilityType::S_AMBERITES, "アンバーの王族", "Lords of Amber", TERM_VIOLET },
        { MonsterAbilityType::S_UNIQUE, "ユニークモンスター", "Unique Monsters", TERM_VIOLET },
        { MonsterAbilityType::S_DEAD_UNIQUE, "ユニークモンスター口寄せ", "animate Unique Monsters", TERM_VIOLET },
    };
    return defs;
}

static const std::vector<AbilityCellDef> &support_magic_defs()
{
    static const std::vector<AbilityCellDef> defs = {
        // set_status_types()
        { MonsterAbilityType::SCARE, "恐怖", "terrify", TERM_SLATE },
        { MonsterAbilityType::BLIND, "目くらまし", "blind", TERM_L_DARK },
        { MonsterAbilityType::CONF, "混乱", "confuse", TERM_L_UMBER },
        { MonsterAbilityType::SLOW, "減速", "slow", TERM_UMBER },
        { MonsterAbilityType::HOLD, "麻痺", "paralyze", TERM_RED },
        { MonsterAbilityType::HASTE, "加速", "haste-self", TERM_L_GREEN },
        { MonsterAbilityType::HEAL, "治癒", "heal-self", TERM_WHITE },
        { MonsterAbilityType::INVULNER, "無敵化", "make invulnerable", TERM_WHITE },
        { MonsterAbilityType::DISPEL, "魔力消去", "dispel-magic", TERM_L_WHITE },

        // set_teleport_types()
        { MonsterAbilityType::BLINK, "ショートテレポート", "blink-self", TERM_UMBER },
        { MonsterAbilityType::TPORT, "テレポート", "teleport-self", TERM_ORANGE },
        { MonsterAbilityType::WORLD, "時を止める", "stop time", TERM_L_BLUE },
        { MonsterAbilityType::TELE_TO, "テレポートバック", "teleport to", TERM_L_UMBER },
        { MonsterAbilityType::TELE_AWAY, "テレポートアウェイ", "teleport away", TERM_UMBER },
        { MonsterAbilityType::TELE_LEVEL, "テレポート・レベル", "teleport level", TERM_ORANGE },

        // set_floor_types()
        { MonsterAbilityType::DARKNESS, "暗闇", "create darkness", TERM_L_DARK },
        { MonsterAbilityType::TRAPS, "トラップ", "create traps", TERM_BLUE },
        { MonsterAbilityType::FORGET, "記憶消去", "cause amnesia", TERM_BLUE },
        { MonsterAbilityType::RAISE_DEAD, "死者復活", "raise dead", TERM_RED },
    };
    return defs;
}
} // namespace

void display_monster_magic_rate(lore_type *lore_ptr)
{
    const bool fully_known = lore_ptr->know_everything || lore_ptr->is_details_known();
    if (!fully_known && lore_ptr->monrace->r_ability_flags.none()) {
        return;
    }

    hooked_roff(fmt::format(_("特技使用率:{} ", "AbilityRate:{} "), rate_tag(lore_ptr)));
    hooked_roff(mind_tag(lore_ptr));
    hooked_roff("\n");
}

void display_monster_magic_tables(PlayerType *player_ptr, lore_type *lore_ptr)
{
    const bool fully_known = lore_ptr->know_everything || lore_ptr->is_details_known();
    if (!fully_known && lore_ptr->monrace->r_ability_flags.none()) {
        return;
    }

    print_table(player_ptr, lore_ptr, _("[発射] ", "[Launching] "), launch_defs());
    print_table(player_ptr, lore_ptr, _("[ブレス] ", "[Breath] "), breath_defs());
    print_table(player_ptr, lore_ptr, _("[攻撃魔法] ", "[Attack magic] "), attack_magic_defs());
    print_table(player_ptr, lore_ptr, _("[召喚魔法] ", "[Summon magic] "), summon_magic_defs());
    print_table(player_ptr, lore_ptr, _("[その他魔法] ", "[Other magic] "), support_magic_defs());
}
