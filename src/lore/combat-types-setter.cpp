#include "lore/combat-types-setter.h"
#include "lore/lore-util.h"
#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-types.h"
#include "system/monster-race-definition.h"
#include "term/term-color-types.h"

void set_monster_blow_method(lore_type *lore_ptr, int m)
{
    rbm_type method = lore_ptr->r_ptr->blow[m].method;
    lore_ptr->p = NULL;
    lore_ptr->pc = TERM_WHITE;
    switch (method) {
    case RBM_HIT:
        lore_ptr->p = _("殴る", "hit");
        lore_ptr->pc = TERM_L_WHITE;
        break;
    case RBM_TOUCH:
        lore_ptr->p = _("触る", "touch");
        break;
    case RBM_PUNCH:
        lore_ptr->p = _("パンチする", "punch");
        lore_ptr->pc = TERM_L_WHITE;
        break;
    case RBM_KICK:
        lore_ptr->p = _("蹴る", "kick");
        lore_ptr->pc = TERM_L_WHITE;
        break;
    case RBM_CLAW:
        lore_ptr->p = _("ひっかく", "claw");
        lore_ptr->pc = TERM_L_UMBER;
        break;
    case RBM_BITE:
        lore_ptr->p = _("噛む", "bite");
        lore_ptr->pc = TERM_L_UMBER;
        break;
    case RBM_STING:
        lore_ptr->p = _("刺す", "sting");
        break;
    case RBM_SLASH:
        lore_ptr->p = _("斬る", "slash");
        lore_ptr->pc = TERM_L_UMBER;
        break;
    case RBM_BUTT:
        lore_ptr->p = _("角で突く", "butt");
        lore_ptr->pc = TERM_L_WHITE;
        break;
    case RBM_CRUSH:
        lore_ptr->p = _("体当たりする", "crush");
        lore_ptr->pc = TERM_L_WHITE;
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
        lore_ptr->pc = TERM_SLATE;
        break;
    case RBM_SPIT:
        lore_ptr->p = _("つばを吐く", "spit");
        lore_ptr->pc = TERM_SLATE;
        break;
    case RBM_EXPLODE:
        lore_ptr->p = _("爆発する", "explode");
        lore_ptr->pc = TERM_L_BLUE;
        break;
    case RBM_GAZE:
        lore_ptr->p = _("にらむ", "gaze");
        lore_ptr->pc = TERM_SLATE;
        break;
    case RBM_WAIL:
        lore_ptr->p = _("泣き叫ぶ", "wail");
        lore_ptr->pc = TERM_SLATE;
        break;
    case RBM_SPORE:
        lore_ptr->p = _("胞子を飛ばす", "release spores");
        lore_ptr->pc = TERM_SLATE;
        break;
    case RBM_XXX4:
        break;
    case RBM_BEG:
        lore_ptr->p = _("金をせがむ", "beg");
        lore_ptr->pc = TERM_SLATE;
        break;
    case RBM_INSULT:
        lore_ptr->p = _("侮辱する", "insult");
        lore_ptr->pc = TERM_SLATE;
        break;
    case RBM_MOAN:
        lore_ptr->p = _("うめく", "moan");
        lore_ptr->pc = TERM_SLATE;
        break;
    case RBM_SHOW:
        lore_ptr->p = _("歌う", "sing");
        lore_ptr->pc = TERM_SLATE;
        break;

    case RBM_NONE:
    case RBM_SHOOT:
    case NB_RBM_TYPE:
        break;
    }
}

void set_monster_blow_effect(lore_type *lore_ptr, int m)
{
    rbe_type effect = lore_ptr->r_ptr->blow[m].effect;
    lore_ptr->q = NULL;
    lore_ptr->qc = TERM_WHITE;
    switch (effect) {
    case RBE_SUPERHURT:
        lore_ptr->q = _("強力に攻撃する", "slaughter");
        lore_ptr->qc = TERM_L_RED;
        break;
    case RBE_HURT:
        lore_ptr->q = _("攻撃する", "attack");
        break;
    case RBE_POISON:
        lore_ptr->q = _("毒をくらわす", "poison");
        lore_ptr->qc = TERM_L_GREEN;
        break;
    case RBE_UN_BONUS:
        lore_ptr->q = _("劣化させる", "disenchant");
        lore_ptr->qc = TERM_VIOLET;
        break;
    case RBE_UN_POWER:
        lore_ptr->q = _("充填魔力を吸収する", "drain charges");
        lore_ptr->qc = TERM_SLATE;
        break;
    case RBE_EAT_GOLD:
        lore_ptr->q = _("金を盗む", "steal gold");
        lore_ptr->qc = TERM_YELLOW;
        break;
    case RBE_EAT_ITEM:
        lore_ptr->q = _("アイテムを盗む", "steal items");
        lore_ptr->qc = TERM_UMBER;
        break;
    case RBE_EAT_FOOD:
        lore_ptr->q = _("あなたの食料を食べる", "eat your food");
        lore_ptr->qc = TERM_L_UMBER;
        break;
    case RBE_EAT_LITE:
        lore_ptr->q = _("明かりを吸収する", "absorb light");
        lore_ptr->qc = TERM_YELLOW;
        break;
    case RBE_ACID:
        lore_ptr->q = _("酸を飛ばす", "shoot acid");
        lore_ptr->qc = TERM_GREEN;
        break;
    case RBE_ELEC:
        lore_ptr->q = _("感電させる", "electrocute");
        lore_ptr->qc = TERM_BLUE;
        break;
    case RBE_FIRE:
        lore_ptr->q = _("燃やす", "burn");
        lore_ptr->qc = TERM_RED;
        break;
    case RBE_COLD:
        lore_ptr->q = _("凍らせる", "freeze");
        lore_ptr->qc = TERM_L_WHITE;
        break;
    case RBE_BLIND:
        lore_ptr->q = _("盲目にする", "blind");
        lore_ptr->qc = TERM_L_DARK;
        break;
    case RBE_CONFUSE:
        lore_ptr->q = _("混乱させる", "confuse");
        lore_ptr->qc = TERM_L_UMBER;
        break;
    case RBE_TERRIFY:
        lore_ptr->q = _("恐怖させる", "terrify");
        lore_ptr->qc = TERM_SLATE;
        break;
    case RBE_PARALYZE:
        lore_ptr->q = _("麻痺させる", "paralyze");
        lore_ptr->qc = TERM_BLUE;
        break;
    case RBE_LOSE_STR:
        lore_ptr->q = _("腕力を減少させる", "reduce strength");
        lore_ptr->qc = TERM_L_GREEN;
        break;
    case RBE_LOSE_INT:
        lore_ptr->q = _("知能を減少させる", "reduce intelligence");
        lore_ptr->qc = TERM_L_GREEN;
        break;
    case RBE_LOSE_WIS:
        lore_ptr->q = _("賢さを減少させる", "reduce wisdom");
        lore_ptr->qc = TERM_L_GREEN;
        break;
    case RBE_LOSE_DEX:
        lore_ptr->q = _("器用さを減少させる", "reduce dexterity");
        lore_ptr->qc = TERM_L_GREEN;
        break;
    case RBE_LOSE_CON:
        lore_ptr->q = _("耐久力を減少させる", "reduce constitution");
        lore_ptr->qc = TERM_L_GREEN;
        break;
    case RBE_LOSE_CHR:
        lore_ptr->q = _("魅力を減少させる", "reduce charisma");
        lore_ptr->qc = TERM_L_GREEN;
        break;
    case RBE_LOSE_ALL:
        lore_ptr->q = _("全ステータスを減少させる", "reduce all stats");
        lore_ptr->qc = TERM_L_GREEN;
        break;
    case RBE_SHATTER:
        lore_ptr->q = _("粉砕する", "shatter");
        lore_ptr->qc = TERM_SLATE;
        break;
    case RBE_EXP_10:
        lore_ptr->q = _("経験値を減少(10d6+)させる", "lower experience (by 10d6+)");
        lore_ptr->qc = TERM_L_BLUE;
        break;
    case RBE_EXP_20:
        lore_ptr->q = _("経験値を減少(20d6+)させる", "lower experience (by 20d6+)");
        lore_ptr->qc = TERM_L_BLUE;
        break;
    case RBE_EXP_40:
        lore_ptr->q = _("経験値を減少(40d6+)させる", "lower experience (by 40d6+)");
        lore_ptr->qc = TERM_L_BLUE;
        break;
    case RBE_EXP_80:
        lore_ptr->q = _("経験値を減少(80d6+)させる", "lower experience (by 80d6+)");
        lore_ptr->qc = TERM_L_BLUE;
        break;
    case RBE_DISEASE:
        lore_ptr->q = _("病気にする", "disease");
        lore_ptr->qc = TERM_L_GREEN;
        break;
    case RBE_TIME:
        lore_ptr->q = _("時間を逆戻りさせる", "time");
        lore_ptr->qc = TERM_L_BLUE;
        break;
    case RBE_DR_LIFE:
        lore_ptr->q = _("生命力を吸収する", "drain life");
        lore_ptr->qc = TERM_L_BLUE;
        break;
    case RBE_DR_MANA:
        lore_ptr->q = _("魔力を奪う", "drain mana force");
        lore_ptr->qc = TERM_SLATE;
        break;
    case RBE_INERTIA:
        lore_ptr->q = _("減速させる", "slow");
        lore_ptr->qc = TERM_UMBER;
        break;
    case RBE_STUN:
        lore_ptr->q = _("朦朧とさせる", "stun");
        lore_ptr->qc = TERM_ORANGE;
        break;
    case RBE_HUNGRY:
        lore_ptr->q = _("空腹を進行させる", "be hangry");
        lore_ptr->qc = TERM_L_BLUE;
        break;
    case RBE_FLAVOR:
        // フレーバー打撃には何の効果もないので付加説明もない。
        break;

    case RBE_NONE:
    case NB_RBE_TYPE:
        break;
    }
}
