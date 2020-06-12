#include "lore/magic-types-setter.h"
#include "lore/lore-calculator.h"
#include "monster-race/race-flags-ability1.h"
#include "monster-race/race-flags4.h"
#include "mspell/mspell-type.h"
#include "term/term-color-types.h"

void set_breath_types(player_type *player_ptr, lore_type *lore_ptr)
{
    lore_ptr->vn = 0;
    if (lore_ptr->flags4 & (RF4_BR_ACID)) {
        set_damage(player_ptr, lore_ptr->r_idx, (MS_BR_ACID), _("酸%s", "acid%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_GREEN;
    }

    if (lore_ptr->flags4 & (RF4_BR_ELEC)) {
        set_damage(player_ptr, lore_ptr->r_idx, (MS_BR_ELEC), _("稲妻%s", "lightning%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_BLUE;
    }

    if (lore_ptr->flags4 & (RF4_BR_FIRE)) {
        set_damage(player_ptr, lore_ptr->r_idx, (MS_BR_FIRE), _("火炎%s", "fire%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_RED;
    }

    if (lore_ptr->flags4 & (RF4_BR_COLD)) {
        set_damage(player_ptr, lore_ptr->r_idx, (MS_BR_COLD), _("冷気%s", "frost%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_L_WHITE;
    }

    if (lore_ptr->flags4 & (RF4_BR_POIS)) {
        set_damage(player_ptr, lore_ptr->r_idx, (MS_BR_POIS), _("毒%s", "poison%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_L_GREEN;
    }

    if (lore_ptr->flags4 & (RF4_BR_NETH)) {
        set_damage(player_ptr, lore_ptr->r_idx, (MS_BR_NETHER), _("地獄%s", "nether%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_L_DARK;
    }

    if (lore_ptr->flags4 & (RF4_BR_LITE)) {
        set_damage(player_ptr, lore_ptr->r_idx, (MS_BR_LITE), _("閃光%s", "light%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_YELLOW;
    }

    if (lore_ptr->flags4 & (RF4_BR_DARK)) {
        set_damage(player_ptr, lore_ptr->r_idx, (MS_BR_DARK), _("暗黒%s", "darkness%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_L_DARK;
    }

    if (lore_ptr->flags4 & (RF4_BR_CONF)) {
        set_damage(player_ptr, lore_ptr->r_idx, (MS_BR_CONF), _("混乱%s", "confusion%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_L_UMBER;
    }

    if (lore_ptr->flags4 & (RF4_BR_SOUN)) {
        set_damage(player_ptr, lore_ptr->r_idx, (MS_BR_SOUND), _("轟音%s", "sound%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_ORANGE;
    }

    if (lore_ptr->flags4 & (RF4_BR_CHAO)) {
        set_damage(player_ptr, lore_ptr->r_idx, (MS_BR_CHAOS), _("カオス%s", "chaos%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_VIOLET;
    }

    if (lore_ptr->flags4 & (RF4_BR_DISE)) {
        set_damage(player_ptr, lore_ptr->r_idx, (MS_BR_DISEN), _("劣化%s", "disenchantment%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_VIOLET;
    }

    if (lore_ptr->flags4 & (RF4_BR_NEXU)) {
        set_damage(player_ptr, lore_ptr->r_idx, (MS_BR_NEXUS), _("因果混乱%s", "nexus%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_VIOLET;
    }

    if (lore_ptr->flags4 & (RF4_BR_TIME)) {
        set_damage(player_ptr, lore_ptr->r_idx, (MS_BR_TIME), _("時間逆転%s", "time%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_L_BLUE;
    }

    if (lore_ptr->flags4 & (RF4_BR_INER)) {
        set_damage(player_ptr, lore_ptr->r_idx, (MS_BR_INERTIA), _("遅鈍%s", "inertia%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_SLATE;
    }

    if (lore_ptr->flags4 & (RF4_BR_GRAV)) {
        set_damage(player_ptr, lore_ptr->r_idx, (MS_BR_GRAVITY), _("重力%s", "gravity%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_SLATE;
    }

    if (lore_ptr->flags4 & (RF4_BR_SHAR)) {
        set_damage(player_ptr, lore_ptr->r_idx, (MS_BR_SHARDS), _("破片%s", "shards%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_L_UMBER;
    }

    if (lore_ptr->flags4 & (RF4_BR_PLAS)) {
        set_damage(player_ptr, lore_ptr->r_idx, (MS_BR_PLASMA), _("プラズマ%s", "plasma%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_L_RED;
    }

    if (lore_ptr->flags4 & (RF4_BR_WALL)) {
        set_damage(player_ptr, lore_ptr->r_idx, (MS_BR_FORCE), _("フォース%s", "force%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_UMBER;
    }

    if (lore_ptr->flags4 & (RF4_BR_MANA)) {
        set_damage(player_ptr, lore_ptr->r_idx, (MS_BR_MANA), _("魔力%s", "mana%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_L_BLUE;
    }

    if (lore_ptr->flags4 & (RF4_BR_NUKE)) {
        set_damage(player_ptr, lore_ptr->r_idx, (MS_BR_NUKE), _("放射性廃棄物%s", "toxic waste%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_L_GREEN;
    }

    if (lore_ptr->flags4 & (RF4_BR_DISI)) {
        set_damage(player_ptr, lore_ptr->r_idx, (MS_BR_DISI), _("分解%s", "disintegration%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_SLATE;
    }
}

void set_ball_types(player_type *player_ptr, lore_type *lore_ptr)
{
    if (lore_ptr->a_ability_flags1 & (RF5_BA_ACID)) {
        set_damage(player_ptr, lore_ptr->breath, (MS_BALL_ACID), _("アシッド・ボール%s", "produce acid balls%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_GREEN;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BA_ELEC)) {
        set_damage(player_ptr, lore_ptr->breath, (MS_BALL_ELEC), _("サンダー・ボール%s", "produce lightning balls%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_BLUE;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BA_FIRE)) {
        set_damage(player_ptr, lore_ptr->breath, (MS_BALL_FIRE), _("ファイア・ボール%s", "produce fire balls%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_RED;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BA_COLD)) {
        set_damage(player_ptr, lore_ptr->breath, (MS_BALL_COLD), _("アイス・ボール%s", "produce frost balls%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_L_WHITE;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BA_POIS)) {
        set_damage(player_ptr, lore_ptr->breath, (MS_BALL_POIS), _("悪臭雲%s", "produce poison balls%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_L_GREEN;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BA_NETH)) {
        set_damage(player_ptr, lore_ptr->breath, (MS_BALL_NETHER), _("地獄球%s", "produce nether balls%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_L_DARK;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BA_WATE)) {
        set_damage(player_ptr, lore_ptr->breath, (MS_BALL_WATER), _("ウォーター・ボール%s", "produce water balls%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_BLUE;
    }

    if (lore_ptr->flags4 & (RF4_BA_NUKE)) {
        set_damage(player_ptr, lore_ptr->breath, (MS_BALL_NUKE), _("放射能球%s", "produce balls of radiation%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_L_GREEN;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BA_MANA)) {
        set_damage(player_ptr, lore_ptr->breath, (MS_BALL_MANA), _("魔力の嵐%s", "invoke mana storms%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_L_BLUE;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BA_DARK)) {
        set_damage(player_ptr, lore_ptr->breath, (MS_BALL_DARK), _("暗黒の嵐%s", "invoke darkness storms%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_L_DARK;
    }

    if (lore_ptr->a_ability_flags1 & (RF5_BA_LITE)) {
        set_damage(player_ptr, lore_ptr->breath, (MS_STARBURST), _("スターバースト%s", "invoke starburst%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_YELLOW;
    }

    if (lore_ptr->flags4 & (RF4_BA_CHAO)) {
        set_damage(player_ptr, lore_ptr->breath, (MS_BALL_CHAOS), _("純ログルス%s", "invoke raw Logrus%s"), lore_ptr->tmp_msg[lore_ptr->vn]);
        lore_ptr->vp[lore_ptr->vn] = lore_ptr->tmp_msg[lore_ptr->vn];
        lore_ptr->color[lore_ptr->vn++] = TERM_VIOLET;
    }
}
