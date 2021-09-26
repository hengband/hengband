/*!
 * @brief モンスター死亡時の特殊処理switch (一般的な処理もdefaultで実施)
 * @date 2020/08/21
 * @author Hourier
 */

#include "monster-floor/special-death-switcher.h"
#include "artifact/fixed-art-generator.h"
#include "artifact/fixed-art-types.h"
#include "artifact/random-art-generator.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/floor-util.h"
#include "floor/geometry.h"
#include "game-option/birth-options.h"
#include "grid/grid.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-death-util.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "object-enchant/apply-magic.h"
#include "object-enchant/item-apply-magic.h"
#include "object/object-kind-hook.h"
#include "spell/spell-types.h"
#include "spell/summon-types.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 死亡時召喚処理 (今のところ自分自身のみ)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param md_ptr モンスター撃破構造体への参照ポインタ
 * @param type 召喚タイプ
 * @param probability 召喚確率 (計算式：1 - 1/probability)
 * @param radius 召喚半径 (モンスターが死亡した座標から半径何マス以内に召喚させるか)
 * @param message 召喚時のメッセージ
 */
static void summon_self(player_type *player_ptr, monster_death_type *md_ptr, summon_type type, int probability, POSITION radius, concptr message)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (floor_ptr->inside_arena || player_ptr->phase_out || one_in_(probability))
        return;

    POSITION wy = md_ptr->md_y;
    POSITION wx = md_ptr->md_x;
    int attempts = 100;
    bool pet = is_pet(md_ptr->m_ptr);
    do {
        scatter(player_ptr, &wy, &wx, md_ptr->md_y, md_ptr->md_x, radius, PROJECT_NONE);
    } while (!(in_bounds(floor_ptr, wy, wx) && is_cave_empty_bold2(player_ptr, wy, wx)) && --attempts);

    if (attempts <= 0)
        return;

    BIT_FLAGS mode = pet ? PM_FORCE_PET : PM_NONE;
    if (summon_specific(player_ptr, (pet ? -1 : md_ptr->m_idx), wy, wx, 100, type, mode) && player_can_see_bold(player_ptr, wy, wx))
        msg_print(message);
}

static void on_dead_pink_horror(player_type *player_ptr, monster_death_type *md_ptr)
{
    if (player_ptr->current_floor_ptr->inside_arena || player_ptr->phase_out)
        return;

    bool notice = false;
    const int blue_horrors = 2;
    for (int i = 0; i < blue_horrors; i++) {
        POSITION wy = md_ptr->md_y;
        POSITION wx = md_ptr->md_x;
        bool pet = is_pet(md_ptr->m_ptr);
        BIT_FLAGS mode = pet ? PM_FORCE_PET : PM_NONE;
        if (summon_specific(player_ptr, (pet ? -1 : md_ptr->m_idx), wy, wx, 100, SUMMON_BLUE_HORROR, mode) && player_can_see_bold(player_ptr, wy, wx))
            notice = true;
    }

    if (notice) {
        sound(SOUND_SUMMON);
        msg_print(_("ピンク・ホラーは分裂した！", "The Pink horror divides!"));
    }
}

static void on_dead_bloodletter(player_type *player_ptr, monster_death_type *md_ptr)
{
    if (!md_ptr->drop_chosen_item || (randint1(100) >= 15))
        return;

    object_type forge;
    object_type *q_ptr = &forge;
    q_ptr->prep(lookup_kind(TV_SWORD, SV_BLADE_OF_CHAOS));
    apply_magic_to_object(player_ptr, q_ptr, player_ptr->current_floor_ptr->object_level, AM_NO_FIXED_ART | md_ptr->mo_mode);
    (void)drop_near(player_ptr, q_ptr, -1, md_ptr->md_y, md_ptr->md_x);
}

static void on_dead_raal(player_type *player_ptr, monster_death_type *md_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (!md_ptr->drop_chosen_item || (floor_ptr->dun_level <= 9))
        return;

    object_type forge;
    object_type *q_ptr = &forge;
    q_ptr->wipe();
    if ((floor_ptr->dun_level > 49) && one_in_(5))
        get_obj_num_hook = kind_is_good_book;
    else
        get_obj_num_hook = kind_is_book;

    make_object(player_ptr, q_ptr, md_ptr->mo_mode);
    (void)drop_near(player_ptr, q_ptr, -1, md_ptr->md_y, md_ptr->md_x);
}

/*!
 * @brief 6/7の確率で、20マス以内に暁の戦士自身を召喚する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param md_ptr モンスター撃破構造体への参照ポインタ
 */
static void on_dead_dawn(player_type *player_ptr, monster_death_type *md_ptr)
{
    summon_self(player_ptr, md_ptr, SUMMON_DAWN, 7, 20, _("新たな戦士が現れた！", "A new warrior steps forth!"));
}

static void on_dead_unmaker(player_type *player_ptr, monster_death_type *md_ptr)
{
    if (is_seen(player_ptr, md_ptr->m_ptr)) {
        GAME_TEXT m_name[MAX_NLEN];
        monster_desc(player_ptr, m_name, md_ptr->m_ptr, MD_NONE);
        msg_format(_("%sは辺りにログルスの残り香を撒き散らした！", "%^s sprinkled the remaining incense from Logrus!"), m_name);
    }

    (void)project(player_ptr, md_ptr->m_idx, 6, md_ptr->md_y, md_ptr->md_x, 100, GF_CHAOS, PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
}

static void on_dead_sacred_treasures(player_type *player_ptr, monster_death_type *md_ptr)
{
    if ((player_ptr->ppersonality != PERSONALITY_LAZY) || !md_ptr->drop_chosen_item)
        return;

    ARTIFACT_IDX a_idx = 0;
    artifact_type *a_ptr = nullptr;
    do {
        switch (randint0(3)) {
        case 0:
            a_idx = ART_NAMAKE_HAMMER;
            break;
        case 1:
            a_idx = ART_NAMAKE_BOW;
            break;
        case 2:
            a_idx = ART_NAMAKE_ARMOR;
            break;
        }

        a_ptr = &a_info[a_idx];
    } while (a_ptr->cur_num == 1);

    if (create_named_art(player_ptr, a_idx, md_ptr->md_y, md_ptr->md_x)) {
        a_ptr->cur_num = 1;
        if (w_ptr->character_dungeon)
            a_ptr->floor_id = player_ptr->floor_id;

        return;
    }

    if (!preserve_mode)
        a_ptr->cur_num = 1;
}

static void on_dead_serpent(player_type *player_ptr, monster_death_type *md_ptr)
{
    if (!md_ptr->drop_chosen_item)
        return;

    object_type forge;
    object_type *q_ptr = &forge;
    q_ptr->prep(lookup_kind(TV_HAFTED, SV_GROND));
    q_ptr->name1 = ART_GROND;
    apply_magic_to_object(player_ptr, q_ptr, -1, AM_GOOD | AM_GREAT);
    (void)drop_near(player_ptr, q_ptr, -1, md_ptr->md_y, md_ptr->md_x);
    q_ptr = &forge;
    q_ptr->prep(lookup_kind(TV_CROWN, SV_CHAOS));
    q_ptr->name1 = ART_CHAOS;
    apply_magic_to_object(player_ptr, q_ptr, -1, AM_GOOD | AM_GREAT);
    (void)drop_near(player_ptr, q_ptr, -1, md_ptr->md_y, md_ptr->md_x);
}

static void on_dead_death_sword(player_type *player_ptr, monster_death_type *md_ptr)
{
    if (!md_ptr->drop_chosen_item)
        return;

    object_type forge;
    object_type *q_ptr = &forge;
    q_ptr->prep(lookup_kind(TV_SWORD, randint1(2)));
    (void)drop_near(player_ptr, q_ptr, -1, md_ptr->md_y, md_ptr->md_x);
}

static void on_dead_can_angel(player_type *player_ptr, monster_death_type *md_ptr)
{
    bool is_drop_can = md_ptr->drop_chosen_item;
    bool is_silver = md_ptr->m_ptr->r_idx == MON_A_SILVER;
    is_silver &= md_ptr->r_ptr->r_akills % 5 == 0;
    is_drop_can &= (md_ptr->m_ptr->r_idx == MON_A_GOLD) || is_silver;
    if (!is_drop_can)
        return;

    object_type forge;
    object_type *q_ptr = &forge;
    q_ptr->prep(lookup_kind(TV_CHEST, SV_CHEST_KANDUME));
    apply_magic_to_object(player_ptr, q_ptr, player_ptr->current_floor_ptr->object_level, AM_NO_FIXED_ART);
    (void)drop_near(player_ptr, q_ptr, -1, md_ptr->md_y, md_ptr->md_x);
}

static void on_dead_rolento(player_type *player_ptr, monster_death_type *md_ptr)
{
    if (is_seen(player_ptr, md_ptr->m_ptr)) {
        GAME_TEXT m_name[MAX_NLEN];
        monster_desc(player_ptr, m_name, md_ptr->m_ptr, MD_NONE);
        msg_format(_("%sは手榴弾を抱えて自爆した！", "%^s blew himself up with grenades!"), m_name);
    }

    (void)project(player_ptr, md_ptr->m_idx, 3, md_ptr->md_y, md_ptr->md_x, damroll(20, 10), GF_FIRE, PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
}

static void on_dead_aqua_illusion(player_type *player_ptr, monster_death_type *md_ptr)
{
    if (player_ptr->current_floor_ptr->inside_arena || player_ptr->phase_out)
        return;

    bool notice = false;
    const int popped_bubbles = 4;
    for (int i = 0; i < popped_bubbles; i++) {
        POSITION wy = md_ptr->md_y;
        POSITION wx = md_ptr->md_x;
        bool pet = is_pet(md_ptr->m_ptr);
        BIT_FLAGS mode = pet ? PM_FORCE_PET : PM_NONE;
        MONSTER_IDX smaller_bubble = md_ptr->m_ptr->r_idx - 1;
        if (summon_named_creature(player_ptr, (pet ? -1 : md_ptr->m_idx), wy, wx, smaller_bubble, mode) && player_can_see_bold(player_ptr, wy, wx))
            notice = true;
    }

    if (notice) {
        msg_print(_("泡が弾けた！", "The bubble pops!"));
        sound(SOUND_SUMMON);
    }
}

/*!
 * @brief 7/8の確率で、5マス以内にトーテムモアイ自身を召喚する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param md_ptr モンスター撃破構造体への参照ポインタ
 */
static void on_dead_totem_moai(player_type *player_ptr, monster_death_type *md_ptr)
{
    summon_self(player_ptr, md_ptr, SUMMON_TOTEM_MOAI, 8, 5, _("新たなモアイが現れた！", "A new moai steps forth!"));
}

static void on_dead_demon_slayer_senior(player_type *player_ptr, monster_death_type *md_ptr)
{
    if (!is_seen(player_ptr, md_ptr->m_ptr))
        return;

    GAME_TEXT m_name[MAX_NLEN];
    monster_desc(player_ptr, m_name, md_ptr->m_ptr, MD_NONE);
    msg_format(_("あなたの闘気が%sの身体をサイコロ状に切り刻んだ！", "Your fighting spirit chopped %^s's body into dice!"), m_name);
}

static void on_dead_mirmulnir(player_type *player_ptr, monster_death_type *md_ptr)
{
    if (!is_seen(player_ptr, md_ptr->m_ptr))
        return;

    GAME_TEXT m_name[MAX_NLEN];
    monster_desc(player_ptr, m_name, md_ptr->m_ptr, MD_NONE);
    msg_format(_("%s「ドヴ＠ーキン、やめろぉ！」", "%^s says, 'Dov@hkiin! No!!'"), m_name);
}

static void on_dead_dragon_centipede(player_type *player_ptr, monster_death_type *md_ptr)
{
    if (player_ptr->current_floor_ptr->inside_arena || player_ptr->phase_out)
        return;

    bool notice = false;
    const int reproduced_centipede = 2;
    for (int i = 0; i < reproduced_centipede; i++) {
        POSITION wy = md_ptr->md_y;
        POSITION wx = md_ptr->md_x;
        bool pet = is_pet(md_ptr->m_ptr);
        BIT_FLAGS mode = pet ? PM_FORCE_PET : PM_NONE;
        MONSTER_IDX smaller_centipede = md_ptr->m_ptr->r_idx - 1;
        if (summon_named_creature(player_ptr, (pet ? -1 : md_ptr->m_idx), wy, wx, smaller_centipede, mode) && player_can_see_bold(player_ptr, wy, wx))
            notice = true;
    }

    GAME_TEXT m_name[MAX_NLEN];
    monster_desc(player_ptr, m_name, md_ptr->m_ptr, MD_NONE);
    if (notice) {
        msg_format(_("%sが再生した！", "The %s reproduced!"), m_name);
        sound(SOUND_SUMMON);
    }
}

/*!
 * @brief ビッグキューちゃん撃破時メッセージ
 * @todo 死亡時の特殊メッセージを表示するだけの処理を複数作るなら、switch/case文に分けられるように汎用化すること
 */
static void on_dead_big_raven(player_type *player_ptr, monster_death_type *md_ptr)
{
    if (!is_seen(player_ptr, md_ptr->m_ptr))
        return;

    GAME_TEXT m_name[MAX_NLEN];
    monster_desc(player_ptr, m_name, md_ptr->m_ptr, MD_NONE);
    msg_format(_("%sはお星さまになった！", "%^s became a constellation!"), m_name);
}

/*
 * @brief 装備品の生成を試みる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param q_ptr 生成中アイテムへの参照ポインタ
 * @param drop_mode ドロップ品の質
 * @param is_object_hook_null アイテム種別が何でもありならtrue、指定されていればfalse
 * @return 生成したアイテムが装備品ならtrue、それ以外ならfalse
 * @todo 汎用的に使えそうだがどこかにいいファイルはないか？
 */
static bool make_equipment(player_type *player_ptr, object_type *q_ptr, const BIT_FLAGS drop_mode, const bool is_object_hook_null)
{
    q_ptr->wipe();
    (void)make_object(player_ptr, q_ptr, drop_mode);
    if (!is_object_hook_null) {
        return true;
    }

    auto tval = q_ptr->tval;
    switch (tval) {
    case TV_BOW:
    case TV_DIGGING:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
    case TV_BOOTS:
    case TV_GLOVES:
    case TV_HELM:
    case TV_CROWN:
    case TV_SHIELD:
    case TV_CLOAK:
    case TV_SOFT_ARMOR:
    case TV_HARD_ARMOR:
    case TV_DRAG_ARMOR:
    case TV_LITE:
    case TV_AMULET:
    case TV_RING:
        return true;
    default:
        return false;
    }
}

/*
 * @brief 死亡時ドロップとしてランダムアーティファクトのみを生成する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param md_ptr モンスター撃破構造体への参照ポインタ
 * @param object_hook_pf アイテム種別指定、特になければnullptrで良い
 * @return なし
 * @details
 * 最初のアイテム生成でいきなり☆が生成された場合を除き、中途半端な☆ (例：呪われている)は生成しない.
 * このルーチンで★は生成されないので、★生成フラグのキャンセルも不要
 */
static void on_dead_random_artifact(player_type *player_ptr, monster_death_type *md_ptr, bool (*object_hook_pf)(KIND_OBJECT_IDX k_idx))
{
    object_type forge;
    object_type *q_ptr = &forge;
    auto is_object_hook_null = object_hook_pf == nullptr;
    auto drop_mode = md_ptr->mo_mode | AM_NO_FIXED_ART;
    while (true) {
        // make_object() の中でアイテム種別をキャンセルしている
        // よってこのwhileループ中へ入れないと、引数で指定していない種別のアイテムが選ばれる可能性がある
        get_obj_num_hook = object_hook_pf;
        if (!make_equipment(player_ptr, q_ptr, drop_mode, is_object_hook_null)) {
            continue;
        }

        if (q_ptr->art_name > 0) {
            break;
        }

        if ((q_ptr->name1 != 0) && q_ptr->name2 != 0) {
            continue;
        }

        (void)become_random_artifact(player_ptr, q_ptr, false);
        auto is_good_random_art = !q_ptr->is_cursed();
        is_good_random_art &= q_ptr->to_h > 0;
        is_good_random_art &= q_ptr->to_d > 0;
        is_good_random_art &= q_ptr->to_a > 0;
        is_good_random_art &= q_ptr->pval > 0;
        if (is_good_random_art) {
            break;
        }
    }

    (void)drop_near(player_ptr, q_ptr, -1, md_ptr->md_y, md_ptr->md_x);
}

/*!
 * @brief マニマニのあくま撃破時メッセージ
 * @todo 死亡時の特殊メッセージを表示するだけの処理を複数作るなら、switch/case文に分けられるように汎用化すること
 */
static void on_dead_manimani(player_type *player_ptr, monster_death_type *md_ptr)
{
    if (!is_seen(player_ptr, md_ptr->m_ptr))
        return;

    msg_print(_("どこからか声が聞こえる…「ハロー！　そして…グッドバイ！」", "Heard a voice from somewhere... 'Hello! And... good bye!'"));
}

static void drop_specific_item_on_dead(player_type *player_ptr, monster_death_type *md_ptr, bool (*object_hook_pf)(KIND_OBJECT_IDX k_idx))
{
    object_type forge;
    object_type *q_ptr = &forge;
    q_ptr->wipe();
    get_obj_num_hook = object_hook_pf;
    (void)make_object(player_ptr, q_ptr, md_ptr->mo_mode);
    (void)drop_near(player_ptr, q_ptr, -1, md_ptr->md_y, md_ptr->md_x);
}

static void on_dead_chest_mimic(player_type *player_ptr, monster_death_type *md_ptr)
{
    if (player_ptr->current_floor_ptr->inside_arena || player_ptr->phase_out)
        return;

    bool notice = false;
    monster_race_type mimic_inside;
    int num_summons;
    auto r_idx = md_ptr->m_ptr->r_idx;
    switch (r_idx) {
    case MON_CHEST_MIMIC_03:
        mimic_inside = MON_CHEST_MIMIC_02;
        num_summons = 1;
        break;
    case MON_CHEST_MIMIC_04:
        mimic_inside = MON_CHEST_MIMIC_03;
        num_summons = 1;
        break;
    case MON_CHEST_MIMIC_11:
        mimic_inside = MON_CHEST_MIMIC_04;
        num_summons = next_bool() ? 3 : 2;
        break;
    default:
        mimic_inside = (monster_race_type)-1;
        num_summons = 0;
        return;
    }

    for (auto i = 0; i < num_summons; i++) {
        auto wy = md_ptr->md_y;
        auto wx = md_ptr->md_x;
        auto pet = is_pet(md_ptr->m_ptr);
        auto mode = pet ? PM_FORCE_PET : PM_NONE;
        if (summon_named_creature(player_ptr, (pet ? -1 : md_ptr->m_idx), wy, wx, (MONSTER_IDX)mimic_inside, (BIT_FLAGS)mode)
            && player_can_see_bold(player_ptr, wy, wx)) {
            notice = true;
        }
    }

    if (notice) {
        msg_print(_("箱の中から新たなミミックが現れた！", "A new mimic appears in the dead mimic!"));
        sound(SOUND_SUMMON);
    }
}

static void on_dead_mimics(player_type *player_ptr, monster_death_type *md_ptr)
{
    if (!md_ptr->drop_chosen_item)
        return;

    switch (md_ptr->r_ptr->d_char) {
    case '(':
        if (player_ptr->current_floor_ptr->dun_level <= 0)
            return;

        drop_specific_item_on_dead(player_ptr, md_ptr, kind_is_cloak);
        return;
    case '/':
        if (player_ptr->current_floor_ptr->dun_level <= 4)
            return;

        drop_specific_item_on_dead(player_ptr, md_ptr, kind_is_polearm);
        return;
    case '[':
        if (player_ptr->current_floor_ptr->dun_level <= 19)
            return;

        drop_specific_item_on_dead(player_ptr, md_ptr, kind_is_armor);
        return;
    case '\\':
        if (player_ptr->current_floor_ptr->dun_level <= 4)
            return;

        drop_specific_item_on_dead(player_ptr, md_ptr, kind_is_hafted);
        return;
    case '|':
        if (md_ptr->m_ptr->r_idx == MON_STORMBRINGER)
            return;

        drop_specific_item_on_dead(player_ptr, md_ptr, kind_is_sword);
        return;
    case ']':
        if (player_ptr->current_floor_ptr->dun_level <= 19)
            return;

        drop_specific_item_on_dead(player_ptr, md_ptr, kind_is_boots);
        return;
    default:
        return;
    }
}

void switch_special_death(player_type *player_ptr, monster_death_type *md_ptr)
{
    switch (md_ptr->m_ptr->r_idx) {
    case MON_PINK_HORROR:
        on_dead_pink_horror(player_ptr, md_ptr);
        return;
    case MON_BLOODLETTER:
        on_dead_bloodletter(player_ptr, md_ptr);
        return;
    case MON_RAAL:
        on_dead_raal(player_ptr, md_ptr);
        return;
    case MON_DAWN:
        on_dead_dawn(player_ptr, md_ptr);
        return;
    case MON_UNMAKER:
        on_dead_unmaker(player_ptr, md_ptr);
        break;
    case MON_UNICORN_ORD:
    case MON_MORGOTH:
    case MON_ONE_RING:
        on_dead_sacred_treasures(player_ptr, md_ptr);
        return;
    case MON_SERPENT:
        on_dead_serpent(player_ptr, md_ptr);
        return;
    case MON_B_DEATH_SWORD:
        on_dead_death_sword(player_ptr, md_ptr);
        return;
    case MON_A_GOLD:
    case MON_A_SILVER:
        on_dead_can_angel(player_ptr, md_ptr);
        return;
    case MON_ROLENTO:
        on_dead_rolento(player_ptr, md_ptr);
        return;
    case MON_MIDDLE_AQUA_FIRST:
    case MON_LARGE_AQUA_FIRST:
    case MON_EXTRA_LARGE_AQUA_FIRST:
    case MON_MIDDLE_AQUA_SECOND:
    case MON_LARGE_AQUA_SECOND:
    case MON_EXTRA_LARGE_AQUA_SECOND:
        on_dead_aqua_illusion(player_ptr, md_ptr);
        return;
    case MON_TOTEM_MOAI:
        on_dead_totem_moai(player_ptr, md_ptr);
        return;
    case MON_DEMON_SLAYER_SENIOR:
        on_dead_demon_slayer_senior(player_ptr, md_ptr);
        return;
    case MON_MIRMULNIR:
        on_dead_mirmulnir(player_ptr, md_ptr);
        return;
    case MON_DRAGON_CENTIPEDE:
    case MON_DRAGON_WORM:
        on_dead_dragon_centipede(player_ptr, md_ptr);
        return;
    case MON_CAIT_SITH:
        drop_specific_item_on_dead(player_ptr, md_ptr, kind_is_boots);
        return;
    case MON_BIG_RAVEN:
        on_dead_big_raven(player_ptr, md_ptr);
        return;
    case MON_YENDOR_WIZARD_1:
        on_dead_random_artifact(player_ptr, md_ptr, kind_is_amulet);
        return;
    case MON_YENDOR_WIZARD_2:
        drop_specific_item_on_dead(player_ptr, md_ptr, kind_is_amulet);
        return;
    case MON_MANIMANI:
        on_dead_manimani(player_ptr, md_ptr);
        return;
    case MON_LOSTRINGIL:
        on_dead_random_artifact(player_ptr, md_ptr, kind_is_sword);
        return;
    case MON_CHEST_MIMIC_03:
    case MON_CHEST_MIMIC_04:
    case MON_CHEST_MIMIC_11:
        on_dead_chest_mimic(player_ptr, md_ptr);
        break;
    default:
        on_dead_mimics(player_ptr, md_ptr);
        return;
    }
}
