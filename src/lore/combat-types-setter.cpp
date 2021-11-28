#include "lore/combat-types-setter.h"
#include "lore/lore-util.h"
#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-types.h"
#include "system/monster-race-definition.h"
#include "term/term-color-types.h"

void set_monster_blow_method(lore_type *lore_ptr, int m)
{
    RaceBlowMethodType method = lore_ptr->r_ptr->blow[m].method;
    lore_ptr->p = nullptr;
    lore_ptr->pc = TERM_WHITE;
    switch (method) {
    case RaceBlowMethodType::HIT:
        lore_ptr->p = _("殴る", "hit");
        lore_ptr->pc = TERM_L_WHITE;
        break;
    case RaceBlowMethodType::TOUCH:
        lore_ptr->p = _("触る", "touch");
        break;
    case RaceBlowMethodType::PUNCH:
        lore_ptr->p = _("パンチする", "punch");
        lore_ptr->pc = TERM_L_WHITE;
        break;
    case RaceBlowMethodType::KICK:
        lore_ptr->p = _("蹴る", "kick");
        lore_ptr->pc = TERM_L_WHITE;
        break;
    case RaceBlowMethodType::CLAW:
        lore_ptr->p = _("ひっかく", "claw");
        lore_ptr->pc = TERM_L_UMBER;
        break;
    case RaceBlowMethodType::BITE:
        lore_ptr->p = _("噛む", "bite");
        lore_ptr->pc = TERM_L_UMBER;
        break;
    case RaceBlowMethodType::STING:
        lore_ptr->p = _("刺す", "sting");
        break;
    case RaceBlowMethodType::SLASH:
        lore_ptr->p = _("斬る", "slash");
        lore_ptr->pc = TERM_L_UMBER;
        break;
    case RaceBlowMethodType::BUTT:
        lore_ptr->p = _("角で突く", "butt");
        lore_ptr->pc = TERM_L_WHITE;
        break;
    case RaceBlowMethodType::CRUSH:
        lore_ptr->p = _("体当たりする", "crush");
        lore_ptr->pc = TERM_L_WHITE;
        break;
    case RaceBlowMethodType::ENGULF:
        lore_ptr->p = _("飲み込む", "engulf");
        break;
    case RaceBlowMethodType::CHARGE:
        lore_ptr->p = _("請求書をよこす", "charge");
        break;
    case RaceBlowMethodType::CRAWL:
        lore_ptr->p = _("体の上を這い回る", "crawl on you");
        break;
    case RaceBlowMethodType::DROOL:
        lore_ptr->p = _("よだれをたらす", "drool on you");
        lore_ptr->pc = TERM_SLATE;
        break;
    case RaceBlowMethodType::SPIT:
        lore_ptr->p = _("つばを吐く", "spit");
        lore_ptr->pc = TERM_SLATE;
        break;
    case RaceBlowMethodType::EXPLODE:
        lore_ptr->p = _("爆発する", "explode");
        lore_ptr->pc = TERM_L_BLUE;
        break;
    case RaceBlowMethodType::GAZE:
        lore_ptr->p = _("にらむ", "gaze");
        lore_ptr->pc = TERM_SLATE;
        break;
    case RaceBlowMethodType::WAIL:
        lore_ptr->p = _("泣き叫ぶ", "wail");
        lore_ptr->pc = TERM_SLATE;
        break;
    case RaceBlowMethodType::SPORE:
        lore_ptr->p = _("胞子を飛ばす", "release spores");
        lore_ptr->pc = TERM_SLATE;
        break;
    case RaceBlowMethodType::XXX4:
        break;
    case RaceBlowMethodType::BEG:
        lore_ptr->p = _("金をせがむ", "beg");
        lore_ptr->pc = TERM_SLATE;
        break;
    case RaceBlowMethodType::INSULT:
        lore_ptr->p = _("侮辱する", "insult");
        lore_ptr->pc = TERM_SLATE;
        break;
    case RaceBlowMethodType::MOAN:
        lore_ptr->p = _("うめく", "moan");
        lore_ptr->pc = TERM_SLATE;
        break;
    case RaceBlowMethodType::SHOW:
        lore_ptr->p = _("歌う", "sing");
        lore_ptr->pc = TERM_SLATE;
        break;

    case RaceBlowMethodType::NONE:
    case RaceBlowMethodType::SHOOT:
    case RaceBlowMethodType::MAX:
        break;
    }
}

void set_monster_blow_effect(lore_type *lore_ptr, int m)
{
    RaceBlowEffectType effect = lore_ptr->r_ptr->blow[m].effect;
    lore_ptr->q = nullptr;
    lore_ptr->qc = TERM_WHITE;
    switch (effect) {
    case RaceBlowEffectType::SUPERHURT:
        lore_ptr->q = _("強力に攻撃する", "slaughter");
        lore_ptr->qc = TERM_L_RED;
        break;
    case RaceBlowEffectType::HURT:
        lore_ptr->q = _("攻撃する", "attack");
        break;
    case RaceBlowEffectType::POISON:
        lore_ptr->q = _("毒をくらわす", "poison");
        lore_ptr->qc = TERM_L_GREEN;
        break;
    case RaceBlowEffectType::UN_BONUS:
        lore_ptr->q = _("劣化させる", "disenchant");
        lore_ptr->qc = TERM_VIOLET;
        break;
    case RaceBlowEffectType::UN_POWER:
        lore_ptr->q = _("充填魔力を吸収する", "drain charges");
        lore_ptr->qc = TERM_SLATE;
        break;
    case RaceBlowEffectType::EAT_GOLD:
        lore_ptr->q = _("金を盗む", "steal gold");
        lore_ptr->qc = TERM_YELLOW;
        break;
    case RaceBlowEffectType::EAT_ITEM:
        lore_ptr->q = _("アイテムを盗む", "steal items");
        lore_ptr->qc = TERM_UMBER;
        break;
    case RaceBlowEffectType::EAT_FOOD:
        lore_ptr->q = _("あなたの食料を食べる", "eat your food");
        lore_ptr->qc = TERM_L_UMBER;
        break;
    case RaceBlowEffectType::EAT_LITE:
        lore_ptr->q = _("明かりを吸収する", "absorb light");
        lore_ptr->qc = TERM_YELLOW;
        break;
    case RaceBlowEffectType::ACID:
        lore_ptr->q = _("酸を飛ばす", "shoot acid");
        lore_ptr->qc = TERM_GREEN;
        break;
    case RaceBlowEffectType::ELEC:
        lore_ptr->q = _("感電させる", "electrocute");
        lore_ptr->qc = TERM_BLUE;
        break;
    case RaceBlowEffectType::FIRE:
        lore_ptr->q = _("燃やす", "burn");
        lore_ptr->qc = TERM_RED;
        break;
    case RaceBlowEffectType::COLD:
        lore_ptr->q = _("凍らせる", "freeze");
        lore_ptr->qc = TERM_L_WHITE;
        break;
    case RaceBlowEffectType::BLIND:
        lore_ptr->q = _("盲目にする", "blind");
        lore_ptr->qc = TERM_L_DARK;
        break;
    case RaceBlowEffectType::CONFUSE:
        lore_ptr->q = _("混乱させる", "confuse");
        lore_ptr->qc = TERM_L_UMBER;
        break;
    case RaceBlowEffectType::TERRIFY:
        lore_ptr->q = _("恐怖させる", "terrify");
        lore_ptr->qc = TERM_SLATE;
        break;
    case RaceBlowEffectType::PARALYZE:
        lore_ptr->q = _("麻痺させる", "paralyze");
        lore_ptr->qc = TERM_BLUE;
        break;
    case RaceBlowEffectType::LOSE_STR:
        lore_ptr->q = _("腕力を減少させる", "reduce strength");
        lore_ptr->qc = TERM_L_GREEN;
        break;
    case RaceBlowEffectType::LOSE_INT:
        lore_ptr->q = _("知能を減少させる", "reduce intelligence");
        lore_ptr->qc = TERM_L_GREEN;
        break;
    case RaceBlowEffectType::LOSE_WIS:
        lore_ptr->q = _("賢さを減少させる", "reduce wisdom");
        lore_ptr->qc = TERM_L_GREEN;
        break;
    case RaceBlowEffectType::LOSE_DEX:
        lore_ptr->q = _("器用さを減少させる", "reduce dexterity");
        lore_ptr->qc = TERM_L_GREEN;
        break;
    case RaceBlowEffectType::LOSE_CON:
        lore_ptr->q = _("耐久力を減少させる", "reduce constitution");
        lore_ptr->qc = TERM_L_GREEN;
        break;
    case RaceBlowEffectType::LOSE_CHR:
        lore_ptr->q = _("魅力を減少させる", "reduce charisma");
        lore_ptr->qc = TERM_L_GREEN;
        break;
    case RaceBlowEffectType::LOSE_ALL:
        lore_ptr->q = _("全ステータスを減少させる", "reduce all stats");
        lore_ptr->qc = TERM_L_GREEN;
        break;
    case RaceBlowEffectType::SHATTER:
        lore_ptr->q = _("粉砕する", "shatter");
        lore_ptr->qc = TERM_SLATE;
        break;
    case RaceBlowEffectType::EXP_10:
        lore_ptr->q = _("経験値を減少(10d6+)させる", "lower experience (by 10d6+)");
        lore_ptr->qc = TERM_L_BLUE;
        break;
    case RaceBlowEffectType::EXP_20:
        lore_ptr->q = _("経験値を減少(20d6+)させる", "lower experience (by 20d6+)");
        lore_ptr->qc = TERM_L_BLUE;
        break;
    case RaceBlowEffectType::EXP_40:
        lore_ptr->q = _("経験値を減少(40d6+)させる", "lower experience (by 40d6+)");
        lore_ptr->qc = TERM_L_BLUE;
        break;
    case RaceBlowEffectType::EXP_80:
        lore_ptr->q = _("経験値を減少(80d6+)させる", "lower experience (by 80d6+)");
        lore_ptr->qc = TERM_L_BLUE;
        break;
    case RaceBlowEffectType::DISEASE:
        lore_ptr->q = _("病気にする", "disease");
        lore_ptr->qc = TERM_L_GREEN;
        break;
    case RaceBlowEffectType::TIME:
        lore_ptr->q = _("時間を逆戻りさせる", "time");
        lore_ptr->qc = TERM_L_BLUE;
        break;
    case RaceBlowEffectType::DR_LIFE:
        lore_ptr->q = _("生命力を吸収する", "drain life");
        lore_ptr->qc = TERM_L_BLUE;
        break;
    case RaceBlowEffectType::DR_MANA:
        lore_ptr->q = _("魔力を奪う", "drain mana force");
        lore_ptr->qc = TERM_SLATE;
        break;
    case RaceBlowEffectType::INERTIA:
        lore_ptr->q = _("減速させる", "slow");
        lore_ptr->qc = TERM_UMBER;
        break;
    case RaceBlowEffectType::STUN:
        lore_ptr->q = _("朦朧とさせる", "stun");
        lore_ptr->qc = TERM_ORANGE;
        break;
    case RaceBlowEffectType::HUNGRY:
        lore_ptr->q = _("空腹を進行させる", "increase hunger");
        lore_ptr->qc = TERM_L_BLUE;
        break;
    case RaceBlowEffectType::FLAVOR:
        // フレーバー打撃には何の効果もないので付加説明もない。
        break;

    case RaceBlowEffectType::NONE:
    case RaceBlowEffectType::MAX:
        break;
    }
}
