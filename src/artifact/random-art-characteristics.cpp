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
#include <sstream>
#include <string_view>

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
        o_ptr->curse_flags.set(CurseTraitType::PERMA_CURSE);

    if (one_in_(3))
        o_ptr->art_flags.set(TR_TY_CURSE);

    if (one_in_(2))
        o_ptr->art_flags.set(TR_AGGRAVATE);

    if (one_in_(3))
        o_ptr->art_flags.set(TR_DRAIN_EXP);

    if (one_in_(6))
        o_ptr->art_flags.set(TR_ADD_L_CURSE);

    if (one_in_(9))
        o_ptr->art_flags.set(TR_ADD_H_CURSE);

    if (one_in_(9))
        o_ptr->art_flags.set(TR_DRAIN_HP);

    if (one_in_(9))
        o_ptr->art_flags.set(TR_DRAIN_MANA);

    if (one_in_(2))
        o_ptr->art_flags.set(TR_TELEPORT);
    else if (one_in_(3))
        o_ptr->art_flags.set(TR_NO_TELE);
}

/*!
 * @brief ランダムアーティファクト生成中、対象のオブジェクトを呪いのアーティファクトにする経過処理。/ generation process of cursed artifact.
 * @details pval、AC、命中、ダメージが正の場合、符号反転の上1d4だけ悪化させ、重い呪い、呪いフラグを必ず付加。
 * 祝福を無効。確率に応じて、永遠の呪い、太古の怨念、経験値吸収、弱い呪いの継続的付加、強い呪いの継続的付加、HP吸収の呪い、
 * MP吸収の呪い、乱テレポート、反テレポート、反魔法をつける。
 * @attention プレイヤーの職業依存処理あり。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void curse_artifact(PlayerType *player_ptr, object_type *o_ptr)
{
    pval_subtraction(o_ptr);
    o_ptr->curse_flags.set({ CurseTraitType::HEAVY_CURSE, CurseTraitType::CURSED });
    o_ptr->art_flags.reset(TR_BLESSED);
    add_negative_flags(o_ptr);
    if ((player_ptr->pclass != PlayerClassType::WARRIOR) && (player_ptr->pclass != PlayerClassType::ARCHER) && (player_ptr->pclass != PlayerClassType::CAVALRY)
        && (player_ptr->pclass != PlayerClassType::BERSERKER) && (player_ptr->pclass != PlayerClassType::SMITH) && one_in_(3))
        o_ptr->art_flags.set(TR_NO_MAGIC);
}

/*!
 * @brief ランダムアーティファクトの名前リストをオブジェクト種別と生成パワーに応じて選択する
 * @param armour 防具かどうか
 * @param power 生成パワー
 * @return ファイル名
 * @detail ss << tmp_grade; と直接呼ぶとC4866警告が出るので、別変数で受けて抑制中.
 */
static std::string get_random_art_filename(const bool armour, const int power)
{
    const std::string_view prefix(armour ? "a_" : "w_");
    constexpr std::string_view suffix(_("_j.txt", ".txt"));
    std::string_view tmp_grade;
    switch (power) {
    case 0:
        tmp_grade = "cursed";
        break;
    case 1:
        tmp_grade = "low";
        break;
    case 2:
        tmp_grade = "med";
        break;
    default:
        tmp_grade = "high";
    }

    std::stringstream ss;
    const auto &grade = tmp_grade;
    ss << prefix << grade << suffix;
    return ss.str();
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

    auto filename = get_random_art_filename(armour, power);
    (void)get_rnd_line(filename.c_str(), o_ptr->artifact_bias, return_name);
#ifdef JP
    if (return_name[0] == 0)
        get_table_name(return_name);
#endif
}

/*対邪平均ダメージの計算処理*/
static HIT_POINT calc_arm_avgdamage(PlayerType *player_ptr, object_type *o_ptr)
{
    auto flgs = object_flags(o_ptr);
    HIT_POINT base, forced, vorpal;
    HIT_POINT s_evil = forced = vorpal = 0;
    HIT_POINT dam = base = (o_ptr->dd * o_ptr->ds + o_ptr->dd) / 2;
    if (flgs.has(TR_KILL_EVIL)) {
        dam = s_evil = dam * 7 / 2;
    } else if (flgs.has_not(TR_KILL_EVIL) && flgs.has(TR_SLAY_EVIL)) {
        dam = s_evil = dam * 2;
    } else
        s_evil = dam;

    if (flgs.has(TR_FORCE_WEAPON)) {
        dam = forced = dam * 3 / 2 + (o_ptr->dd * o_ptr->ds + o_ptr->dd);
    } else
        forced = dam;

    if (flgs.has(TR_VORPAL)) {
        dam = vorpal = dam * 11 / 9;
    } else
        vorpal = dam;

    dam = dam + o_ptr->to_d;
    msg_format_wizard(player_ptr, CHEAT_OBJECT, "素:%d> 対邪:%d> 理力:%d> 切:%d> 最終:%d", base, s_evil, forced, vorpal, dam);
    return dam;
}

bool has_extreme_damage_rate(PlayerType *player_ptr, object_type *o_ptr)
{
    auto flgs = object_flags(o_ptr);
    if (flgs.has(TR_VAMPIRIC)) {
        if (flgs.has(TR_BLOWS) && (o_ptr->pval == 1) && (calc_arm_avgdamage(player_ptr, o_ptr) > 52)) {
            return true;
        }

        if (flgs.has(TR_BLOWS) && (o_ptr->pval == 2) && (calc_arm_avgdamage(player_ptr, o_ptr) > 43)) {
            return true;
        }

        if (flgs.has(TR_BLOWS) && (o_ptr->pval == 3) && (calc_arm_avgdamage(player_ptr, o_ptr) > 33)) {
            return true;
        }

        if (calc_arm_avgdamage(player_ptr, o_ptr) > 63) {
            return true;
        }

        return false;
    }

    if (flgs.has(TR_BLOWS) && (o_ptr->pval == 1) && (calc_arm_avgdamage(player_ptr, o_ptr) > 65)) {
        return true;
    }

    if (flgs.has(TR_BLOWS) && (o_ptr->pval == 2) && (calc_arm_avgdamage(player_ptr, o_ptr) > 52)) {
        return true;
    }

    if (flgs.has(TR_BLOWS) && (o_ptr->pval == 3) && (calc_arm_avgdamage(player_ptr, o_ptr) > 40)) {
        return true;
    }

    if (calc_arm_avgdamage(player_ptr, o_ptr) > 75) {
        return true;
    }

    return false;
}
