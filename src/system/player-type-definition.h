#pragma once

#include "mutation/mutation-flag-types.h"
#include "object-enchant/trc-types.h"
#include "player-ability/player-ability-types.h"
#include "player-info/class-specific-data.h"
#include "player-info/class-types.h"
#include "player-info/race-types.h"
#include "player/player-personality-types.h"
#include "player/player-sex.h"
#include "system/angband.h"
#include "system/system-variables.h"
#include "util/flag-group.h"
#include <array>
#include <map>
#include <string>

enum class ItemKindType : short;
enum class PlayerSkillKindType;
enum class MimicKindType;
enum class MonsterAbilityType;
enum class MonsterRaceId : int16_t;
enum class Virtue : short;

class FloorType;
class ItemEntity;
class TimedEffects;
class PlayerType {
public:
    PlayerType();
    bool is_true_winner() const;

    FloorType *current_floor_ptr{};
    POSITION oldpy{}; /* Previous player location -KMW- */
    POSITION oldpx{}; /* Previous player location -KMW- */

    player_sex psex{}; /* Sex index */
    PlayerRaceType prace{}; /* Race index */
    PlayerClassType pclass{}; /* Class index */
    player_personality_type ppersonality{}; /* Personality index */
    int16_t realm1{}; /* First magic realm */
    int16_t realm2{}; /* Second magic realm */
    int16_t element{}; //!< 元素使い領域番号 / Elementalist system index

    DICE_SID hitdie{}; /* Hit dice (sides) */
    uint16_t expfact{}; /* Experience factor
                         * Note: was byte, causing overflow for Amberite
                         * characters (such as Amberite Paladins)
                         */

    int16_t age{}; /* Characters age */
    int16_t ht{}; /* Height */
    int16_t wt{}; /* Weight */
    int16_t sc{}; /* Social Class */

    PRICE au{}; /* Current Gold */

    EXP max_max_exp{}; /* Max max experience (only to calculate score) */
    EXP max_exp{}; /* Max experience */
    EXP exp{}; /* Cur experience */
    uint32_t exp_frac{}; /* Cur exp frac (times 2^16) */

    PLAYER_LEVEL lev{}; /* Level */

    int16_t town_num{}; /* Current town number */
    int16_t arena_number{}; /* monster number in on_defeat_arena_monster -KMW- */
    bool phase_out{}; /*!< フェイズアウト状態(闘技場観戦状態などに利用、NPCの処理の対象にならず自身もほとんどの行動ができない) */

    POSITION wilderness_x{}; /* Coordinates in the wilderness */
    POSITION wilderness_y{};
    bool wild_mode{};

    int mhp{}; /* Max hit pts */
    int chp{}; /* Cur hit pts */
    uint32_t chp_frac{}; /* Cur hit frac (times 2^16) */
    PERCENTAGE mutant_regenerate_mod{};

    MANA_POINT msp{}; /* Max mana pts */
    MANA_POINT csp{}; /* Cur mana pts */
    uint32_t csp_frac{}; /* Cur mana frac (times 2^16) */

    int16_t max_plv{}; /* Max Player Level */

    short stat_max[A_MAX]{}; /* Current "maximal" stat values */
    short stat_max_max[A_MAX]{}; /* Maximal "maximal" stat values */
    short stat_cur[A_MAX]{}; /* Current "natural" stat values */

    int16_t learned_spells{};
    int16_t add_spells{};

    uint32_t count{};

    TIME_EFFECT protevil{}; /* Timed -- Protection */
    TIME_EFFECT invuln{}; /* Timed -- Invulnerable */
    TIME_EFFECT ult_res{}; /* Timed -- Ultimate Resistance */
    TIME_EFFECT hero{}; /* Timed -- Heroism */
    TIME_EFFECT shero{}; /* Timed -- Super Heroism */
    TIME_EFFECT shield{}; /* Timed -- Shield Spell */
    TIME_EFFECT blessed{}; /* Timed -- Blessed */
    TIME_EFFECT tim_invis{}; /* Timed -- See Invisible */
    TIME_EFFECT tim_infra{}; /* Timed -- Infra Vision */
    TIME_EFFECT tsuyoshi{}; /* Timed -- Tsuyoshi Special */
    TIME_EFFECT ele_attack{}; /* Timed -- Elemental Attack */
    TIME_EFFECT ele_immune{}; /* Timed -- Elemental Immune */

    TIME_EFFECT oppose_acid{}; /* Timed -- oppose acid */
    TIME_EFFECT oppose_elec{}; /* Timed -- oppose lightning */
    TIME_EFFECT oppose_fire{}; /* Timed -- oppose heat */
    TIME_EFFECT oppose_cold{}; /* Timed -- oppose cold */
    TIME_EFFECT oppose_pois{}; /* Timed -- oppose poison */

    TIME_EFFECT tim_esp{}; /* Timed ESP */
    TIME_EFFECT wraith_form{}; /* Timed wraithform */

    TIME_EFFECT resist_magic{}; /* Timed Resist Magic (later) */
    TIME_EFFECT tim_regen{};
    TIME_EFFECT tim_pass_wall{};
    TIME_EFFECT tim_stealth{};
    TIME_EFFECT tim_levitation{};
    TIME_EFFECT tim_sh_touki{};
    TIME_EFFECT lightspeed{};
    TIME_EFFECT tsubureru{};
    TIME_EFFECT magicdef{};
    TIME_EFFECT tim_res_nether{}; /* Timed -- Nether resistance */
    TIME_EFFECT tim_res_time{}; /* Timed -- Time resistance */
    MimicKindType mimic_form{};
    TIME_EFFECT tim_mimic{};
    TIME_EFFECT tim_sh_fire{};
    TIME_EFFECT tim_sh_holy{};
    TIME_EFFECT tim_eyeeye{};

    /* for mirror master */
    TIME_EFFECT tim_reflect{}; /* Timed -- Reflect */
    TIME_EFFECT multishadow{}; /* Timed -- Multi-shadow */
    TIME_EFFECT dustrobe{}; /* Timed -- Robe of dust */

    bool timewalk{};

#define COMMAND_ARG_REST_UNTIL_DONE -2 /*!<休憩コマンド引数 … 必要な分だけ回復 */
#define COMMAND_ARG_REST_FULL_HEALING -1 /*!<休憩コマンド引数 … HPとMPが全回復するまで */
    GAME_TURN resting{}; /* Current counter for resting, if any */

    int16_t chaos_patron{};

    EnumClassFlagGroup<PlayerMutationType> muta{}; /*!< 突然変異 / mutations */

    int16_t virtues[8]{};
    Virtue vir_types[8]{};

    TIME_EFFECT word_recall{}; /* Word of recall counter */
    TIME_EFFECT alter_reality{}; /* Alter reality counter */
    DUNGEON_IDX recall_dungeon{}; /* Dungeon set to be recalled */

    ENERGY energy_need{}; /* Energy needed for next move */
    ENERGY enchant_energy_need{}; /* Energy needed for next upkeep effect	 */

    int16_t food{}; /*!< ゲーム中の滋養度の型定義 / Current nutrition */

    /*
     * p_ptr->special_attackによるプレイヤーの攻撃状態の定義 / Bit flags for the "p_ptr->special_attack" variable. -LM-
     *
     * Note:  The elemental and poison attacks should be managed using the
     * function "set_ele_attack", in spell2.c.  This provides for timeouts and
     * prevents the player from getting more than one at a time.
     */
    BIT_FLAGS special_attack{};

    /* プレイヤーの防御状態の定義 / Bit flags for the "p_ptr->special_defense" variable. -LM- */
    BIT_FLAGS special_defense{};
    byte action{}; /*!< プレイヤーが現在取っている常時行動のID / Currently action */
    BIT_FLAGS spell_learned1{}; /* bit mask of spells learned */
    BIT_FLAGS spell_learned2{}; /* bit mask of spells learned */
    BIT_FLAGS spell_worked1{}; /* bit mask of spells tried and worked */
    BIT_FLAGS spell_worked2{}; /* bit mask of spells tried and worked */
    BIT_FLAGS spell_forgotten1{}; /* bit mask of spells learned but forgotten */
    BIT_FLAGS spell_forgotten2{}; /* bit mask of spells learned but forgotten */
    SPELL_IDX spell_order[64]{}; /* order spells learned/remembered/forgotten */

    SUB_EXP spell_exp[64]{}; /* Proficiency of spells */
    std::map<ItemKindType, std::array<SUB_EXP, 64>> weapon_exp{}; /* Proficiency of weapons */
    std::map<ItemKindType, std::array<SUB_EXP, 64>> weapon_exp_max{}; /* Maximum proficiency of weapons */
    std::map<PlayerSkillKindType, SUB_EXP> skill_exp{}; /* Proficiency of misc. skill */

    ClassSpecificData class_specific_data;

    int player_hp[PY_MAX_LEVEL]{};
    std::string died_from{}; /* What killed the player */
    std::string last_message = ""; /* Last message on death or retirement */
    char history[4][60]{}; /* Textual "history" for the Player */

    uint16_t panic_save{}; /* Panic save */

    bool wait_report_score{}; /* Waiting to report score */
    bool is_dead{}; /* Player is dead */
    bool now_damaged{};
    bool ambush_flag{};
    BIT_FLAGS change_floor_mode{}; /*!<フロア移行処理に関するフラグ / Mode flags for changing floor */

    MONSTER_IDX riding{}; /* Riding on a monster of this index */

#define KNOW_STAT 0x01
#define KNOW_HPRATE 0x02
    BIT_FLAGS8 knowledge{}; /* Knowledge about yourself */
    BIT_FLAGS visit{}; /* Visited towns */

    PlayerRaceType start_race{}; /* Race at birth */
    BIT_FLAGS old_race1{}; /* Record of race changes */
    BIT_FLAGS old_race2{}; /* Record of race changes */
    int16_t old_realm{}; /* Record of realm changes */

    int16_t pet_follow_distance{}; /* Length of the imaginary "leash" for pets */
    BIT_FLAGS16 pet_extra_flags{}; /* Various flags for controling pets */

    bool knows_daily_bounty{}; //!< 日替わり賞金首を知っているか否か

    bool dtrap{}; /* Whether you are on trap-safe grids */
    FLOOR_IDX floor_id{}; /* Current floor location */

    bool autopick_autoregister{}; /* auto register is in-use or not */

    byte feeling{}; /* Most recent dungeon feeling */
    int32_t feeling_turn{}; /* The turn of the last dungeon feeling */

    std::shared_ptr<ItemEntity[]> inventory_list{}; /* The player's inventory */
    int16_t inven_cnt{}; /* Number of items in inventory */
    int16_t equip_cnt{}; /* Number of items in equipment */

    /*** Temporary fields ***/

    bool select_ring_slot{};

    bool playing{}; /* True if player is playing */
    bool leaving{}; /* True if player is leaving */

    bool monk_notify_aux{};

    byte leave_bldg{};
    byte exit_bldg{}; /* Goal obtained in on_defeat_arena_monster? -KMW- */

    bool leaving_dungeon{}; /* True if player is leaving the dungeon */
    bool teleport_town{};
    bool enter_dungeon{}; /* Just enter the dungeon */

    IDX health_who{}; /* Health bar trackee */

    MonsterRaceId monster_race_idx{}; /* Monster race trackee */

    short tracking_bi_id{}; /* Object kind trackee */

    int16_t new_spells{}; /* Number of spells available */
    int16_t old_spells{};

    int16_t old_food_aux{}; /* Old value of food */

    bool old_cumber_armor{};
    bool old_cumber_glove{};
    bool old_heavy_wield[2]{};
    bool old_heavy_shoot{};
    bool old_icky_wield[2]{};
    bool old_riding_wield[2]{};
    bool old_riding_ryoute{};
    bool old_monlite{};
    int extra_blows[2]{};

    POSITION old_lite{}; /* Old radius of lite (if any) */

    bool cumber_armor{}; /* Mana draining armor */
    bool cumber_glove{}; /* Mana draining gloves */
    bool heavy_wield[2]{}; /* Heavy weapon */
    bool is_icky_wield[2]{}; /* クラスにふさわしくない装備をしている / Icky weapon */
    bool is_icky_riding_wield[2]{}; /* 乗馬中に乗馬にふさわしくない装備をしている / Riding weapon */
    bool riding_ryoute{}; /* Riding weapon */
    bool monlite{};
    BIT_FLAGS yoiyami{};
    BIT_FLAGS easy_2weapon{};
    BIT_FLAGS down_saving{};

    POSITION cur_lite{}; /* Radius of lite (if any) */

    int16_t stat_use[A_MAX]{}; /* Current modified stats */
    int16_t stat_top[A_MAX]{}; /* Maximal modified stats */

    bool sutemi{};
    bool counter{};

    int alignment{}; /* Good/evil/neutral */
    POSITION run_py{};
    POSITION run_px{};
    DIRECTION fishing_dir{};

    MONSTER_IDX pet_t_m_idx{};
    MONSTER_IDX riding_t_m_idx{};

    /*** Extracted fields ***/

    int16_t running{}; /* Current counter for running, if any */
    bool suppress_multi_reward{}; /*!< 複数レベルアップ時のパトロンからの報酬多重受け取りを防止 */

    int16_t stat_add[A_MAX]{}; /* Modifiers to stat values */
    int16_t stat_index[A_MAX]{}; /* Indexes into stat tables */

    bool hack_mutation{};
    bool is_fired{};
    bool level_up_message{};

    BIT_FLAGS anti_magic{}; /* Anti-magic */
    BIT_FLAGS anti_tele{}; /* Prevent teleportation */

    EnumClassFlagGroup<CurseTraitType> cursed{}; /* Player is cursed */
    EnumClassFlagGroup<CurseSpecialTraitType> cursed_special{}; /* Player is special type cursed */

    bool can_swim{}; /* No damage falling */
    BIT_FLAGS levitation{}; /* No damage falling */
    BIT_FLAGS lite{}; /* Permanent light */
    BIT_FLAGS free_act{}; /* Never paralyzed */
    BIT_FLAGS see_inv{}; /* Can see invisible */
    BIT_FLAGS regenerate{}; /* Regenerate hit pts */
    BIT_FLAGS hold_exp{}; /* Resist exp draining */

    BIT_FLAGS telepathy{}; /* Telepathy */
    BIT_FLAGS esp_animal{};
    BIT_FLAGS esp_undead{};
    BIT_FLAGS esp_demon{};
    BIT_FLAGS esp_orc{};
    BIT_FLAGS esp_troll{};
    BIT_FLAGS esp_giant{};
    BIT_FLAGS esp_dragon{};
    BIT_FLAGS esp_human{};
    BIT_FLAGS esp_evil{};
    BIT_FLAGS esp_good{};
    BIT_FLAGS esp_nonliving{};
    BIT_FLAGS esp_unique{};

    BIT_FLAGS slow_digest{}; /* Slower digestion */
    BIT_FLAGS bless_blade{}; //!< 祝福された装備をしている / Blessed by inventory items
    BIT_FLAGS xtra_might{}; /* Extra might bow */
    BIT_FLAGS impact{}; //!< クリティカル率を上げる装備をしている / Critical blows
    BIT_FLAGS earthquake{}; //!< 地震を起こす装備をしている / Earthquake blows
    BIT_FLAGS dec_mana{};
    BIT_FLAGS easy_spell{};
    BIT_FLAGS heavy_spell{};
    BIT_FLAGS warning{};
    BIT_FLAGS mighty_throw{};
    BIT_FLAGS see_nocto{}; /* Noctovision */
    bool invoking_midnight_curse{};

    DICE_NUMBER to_dd[2]{}; /* Extra dice/sides */
    DICE_SID to_ds[2]{};

    HIT_PROB dis_to_h[2]{}; /*!< 判明している現在の表記上の近接武器命中修正値 /  Known bonus to hit (wield) */
    HIT_PROB dis_to_h_b{}; /*!< 判明している現在の表記上の射撃武器命中修正値 / Known bonus to hit (bow) */
    int dis_to_d[2]{}; /*!< 判明している現在の表記上の近接武器ダメージ修正値 / Known bonus to dam (wield) */
    ARMOUR_CLASS dis_to_a{}; /*!< 判明している現在の表記上の装備AC修正値 / Known bonus to ac */
    ARMOUR_CLASS dis_ac{}; /*!< 判明している現在の表記上の装備AC基礎値 / Known base ac */

    int16_t to_h[2]{}; /* Bonus to hit (wield) */
    int16_t to_h_b{}; /* Bonus to hit (bow) */
    int16_t to_h_m{}; /* Bonus to hit (misc) */
    int16_t to_d[2]{}; /* Bonus to dam (wield) */
    int16_t to_d_m{}; /* Bonus to dam (misc) */
    ARMOUR_CLASS to_a{}; /* Bonus to ac */

    int16_t to_m_chance{}; /* Minusses to cast chance */

    bool no_flowed{};

    ARMOUR_CLASS ac{}; /*!< 装備無しの基本AC / Base ac */

    ACTION_SKILL_POWER see_infra{}; /*!< 赤外線視能力の強さ /Infravision range */
    ACTION_SKILL_POWER skill_dis{}; /*!< 行動技能値:解除能力 / Skill: Disarming */
    ACTION_SKILL_POWER skill_dev{}; /*!< 行動技能値:魔道具使用 / Skill: Magic Devices */
    ACTION_SKILL_POWER skill_sav{}; /*!< 行動技能値:魔法防御 / Skill: Saving throw */
    ACTION_SKILL_POWER skill_stl{}; /*!< 行動技能値:隠密 / Skill: Stealth factor */

    /*!
     * 行動技能値:知覚 / Skill: Searching ability
     * この値はsearch()による地形の隠し要素発見処理などで混乱、盲目、幻覚、無光源などの
     * 状態異常がない限り、難易度修正などがないままそのままパーセンテージ値として使われる。
     * 100以上ならば必ず全てのトラップなどを見つけることが出来る。
     */
    ACTION_SKILL_POWER skill_srh{};

    ACTION_SKILL_POWER skill_fos{}; /*!< 行動技能値:探索 / Skill: Searching frequency */
    ACTION_SKILL_POWER skill_thn{}; /*!< 行動技能値:打撃命中能力 / Skill: To hit (normal) */
    ACTION_SKILL_POWER skill_thb{}; /*!< 行動技能値:射撃命中能力 / Skill: To hit (shooting) */
    ACTION_SKILL_POWER skill_tht{}; /*!< 行動技能値:投射命中能力 / Skill: To hit (throwing) */
    ACTION_SKILL_POWER skill_dig{}; /*!< 行動技能値:掘削 / Skill: Digging */

    int16_t num_blow[2]{}; /* Number of blows */
    int16_t num_fire{}; /* Number of shots */

    byte tval_xtra{}; /* (Unused)Correct xtra tval */
    ItemKindType tval_ammo{}; /* Correct ammo tval */

    int16_t pspeed{}; /*!< 現在の速度 / Current speed */

    ENERGY energy_use{}; /*!< 直近のターンに消費したエネルギー / Energy use this turn */

    POSITION y{}; /*!< ダンジョンの現在Y座標 / Player location in dungeon */
    POSITION x{}; /*!< ダンジョンの現在X座標 / Player location in dungeon */
    GAME_TEXT name[32]{}; /*!< 現在のプレイヤー名 / Current player's character name */
    char base_name[32]{}; /*!< Stripped version of "player_name" */

    std::shared_ptr<TimedEffects> effects() const;
    bool is_fully_healthy() const;
    std::string decrease_ability_random();
    std::string decrease_ability_all();

private:
    std::shared_ptr<TimedEffects> timed_effects;
};

extern PlayerType *p_ptr;
