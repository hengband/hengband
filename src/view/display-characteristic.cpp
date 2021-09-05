/*!
 * @brief キャラクタの特性を表示する
 * @date 2020/02/25
 * @author Hourier
 * @details
 * Arranged by iks 2021/04/23
 */

#include "display-characteristic.h"
#include "flavor/flavor-util.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object/object-flags.h"
#include "perception/object-perception.h"
#include "player/permanent-resistances.h"
#include "player/race-resistances.h"
#include "player/temporary-resistances.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"
#include <array>
#include <string>
#include <unordered_map>

struct all_player_flags {
    TrFlags player_flags{};
    TrFlags tim_player_flags{};
    TrFlags player_imm{};
    TrFlags tim_player_imm{};
    TrFlags player_vuln{};
    TrFlags known_obj_imm{};
    TrFlags riding_flags{};
    TrFlags riding_negative_flags{};
};

/*!
 * @brief 特徴状況構造体
 */
struct char_stat {
    bool has_imm{}; //!< 免疫を持つ
    bool has_res{}; //!< 耐性を持つ
    bool has_tim{}; //!< 一時付加されている
    bool has_vul{}; //!< 弱点を持つ
    bool has_rid{}; //!< 乗馬に関係する
    std::vector<std::string> syms; //!< 表示するシンボル
};

/*!
 * @brief 上位の属性フラグへの変換辞書
 */
static std::unordered_map<tr_type, tr_type> flag_to_greater_flag = {
    { TR_RES_ACID, TR_IM_ACID },
    { TR_RES_COLD, TR_IM_COLD },
    { TR_RES_ELEC, TR_IM_ELEC },
    { TR_RES_FIRE, TR_IM_FIRE },
    { TR_RES_DARK, TR_IM_DARK },
    { TR_SLAY_EVIL, TR_KILL_EVIL },
    { TR_SLAY_GOOD, TR_KILL_GOOD },
    { TR_SLAY_HUMAN, TR_KILL_HUMAN },
    { TR_SLAY_ANIMAL, TR_KILL_ANIMAL },
    { TR_SLAY_DRAGON, TR_KILL_DRAGON },
    { TR_SLAY_ORC, TR_KILL_ORC },
    { TR_SLAY_TROLL, TR_KILL_TROLL },
    { TR_SLAY_GIANT, TR_KILL_GIANT },
    { TR_SLAY_DEMON, TR_KILL_DEMON },
    { TR_SLAY_UNDEAD, TR_KILL_UNDEAD },
};

/*!
 * @brief 下位の属性フラグへの変換辞書
 */
static std::unordered_map<tr_type, tr_type> flag_to_lesser_flag = {
    { TR_RES_ACID, TR_VUL_ACID },
    { TR_RES_COLD, TR_VUL_COLD },
    { TR_RES_ELEC, TR_VUL_ELEC },
    { TR_RES_FIRE, TR_VUL_FIRE },
    { TR_RES_LITE, TR_VUL_LITE },
};

/*!
 * pbrief 光源扱いするフラグ一覧
 */
static std::array<tr_type, 6> lite_flags = {
    TR_LITE_1,
    TR_LITE_2,
    TR_LITE_3,
    TR_LITE_M1,
    TR_LITE_M2,
    TR_LITE_M3,
};

/*!
 * @brief 装備品の呪い状況文字列を作成する
 * @param creature_ptr プレイヤー情報への参照ポインタ
 * @param flag 判定する特性フラグ
 * @param f プレイヤーの特性情報への参照ポインタ
 * @param mode 参照モード(DP_WP)
 * @param char_stat その行の特性の状況(参照渡し)
 * その行の表示色用の判定も行う
 */
static void process_cursed_equipment_characteristics(player_type *creature_ptr, uint16_t mode, char_stat &char_stat)
{
    int max_i = (mode & DP_WP) ? INVEN_BOW + 1 : INVEN_TOTAL;
    for (int i = INVEN_MAIN_HAND; i < max_i; i++) {
        auto *o_ptr = &creature_ptr->inventory_list[i];
        auto is_known = o_ptr->is_known();
        auto is_sensed = is_known || o_ptr->ident & IDENT_SENSE;
        auto flags = object_flags_known(o_ptr);

        if (has_flag(flags, TR_ADD_L_CURSE) || has_flag(flags, TR_ADD_H_CURSE)) {
            if (is_known) {
                char_stat.syms.emplace_back("+");
                char_stat.has_res = true;
                continue;
            }
        }

        if (o_ptr->curse_flags.has(TRC::PERMA_CURSE)) {
            if (is_known) {
                char_stat.syms.emplace_back("*");
                char_stat.has_imm = true;
                continue;
            }
        }

        if (o_ptr->curse_flags.has_any_of({ TRC::CURSED, TRC::HEAVY_CURSE, TRC::PERMA_CURSE })) {
            if (is_sensed) {
                char_stat.syms.emplace_back("+");
                char_stat.has_res = true;
                continue;
            }
        }

        char_stat.syms.emplace_back(".");
    }

    char_stat.syms.emplace_back(".");
}

/*!
 * @brief 装備品の光源状況文字列を作成する
 * @param creature_ptr プレイヤー情報への参照ポインタ
 * @param flag 判定する特性フラグ
 * @param f プレイヤーの特性情報への参照ポインタ
 * @param mode 参照モード(DP_WP)
 * @param char_stat その行の特性の状況(参照渡し)
 * @details
 * その行の表示色用の判定も行う
 */
static void process_light_equipment_characteristics(player_type *creature_ptr, all_player_flags *f, uint16_t mode, char_stat &char_stat)
{
    int max_i = (mode & DP_WP) ? INVEN_BOW + 1 : INVEN_TOTAL;
    for (int i = INVEN_MAIN_HAND; i < max_i; i++) {
        auto *o_ptr = &creature_ptr->inventory_list[i];
        auto flags = object_flags_known(o_ptr);

        auto b = false;
        for (auto flg : lite_flags) {
            if (has_flag(flags, flg)) {
                b = true;
                break;
            }
        }
        if (b) {
            char_stat.syms.emplace_back("+");
            char_stat.has_res = true;
            continue;
        }

        char_stat.syms.emplace_back(".");
    }

    for (auto flg : lite_flags) {
        if (has_flag(f->tim_player_flags, flg)) {
            char_stat.syms.emplace_back("#");
            char_stat.has_tim = true;
            return;
        }
        if (has_flag(f->player_flags, flg)) {
            char_stat.syms.emplace_back("+");
            char_stat.has_res = true;
            return;
        }
    }

    char_stat.syms.emplace_back(".");
}

/*!
 * @brief 装備品の状況文字列を作成する
 * @param creature_ptr プレイヤー情報への参照ポインタ
 * @param flag 判定する特性フラグ
 * @param f プレイヤーの特性情報への参照ポインタ
 * @param mode 参照モード(DP_WP)
 * @param char_stat その行の特性の状況(参照渡し)
 * @details
 * その行の表示色用の判定も行う
 */
static void process_inventory_characteristic(player_type *creature_ptr, tr_type flag, all_player_flags *f, uint16_t mode, char_stat &char_stat)
{
    int max_i = (mode & DP_WP) ? INVEN_BOW + 1 : INVEN_TOTAL;
    for (int i = INVEN_MAIN_HAND; i < max_i; i++) {
        auto *o_ptr = &creature_ptr->inventory_list[i];
        auto flags = object_flags_known(o_ptr);

        auto f_imm = flag_to_greater_flag.find(flag);
        if (f_imm != flag_to_greater_flag.end()) {
            if (has_flag(flags, f_imm->second)) {
                char_stat.syms.emplace_back("*");
                char_stat.has_imm = true;
                continue;
            }
        }

        auto b_vul = false;
        auto f_vul = flag_to_lesser_flag.find(flag);
        if (f_vul != flag_to_lesser_flag.end()) {
            if (has_flag(flags, f_vul->second)) {
                char_stat.has_vul = true;
                b_vul = true;
            }
        }

        if (has_flag(flags, flag)) {
            char_stat.syms.emplace_back(b_vul ? "-" : "+");
            char_stat.has_res = true;
            continue;
        }

        char_stat.syms.emplace_back(b_vul ? "v" : ".");
    }

    if (has_flag(f->player_imm, flag) || has_flag(f->tim_player_imm, flag)) {
        char_stat.syms.emplace_back("*");
        char_stat.has_imm = true;
    } else if (has_flag(f->tim_player_flags, flag)) {
        char_stat.syms.emplace_back("#");
        char_stat.has_tim = true;
    } else {
        auto b_vul = has_flag(f->player_vuln, flag);
        if (has_flag(f->player_flags, flag)) {
            char_stat.syms.emplace_back(b_vul ? "-" : "+");
            char_stat.has_res = true;
        } else {
            char_stat.syms.emplace_back(".");
            if (b_vul)
                char_stat.has_vul = true;
        }
    }
}

/*!
 * @brief プレイヤーの特性フラグ一種表示を処理するメインルーチン
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param row コンソール表示位置の左上行
 * @param col コンソール表示位置の左上列
 * @param header コンソール上で表示する特性名
 * @param flag1 参照する特性ID
 * @param f プレイヤーの特性情報構造体
 * @param mode 表示オプション
 */
static void process_one_characteristic(player_type *creature_ptr, TERM_LEN row, TERM_LEN col, std::string_view header, tr_type flag, all_player_flags *f, uint16_t mode)
{
    char_stat char_stat;

    if (has_flag(f->riding_flags, flag))
        char_stat.has_rid = true;
    if (has_flag(f->riding_negative_flags, flag)) {
        char_stat.has_rid = true;
        char_stat.has_vul = true;
    }

    if (mode & DP_LITE)
        process_light_equipment_characteristics(creature_ptr, f, mode, char_stat);
    else if (mode & DP_CURSE)
        process_cursed_equipment_characteristics(creature_ptr, mode, char_stat);
    else
        process_inventory_characteristic(creature_ptr, flag, f, mode, char_stat);

    if (char_stat.has_vul && !char_stat.has_imm && !char_stat.has_res && !char_stat.has_tim)
        char_stat.syms.emplace_back("v");

    auto row_clr = char_stat.has_rid ? TERM_L_GREEN : TERM_WHITE;
    if (!char_stat.has_imm) {
        if (char_stat.has_res && char_stat.has_tim)
            row_clr = char_stat.has_vul ? TERM_YELLOW : row_clr;
        else if (char_stat.has_res || char_stat.has_tim)
            row_clr = char_stat.has_vul ? TERM_ORANGE : row_clr;
        else if (char_stat.has_vul)
            row_clr = TERM_RED;
        else
            row_clr = TERM_L_DARK;
    }

    c_put_str(row_clr, header.data(), row, col);
    col += header.length() + 1;

    for (auto s : char_stat.syms) {
        auto clr = row_clr;
        if (s == "#")
            clr = char_stat.has_vul && !char_stat.has_imm ? TERM_ORANGE : TERM_YELLOW;
        if (s == "-" && !char_stat.has_imm)
            clr = TERM_ORANGE;
        if (s == "." && (!char_stat.has_imm && !char_stat.has_vul))
            clr = TERM_L_DARK;
        c_put_str(clr, s.c_str(), row, col++);
    }
}

/*!
 * @brief プレーヤーの基本耐性を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param display_player_equippy 表示へのコールバック
 * @param f 特性フラグへの参照ポインタ
 */
static void display_basic_resistance_info(
    player_type *creature_ptr, void (*display_player_equippy)(player_type *, TERM_LEN, TERM_LEN, BIT_FLAGS16), all_player_flags *f)
{
    TERM_LEN row = 12;
    TERM_LEN col = 1;
    (*display_player_equippy)(creature_ptr, row - 2, col + 8, 0);
    c_put_str(TERM_WHITE, "abcdefghijkl@", row - 1, col + 8);

    process_one_characteristic(creature_ptr, row++, col, _("耐酸  :", "Acid  :"), TR_RES_ACID, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("耐電撃:", "Elec  :"), TR_RES_ELEC, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("耐火炎:", "Fire  :"), TR_RES_FIRE, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("耐冷気:", "Cold  :"), TR_RES_COLD, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("耐毒  :", "Poison:"), TR_RES_POIS, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("耐閃光:", "Light :"), TR_RES_LITE, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("耐暗黒:", "Dark  :"), TR_RES_DARK, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("耐破片:", "Shard :"), TR_RES_SHARDS, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("耐盲目:", "Blind :"), TR_RES_BLIND, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("耐混乱:", "Conf  :"), TR_RES_CONF, f, 0);
}

/*!
 * @brief プレーヤーの上位耐性を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param display_player_equippy 表示へのコールバック
 * @param f 特性フラグへの参照ポインタ
 */
static void display_advanced_resistance_info(
    player_type *creature_ptr, void (*display_player_equippy)(player_type *, TERM_LEN, TERM_LEN, BIT_FLAGS16), all_player_flags *f)
{
    TERM_LEN row = 12;
    TERM_LEN col = 26;
    (*display_player_equippy)(creature_ptr, row - 2, col + 8, 0);
    c_put_str(TERM_WHITE, "abcdefghijkl@", row - 1, col + 8);

    process_one_characteristic(creature_ptr, row++, col, _("耐轟音:", "Sound :"), TR_RES_SOUND, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("耐地獄:", "Nether:"), TR_RES_NETHER, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("耐因混:", "Nexus :"), TR_RES_NEXUS, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("耐カオ:", "Chaos :"), TR_RES_CHAOS, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("耐劣化:", "Disnch:"), TR_RES_DISEN, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("耐時間:", "Time  :"), TR_RES_TIME, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("耐水  :", "Water :"), TR_RES_WATER, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("耐恐怖:", "Fear  :"), TR_RES_FEAR, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("耐呪力:", "ResCur:"), TR_RES_CURSE, f, 0);
}

/*!
 * @brief プレーヤーのその他耐性を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param display_player_equippy 表示へのコールバック
 * @param f 特性フラグへの参照ポインタ
 */
static void display_other_resistance_info(
    player_type *creature_ptr, void (*display_player_equippy)(player_type *, TERM_LEN, TERM_LEN, BIT_FLAGS16), all_player_flags *f)
{
    TERM_LEN row = 12;
    TERM_LEN col = 51;
    (*display_player_equippy)(creature_ptr, row - 2, col + 12, 0);
    c_put_str(TERM_WHITE, "abcdefghijkl@", row - 1, col + 12);

    process_one_characteristic(creature_ptr, row++, col, _("加速      :", "Speed     :"), TR_SPEED, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("耐麻痺    :", "FreeAction:"), TR_FREE_ACT, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("透明体視認:", "SeeInvisi.:"), TR_SEE_INVIS, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("経験値保持:", "Hold Exp  :"), TR_HOLD_EXP, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("警告      :", "Warning   :"), TR_WARNING, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("遅消化    :", "SlowDigest:"), TR_SLOW_DIGEST, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("急回復    :", "Regene.   :"), TR_REGEN, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("浮遊      :", "Levitation:"), TR_LEVITATION, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("反射      :", "Reflct    :"), TR_REFLECT, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("呪い      :", "Cursed    :"), TR_FLAG_MAX, f, DP_CURSE);
}

/*!
 * @brief プレイヤーの特性フラグを集計する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @todo 将来的には装備系とまとめたいが、乗馬による特性変化や一時能力変化等の扱いがあるので据え置き。
 */
 all_player_flags get_player_state_flags(player_type *creature_ptr)
{
    all_player_flags f;
    player_flags(creature_ptr, f.player_flags);
    tim_player_flags(creature_ptr, f.tim_player_flags);
    player_immunity(creature_ptr, f.player_imm);
    tim_player_immunity(creature_ptr, f.tim_player_imm);
    known_obj_immunity(creature_ptr, f.known_obj_imm);
    player_vulnerability_flags(creature_ptr, f.player_vuln);
    riding_flags(creature_ptr, f.riding_flags, f.riding_negative_flags);
    return f;
}

/*!
 * @brief プレイヤーの特性フラグ一覧表示1
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param display_player_equippy 表示へのコールバック
 * Special display, part 1
 */
void display_player_flag_info_1(player_type *creature_ptr, void (*display_player_equippy)(player_type *, TERM_LEN, TERM_LEN, BIT_FLAGS16))
{
    all_player_flags f = get_player_state_flags(creature_ptr);

    display_basic_resistance_info(creature_ptr, display_player_equippy, &f);
    display_advanced_resistance_info(creature_ptr, display_player_equippy, &f);
    display_other_resistance_info(creature_ptr, display_player_equippy, &f);
}

/*!
 * @brief スレイ系の特性フラグを表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param display_player_equippy 表示へのコールバック
 * @param f 特性フラグへの参照ポインタ
 */
static void display_slay_info(player_type *creature_ptr, void (*display_player_equippy)(player_type *, TERM_LEN, TERM_LEN, BIT_FLAGS16), all_player_flags *f)
{
    TERM_LEN row = 3;
    TERM_LEN col = 1;
    (*display_player_equippy)(creature_ptr, row - 2, col + 14, DP_WP);
    c_put_str(TERM_WHITE, "abc@", row - 1, col + 14);

    process_one_characteristic(creature_ptr, row++, col, _("邪悪    倍打:", "Slay Evil   :"), TR_SLAY_EVIL, f, DP_WP);
    process_one_characteristic(creature_ptr, row++, col, _("善良    倍打:", "Slay Good   :"), TR_SLAY_GOOD, f, DP_WP);
    process_one_characteristic(creature_ptr, row++, col, _("不死    倍打:", "Slay Undead :"), TR_SLAY_UNDEAD, f, DP_WP);
    process_one_characteristic(creature_ptr, row++, col, _("デーモン倍打:", "Slay Demon  :"), TR_SLAY_DEMON, f, DP_WP);
    process_one_characteristic(creature_ptr, row++, col, _("ドラゴン倍打:", "Slay Dragon :"), TR_SLAY_DRAGON, f, DP_WP);
    process_one_characteristic(creature_ptr, row++, col, _("人間    倍打:", "Slay Human  :"), TR_SLAY_HUMAN, f, DP_WP);
    process_one_characteristic(creature_ptr, row++, col, _("動物    倍打:", "Slay Animal :"), TR_SLAY_ANIMAL, f, DP_WP);
    process_one_characteristic(creature_ptr, row++, col, _("オーク  倍打:", "Slay Orc    :"), TR_SLAY_ORC, f, DP_WP);
    process_one_characteristic(creature_ptr, row++, col, _("トロル  倍打:", "Slay Troll  :"), TR_SLAY_TROLL, f, DP_WP);
    process_one_characteristic(creature_ptr, row++, col, _("巨人    倍打:", "Slay Giant  :"), TR_SLAY_GIANT, f, DP_WP);
}

/*!
 * @brief ブランド系の特性フラグを表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param display_player_equippy 表示へのコールバック
 * @param f 特性フラグへの参照ポインタ
 */
static void display_brand_info(player_type *creature_ptr, void (*display_player_equippy)(player_type *, TERM_LEN, TERM_LEN, BIT_FLAGS16), all_player_flags *f)
{
    TERM_LEN row = 3;
    TERM_LEN col = 1;
    (*display_player_equippy)(creature_ptr, row - 2, col + 14, DP_WP);
    c_put_str(TERM_WHITE, "abc@", row - 1, col + 14);
    process_one_characteristic(creature_ptr, row++, col, _("溶解        :", "Acid Brand  :"), TR_BRAND_ACID, f, DP_WP);
    process_one_characteristic(creature_ptr, row++, col, _("電撃        :", "Elec Brand  :"), TR_BRAND_ELEC, f, DP_WP);
    process_one_characteristic(creature_ptr, row++, col, _("焼棄        :", "Fire Brand  :"), TR_BRAND_FIRE, f, DP_WP);
    process_one_characteristic(creature_ptr, row++, col, _("凍結        :", "Cold Brand  :"), TR_BRAND_COLD, f, DP_WP);
    process_one_characteristic(creature_ptr, row++, col, _("毒殺        :", "Poison Brand:"), TR_BRAND_POIS, f, DP_WP);
    process_one_characteristic(creature_ptr, row++, col, _("切れ味      :", "Sharpness   :"), TR_VORPAL, f, DP_WP);
    process_one_characteristic(creature_ptr, row++, col, _("強撃        :", "Impactive   :"), TR_IMPACT, f, DP_WP);
    process_one_characteristic(creature_ptr, row++, col, _("地震        :", "Quake       :"), TR_EARTHQUAKE, f, DP_WP);
    process_one_characteristic(creature_ptr, row++, col, _("吸血        :", "Vampiric    :"), TR_VAMPIRIC, f, DP_WP);
    process_one_characteristic(creature_ptr, row++, col, _("カオス効果  :", "Chaotic     :"), TR_CHAOTIC, f, DP_WP);
    process_one_characteristic(creature_ptr, row++, col, _("魔術効果    :", "Magic Brand :"), TR_BRAND_MAGIC, f, DP_WP);
    process_one_characteristic(creature_ptr, row++, col, _("理力        :", "Force Weapon:"), TR_FORCE_WEAPON, f, DP_WP);
    process_one_characteristic(creature_ptr, row++, col, _("乗馬        :", "For Riding  :"), TR_RIDING, f, DP_WP);
    process_one_characteristic(creature_ptr, row++, col, _("補助        :", "Supportive  :"), TR_SUPPORTIVE, f, DP_WP);
}

/*!
 * @brief その他の特性フラグを表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param display_player_equippy 表示へのコールバック
 * @param f 特性フラグへの参照ポインタ
 */
static void display_tval_misc_info(
    player_type *creature_ptr, void (*display_player_equippy)(player_type *, TERM_LEN, TERM_LEN, BIT_FLAGS16), all_player_flags *f)
{
    TERM_LEN row = 3;
    TERM_LEN col = 49;
    (*display_player_equippy)(creature_ptr, row - 2, col + 14, 0);
    c_put_str(TERM_WHITE, "abcdefghijkl@", row - 1, col + 14);

    process_one_characteristic(creature_ptr, row++, col, _("追加攻撃    :", "Add Blows   :"), TR_BLOWS, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("採掘        :", "Add Tunnel  :"), TR_TUNNEL, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("赤外線視力  :", "Add Infra   :"), TR_INFRA, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("魔法道具支配:", "Add Device  :"), TR_MAGIC_MASTERY, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("隠密        :", "Add Stealth :"), TR_STEALTH, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("探索        :", "Add Search  :"), TR_SEARCH, f, 0);
    row++;
    process_one_characteristic(creature_ptr, row++, col, _("追加射撃    :", "Extra Shots :"), TR_XTRA_SHOTS, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("投擲        :", "Throw       :"), TR_THROW, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("祝福        :", "Blessed     :"), TR_BLESSED, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("永遠光源    :", "Perm Lite   :"), TR_LITE_1, f, DP_LITE);
    process_one_characteristic(creature_ptr, row++, col, _("消費魔力減少:", "Econom. Mana:"), TR_DEC_MANA, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("魔法難度減少:", "Easy Spell  :"), TR_EASY_SPELL, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("発動        :", "Activate    :"), TR_ACTIVATE, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("反魔法      :", "Anti Magic  :"), TR_NO_MAGIC, f, 0);
}

/*!
 * @brief ESPの特性フラグを表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param display_player_equippy 表示へのコールバック
 * @param f 特性フラグへの参照ポインタ
 */
static void display_esc_info(player_type *creature_ptr, void (*display_player_equippy)(player_type *, TERM_LEN, TERM_LEN, BIT_FLAGS16), all_player_flags *f)
{
    TERM_LEN row = 3;
    TERM_LEN col = 21;
    (*display_player_equippy)(creature_ptr, row - 2, col + 13, 0);
    c_put_str(TERM_WHITE, "abcdefghijkl@", row - 1, col + 13);
    process_one_characteristic(creature_ptr, row++, col, _("テレパシー :", "Telepathy  :"), TR_TELEPATHY, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("邪悪    ESP:", "ESP Evil   :"), TR_ESP_EVIL, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("無生物  ESP:", "ESP Noliv. :"), TR_ESP_NONLIVING, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("善良    ESP:", "ESP Good   :"), TR_ESP_GOOD, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("不死    ESP:", "ESP Undead :"), TR_ESP_UNDEAD, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("デーモンESP:", "ESP Demon  :"), TR_ESP_DEMON, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("ドラゴンESP:", "ESP Dragon :"), TR_ESP_DRAGON, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("人間    ESP:", "ESP Human  :"), TR_ESP_HUMAN, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("動物    ESP:", "ESP Animal :"), TR_ESP_ANIMAL, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("オーク  ESP:", "ESP Orc    :"), TR_ESP_ORC, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("トロル  ESP:", "ESP Troll  :"), TR_ESP_TROLL, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("巨人    ESP:", "ESP Giant  :"), TR_ESP_GIANT, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("ユニークESP:", "ESP Unique :"), TR_ESP_UNIQUE, f, 0);
}


/*!
 * @brief ESP/能力維持の特性フラグを表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param display_player_equippy 表示へのコールバック
 * @param f 特性フラグへの参照ポインタ
 */
static void display_stustain_aura_info(
    player_type *creature_ptr, void (*display_player_equippy)(player_type *, TERM_LEN, TERM_LEN, BIT_FLAGS16), all_player_flags *f)
{
    TERM_LEN row = 3;
    TERM_LEN col = 21;
    (*display_player_equippy)(creature_ptr, row - 2, col + 12, 0);
    c_put_str(TERM_WHITE, "abcdefghijkl@", row - 1, col + 12);

    process_one_characteristic(creature_ptr, row++, col, _("腕力  維持:", "Sust Str  :"), TR_SUST_STR, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("知力  維持:", "Sust Int  :"), TR_SUST_INT, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("賢さ  維持:", "Sust Wis  :"), TR_SUST_WIS, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("器用  維持:", "Sust Dex  :"), TR_SUST_DEX, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("耐久力維持:", "Sust Con  :"), TR_SUST_CON, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("魅力  維持:", "Sust Chr  :"), TR_SUST_CHR, f, 0);
    row++;
    process_one_characteristic(creature_ptr, row++, col, _("火炎オーラ:", "Aura Fire :"), TR_SH_FIRE, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("電気オーラ:", "Aura Elec :"), TR_SH_ELEC, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("冷気オーラ:", "Aura Cold :"), TR_SH_COLD, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("射撃無効  :", "Invl Arrow:"), TR_INVULN_ARROW, f, 0);
}

/*!
 * @brief その他の特性フラグを表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param display_player_equippy 表示へのコールバック
 * @param f 特性フラグへの参照ポインタ
 */
static void display_curse_info(player_type *creature_ptr, void (*display_player_equippy)(player_type *, TERM_LEN, TERM_LEN, BIT_FLAGS16), all_player_flags *f)
{
    TERM_LEN row = 3;
    TERM_LEN col = 49;
    (*display_player_equippy)(creature_ptr, row - 2, col + 14, 0);
    c_put_str(TERM_WHITE, "abcdefghijkl@", row - 1, col + 14);

    process_one_characteristic(creature_ptr, row++, col, _("太古の怨念  :", "TY Curse    :"), TR_TY_CURSE, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("反感        :", "Aggravate   :"), TR_AGGRAVATE, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("弱い呪い増殖:", "Add Curse   :"), TR_ADD_L_CURSE, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("強い呪い増殖:", "AddHeavyCur.:"), TR_ADD_H_CURSE, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("経験値減少  :", "Drain Exp   :"), TR_DRAIN_EXP, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("体力吸収    :", "Drain HP    :"), TR_DRAIN_HP, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("魔力吸収    :", "Drain Mana  :"), TR_DRAIN_MANA, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("反テレポート:", "No Teleport :"), TR_NO_TELE, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("魔法防御半減:", "Vuln. Magic :"), TR_DOWN_SAVING, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("速消化      :", "Fast Digest :"), TR_FAST_DIGEST, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("回復力低下  :", "Slow Regen  :"), TR_SLOW_REGEN, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("恐怖発生    :", "Cowardice   :"), TR_COWARDICE, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("魔法難度増加:", "Dfclt. Spell:"), TR_HARD_SPELL, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("打撃力低下  :", "Low Melee   :"), TR_LOW_MELEE, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("AC低下      :", "Low AC      :"), TR_LOW_AC, f, 0);
    process_one_characteristic(creature_ptr, row++, col, _("狂戦士化    :", "Berserk Rage:"), TR_BERS_RAGE, f, 0);
}

/*!
 * @brief プレイヤーの特性フラグ一覧表示2
 * @param creature_ptr プレーヤーへの参照ポインタ
 * Special display, part 2
 */
void display_player_flag_info_2(player_type *creature_ptr, void (*display_player_equippy)(player_type *, TERM_LEN, TERM_LEN, BIT_FLAGS16))
{
    /* Extract flags and store */
    all_player_flags f = get_player_state_flags(creature_ptr);

    display_slay_info(creature_ptr, display_player_equippy, &f);
    display_esc_info(creature_ptr, display_player_equippy, &f);
    display_tval_misc_info(creature_ptr, display_player_equippy, &f);
}

/*!
 * @brief プレイヤーの特性フラグ一覧表示3
 * @param creature_ptr プレーヤーへの参照ポインタ
 * Special display, part 3
 */
void display_player_flag_info_3(player_type *creature_ptr, void (*display_player_equippy)(player_type *, TERM_LEN, TERM_LEN, BIT_FLAGS16))
{
    /* Extract flags and store */
    all_player_flags f = get_player_state_flags(creature_ptr);

    display_brand_info(creature_ptr, display_player_equippy, &f);
    display_stustain_aura_info(creature_ptr, display_player_equippy, &f);
    display_curse_info(creature_ptr, display_player_equippy, &f);
}
