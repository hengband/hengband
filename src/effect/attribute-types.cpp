#include "attribute-types.h"

std::string getAttributeName(AttributeType attribute) {
    switch (attribute) {
    case AttributeType::ABYSS:
        return _("深淵", "abyss");
    case AttributeType::ACID:
        return _("酸", "acid");
    case AttributeType::ANIM_DEAD:
        return _("死者復活", "animate dead");
    case AttributeType::ATTACK:
        return _("白兵", "attack");
    case AttributeType::AWAY_ALL:
        return _("テレポート・アウェイ", "teleport away");
    case AttributeType::AWAY_EVIL:
        return _("邪悪飛ばし", "away evil");
    case AttributeType::AWAY_UNDEAD:
        return _("アンデッド・アウェイ", "away undead");
    case AttributeType::BLOOD_CURSE:
        return _("血の呪い", "blood curse");
    case AttributeType::BRAIN_SMASH:
        return _("脳攻撃", "brain smash");
    case AttributeType::CAPTURE:
        return _("捕縛", "capture");
    case AttributeType::CAUSE_1:
        return _("軽傷の呪い", "cause light wounds");
    case AttributeType::CAUSE_2:
        return _("重傷の呪い", "cause serious wounds");
    case AttributeType::CAUSE_3:
        return _("致命傷の呪い", "cause mortal wounds");
    case AttributeType::CAUSE_4:
        return _("秘孔を突く", "cause critical wounds");
    case AttributeType::CHAOS:
        return _("混沌", "chaos");
    case AttributeType::CHARM:
        return _("モンスター魅了", "charm monster");
    case AttributeType::CHARM_LIVING:
        return _("生命魅了", "charm living");
    case AttributeType::COLD:
        return _("冷気", "cold");
    case AttributeType::CONFUSION:
        return _("混乱", "confusion");
    case AttributeType::CONTROL_ANIMAL:
        return _("動物支配", "control animal");
    case AttributeType::CONTROL_DEMON:
        return _("悪魔支配", "control demon");
    case AttributeType::CONTROL_UNDEAD:
        return _("アンデッド支配", "control undead");
    case AttributeType::CRUSADE:
        return _("聖戦", "crusade");
    case AttributeType::DARK:
        return _("暗黒", "dark");
    case AttributeType::DARK_WEAK:
        return _("弱い暗黒", "dark weak");
    case AttributeType::DEATH_RAY:
        return _("死の光線", "death ray");
    case AttributeType::DEBUG:
        return _("デバッグ", "debug");
    case AttributeType::DISENCHANT:
        return _("劣化", "disenchant");
    case AttributeType::DISINTEGRATE:
        return _("分解", "disintegrate");
    case AttributeType::DISP_ALL:
        return _("モンスター退散", "dispel all");
    case AttributeType::DISP_DEMON:
        return _("悪魔退散", "dispel demon");
    case AttributeType::DISP_EVIL:
        return _("邪悪退散", "dispel evil");
    case AttributeType::DISP_GOOD:
        return _("善良退散", "dispel good");
    case AttributeType::DISP_LIVING:
        return _("生命退散", "dispel living");
    case AttributeType::DISP_UNDEAD:
        return _("アンデッド退散", "dispel undead");
    case AttributeType::DOMINATION:
        return _("精神支配", "domination");
    case AttributeType::DRAIN_MANA:
        return _("魔力吸収", "drain mana");
    case AttributeType::ELEC:
        return _("電撃", "lightning");
    case AttributeType::ENGETSU:
        return _("円月", "engetsu");
    case AttributeType::E_GENOCIDE:
        return _("元素抹殺", "elemental genocide");
    case AttributeType::FIRE:
        return _("火炎", "fire");
    case AttributeType::FORCE:
        return _("フォース", "force");
    case AttributeType::GENOCIDE:
        return _("抹殺", "genocide");
    case AttributeType::GRAVITY:
        return _("重力", "gravity");
    case AttributeType::HAND_DOOM:
        return _("破滅の手", "hand of doom");
    case AttributeType::HELL_FIRE:
        return _("地獄の劫火", "hellfire");
    case AttributeType::HOLY_FIRE:
        return _("聖光", "holy fire");
    case AttributeType::HUNGRY:
        return _("空腹", "hungry");
    case AttributeType::HYPODYNAMIA:
        return _("衰弱", "hypodynamia");
    case AttributeType::ICE:
        return _("極寒", "ice");
    case AttributeType::IDENTIFY:
        return _("鑑定", "identify");
    case AttributeType::INERTIAL:
        return _("遅鈍", "inertia");
    case AttributeType::JAM_DOOR:
        return _("施錠", "jam door");
    case AttributeType::KILL_DOOR:
        return _("ドア破壊", "kill door");
    case AttributeType::KILL_TRAP:
        return _("トラップ破壊", "kill trap");
    case AttributeType::KILL_WALL:
        return _("岩石溶解", "kill wall");
    case AttributeType::LAVA_FLOW:
        return _("溶岩流", "lava flow");
    case AttributeType::LITE:
        return _("閃光", "light");
    case AttributeType::LITE_WEAK:
        return _("弱い閃光", "light weak");
    case AttributeType::MAKE_DOOR:
        return _("ドア生成", "make door");
    case AttributeType::MAKE_RUNE_PROTECTION:
        return _("守りのルーン生成", "make rune protection");
    case AttributeType::MAKE_TRAP:
        return _("トラップ生成", "make trap");
    case AttributeType::MAKE_TREE:
        return _("森林生成", "make tree");
    case AttributeType::MAKE_WALL:
        return _("壁生成", "make wall");
    case AttributeType::MANA:
        return _("純粋魔力", "mana");
    case AttributeType::METEOR:
        return _("隕石", "meteor");
    case AttributeType::MIND_BLAST:
        return _("精神攻撃", "mind blast");
    case AttributeType::MISSILE:
        return _("弱魔力", "missile");
    case AttributeType::MONSTER_MELEE:
        return _("モンスターの近接攻撃", "monster melee");
    case AttributeType::MONSTER_SHOOT:
        return _("モンスターの射撃", "monster shoot");
    case AttributeType::NETHER:
        return _("地獄", "nether");
    case AttributeType::NEXUS:
        return _("因果混乱", "nexus");
    case AttributeType::NONE:
        return _("無し", "none");
    case AttributeType::NUKE:
        return _("放射性廃棄物", "nuke");
    case AttributeType::OLD_CLONE:
        return _("クローン・モンスター", "clone monster");
    case AttributeType::OLD_CONF:
        return _("パニック・モンスター", "panic monster");
    case AttributeType::OLD_HEAL:
        return _("回復モンスター", "heal monster");
    case AttributeType::OLD_POLY:
        return _("チェンジ・モンスター", "change monster");
    case AttributeType::OLD_SLEEP:
        return _("スリープ・モンスター", "sleep monster");
    case AttributeType::OLD_SLOW:
        return _("スロウ・モンスター", "slow monster");
    case AttributeType::OLD_SPEED:
        return _("スピード・モンスター", "speed monster");
    case AttributeType::PHOTO:
        return _("撮影", "photo");
    case AttributeType::PLASMA:
        return _("プラズマ", "plasma");
    case AttributeType::PLAYER_MELEE:
        return _("プレイヤーの近接攻撃", "player melee");
    case AttributeType::PLAYER_SHOOT:
        return _("プレイヤーの射撃", "player shoot");
    case AttributeType::POIS:
        return _("毒", " poison");
    case AttributeType::PSI:
        return _("精神エネルギー", "mental energy");
    case AttributeType::PSI_DRAIN:
        return _("精神吸収", "psi drain");
    case AttributeType::PSY_SPEAR:
        return _("光の剣", "psycho-spear");
    case AttributeType::ROCKET:
        return _("ロケット", "rocket");
    case AttributeType::SEEKER:
        return _("シーカーレイ", "seeker lay");
    case AttributeType::SHARDS:
        return _("破片", "shards");
    case AttributeType::SOUND:
        return _("轟音", "sound");
    case AttributeType::STAR_HEAL:
        return _("星の癒し", "star heal");
    case AttributeType::STASIS:
        return _("モンスター拘束", "stasis");
    case AttributeType::STASIS_EVIL:
        return _("邪悪拘束", "stasis evil");
    case AttributeType::STONE_WALL:
        return _("壁生成", "make stone wall");
    case AttributeType::STUN:
        return _("朦朧", "stun");
    case AttributeType::SUPER_RAY:
        return _("スーパーレイ", "super ray");
    case AttributeType::TELEKINESIS:
        return _("テレキネシス", "telekinesis");
    case AttributeType::TIME:
        return _("時間逆転", "time");
    case AttributeType::TURN_ALL:
        return _("モンスター恐慌", "turn all");
    case AttributeType::TURN_EVIL:
        return _("邪悪恐慌", "turn evil");
    case AttributeType::TURN_UNDEAD:
        return _("アンデッド恐慌", "turn undead");
    case AttributeType::VOID_MAGIC:
        return _("虚無", "void magic");
    case AttributeType::WATER:
        return _("水流", "water");
    case AttributeType::WATER_FLOW:
        return _("流水", "water flow");
    case AttributeType::WOUNDS:
        return _("創傷", "wounds");
    default:
        return _("不明", "unknown");
    }
}
