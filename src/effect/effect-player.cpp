/*!
 * @brief 魔法によるプレイヤーへの効果まとめ
 * @date 2020/04/29
 * @author Hourier
 */

#include "effect/effect-player.h"
#include "core/disturbance.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-player-switcher.h"
#include "effect/effect-player-util.h"
#include "effect/effect-processor.h"
#include "effect/spells-effect-util.h"
#include "floor/cave.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-ninja.h"
#include "monster-race/monster-race.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "spell-realm/spells-crusade.h"
#include "spell-realm/spells-hex.h"
#include "spell/spell-types.h"
#include "spell/spells-util.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "view/display-messages.h"

/*! 盲目時の代替メッセージ // TODO: 各種の盲目時メッセージからまとめ上げて統合
 */
concptr blind_spell_effect_messages[MAX_GF] = {
    "", // GF_NONE = 0,
    "", // GF_ELEC = 1, /*!< 魔法効果: 電撃*/
    "", // GF_POIS = 2, /*!< 魔法効果: 毒*/
    "", // GF_ACID = 3, /*!< 魔法効果: 酸*/
    "", // GF_COLD = 4, /*!< 魔法効果: 冷気*/
    "", // GF_FIRE = 5, /*!< 魔法効果: 火炎*/
    "", "", "",
    "", // GF_PSY_SPEAR = 9, /*!< 魔法効果: 光の剣*/
    "", // GF_MISSILE = 10, /*!< 魔法効果: 弱魔力*/
    "", // GF_ARROW = 11, /*!< 魔法効果: 射撃*/
    "", // GF_PLASMA = 12, /*!< 魔法効果: プラズマ*/
    "",
    "", // GF_WATER = 14, /*!< 魔法効果: 水流*/
    "", // GF_LITE = 15, /*!< 魔法効果: 閃光*/
    "", // GF_DARK = 16, /*!< 魔法効果: 暗黒*/
    "", // GF_LITE_WEAK = 17, /*!< 魔法効果: 弱光*/
    "", // GF_DARK_WEAK = 18, /*!< 魔法効果: 弱暗*/
    "",
    "", // GF_SHARDS = 20, /*!< 魔法効果: 破片*/
    "", // GF_SOUND = 21, /*!< 魔法効果: 轟音*/
    "", // GF_CONFUSION = 22, /*!< 魔法効果: 混乱*/
    "", // GF_FORCE = 23, /*!< 魔法効果: フォース*/
    "", // GF_INERTIAL = 24, /*!< 魔法効果: 遅鈍*/
    "",
    "", // GF_MANA = 26, /*!< 魔法効果: 純粋魔力*/
    "", // GF_METEOR = 27, /*!< 魔法効果: 隕石*/
    "", // GF_ICE = 28, /*!< 魔法効果: 極寒*/
    "",
    "", // GF_CHAOS = 30, /*!< 魔法効果: カオス*/
    "", // GF_NETHER = 31, /*!< 魔法効果: 地獄*/
    "", // GF_DISENCHANT = 32, /*!< 魔法効果: 劣化*/
    "", // GF_NEXUS = 33, /*!< 魔法効果: 因果混乱*/
    "", // GF_TIME = 34, /*!< 魔法効果: 時間逆転*/
    "", // GF_GRAVITY = 35, /*!< 魔法効果: 重力*/
    "", "", "", "", "",
    "", // GF_KILL_WALL = 40, /*!< 魔法効果: 岩石溶解*/
    "", // GF_KILL_DOOR = 41, /*!< 魔法効果: ドア破壊*/
    "", // GF_KILL_TRAP = 42, /*!< 魔法効果: トラップ破壊*/
    "", "",
    "", // GF_MAKE_WALL = 45, /*!< 魔法効果: 壁生成*/
    "", // GF_MAKE_DOOR = 46, /*!< 魔法効果: ドア生成*/
    "", // GF_MAKE_TRAP = 47, /*!< 魔法効果: トラップ生成*/
    "", // GF_MAKE_TREE = 48, /*!< 魔法効果: 森林生成*/
    "",
    "", // GF_OLD_CLONE = 51, /*!< 魔法効果: クローン・モンスター*/
    "", // GF_OLD_POLY = 52, /*!< 魔法効果: チェンジ・モンスター*/
    "", // GF_OLD_HEAL = 53, /*!< 魔法効果: 回復モンスター*/
    "", // GF_OLD_SPEED = 54, /*!< 魔法効果: スピード・モンスター*/
    "", // GF_OLD_SLOW = 55, /*!< 魔法効果: スロウ・モンスター*/
    "", // GF_OLD_CONF = 56, /*!< 魔法効果: パニック・モンスター*/
    "", // GF_OLD_SLEEP = 57, /*!< 魔法効果: スリープ・モンスター*/
    "", // GF_HYPODYNAMIA = 58, /*!< 魔法効果: 衰弱*/
    "", "",
    "", // GF_AWAY_UNDEAD = 61, /*!< 魔法効果: アンデッド・アウェイ*/
    "", // GF_AWAY_EVIL = 62, /*!< 魔法効果: 邪悪飛ばし*/
    "", // GF_AWAY_ALL = 63, /*!< 魔法効果: テレポート・アウェイ*/
    "", // GF_TURN_UNDEAD = 64, /*!< 魔法効果: アンデッド恐慌*/
    "", // GF_TURN_EVIL = 65, /*!< 魔法効果: 邪悪恐慌*/
    "", // GF_TURN_ALL = 66, /*!< 魔法効果: モンスター恐慌*/
    "", // GF_DISP_UNDEAD = 67, /*!< 魔法効果: アンデッド退散*/
    "", // GF_DISP_EVIL = 68, /*!< 魔法効果: 邪悪退散*/
    "", // GF_DISP_ALL = 69, /*!< 魔法効果: モンスター退散*/
    "", // GF_DISP_DEMON = 70, /*!< 魔法効果: 悪魔退散*/
    "", // GF_DISP_LIVING = 71, /*!< 魔法効果: 生命退散*/
    "", // GF_ROCKET = 72, /*!< 魔法効果: ロケット*/
    "", // GF_NUKE = 73, /*!< 魔法効果: 放射性廃棄物*/
    "", // GF_MAKE_RUNE_PROTECTION = 74, /*!< 魔法効果: 守りのルーン生成*/
    "", // GF_STASIS = 75, /*!< 魔法効果: モンスター拘束*/
    "", // GF_STONE_WALL = 76, /*!< 魔法効果: 壁生成*/
    "", // GF_DEATH_RAY = 77, /*!< 魔法効果: 死の光線*/
    "", // GF_STUN = 78, /*!< 魔法効果: 朦朧*/
    "", // GF_HOLY_FIRE = 79, /*!< 魔法効果: 聖光*/
    "", // GF_HELL_FIRE = 80, /*!< 魔法効果: 地獄の劫火*/
    "", // GF_DISINTEGRATE = 81, /*!< 魔法効果: 分解*/
    "", // GF_CHARM = 82, /*!< 魔法効果: モンスター魅了*/
    "", // GF_CONTROL_UNDEAD = 83, /*!< 魔法効果: アンデッド支配*/
    "", // GF_CONTROL_ANIMAL = 84, /*!< 魔法効果: 動物支配*/
    "", // GF_PSI = 85, /*!< 魔法効果: サイキック攻撃*/
    "", // GF_PSI_DRAIN = 86, /*!< 魔法効果: 精神吸収*/
    "", // GF_TELEKINESIS = 87, /*!< 魔法効果: テレキシネス*/
    "", // GF_JAM_DOOR = 88, /*!< 魔法効果: 施錠*/
    "", // GF_DOMINATION = 89, /*!< 魔法効果: 精神支配*/
    "", // GF_DISP_GOOD = 90, /*!< 魔法効果: 善良退散*/
    "", // GF_DRAIN_MANA = 91, /*!< 魔法効果: 魔力吸収*/
    "", // GF_MIND_BLAST = 92, /*!< 魔法効果: 精神攻撃*/
    "", // GF_BRAIN_SMASH = 93, /*!< 魔法効果: 脳攻撃*/
    "", // GF_CAUSE_1 = 94, /*!< 魔法効果: 軽傷の呪い*/
    "", // GF_CAUSE_2 = 95, /*!< 魔法効果: 重傷の呪い*/
    "", // GF_CAUSE_3 = 96, /*!< 魔法効果: 致命傷の呪い*/
    "", // GF_CAUSE_4 = 97, /*!< 魔法効果: 秘孔を突く*/
    "", // GF_HAND_DOOM = 98, /*!< 魔法効果: 破滅の手*/
    "", // GF_CAPTURE = 99, /*!< 魔法効果: 捕縛*/
    "", // GF_ANIM_DEAD = 100, /*!< 魔法効果: 死者復活*/
    "", // GF_CHARM_LIVING = 101, /*!< 魔法効果: 生命魅了*/
    "", // GF_IDENTIFY = 102, /*!< 魔法効果: 鑑定*/
    "", // GF_ATTACK = 103, /*!< 魔法効果: 白兵*/
    "", // GF_ENGETSU = 104, /*!< 魔法効果: 円月*/
    "", // GF_GENOCIDE = 105, /*!< 魔法効果: 抹殺*/
    "", // GF_PHOTO = 106, /*!< 魔法効果: 撮影*/
    "", // GF_CONTROL_DEMON = 107, /*!< 魔法効果: 悪魔支配*/
    "", // GF_LAVA_FLOW = 108, /*!< 魔法効果: 溶岩噴出*/
    "", // GF_BLOOD_CURSE = 109, /*!< 魔法効果: 血の呪い*/
    "", // GF_SEEKER = 110, /*!< 魔法効果: シーカーレイ*/
    "", // GF_SUPER_RAY = 111, /*!< 魔法効果: スーパーレイ*/
    "", // GF_STAR_HEAL = 112, /*!< 魔法効果: 星の癒し*/
    "", // GF_WATER_FLOW = 113, /*!< 魔法効果: 流水*/
    "", // GF_CRUSADE = 114, /*!< 魔法効果: 聖戦*/
    "", // GF_STASIS_EVIL = 115, /*!< 魔法効果: 邪悪拘束*/
    "", // GF_WOUNDS = 116, /*!< 魔法効果: 創傷*/
};

enum ep_check_result {
    EP_CHECK_FALSE = 0,
    EP_CHECK_TRUE = 1,
    EP_CHECK_CONTINUE = 2,
};

/*!
 * @brief effect_player_type構造体を初期化する
 * @param ep_ptr 初期化前の構造体
 * @param who 魔法を唱えたモンスター (0ならプレイヤー自身)
 * @param dam 基本威力
 * @param effect_type 効果属性
 * @param flag 効果フラグ
 * @param monspell 効果元のモンスター魔法ID
 * @return 初期化後の構造体ポインタ
 */
static effect_player_type *initialize_effect_player(effect_player_type *ep_ptr, MONSTER_IDX who, HIT_POINT dam, EFFECT_ID effect_type, BIT_FLAGS flag)
{
    ep_ptr->rlev = 0;
    ep_ptr->m_ptr = nullptr;
    ep_ptr->get_damage = 0;
    ep_ptr->who = who;
    ep_ptr->dam = dam;
    ep_ptr->effect_type = effect_type;
    ep_ptr->flag = flag;
    return ep_ptr;
}

/*!
 * @brief ボルト魔法を反射する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param ep_ptr プレイヤー効果構造体への参照ポインタ
 * @return 当たったらFALSE、反射したらTRUE
 */
static bool process_bolt_reflection(player_type *player_ptr, effect_player_type *ep_ptr, project_func project)
{
    bool can_bolt_hit = has_reflect(player_ptr) || (((player_ptr->special_defense & KATA_FUUJIN) != 0) && !player_ptr->blind);
    can_bolt_hit &= (ep_ptr->flag & PROJECT_REFLECTABLE) != 0;
    can_bolt_hit &= !one_in_(10);
    if (!can_bolt_hit)
        return false;

    POSITION t_y, t_x;
    int max_attempts = 10;
    sound(SOUND_REFLECT);

    if (player_ptr->blind)
        msg_print(_("何かが跳ね返った！", "Something bounces!"));
    else if (player_ptr->special_defense & KATA_FUUJIN)
        msg_print(_("風の如く武器を振るって弾き返した！", "The attack bounces!"));
    else
        msg_print(_("攻撃が跳ね返った！", "The attack bounces!"));

    if (ep_ptr->who > 0) {
        floor_type *floor_ptr = player_ptr->current_floor_ptr;
        monster_type m_type = floor_ptr->m_list[ep_ptr->who];
        do {
            t_y = m_type.fy - 1 + randint1(3);
            t_x = m_type.fx - 1 + randint1(3);
            max_attempts--;
        } while (max_attempts && in_bounds2u(floor_ptr, t_y, t_x) && !projectable(player_ptr, player_ptr->y, player_ptr->x, t_y, t_x));

        if (max_attempts < 1) {
            t_y = m_type.fy;
            t_x = m_type.fx;
        }
    } else {
        t_y = player_ptr->y - 1 + randint1(3);
        t_x = player_ptr->x - 1 + randint1(3);
    }

    (*project)(player_ptr, 0, 0, t_y, t_x, ep_ptr->dam, ep_ptr->effect_type, (PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE));
    disturb(player_ptr, true, true);
    return true;
}

/*!
 * @brief 反射・忍者の変わり身などでそもそも当たらない状況を判定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param ep_ptr プレイヤー効果構造体への参照ポインタ
 * @param y 目標Y座標
 * @param x 目標X座標
 * @return 当たらなかったらFALSE、反射したらTRUE、当たったらCONTINUE
 */
static ep_check_result check_continue_player_effect(player_type *player_ptr, effect_player_type *ep_ptr, POSITION y, POSITION x, project_func project)
{
    if (!player_bold(player_ptr, y, x))
        return EP_CHECK_FALSE;

    if (((player_ptr->special_defense & NINJA_KAWARIMI) != 0) && (ep_ptr->dam > 0) && (randint0(55) < (player_ptr->lev * 3 / 5 + 20)) && (ep_ptr->who > 0)
        && (ep_ptr->who != player_ptr->riding) && kawarimi(player_ptr, true))
        return EP_CHECK_FALSE;

    if ((ep_ptr->who == 0) || (ep_ptr->who == player_ptr->riding))
        return EP_CHECK_FALSE;

    if (process_bolt_reflection(player_ptr, ep_ptr, project))
        return EP_CHECK_TRUE;

    return EP_CHECK_CONTINUE;
}

/*!
 * @brief 魔法を発したモンスター名を記述する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param ep_ptr プレイヤー効果構造体への参照ポインタ
 * @param who_name モンスター名
 */
static void describe_effect_source(player_type *player_ptr, effect_player_type *ep_ptr, concptr who_name)
{
    if (ep_ptr->who > 0) {
        ep_ptr->m_ptr = &player_ptr->current_floor_ptr->m_list[ep_ptr->who];
        ep_ptr->rlev = (&r_info[ep_ptr->m_ptr->r_idx])->level >= 1 ? (&r_info[ep_ptr->m_ptr->r_idx])->level : 1;
        monster_desc(player_ptr, ep_ptr->m_name, ep_ptr->m_ptr, 0);
        strcpy(ep_ptr->killer, who_name);
        return;
    }

    switch (ep_ptr->who) {
    case PROJECT_WHO_UNCTRL_POWER:
        strcpy(ep_ptr->killer, _("制御できない力の氾流", "uncontrollable power storm"));
        break;
    case PROJECT_WHO_GLASS_SHARDS:
        strcpy(ep_ptr->killer, _("ガラスの破片", "shards of glass"));
        break;
    default:
        strcpy(ep_ptr->killer, _("罠", "a trap"));
        break;
    }

    strcpy(ep_ptr->m_name, ep_ptr->killer);
}

/*!
 * @brief 汎用的なビーム/ボルト/ボール系によるプレイヤーへの効果処理 / Helper function for "project()" below.
 * @param who 魔法を発動したモンスター(0ならばプレイヤー、負値ならば自然発生) / Index of "source" monster (zero for "player")
 * @param who_name 効果を起こしたモンスターの名前
 * @param r 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標Y座標 / Target y location (or location to travel "towards")
 * @param x 目標X座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param effect_type 効果属性 / Type of damage to apply to monsters (and objects)
 * @param flag 効果フラグ
 * @param monspell 効果元のモンスター魔法ID
 * @return 何か一つでも効力があればTRUEを返す / TRUE if any "effects" of the projection were observed, else FALSE
 */
bool affect_player(MONSTER_IDX who, player_type *player_ptr, concptr who_name, int r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID effect_type,
    BIT_FLAGS flag, project_func project)
{
    effect_player_type tmp_effect;
    effect_player_type *ep_ptr = initialize_effect_player(&tmp_effect, who, dam, effect_type, flag);
    ep_check_result check_result = check_continue_player_effect(player_ptr, ep_ptr, y, x, project);
    if (check_result != EP_CHECK_CONTINUE)
        return (bool)check_result;

    if (ep_ptr->dam > 1600)
        ep_ptr->dam = 1600;

    ep_ptr->dam = (ep_ptr->dam + r) / (r + 1);
    describe_effect_source(player_ptr, ep_ptr, who_name);
    switch_effects_player(player_ptr, ep_ptr);

    SpellHex(player_ptr).store_vengeful_damage(ep_ptr->get_damage);
    if ((player_ptr->tim_eyeeye || SpellHex(player_ptr).is_spelling_specific(HEX_EYE_FOR_EYE)) && (ep_ptr->get_damage > 0) && !player_ptr->is_dead && (ep_ptr->who > 0)) {
        GAME_TEXT m_name_self[MAX_MONSTER_NAME];
        monster_desc(player_ptr, m_name_self, ep_ptr->m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE | MD_OBJECTIVE);
        msg_format(_("攻撃が%s自身を傷つけた！", "The attack of %s has wounded %s!"), ep_ptr->m_name, m_name_self);
        (*project)(player_ptr, 0, 0, ep_ptr->m_ptr->fy, ep_ptr->m_ptr->fx, ep_ptr->get_damage, GF_MISSILE, PROJECT_KILL);
        if (player_ptr->tim_eyeeye)
            set_tim_eyeeye(player_ptr, player_ptr->tim_eyeeye - 5, true);
    }

    if (player_ptr->riding && ep_ptr->dam > 0) {
        rakubadam_p = (ep_ptr->dam > 200) ? 200 : ep_ptr->dam;
    }

    disturb(player_ptr, true, true);
    if ((player_ptr->special_defense & NINJA_KAWARIMI) && ep_ptr->dam && ep_ptr->who && (ep_ptr->who != player_ptr->riding)) {
        (void)kawarimi(player_ptr, false);
    }

    return true;
}
