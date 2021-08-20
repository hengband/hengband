/*!
 * @file random-art-characteristics.cpp
 * @brief ランダムアーティファクトのバイアス付加処理実装
 */

#include "artifact/random-art-characteristics.h"
#include "flavor/object-flavor.h"
#include "game-option/cheat-types.h"
#include "io/files-util.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object/object-flags.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "wizard/wizard-messages.h"

static void pval_subtraction(object_type *o_ptr)
{
    if (o_ptr->pval > 0)
        o_ptr->pval = 0 - (o_ptr->pval + randint1(4));

    if (o_ptr->to_a > 0)
        o_ptr->to_a = 0 - (o_ptr->to_a + randint1(4));

    if (o_ptr->to_h > 0)
        o_ptr->to_h = 0 - (o_ptr->to_h + randint1(4));

    if (o_ptr->to_d > 0)
        o_ptr->to_d = 0 - (o_ptr->to_d + randint1(4));
}

static void add_negative_flags(object_type *o_ptr)
{
    if (one_in_(4))
        o_ptr->curse_flags.set(TRC::PERMA_CURSE);

    if (one_in_(3))
        add_flag(o_ptr->art_flags, TR_TY_CURSE);

    if (one_in_(2))
        add_flag(o_ptr->art_flags, TR_AGGRAVATE);

    if (one_in_(3))
        add_flag(o_ptr->art_flags, TR_DRAIN_EXP);

    if (one_in_(6))
        add_flag(o_ptr->art_flags, TR_ADD_L_CURSE);

    if (one_in_(9))
        add_flag(o_ptr->art_flags, TR_ADD_H_CURSE);

    if (one_in_(9))
        add_flag(o_ptr->art_flags, TR_DRAIN_HP);

    if (one_in_(9))
        add_flag(o_ptr->art_flags, TR_DRAIN_MANA);

    if (one_in_(2))
        add_flag(o_ptr->art_flags, TR_TELEPORT);
    else if (one_in_(3))
        add_flag(o_ptr->art_flags, TR_NO_TELE);
}

/*!
 * @brief ランダムアーティファクト生成中、対象のオブジェクトを呪いのアーティファクトにする経過処理。/ generation process of cursed artifact.
 * @details pval、AC、命中、ダメージが正の場合、符号反転の上1d4だけ悪化させ、重い呪い、呪いフラグを必ず付加。
 * 祝福を無効。確率に応じて、永遠の呪い、太古の怨念、経験値吸収、弱い呪いの継続的付加、強い呪いの継続的付加、HP吸収の呪い、
 * MP吸収の呪い、乱テレポート、反テレポート、反魔法をつける。
 * @attention プレイヤーの職業依存処理あり。
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void curse_artifact(player_type *player_ptr, object_type *o_ptr)
{
    pval_subtraction(o_ptr);
    o_ptr->curse_flags.set({ TRC::HEAVY_CURSE, TRC::CURSED });
    remove_flag(o_ptr->art_flags, TR_BLESSED);
    add_negative_flags(o_ptr);
    if ((player_ptr->pclass != CLASS_WARRIOR) && (player_ptr->pclass != CLASS_ARCHER) && (player_ptr->pclass != CLASS_CAVALRY)
        && (player_ptr->pclass != CLASS_BERSERKER) && (player_ptr->pclass != CLASS_SMITH) && one_in_(3))
        add_flag(o_ptr->art_flags, TR_NO_MAGIC);
}

/*!
 * @brief ランダムアーティファクトの名前リストをオブジェクト種別と生成パワーに応じて選択する
 * @param armour 防具かどうか
 * @param power 生成パワー
 * @return ファイル名
 * @details 二重switch文だが短いので執行猶予とする
 */
static concptr get_random_art_filename(const bool armour, const int power)
{
    concptr filename;
    if (armour) {
        switch (power) {
        case 0:
            filename = _("a_cursed_j.txt", "a_cursed.txt");
            break;
        case 1:
            filename = _("a_low_j.txt", "a_low.txt");
            break;
        case 2:
            filename = _("a_med_j.txt", "a_med.txt");
            break;
        default:
            filename = _("a_high_j.txt", "a_high.txt");
        }
    } else {
        switch (power) {
        case 0:
            filename = _("w_cursed_j.txt", "w_cursed.txt");
            break;
        case 1:
            filename = _("w_low_j.txt", "w_low.txt");
            break;
        case 2:
            filename = _("w_med_j.txt", "w_med.txt");
            break;
        default:
            filename = _("w_high_j.txt", "w_high.txt");
        }
    }

    return filename;
}

/*!
 * @brief ランダムアーティファクト生成中、対象のオブジェクトに名前を与える。/ Set name of randomartifact.
 * @details 確率によって、シンダリン銘、漢字銘、固定名のいずれか一つが与えられる。
 * @param o_ptr 処理中のアイテム参照ポインタ
 * @param return_name 名前を返すための文字列参照ポインタ
 * @param armour 対象のオブジェクトが防具が否か
 * @param power 銘の基準となるオブジェクトの価値レベル(0=呪い、1=低位、2=中位、3以上=高位)
 */
void get_random_name(object_type *o_ptr, char *return_name, bool armour, int power)
{
    PERCENTAGE prob = randint1(100);
    if (prob <= SINDARIN_NAME) {
        get_table_sindarin(return_name);
        return;
    }

    if (prob <= TABLE_NAME) {
        get_table_name(return_name);
        return;
    }

    concptr filename = get_random_art_filename(armour, power);
    (void)get_rnd_line(filename, o_ptr->artifact_bias, return_name);
#ifdef JP
    if (return_name[0] == 0)
        get_table_name(return_name);
#endif
}

/*対邪平均ダメージの計算処理*/
static HIT_POINT calc_arm_avgdamage(player_type *player_ptr, object_type *o_ptr)
{
    TrFlags flgs;
    object_flags(player_ptr, o_ptr, flgs);
    HIT_POINT base, forced, vorpal;
    HIT_POINT s_evil = forced = vorpal = 0;
    HIT_POINT dam = base = (o_ptr->dd * o_ptr->ds + o_ptr->dd) / 2;
    if (has_flag(flgs, TR_KILL_EVIL)) {
        dam = s_evil = dam * 7 / 2;
    } else if (!has_flag(flgs, TR_KILL_EVIL) && has_flag(flgs, TR_SLAY_EVIL)) {
        dam = s_evil = dam * 2;
    } else
        s_evil = dam;

    if (has_flag(flgs, TR_FORCE_WEAPON)) {
        dam = forced = dam * 3 / 2 + (o_ptr->dd * o_ptr->ds + o_ptr->dd);
    } else
        forced = dam;

    if (has_flag(flgs, TR_VORPAL)) {
        dam = vorpal = dam * 11 / 9;
    } else
        vorpal = dam;

    dam = dam + o_ptr->to_d;
    msg_format_wizard(player_ptr, CHEAT_OBJECT, "素:%d> 対邪:%d> 理力:%d> 切:%d> 最終:%d", base, s_evil, forced, vorpal, dam);
    return dam;
}

bool has_extreme_damage_rate(player_type *player_ptr, object_type *o_ptr)
{
    TrFlags flgs;
    object_flags(player_ptr, o_ptr, flgs);
    if (has_flag(flgs, TR_VAMPIRIC)) {
        if (has_flag(flgs, TR_BLOWS) && (o_ptr->pval == 1) && (calc_arm_avgdamage(player_ptr, o_ptr) > 52)) {
            return true;
        }

        if (has_flag(flgs, TR_BLOWS) && (o_ptr->pval == 2) && (calc_arm_avgdamage(player_ptr, o_ptr) > 43)) {
            return true;
        }

        if (has_flag(flgs, TR_BLOWS) && (o_ptr->pval == 3) && (calc_arm_avgdamage(player_ptr, o_ptr) > 33)) {
            return true;
        }

        if (calc_arm_avgdamage(player_ptr, o_ptr) > 63) {
            return true;
        }

        return false;
    }

    if (has_flag(flgs, TR_BLOWS) && (o_ptr->pval == 1) && (calc_arm_avgdamage(player_ptr, o_ptr) > 65)) {
        return true;
    }

    if (has_flag(flgs, TR_BLOWS) && (o_ptr->pval == 2) && (calc_arm_avgdamage(player_ptr, o_ptr) > 52)) {
        return true;
    }

    if (has_flag(flgs, TR_BLOWS) && (o_ptr->pval == 3) && (calc_arm_avgdamage(player_ptr, o_ptr) > 40)) {
        return true;
    }

    if (calc_arm_avgdamage(player_ptr, o_ptr) > 75) {
        return true;
    }

    return false;
}
