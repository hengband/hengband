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
#include "floor/floor-object.h"
#include "floor/floor-util.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-death-util.h"
#include "monster-floor/monster-death.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/item-magic-applier.h"
#include "object/object-kind-hook.h"
#include "spell/summon-types.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/artifact-type-definition.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 死亡召喚に使用するモード選択
 * @param md_ptr モンスター撃破構造体への参照ポインタ
 * @return 撃破モンスターがPETであればPM_FORCE_PETを、CLONEであればPM_CLONEを立てる
 */
static BIT_FLAGS dead_mode(MonsterDeath *md_ptr)
{
    bool pet = md_ptr->m_ptr->is_pet();
    bool clone = md_ptr->m_ptr->mflag2.has(MonsterConstantFlagType::CLONED);
    BIT_FLAGS mode = pet ? PM_FORCE_PET : PM_NONE;
    if (clone) {
        mode = mode | PM_CLONE;
    }

    return mode;
}

/*!
 * @brief 死亡時召喚処理 (今のところ自分自身のみ)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param md_ptr モンスター撃破構造体への参照ポインタ
 * @param type 召喚タイプ
 * @param probability 召喚確率 (計算式：1 - 1/probability)
 * @param radius 召喚半径 (モンスターが死亡した座標から半径何マス以内に召喚させるか)
 * @param message 召喚時のメッセージ
 */
static void summon_self(PlayerType *player_ptr, MonsterDeath *md_ptr, summon_type type, int probability, POSITION radius, concptr message)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    if (floor.inside_arena || AngbandSystem::get_instance().is_phase_out() || one_in_(probability)) {
        return;
    }

    const auto p_pos = player_ptr->get_position();
    auto m_pos = md_ptr->get_position();
    auto attempts = 0;
    for (; attempts < 100; attempts++) {
        m_pos = scatter(player_ptr, md_ptr->get_position(), radius, PROJECT_NONE);
        if (floor.contains(m_pos) && floor.can_generate_monster_at(m_pos) && (p_pos != m_pos)) {
            break;
        }
    }

    if (attempts == 100) {
        return;
    }

    BIT_FLAGS mode = dead_mode(md_ptr);
    if (summon_specific(player_ptr, m_pos.y, m_pos.x, 100, type, mode) && player_can_see_bold(player_ptr, m_pos.y, m_pos.x)) {
        msg_print(message);
    }
}

static void on_dead_pink_horror(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    if (player_ptr->current_floor_ptr->inside_arena || AngbandSystem::get_instance().is_phase_out()) {
        return;
    }

    bool notice = false;
    const int blue_horrors = 2;
    for (int i = 0; i < blue_horrors; i++) {
        POSITION wy = md_ptr->md_y;
        POSITION wx = md_ptr->md_x;
        BIT_FLAGS mode = dead_mode(md_ptr);
        if (summon_specific(player_ptr, wy, wx, 100, SUMMON_BLUE_HORROR, mode) && player_can_see_bold(player_ptr, wy, wx)) {
            notice = true;
        }
    }

    if (notice) {
        sound(SoundKind::SUMMON);
        msg_print(_("ピンク・ホラーは分裂した！", "The Pink horror divides!"));
    }
}

static void on_dead_bloodletter(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    if (!md_ptr->drop_chosen_item || (randint1(100) >= 15)) {
        return;
    }

    ItemEntity item({ ItemKindType::SWORD, SV_BLADE_OF_CHAOS });
    ItemMagicApplier(player_ptr, &item, player_ptr->current_floor_ptr->object_level, AM_NO_FIXED_ART | md_ptr->mo_mode).execute();
    (void)drop_near(player_ptr, &item, md_ptr->get_position());
}

static void on_dead_raal(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    if (!md_ptr->drop_chosen_item || (floor.dun_level <= 9)) {
        return;
    }

    ItemEntity forge;
    auto *q_ptr = &forge;
    q_ptr->wipe();
    if ((floor.dun_level > 49) && one_in_(5)) {
        select_baseitem_id_hook = kind_is_good_book;
    } else {
        select_baseitem_id_hook = kind_is_book;
    }

    (void)make_object(player_ptr, q_ptr, md_ptr->mo_mode);
    (void)drop_near(player_ptr, q_ptr, md_ptr->get_position());
}

/*!
 * @brief 6/7の確率で、20マス以内に暁の戦士自身を召喚する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param md_ptr モンスター撃破構造体への参照ポインタ
 */
static void on_dead_dawn(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    summon_self(player_ptr, md_ptr, SUMMON_DAWN, 7, 20, _("新たな戦士が現れた！", "A new warrior steps forth!"));
}

static void on_dead_sacred_treasures(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    if ((player_ptr->ppersonality != PERSONALITY_LAZY) || !md_ptr->drop_chosen_item) {
        return;
    }

    constexpr static auto namake_equipments = {
        FixedArtifactId::NAMAKE_HAMMER,
        FixedArtifactId::NAMAKE_BOW,
        FixedArtifactId::NAMAKE_ARMOR
    };

    std::vector<FixedArtifactId> candidates;
    std::copy_if(namake_equipments.begin(), namake_equipments.end(), std::back_inserter(candidates),
        [](FixedArtifactId a_idx) {
            const auto &artifact = ArtifactList::get_instance().get_artifact(a_idx);
            return !artifact.is_generated;
        });

    if (candidates.empty()) {
        return;
    }

    const auto a_idx = rand_choice(candidates);
    create_named_art(player_ptr, a_idx, md_ptr->md_y, md_ptr->md_x);
}

static void on_dead_serpent(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    if (!md_ptr->drop_chosen_item) {
        return;
    }

    ItemEntity item_grond({ ItemKindType::HAFTED, SV_GROND });
    item_grond.fa_id = FixedArtifactId::GROND;
    ItemMagicApplier(player_ptr, &item_grond, -1, AM_GOOD | AM_GREAT).execute();
    (void)drop_near(player_ptr, &item_grond, md_ptr->get_position());

    ItemEntity item_chaos({ ItemKindType::CROWN, SV_CHAOS });
    item_chaos.fa_id = FixedArtifactId::CHAOS;
    ItemMagicApplier(player_ptr, &item_chaos, -1, AM_GOOD | AM_GREAT).execute();
    (void)drop_near(player_ptr, &item_chaos, md_ptr->get_position());
}

static void on_dead_death_sword(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    if (!md_ptr->drop_chosen_item) {
        return;
    }

    ItemEntity item({ ItemKindType::SWORD, randint1(2) });
    (void)drop_near(player_ptr, &item, md_ptr->get_position());
}

static void on_dead_can_angel(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    auto is_drop_can = md_ptr->drop_chosen_item;
    auto is_silver = md_ptr->m_ptr->r_idx == MonraceId::A_SILVER;
    is_silver &= md_ptr->r_ptr->r_akills % 5 == 0;
    is_drop_can &= (md_ptr->m_ptr->r_idx == MonraceId::A_GOLD) || is_silver;
    if (!is_drop_can) {
        return;
    }

    ItemEntity item({ ItemKindType::CHEST, SV_CHEST_KANDUME });
    ItemMagicApplier(player_ptr, &item, player_ptr->current_floor_ptr->object_level, AM_NO_FIXED_ART).execute();
    (void)drop_near(player_ptr, &item, md_ptr->get_position());
}

static void on_dead_aqua_illusion(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    if (player_ptr->current_floor_ptr->inside_arena || AngbandSystem::get_instance().is_phase_out()) {
        return;
    }

    bool notice = false;
    const int popped_bubbles = 4;
    for (int i = 0; i < popped_bubbles; i++) {
        POSITION wy = md_ptr->md_y;
        POSITION wx = md_ptr->md_x;
        bool pet = md_ptr->m_ptr->is_pet();
        BIT_FLAGS mode = dead_mode(md_ptr);
        auto smaller_bubble = md_ptr->m_ptr->r_idx - 1;
        if (summon_named_creature(player_ptr, (pet ? -1 : md_ptr->m_idx), wy, wx, smaller_bubble, mode) && player_can_see_bold(player_ptr, wy, wx)) {
            notice = true;
        }
    }

    if (notice) {
        msg_print(_("泡が弾けた！", "The bubble pops!"));
        sound(SoundKind::SUMMON);
    }
}

/*!
 * @brief 7/8の確率で、5マス以内にトーテムモアイ自身を召喚する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param md_ptr モンスター撃破構造体への参照ポインタ
 */
static void on_dead_totem_moai(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    summon_self(player_ptr, md_ptr, SUMMON_TOTEM_MOAI, 8, 5, _("新たなモアイが現れた！", "A new moai steps forth!"));
}

static void on_dead_dragon_centipede(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    if (player_ptr->current_floor_ptr->inside_arena || AngbandSystem::get_instance().is_phase_out()) {
        return;
    }

    bool notice = false;
    const int reproduced_centipede = 2;
    for (int i = 0; i < reproduced_centipede; i++) {
        POSITION wy = md_ptr->md_y;
        POSITION wx = md_ptr->md_x;
        bool pet = md_ptr->m_ptr->is_pet();
        BIT_FLAGS mode = dead_mode(md_ptr);

        auto smaller_centipede = md_ptr->m_ptr->r_idx - 1;
        if (summon_named_creature(player_ptr, (pet ? -1 : md_ptr->m_idx), wy, wx, smaller_centipede, mode) && player_can_see_bold(player_ptr, wy, wx)) {
            notice = true;
        }
    }

    if (notice) {
        const auto m_name = monster_desc(player_ptr, *md_ptr->m_ptr, MD_NONE);
        msg_format(_("%sが再生した！", "The %s reproduced!"), m_name.data());
        sound(SoundKind::SUMMON);
    }
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
static bool make_equipment(PlayerType *player_ptr, ItemEntity *q_ptr, const BIT_FLAGS drop_mode, const bool is_object_hook_null)
{
    q_ptr->wipe();
    (void)make_object(player_ptr, q_ptr, drop_mode);
    if (!is_object_hook_null) {
        return true;
    }

    return q_ptr->is_wearable() && (q_ptr->bi_key.tval() != ItemKindType::CARD);
}

/*
 * @brief 死亡時ドロップとしてランダムアーティファクトのみを生成する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param md_ptr モンスター撃破構造体への参照ポインタ
 * @param hook_pf アイテム種別指定、特になければnullptrで良い
 * @return なし
 * @details
 * 最初のアイテム生成でいきなり☆が生成された場合を除き、中途半端な☆ (例：呪われている)は生成しない.
 * このルーチンで★は生成されないので、★生成フラグのキャンセルも不要
 */
static void on_dead_random_artifact(PlayerType *player_ptr, MonsterDeath *md_ptr, bool (*hook_pf)(short bi_id))
{
    ItemEntity forge;
    auto *q_ptr = &forge;
    auto is_object_hook_null = hook_pf == nullptr;
    auto drop_mode = md_ptr->mo_mode | AM_NO_FIXED_ART;
    while (true) {
        // make_object() の中でアイテム種別をキャンセルしている
        // よってこのwhileループ中へ入れないと、引数で指定していない種別のアイテムが選ばれる可能性がある
        select_baseitem_id_hook = hook_pf;
        if (!make_equipment(player_ptr, q_ptr, drop_mode, is_object_hook_null)) {
            continue;
        }

        if (q_ptr->is_random_artifact()) {
            break;
        }

        if (q_ptr->is_ego()) {
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

    (void)drop_near(player_ptr, q_ptr, md_ptr->get_position());
}

/*!
 * @brief マニマニのあくま撃破時メッセージ
 * @todo 死亡時の特殊メッセージを表示するだけの処理を複数作るなら、switch/case文に分けられるように汎用化すること
 */
static void on_dead_manimani(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    if (!is_seen(player_ptr, *md_ptr->m_ptr)) {
        return;
    }

    msg_print(_("どこからか声が聞こえる…「ハロー！　そして…グッドバイ！」", "Heard a voice from somewhere... 'Hello! And... good bye!'"));
}

static void drop_specific_item_on_dead(PlayerType *player_ptr, MonsterDeath *md_ptr, bool (*object_hook_pf)(short bi_id))
{
    ItemEntity forge;
    auto *q_ptr = &forge;
    q_ptr->wipe();
    select_baseitem_id_hook = object_hook_pf;
    (void)make_object(player_ptr, q_ptr, md_ptr->mo_mode);
    (void)drop_near(player_ptr, q_ptr, md_ptr->get_position());
}

static void on_dead_chest_mimic(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    if (player_ptr->current_floor_ptr->inside_arena || AngbandSystem::get_instance().is_phase_out()) {
        return;
    }

    auto notice = false;
    auto mimic_inside = MonraceList::empty_id();
    auto num_summons = 0;
    switch (md_ptr->m_ptr->r_idx) {
    case MonraceId::CHEST_MIMIC_03:
        mimic_inside = MonraceId::CHEST_MIMIC_02;
        num_summons = 1;
        break;
    case MonraceId::CHEST_MIMIC_04:
        mimic_inside = MonraceId::CHEST_MIMIC_03;
        num_summons = 1;
        break;
    case MonraceId::CHEST_MIMIC_11:
        mimic_inside = MonraceId::CHEST_MIMIC_04;
        num_summons = one_in_(2) ? 3 : 2;
        break;
    default:
        return;
    }

    for (auto i = 0; i < num_summons; i++) {
        auto wy = md_ptr->md_y;
        auto wx = md_ptr->md_x;
        auto pet = md_ptr->m_ptr->is_pet();
        BIT_FLAGS mode = dead_mode(md_ptr);
        if (summon_named_creature(player_ptr, (pet ? -1 : md_ptr->m_idx), wy, wx, mimic_inside, (BIT_FLAGS)mode) && player_can_see_bold(player_ptr, wy, wx)) {
            notice = true;
        }
    }

    if (notice) {
        msg_print(_("箱の中から新たなミミックが現れた！", "A new mimic appears in the dead mimic!"));
        sound(SoundKind::SUMMON);
    }
}

static void on_dead_mimics(PlayerType *player_ptr, MonsterDeath *md_ptr)
{
    if (!md_ptr->drop_chosen_item) {
        return;
    }

    switch (md_ptr->r_ptr->symbol_definition.character) {
    case '(':
        if (player_ptr->current_floor_ptr->dun_level <= 0) {
            return;
        }

        drop_specific_item_on_dead(player_ptr, md_ptr, kind_is_cloak);
        return;
    case '/':
        if (player_ptr->current_floor_ptr->dun_level <= 4) {
            return;
        }

        drop_specific_item_on_dead(player_ptr, md_ptr, kind_is_polearm);
        return;
    case '[':
        if (player_ptr->current_floor_ptr->dun_level <= 19) {
            return;
        }

        drop_specific_item_on_dead(player_ptr, md_ptr, kind_is_armor);
        return;
    case '\\':
        if (player_ptr->current_floor_ptr->dun_level <= 4) {
            return;
        }

        drop_specific_item_on_dead(player_ptr, md_ptr, kind_is_hafted);
        return;
    case '|':
        if (md_ptr->m_ptr->r_idx == MonraceId::STORMBRINGER) {
            return;
        }

        drop_specific_item_on_dead(player_ptr, md_ptr, kind_is_sword);
        return;
    case ']':
        if (player_ptr->current_floor_ptr->dun_level <= 19) {
            return;
        }

        drop_specific_item_on_dead(player_ptr, md_ptr, kind_is_boots);
        return;
    default:
        return;
    }
}

static void on_dead_swordfish(PlayerType *player_ptr, MonsterDeath *md_ptr, AttributeFlags attribute_flags)
{
    if (attribute_flags.has_not(AttributeType::COLD) || !md_ptr->drop_chosen_item || (randint1(100) >= 10)) {
        return;
    }

    drop_single_artifact(player_ptr, md_ptr, FixedArtifactId::FROZEN_SWORDFISH);
}

void switch_special_death(PlayerType *player_ptr, MonsterDeath *md_ptr, AttributeFlags attribute_flags)
{
    switch (md_ptr->m_ptr->r_idx) {
    case MonraceId::PINK_HORROR:
        on_dead_pink_horror(player_ptr, md_ptr);
        return;
    case MonraceId::BLOODLETTER:
        on_dead_bloodletter(player_ptr, md_ptr);
        return;
    case MonraceId::RAAL:
        on_dead_raal(player_ptr, md_ptr);
        return;
    case MonraceId::DAWN:
        on_dead_dawn(player_ptr, md_ptr);
        return;
    case MonraceId::UNMAKER:
        (void)project(player_ptr, md_ptr->m_idx, 6, md_ptr->md_y, md_ptr->md_x, 100, AttributeType::CHAOS, PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
        break;
    case MonraceId::UNICORN_ORD:
    case MonraceId::MORGOTH:
    case MonraceId::ONE_RING:
        on_dead_sacred_treasures(player_ptr, md_ptr);
        return;
    case MonraceId::SERPENT:
        on_dead_serpent(player_ptr, md_ptr);
        return;
    case MonraceId::B_DEATH_SWORD:
        on_dead_death_sword(player_ptr, md_ptr);
        return;
    case MonraceId::A_GOLD:
    case MonraceId::A_SILVER:
        on_dead_can_angel(player_ptr, md_ptr);
        return;
    case MonraceId::ROLENTO:
        (void)project(player_ptr, md_ptr->m_idx, 3, md_ptr->md_y, md_ptr->md_x, Dice::roll(20, 10), AttributeType::FIRE, PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
        return;
    case MonraceId::MIDDLE_AQUA_FIRST:
    case MonraceId::LARGE_AQUA_FIRST:
    case MonraceId::EXTRA_LARGE_AQUA_FIRST:
    case MonraceId::MIDDLE_AQUA_SECOND:
    case MonraceId::LARGE_AQUA_SECOND:
    case MonraceId::EXTRA_LARGE_AQUA_SECOND:
        on_dead_aqua_illusion(player_ptr, md_ptr);
        return;
    case MonraceId::TOTEM_MOAI:
        on_dead_totem_moai(player_ptr, md_ptr);
        return;
    case MonraceId::DRAGON_CENTIPEDE:
    case MonraceId::DRAGON_WORM:
        on_dead_dragon_centipede(player_ptr, md_ptr);
        return;
    case MonraceId::CAIT_SITH:
        if (player_ptr->current_floor_ptr->dun_level <= 0) {
            return;
        }
        drop_specific_item_on_dead(player_ptr, md_ptr, kind_is_boots);
        return;
    case MonraceId::YENDOR_WIZARD_1:
        on_dead_random_artifact(player_ptr, md_ptr, kind_is_amulet);
        return;
    case MonraceId::YENDOR_WIZARD_2:
        if (player_ptr->current_floor_ptr->dun_level <= 0) {
            return;
        }
        drop_specific_item_on_dead(player_ptr, md_ptr, kind_is_amulet);
        return;
    case MonraceId::MANIMANI:
        on_dead_manimani(player_ptr, md_ptr);
        return;
    case MonraceId::LOSTRINGIL:
        on_dead_random_artifact(player_ptr, md_ptr, kind_is_sword);
        return;
    case MonraceId::CHEST_MIMIC_03:
    case MonraceId::CHEST_MIMIC_04:
    case MonraceId::CHEST_MIMIC_11:
        on_dead_chest_mimic(player_ptr, md_ptr);
        break;
    case MonraceId::SWORDFISH:
        on_dead_swordfish(player_ptr, md_ptr, attribute_flags);
        break;
    default:
        on_dead_mimics(player_ptr, md_ptr);
        return;
    }
}
