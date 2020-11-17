#include "lore/combat-types-setter.h"
#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-types.h"

void set_monster_blow_method(lore_type *lore_ptr, int m)
{
    rbm_type method = lore_ptr->r_ptr->blow[m].method;
    lore_ptr->p = NULL;
    switch (method) {
    case RBM_HIT:
        lore_ptr->p = _("殴る", "hit");
        break;
    case RBM_TOUCH:
        lore_ptr->p = _("触る", "touch");
        break;
    case RBM_PUNCH:
        lore_ptr->p = _("パンチする", "punch");
        break;
    case RBM_KICK:
        lore_ptr->p = _("蹴る", "kick");
        break;
    case RBM_CLAW:
        lore_ptr->p = _("ひっかく", "claw");
        break;
    case RBM_BITE:
        lore_ptr->p = _("噛む", "bite");
        break;
    case RBM_STING:
        lore_ptr->p = _("刺す", "sting");
        break;
    case RBM_SLASH:
        lore_ptr->p = _("斬る", "slash");
        break;
    case RBM_BUTT:
        lore_ptr->p = _("角で突く", "butt");
        break;
    case RBM_CRUSH:
        lore_ptr->p = _("体当たりする", "crush");
        break;
    case RBM_ENGULF:
        lore_ptr->p = _("飲み込む", "engulf");
        break;
    case RBM_CHARGE:
        lore_ptr->p = _("請求書をよこす", "charge");
        break;
    case RBM_CRAWL:
        lore_ptr->p = _("体の上を這い回る", "crawl on you");
        break;
    case RBM_DROOL:
        lore_ptr->p = _("よだれをたらす", "drool on you");
        break;
    case RBM_SPIT:
        lore_ptr->p = _("つばを吐く", "spit");
        break;
    case RBM_EXPLODE:
        lore_ptr->p = _("爆発する", "explode");
        break;
    case RBM_GAZE:
        lore_ptr->p = _("にらむ", "gaze");
        break;
    case RBM_WAIL:
        lore_ptr->p = _("泣き叫ぶ", "wail");
        break;
    case RBM_SPORE:
        lore_ptr->p = _("胞子を飛ばす", "release spores");
        break;
    case RBM_XXX4:
        break;
    case RBM_BEG:
        lore_ptr->p = _("金をせがむ", "beg");
        break;
    case RBM_INSULT:
        lore_ptr->p = _("侮辱する", "insult");
        break;
    case RBM_MOAN:
        lore_ptr->p = _("うめく", "moan");
        break;
    case RBM_SHOW:
        lore_ptr->p = _("歌う", "sing");
        break;
    }
}

void set_monster_blow_effect(lore_type *lore_ptr, int m)
{
    rbe_type effect = lore_ptr->r_ptr->blow[m].effect;
    lore_ptr->q = NULL;
    switch (effect) {
    case RBE_SUPERHURT:
        lore_ptr->q = _("強力に攻撃する", "slaughter");
        break;
    case RBE_HURT:
        lore_ptr->q = _("攻撃する", "attack");
        break;
    case RBE_POISON:
        lore_ptr->q = _("毒をくらわす", "poison");
        break;
    case RBE_UN_BONUS:
        lore_ptr->q = _("劣化させる", "disenchant");
        break;
    case RBE_UN_POWER:
        lore_ptr->q = _("充填魔力を吸収する", "drain charges");
        break;
    case RBE_EAT_GOLD:
        lore_ptr->q = _("金を盗む", "steal gold");
        break;
    case RBE_EAT_ITEM:
        lore_ptr->q = _("アイテムを盗む", "steal items");
        break;
    case RBE_EAT_FOOD:
        lore_ptr->q = _("あなたの食料を食べる", "eat your food");
        break;
    case RBE_EAT_LITE:
        lore_ptr->q = _("明かりを吸収する", "absorb light");
        break;
    case RBE_ACID:
        lore_ptr->q = _("酸を飛ばす", "shoot acid");
        break;
    case RBE_ELEC:
        lore_ptr->q = _("感電させる", "electrocute");
        break;
    case RBE_FIRE:
        lore_ptr->q = _("燃やす", "burn");
        break;
    case RBE_COLD:
        lore_ptr->q = _("凍らせる", "freeze");
        break;
    case RBE_BLIND:
        lore_ptr->q = _("盲目にする", "blind");
        break;
    case RBE_CONFUSE:
        lore_ptr->q = _("混乱させる", "confuse");
        break;
    case RBE_TERRIFY:
        lore_ptr->q = _("恐怖させる", "terrify");
        break;
    case RBE_PARALYZE:
        lore_ptr->q = _("麻痺させる", "paralyze");
        break;
    case RBE_LOSE_STR:
        lore_ptr->q = _("腕力を減少させる", "reduce strength");
        break;
    case RBE_LOSE_INT:
        lore_ptr->q = _("知能を減少させる", "reduce intelligence");
        break;
    case RBE_LOSE_WIS:
        lore_ptr->q = _("賢さを減少させる", "reduce wisdom");
        break;
    case RBE_LOSE_DEX:
        lore_ptr->q = _("器用さを減少させる", "reduce dexterity");
        break;
    case RBE_LOSE_CON:
        lore_ptr->q = _("耐久力を減少させる", "reduce constitution");
        break;
    case RBE_LOSE_CHR:
        lore_ptr->q = _("魅力を減少させる", "reduce charisma");
        break;
    case RBE_LOSE_ALL:
        lore_ptr->q = _("全ステータスを減少させる", "reduce all stats");
        break;
    case RBE_SHATTER:
        lore_ptr->q = _("粉砕する", "shatter");
        break;
    case RBE_EXP_10:
        lore_ptr->q = _("経験値を減少(10d6+)させる", "lower experience (by 10d6+)");
        break;
    case RBE_EXP_20:
        lore_ptr->q = _("経験値を減少(20d6+)させる", "lower experience (by 20d6+)");
        break;
    case RBE_EXP_40:
        lore_ptr->q = _("経験値を減少(40d6+)させる", "lower experience (by 40d6+)");
        break;
    case RBE_EXP_80:
        lore_ptr->q = _("経験値を減少(80d6+)させる", "lower experience (by 80d6+)");
        break;
    case RBE_DISEASE:
        lore_ptr->q = _("病気にする", "disease");
        break;
    case RBE_TIME:
        lore_ptr->q = _("時間を逆戻りさせる", "time");
        break;
    case RBE_DR_LIFE:
        lore_ptr->q = _("生命力を吸収する", "drain life");
        break;
    case RBE_DR_MANA:
        lore_ptr->q = _("魔力を奪う", "drain mana force");
        break;
    case RBE_INERTIA:
        lore_ptr->q = _("減速させる", "slow");
        break;
    case RBE_STUN:
        lore_ptr->q = _("朦朧とさせる", "stun");
        break;
    }
}
