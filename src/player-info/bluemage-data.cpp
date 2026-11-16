#include "player-info/bluemage-data.h"
#include "blue-magic/blue-magic-checker.h"
#include "locale/language-switcher.h"
#include "locale/multibyte-char.h"
#include <iterator>
#include <map>
#include <string_view>

namespace {

/*!
 * @brief モンスター魔法名テーブル
 */
const std::map<MonsterAbilityType, std::string_view> monster_powers_short = {
    { MonsterAbilityType::SHRIEK, _("叫ぶ", "Shriek") },
    { MonsterAbilityType::XXX1, _("何か", "Something") },
    { MonsterAbilityType::DISPEL, _("魔力消去", "Dispel-magic") },
    { MonsterAbilityType::ROCKET, _("ロケット", "Rocket") },
    { MonsterAbilityType::SHOOT, _("射撃", "Arrow") },
    { MonsterAbilityType::XXX2, _("何か", "Arrows") },
    { MonsterAbilityType::XXX3, _("何か", "Missile") },
    { MonsterAbilityType::XXX4, _("何か", "Missiles") },
    { MonsterAbilityType::BR_ACID, _("酸", "Acid") },
    { MonsterAbilityType::BR_ELEC, _("電撃", "Lightning") },
    { MonsterAbilityType::BR_FIRE, _("火炎", "Fire") },
    { MonsterAbilityType::BR_COLD, _("冷気", "Cold") },
    { MonsterAbilityType::BR_POIS, _("毒", "Poison") },
    { MonsterAbilityType::BR_NETH, _("地獄", "Nether") },
    { MonsterAbilityType::BR_LITE, _("閃光", "Light") },
    { MonsterAbilityType::BR_DARK, _("暗黒", "Dark") },
    { MonsterAbilityType::BR_CONF, _("混乱", "Confusion") },
    { MonsterAbilityType::BR_SOUN, _("轟音", "Sound") },
    { MonsterAbilityType::BR_CHAO, _("カオス", "Chaos") },
    { MonsterAbilityType::BR_DISE, _("劣化", "Disenchantment") },
    { MonsterAbilityType::BR_NEXU, _("因果混乱", "Nexus") },
    { MonsterAbilityType::BR_TIME, _("時間逆転", "Time") },
    { MonsterAbilityType::BR_INER, _("遅鈍", "Inertia") },
    { MonsterAbilityType::BR_GRAV, _("重力", "Gravity") },
    { MonsterAbilityType::BR_SHAR, _("破片", "Shards") },
    { MonsterAbilityType::BR_PLAS, _("プラズマ", "Plasma") },
    { MonsterAbilityType::BR_FORC, _("フォース", "Force") },
    { MonsterAbilityType::BR_MANA, _("魔力", "Mana") },
    { MonsterAbilityType::BA_NUKE, _("放射能球", "Nuke") },
    { MonsterAbilityType::BR_NUKE, _("放射性廃棄物", "Nuke") },
    { MonsterAbilityType::BA_CHAO, _("純ログルス", "Logrus") },
    { MonsterAbilityType::BR_DISI, _("分解", "Disintegration") },
    { MonsterAbilityType::BR_VOID, _("虚無", "Void") },
    { MonsterAbilityType::BR_ABYSS, _("深淵", "Abyss") },

    { MonsterAbilityType::BA_ACID, _("酸", "Acid") },
    { MonsterAbilityType::BA_ELEC, _("電撃", "Lightning") },
    { MonsterAbilityType::BA_FIRE, _("火炎", "Fire") },
    { MonsterAbilityType::BA_COLD, _("冷気", "Frost") },
    { MonsterAbilityType::BA_POIS, _("悪臭雲", "Stinking Cloud") },
    { MonsterAbilityType::BA_NETH, _("地獄球", "Nether") },
    { MonsterAbilityType::BA_WATE, _("ウォーター", "Water") },
    { MonsterAbilityType::BA_MANA, _("魔力の嵐", "Mana storm") },
    { MonsterAbilityType::BA_DARK, _("暗黒の嵐", "Darkness storm") },
    { MonsterAbilityType::BA_METEOR, _("メテオ・スウォーム", "Meteor Swarm") },
    { MonsterAbilityType::BA_VOID, _("虚無", "Void") },
    { MonsterAbilityType::BA_ABYSS, _("深淵", "Abyss") },
    { MonsterAbilityType::DRAIN_MANA, _("魔力吸収", "Drain mana") },
    { MonsterAbilityType::MIND_BLAST, _("精神攻撃", "Mind blast") },
    { MonsterAbilityType::BRAIN_SMASH, _("脳攻撃", "Brain smash") },
    { MonsterAbilityType::CAUSE_1, _("軽傷", "Cause Light Wound") },
    { MonsterAbilityType::CAUSE_2, _("重傷", "Cause Serious Wound") },
    { MonsterAbilityType::CAUSE_3, _("致命傷", "Cause Critical Wound") },
    { MonsterAbilityType::CAUSE_4, _("秘孔を突く", "Cause Mortal Wound") },
    { MonsterAbilityType::BO_ACID, _("酸", "Acid") },
    { MonsterAbilityType::BO_ELEC, _("電撃", "Lightning") },
    { MonsterAbilityType::BO_FIRE, _("火炎", "Fire") },
    { MonsterAbilityType::BO_COLD, _("冷気", "Frost") },
    { MonsterAbilityType::BA_LITE, _("スターバースト", "Starburst") },
    { MonsterAbilityType::BO_NETH, _("地獄の矢", "Nether") },
    { MonsterAbilityType::BO_WATE, _("ウォーター", "Water") },
    { MonsterAbilityType::BO_MANA, _("魔力の矢", "Mana") },
    { MonsterAbilityType::BO_PLAS, _("プラズマ", "Plasma") },
    { MonsterAbilityType::BO_ICEE, _("極寒", "Ice") },
    { MonsterAbilityType::BO_VOID, _("ヴォイド", "Void") },
    { MonsterAbilityType::BO_ABYSS, _("アビス", "Abyss") },
    { MonsterAbilityType::BO_METEOR, { _("メテオストライク", "meteor strike") } },
    { MonsterAbilityType::BO_LITE, { _("スターライトアロー", "starlight arrow") } },
    { MonsterAbilityType::MISSILE, _("マジックミサイル", "Magic missile") },
    { MonsterAbilityType::SCARE, _("恐慌", "Scare") },
    { MonsterAbilityType::BLIND, _("盲目", "Blind") },
    { MonsterAbilityType::CONF, _("混乱", "Confuse") },
    { MonsterAbilityType::SLOW, _("減速", "Slow") },
    { MonsterAbilityType::HOLD, _("睡眠", "Sleep") },

    { MonsterAbilityType::HASTE, _("加速", "Speed") },
    { MonsterAbilityType::HAND_DOOM, _("破滅の手", "Hand of doom") },
    { MonsterAbilityType::HEAL, _("体力回復", "Heal-self") },
    { MonsterAbilityType::INVULNER, _("無傷の球", "Invulnerable") },
    { MonsterAbilityType::BLINK, _("ショートテレポート", "Blink") },
    { MonsterAbilityType::TPORT, _("テレポート", "Teleport") },
    { MonsterAbilityType::WORLD, _("時を止める", "The world") },
    { MonsterAbilityType::SPECIAL, _("何か", "Something") },
    { MonsterAbilityType::TELE_TO, _("引きよせる", "Teleport to") },
    { MonsterAbilityType::TELE_AWAY, _("テレポートアウェイ", "Teleport away") },
    { MonsterAbilityType::TELE_LEVEL, _("テレポートレベル", "Teleport level") },
    { MonsterAbilityType::PSY_SPEAR, _("光の剣", "Psycho-spear") },
    { MonsterAbilityType::DARKNESS, _("暗闇", "Create darkness") },
    { MonsterAbilityType::TRAPS, _("トラップ創造", "Create traps") },
    { MonsterAbilityType::FORGET, _("記憶喪失", "Amnesia") },
    { MonsterAbilityType::RAISE_DEAD, _("死者復活", "Raise dead") },
    { MonsterAbilityType::S_KIN, _("援軍", "Aid") },
    { MonsterAbilityType::S_CYBER, _("サイバーデーモン", "Cyberdeamons") },
    { MonsterAbilityType::S_MONSTER, _("モンスター", "A monster") },
    { MonsterAbilityType::S_MONSTERS, _("複数のモンスター", "Monsters") },
    { MonsterAbilityType::S_ANT, _("蟻", "Ants") },
    { MonsterAbilityType::S_SPIDER, _("蜘蛛", "Spiders") },
    { MonsterAbilityType::S_HOUND, _("ハウンド", "Hounds") },
    { MonsterAbilityType::S_HYDRA, _("ヒドラ", "Hydras") },
    { MonsterAbilityType::S_ANGEL, _("天使", "Angel") },
    { MonsterAbilityType::S_DEMON, _("悪魔", "Daemon") },
    { MonsterAbilityType::S_UNDEAD, _("アンデッド", "Undead") },
    { MonsterAbilityType::S_DRAGON, _("ドラゴン", "Dragon") },
    { MonsterAbilityType::S_HI_UNDEAD, _("上級アンデッド", "Greater Undead") },
    { MonsterAbilityType::S_HI_DRAGON, _("古代ドラゴン", "Ancient Dragon") },
    { MonsterAbilityType::S_AMBERITES, _("アンバーの王族", "Lords of Amber") },
    { MonsterAbilityType::S_UNIQUE, _("ユニーク", "Unique monsters") },
    { MonsterAbilityType::S_DEAD_UNIQUE, _("黄泉のユニーク", "Dead unique monsters") },
};

struct learnt_spell_table {
    EnumClassFlagGroup<MonsterAbilityType> ability_flags;
    std::string add_monster_spell_type(BlueMagicType blue_magic_type);
};

/*!
 * @brief ダンプする情報に学習済魔法の種類を追加する
 * @param col 行数
 * @param blue_magic_type 魔法の種類
 * @param learnt_spell_ptr 学習済魔法のテーブル
 * @return ダンプ用情報
 */
std::string learnt_spell_table::add_monster_spell_type(BlueMagicType blue_magic_type)
{
    this->ability_flags.clear();
    set_rf_masks(this->ability_flags, blue_magic_type);
    switch (blue_magic_type) {
    case BlueMagicType::BOLT:
        return _("\n     [ボルト型]\n", "\n     [Bolt  Type]\n");
    case BlueMagicType::BALL:
        return _("\n     [ボール型]\n", "\n     [Ball  Type]\n");
    case BlueMagicType::BREATH:
        return _("\n     [ブレス型]\n", "\n     [  Breath  ]\n");
    case BlueMagicType::SUMMON:
        return _("\n     [召喚魔法]\n", "\n     [Summonning]\n");
    case BlueMagicType::OTHER:
        return _("\n     [ その他 ]\n", "\n     [Other Type]\n");
    default:
        return "";
    }
}
}

/*!
 * @brief 青魔道士の学習済魔法情報を集めて返す
 * @return ダンプ用情報の行リスト
 */
std::vector<std::string> BluemageData::build_learnt_magics_info() const
{
    std::vector<std::string> lines;
    lines.emplace_back(_("\n\n  [学習済みの青魔法]\n", "\n\n  [Learned Blue Magic]\n"));
    for (auto blue_magic_type : BLUE_MAGIC_TYPE_LIST) {
        learnt_spell_table learnt_magic;
        lines.push_back(learnt_magic.add_monster_spell_type(blue_magic_type));
        learnt_magic.ability_flags &= this->learnt_blue_magics;

        std::vector<MonsterAbilityType> learnt_spells;
        EnumClassFlagGroup<MonsterAbilityType>::get_flags(learnt_magic.ability_flags, std::back_inserter(learnt_spells));
        std::string current_line = "       ";
        for (size_t i = 0; i < learnt_spells.size(); ++i) {
            const auto &spell_name = monster_powers_short.at(learnt_spells[i]);
            if (str_display_width(current_line) + str_display_width(spell_name) >= 75) {
                lines.push_back(current_line.append("\n"));
                current_line = "       ";
            }

            current_line.append(spell_name);
            if (i < learnt_spells.size() - 1) {
                current_line.append(", ");
            }
        }

        const auto spell_name = learnt_spells.empty() ? _("なし", "None") : "";
        lines.push_back(current_line.append(spell_name).append("\n"));
    }

    return lines;
}
