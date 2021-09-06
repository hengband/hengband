#pragma once

#include <optional>
#include <unordered_map>
#include <vector>

#include "object-enchant/tr-flags.h"
#include "object-enchant/tr-types.h"
#include "player-ability/player-ability-types.h"
#include "player/player-class-types.h"
#include "player/player-race-types.h"
#include "system/angband.h"

/*
 * Constant for kinds of mimic
 */
#define MIMIC_NONE 0
#define MIMIC_DEMON 1
#define MIMIC_DEMON_LORD 2
#define MIMIC_VAMPIRE 3

#define MIMIC_IS_NONLIVING 0x00000001
#define MIMIC_IS_DEMON 0x00000002
#define MIMIC_IS_UNDEAD 0x00000004

/*!
 * @brief プレイヤー種族の生命形態
 */
enum class PlayerRaceLife {
    LIVING = 0, //!< 生きている
    UNDEAD = 1, //!< 不死
    DEMON = 2, //!< 悪魔
    NONLIVING = 3, //!< 生きてない
    MAX
};

/*!
 * @brief プレイヤー種族の食料形態
 */
enum class PlayerRaceFood {
    RATION = 0, //!< 食料
    WATER = 1, //!< 水
    OIL = 2, //!< 油
    BLOOD = 3, //!< 血
    MANA = 4, //!< 魔力
    CORPSE = 5, //!< 死体(捧げる)
    MAX
};

/*!
 * @brief プレイヤー種族の条件設定構造体
 */
struct player_race_condition {
    tr_type type{};
    PLAYER_LEVEL level{};
    std::optional<player_class_type> pclass{};
    bool not_class{};

    player_race_condition(tr_type t, PLAYER_LEVEL l = 1, const std::optional<player_class_type> &c = std::nullopt, bool nc = false)
        : type(t)
        , level(l)
        , pclass(c)
        , not_class(nc)
    {
    }
};

/*!
 * @brief プレイヤー種族構造体 / Player racial info
 */
struct player_race {
    concptr title{}; //!< 種族名 / Title of race
#ifdef JP
    concptr E_title{}; //!< 英語種族名
#endif
    concptr symbol{}; //!< 種族シンボル(救援召喚) / Race symbols
    int16_t r_adj[A_MAX]{}; //!< 能力値ボーナス / Racial stat bonuses

    int16_t r_dis{}; //!< 解除 / disarming
    int16_t r_dev{}; //!< 魔道具使用 /magic devices
    int16_t r_sav{}; //!< 魔法防御 / saving throw
    int16_t r_stl{}; //!< 隠密 / stealth
    int16_t r_srh{}; //!< 探索 / search ability
    int16_t r_fos{}; //!< 知覚 / search frequency
    int16_t r_thn{}; //!< 打撃修正(命中) / combat (normal)
    int16_t r_thb{}; //!< 射撃修正(命中) / combat (shooting)

    byte r_mhp{}; //!< ヒットダイス /  Race hit-dice modifier
    byte r_exp{}; //!< 経験値修正 /Race experience factor

    byte b_age{}; //!< 年齢最小値 / base age
    byte m_age{}; //!< 年齢加算範囲 / mod age

    byte m_b_ht{}; //!< 身長最小値(男) / base height (males)
    byte m_m_ht{}; //!< 身長加算範囲(男) / mod height (males)
    byte m_b_wt{}; //!< 体重最小値(男) / base weight (males)
    byte m_m_wt{}; //!< 体重加算範囲(男) / mod weight (males)

    byte f_b_ht{}; //!< 身長最小値(女) / base height (females)
    byte f_m_ht{}; //!< 身長加算範囲(女) / mod height (females)
    byte f_b_wt{}; //!< 体重最小値(女) / base weight (females)
    byte f_m_wt{}; //!< 体重加算範囲(女) / mod weight (females)

    byte infra{}; //!< 赤外線視力 / Infra-vision range

    uint32_t choice{}; //!< 似つかわしい職業(ミミック時はミミック種族属性) / Legal class choices
    PlayerRaceLife life{}; //!< 生命の形態
    PlayerRaceFood food{}; //!< 食料の形態

    std::vector<player_race_condition> extra_flags;
};

extern const player_race *rp_ptr;

struct player_type;
SYMBOL_CODE get_summon_symbol_from_player(player_type *creature_ptr);
bool is_specific_player_race(player_type *creature_ptr, player_race_type prace);
bool player_race_has_flag(player_type *creature_ptr, tr_type flag, bool base_race = false);
void add_player_race_flags(player_type *creature_ptr, TrFlags &flags, bool base_race = false);
PlayerRaceLife player_race_life(player_type *creature_ptr, bool base_race = false);
PlayerRaceFood player_race_food(player_type *creature_ptr, bool base_race = false);
