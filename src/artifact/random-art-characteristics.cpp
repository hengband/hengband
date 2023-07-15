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
#include "player-base/player-class.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "wizard/wizard-messages.h"
#include <sstream>
#include <string_view>

static void pval_subtraction(ItemEntity *o_ptr)
{
    if (o_ptr->pval > 0) {
        o_ptr->pval = 0 - (o_ptr->pval + randint1(4));
    }

    if (o_ptr->to_a > 0) {
        o_ptr->to_a = 0 - (o_ptr->to_a + randint1(4));
    }

    if (o_ptr->to_h > 0) {
        o_ptr->to_h = 0 - (o_ptr->to_h + randint1(4));
    }

    if (o_ptr->to_d > 0) {
        o_ptr->to_d = 0 - (o_ptr->to_d + randint1(4));
    }
}

static void add_negative_flags(ItemEntity *o_ptr)
{
    if (one_in_(4)) {
        o_ptr->curse_flags.set(CurseTraitType::PERMA_CURSE);
    }

    if (one_in_(3)) {
        o_ptr->art_flags.set(TR_TY_CURSE);
    }

    if (one_in_(2)) {
        o_ptr->art_flags.set(TR_AGGRAVATE);
    }

    if (one_in_(3)) {
        o_ptr->art_flags.set(TR_DRAIN_EXP);
    }

    if (one_in_(6)) {
        o_ptr->art_flags.set(TR_ADD_L_CURSE);
    }

    if (one_in_(9)) {
        o_ptr->art_flags.set(TR_ADD_H_CURSE);
    }

    if (one_in_(9)) {
        o_ptr->art_flags.set(TR_DRAIN_HP);
    }

    if (one_in_(9)) {
        o_ptr->art_flags.set(TR_DRAIN_MANA);
    }

    if (one_in_(2)) {
        o_ptr->art_flags.set(TR_TELEPORT);
    } else if (one_in_(3)) {
        o_ptr->art_flags.set(TR_NO_TELE);
    }
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
void curse_artifact(PlayerType *player_ptr, ItemEntity *o_ptr)
{
    pval_subtraction(o_ptr);
    o_ptr->curse_flags.set({ CurseTraitType::HEAVY_CURSE, CurseTraitType::CURSED });
    o_ptr->art_flags.reset(TR_BLESSED);
    add_negative_flags(o_ptr);
    if (!PlayerClass(player_ptr).is_soldier() && one_in_(3)) {
        o_ptr->art_flags.set(TR_NO_MAGIC);
    }
}

/*!
 * @brief ランダムアーティファクトの名前リストをオブジェクト種別と生成パワーに応じて選択する
 * @param armour 防具かどうか
 * @param power 生成パワー
 * @return ファイル名
 */
static std::string get_random_art_filename(const bool armour, const int power)
{
    std::stringstream ss;
    ss << (armour ? "a_" : "w_");
    switch (power) {
    case 0:
        ss << "cursed";
        break;
    case 1:
        ss << "low";
        break;
    case 2:
        ss << "med";
        break;
    default:
        ss << "high";
    }

    ss << _("_j.txt", ".txt");
    return ss.str();
}

/*!
 * @brief ランダムアーティファクト生成中、対象のオブジェクトに名前を与える.
 * @param o_ptr 処理中のアイテム参照ポインタ
 * @param armour 対象のオブジェクトが防具が否か
 * @param power 銘の基準となるオブジェクトの価値レベル(0=呪い、1=低位、2=中位、3以上=高位)
 * @return ランダムアーティファクト名
 * @details 確率によって、シンダリン銘、漢字銘 (anameからランダム)、固定名のいずれか一つが与えられる.
 * シンダリン銘：10%、漢字銘18%、固定銘72% (固定銘が尽きていたら漢字銘になることもある).
 */
std::string get_random_name(const ItemEntity &item, bool armour, int power)
{
    const auto prob = randint1(100);
    constexpr auto chance_sindarin = 10;
    if (prob <= chance_sindarin) {
        return get_table_sindarin();
    }

    constexpr auto chance_table = 20;
    if (prob <= chance_table) {
        return get_table_name();
    }

    auto filename = get_random_art_filename(armour, power);
    auto random_artifact_name = get_random_line(filename.data(), item.artifact_bias);
#ifdef JP
    if (random_artifact_name.has_value()) {
        return random_artifact_name.value();
    }

    return get_table_name();
#else
    return random_artifact_name.value();
#endif
}

/*対邪平均ダメージの計算処理*/
static int calc_arm_avgdamage(PlayerType *player_ptr, ItemEntity *o_ptr)
{
    auto flags = object_flags(o_ptr);
    int base, forced, vorpal;
    int s_evil = forced = vorpal = 0;
    int dam = base = (o_ptr->dd * o_ptr->ds + o_ptr->dd) / 2;
    if (flags.has(TR_KILL_EVIL)) {
        dam = s_evil = dam * 7 / 2;
    } else if (flags.has_not(TR_KILL_EVIL) && flags.has(TR_SLAY_EVIL)) {
        dam = s_evil = dam * 2;
    } else {
        s_evil = dam;
    }

    if (flags.has(TR_FORCE_WEAPON)) {
        dam = forced = dam * 3 / 2 + (o_ptr->dd * o_ptr->ds + o_ptr->dd);
    } else {
        forced = dam;
    }

    if (flags.has(TR_VORPAL)) {
        dam = vorpal = dam * 11 / 9;
    } else {
        vorpal = dam;
    }

    dam = dam + o_ptr->to_d;
    constexpr auto fmt = _("素:%d> 対邪:%d> 理力:%d> 切:%d> 最終:%d", "Normal:%d> Evil:%d> Force:%d> Vorpal:%d> Total:%d");
    msg_format_wizard(player_ptr, CHEAT_OBJECT, fmt, base, s_evil, forced, vorpal, dam);
    return dam;
}

bool has_extreme_damage_rate(PlayerType *player_ptr, ItemEntity *o_ptr)
{
    auto flags = object_flags(o_ptr);
    if (flags.has(TR_VAMPIRIC)) {
        if (flags.has(TR_BLOWS) && (o_ptr->pval == 1) && (calc_arm_avgdamage(player_ptr, o_ptr) > 52)) {
            return true;
        }

        if (flags.has(TR_BLOWS) && (o_ptr->pval == 2) && (calc_arm_avgdamage(player_ptr, o_ptr) > 43)) {
            return true;
        }

        if (flags.has(TR_BLOWS) && (o_ptr->pval == 3) && (calc_arm_avgdamage(player_ptr, o_ptr) > 33)) {
            return true;
        }

        if (calc_arm_avgdamage(player_ptr, o_ptr) > 63) {
            return true;
        }

        return false;
    }

    if (flags.has(TR_BLOWS) && (o_ptr->pval == 1) && (calc_arm_avgdamage(player_ptr, o_ptr) > 65)) {
        return true;
    }

    if (flags.has(TR_BLOWS) && (o_ptr->pval == 2) && (calc_arm_avgdamage(player_ptr, o_ptr) > 52)) {
        return true;
    }

    if (flags.has(TR_BLOWS) && (o_ptr->pval == 3) && (calc_arm_avgdamage(player_ptr, o_ptr) > 40)) {
        return true;
    }

    if (calc_arm_avgdamage(player_ptr, o_ptr) > 75) {
        return true;
    }

    return false;
}
