#include "monster/monster-pain-describer.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-status.h"
#include "system/floor-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "term/z-form.h"
#include "util/string-processor.h"
#include <functional>
#include <vector>

struct pain_message_type {
    // message_table を選択するかどうかを判定する関数
    std::function<bool(const MonsterEntity &)> pred;

    // percentage, message のペアのリスト
    // (ダメージを受けた後のHP / ダメージを受ける前のHP)[%]が percentage 以上ならそのメッセージが選択される
    std::map<int, concptr, std::greater<int>> message_table;
};

struct pain_message_type_of_diminisher {
    // message_table を選択するかどうかを判定する関数
    std::function<bool(const MonsterEntity &)> pred;

    // percentage, message のペアのリスト
    // (最大HP / ダメージを受ける前のHP)[%]が percentage1 以上、
    // （ダメージキャップ / 受けたダメージ)[%]が percentage2 以上ならそのメッセージが選択される
    std::map<int, std::map<int, concptr, std::greater<int>>, std::greater<int>> message_table;
};

/*!
 * @brief MonsterEntity を引数を受け取り文字列 symbols の中にそのモンスターのシンボルが含まれていれば true を返す関数オブジェクトを生成する
 *
 * @param symbols 文字が含まれるか調べる文字列
 * @return 生成した関数オブジェクト(ラムダ式)
 */
static auto d_char_is_any_of(concptr symbols)
{
    return [symbols](const MonsterEntity &monster) {
        const auto &monrace = monster.get_monrace();
        return angband_strchr(symbols, monrace.d_char) != nullptr;
    };
}

static bool is_personified(const MonsterEntity &m_info)
{
    switch (m_info.r_idx) {
    case MonsterRaceId::LAFFEY_II:
        return true;
    default:
        return false;
    }
}

static const std::map<int, concptr, std::greater<int>> pain_messages_common = {
    { 95, _("は攻撃に肩をすくめた。", " shrugs off the attack.") },
    { 75, _("は痛みでうなった。", " grunts with pain.") },
    { 50, _("は痛みで叫んだ。", " cries out in pain.") },
    { 35, _("は痛みで絶叫した。", " screams in pain.") },
    { 20, _("は苦痛のあまり絶叫した。", " screams in agony.") },
    { 10, _("は苦痛でもだえ苦しんだ。", " writhes in agony.") },
    { 0, _("は弱々しく叫んだ。", " cries out feebly.") },
};

static const std::vector<pain_message_type>
    pain_messages{
        { is_personified, pain_messages_common },
        { d_char_is_any_of(",ejmvwQ"), {
                                           { 95, _("はほとんど気にとめていない。", " barely notices.") },
                                           { 75, _("はしり込みした。", " flinches.") },
                                           { 50, _("は縮こまった。", " squelches.") },
                                           { 35, _("は痛みに震えた。", " quivers in pain.") },
                                           { 20, _("は身もだえした。", " writhes about.") },
                                           { 10, _("は苦痛で身もだえした。", " writhes in agony.") },
                                           { 0, _("はぐにゃぐにゃと痙攣した。", " jerks limply.") },
                                       } },
        { d_char_is_any_of("l"), {
                                     { 95, _("はほとんど気にとめていない。", " barely notices.") },
                                     { 75, _("はしり込みした。", " flinches.") },
                                     { 50, _("は躊躇した。", " hesitates.") },
                                     { 35, _("は痛みに震えた。", " quivers in pain.") },
                                     { 20, _("は身もだえした。", " writhes about.") },
                                     { 10, _("は苦痛で身もだえした。", " writhes in agony.") },
                                     { 0, _("はぐにゃぐにゃと痙攣した。", " jerks limply.") },
                                 } },
        { d_char_is_any_of("g#+<>"), {
                                         { 95, _("は攻撃に肩をすくめた。", " shrugs off the attack.") },
                                         { 75, _("は雷鳴のように吠えた。", " roars thunderously.") },
                                         { 50, _("は苦しげに吠えた。", " rumbles.") },
                                         { 35, _("はうめいた。", " grunts.") },
                                         { 20, _("は攻撃を気にとめていない。", " ignores the attack.") },
                                         { 10, _("は躊躇した。", " hesitates.") },
                                         { 0, _("はくしゃくしゃになった。", " crumples.") },
                                     } },
        { [](const MonsterEntity &monster) {
        const auto &monrace = monster.get_monrace();
        return d_char_is_any_of("JMR")(monster) || !isalpha(monrace.d_char); }, {
                                                    { 95, _("はほとんど気にとめていない。", " barely notices.") },
                                                    { 75, _("はシーッと鳴いた。", " hisses.") },
                                                    { 50, _("は怒って頭を上げた。", " rears up in anger.") },
                                                    { 35, _("は猛然と威嚇した。", " hisses furiously.") },
                                                    { 20, _("は身もだえした。", " writhes about.") },
                                                    { 10, _("は苦痛で身もだえした。", " writhes in agony.") },
                                                    { 0, _("はぐにゃぐにゃと痙攣した。", " jerks limply.") },
                                                } },
        { d_char_is_any_of("f"), {
                                     { 95, _("は攻撃に肩をすくめた。", " shrugs off the attack.") },
                                     { 75, _("は吠えた。", " roars.") },
                                     { 50, _("は怒って吠えた。", " growls angrily.") },
                                     { 35, _("は痛みでシーッと鳴いた。", " hisses with pain.") },
                                     { 20, _("は痛みで弱々しく鳴いた。", " mewls in pain.") },
                                     { 10, _("は苦痛にうめいた。", " hisses in agony.") },
                                     { 0, _("は哀れな鳴き声を出した。", " mewls pitifully.") },
                                 } },
        { d_char_is_any_of("acFIKS"), {
                                          { 95, _("は攻撃を気にとめていない。", " ignores the attack.") },
                                          { 75, _("はキーキー鳴いた。", " chitters.") },
                                          { 50, _("はヨロヨロ逃げ回った。", " scuttles about.") },
                                          { 35, _("はうるさく鳴いた。", " twitters.") },
                                          { 20, _("は痛みに痙攣した。", " jerks in pain.") },
                                          { 10, _("は苦痛で痙攣した。", " jerks in agony.") },
                                          { 0, _("はピクピクひきつった。", " twitches.") },
                                      } },
        { d_char_is_any_of("B"), {
                                     { 95, _("はさえずった。", " chirps.") },
                                     { 75, _("はピーピー鳴いた。", " twitters.") },
                                     { 50, _("はギャーギャー鳴いた。", " squawks.") },
                                     { 35, _("はギャーギャー鳴きわめいた。", " chatters.") },
                                     { 20, _("は苦しんだ。", " jeers.") },
                                     { 10, _("はのたうち回った。", " flutters about.") },
                                     { 0, _("はキーキーと鳴き叫んだ。", " squeaks.") },
                                 } },
        { d_char_is_any_of("duDLUW"), {
                                          { 95, _("は攻撃を気にとめていない。", " ignores the attack.") },
                                          { 75, _("はしり込みした。", " flinches.") },
                                          { 50, _("は痛みでシーッと鳴いた。", " hisses in pain.") },
                                          { 35, _("は痛みでうなった。", " snarls with pain.") },
                                          { 20, _("は痛みに吠えた。", " roars with pain.") },
                                          { 10, _("は苦しげに叫んだ。", " gasps.") },
                                          { 0, _("は弱々しくうなった。", " snarls feebly.") },
                                      } },
        { d_char_is_any_of("s"), {
                                     { 95, _("は攻撃を気にとめていない。", " ignores the attack.") },
                                     { 75, _("は攻撃に肩をすくめた。", " shrugs off the attack.") },
                                     { 50, _("はカタカタと笑った。", " rattles.") },
                                     { 35, _("はよろめいた。", " stumbles.") },
                                     { 20, _("はカタカタ言った。", " rattles.") },
                                     { 10, _("はよろめいた。", " staggers.") },
                                     { 0, _("はガタガタ言った。", " clatters.") },
                                 } },
        { d_char_is_any_of("z"), {
                                     { 95, _("は攻撃を気にとめていない。", " ignores the attack.") },
                                     { 75, _("は攻撃に肩をすくめた。", " shrugs off the attack.") },
                                     { 50, _("はうめいた。", " groans.") },
                                     { 35, _("は苦しげにうめいた。", " moans.") },
                                     { 20, _("は躊躇した。", " hesitates.") },
                                     { 10, _("はうなった。", " grunts.") },
                                     { 0, _("はよろめいた。", " staggers.") },
                                 } },
        { d_char_is_any_of("G"), {
                                     { 95, _("は攻撃を気にとめていない。", " ignores the attack.") },
                                     { 75, _("は攻撃に肩をすくめた。", " shrugs off the attack.") },
                                     { 50, _("はうめいた。", " moans.") },
                                     { 35, _("は泣きわめいた。", " wails.") },
                                     { 20, _("は吠えた。", " howls.") },
                                     { 10, _("は弱々しくうめいた。", " moans softly.") },
                                     { 0, _("はかすかにうめいた。", " sighs.") },
                                 } },
        { d_char_is_any_of("CZ"), {
                                      { 95, _("は攻撃に肩をすくめた。", " shrugs off the attack.") },
                                      { 75, _("は痛みでうなった。", " snarls with pain.") },
                                      { 50, _("は痛みでキャンキャン吠えた。", " yelps in pain.") },
                                      { 35, _("は痛みで鳴きわめいた。", " howls in pain.") },
                                      { 20, _("は苦痛のあまり鳴きわめいた。", " howls in agony.") },
                                      { 10, _("は苦痛でもだえ苦しんだ。", " writhes in agony.") },
                                      { 0, _("は弱々しく吠えた。", " yelps feebly.") },
                                  } },
        { d_char_is_any_of("Xbilqrt"), {
                                           { 95, _("は攻撃を気にとめていない。", " ignores the attack.") },
                                           { 75, _("は痛みでうなった。", " grunts with pain.") },
                                           { 50, _("は痛みで叫んだ。", " squeals in pain.") },
                                           { 35, _("は痛みで絶叫した。", " shrieks in pain.") },
                                           { 20, _("は苦痛のあまり絶叫した。", " shrieks in agony.") },
                                           { 10, _("は苦痛でもだえ苦しんだ。", " writhes in agony.") },
                                           { 0, _("は弱々しく叫んだ。", " cries out feebly.") },
                                       } },
        { [](const MonsterEntity &) { return true; }, pain_messages_common },
    };

static const std::vector<pain_message_type_of_diminisher>
    pain_massages_of_diminisher{
        { [](const MonsterEntity &) { return true; }, {
                                                          { 80, {
                                                                    { 0, _("は余裕そうな表情を見せている。", " has usual expression.") },
                                                                } },
                                                          { 50, {
                                                                    { 75, _("は痛みで声を漏らした。", " gasps by pain.") },
                                                                    { 0, _("は攻撃を気にとめていない。", " ignores the attack.") },
                                                                } },
                                                          { 25, {
                                                                    { 50, _("は痛みでうなった。", " grunts with pain.") },
                                                                    { 0, _("は怒った。", " angers.") },
                                                                } },
                                                          { 15, {
                                                                    { 75, _("は半泣きでもだえた。", " is doubled over with close to crying.") },
                                                                    { 30, _("は痛みでうめいた。", " groans with pain.") },
                                                                    { 0, _("は息を荒げながら怒った。", " angers in a pant.") },
                                                                } },
                                                          { 10, {
                                                                    { 30, _("は目に涙を浮かべながら怒った。", " angers with tear-filled eyes.") },
                                                                    { 0, _("は息を切らしながら怒った。", " angers in a gasp.") },
                                                                } },
                                                          { 6, {
                                                                   { 30, _("は痛みで叫んだ。", " cries out in pain.") },
                                                                   { 0, _("は目に涙を浮かべながらうめいた。", " groans with tear-filled eyes.") },
                                                               } },
                                                          { 3, {
                                                                   { 50, _("は苦痛のあまり絶叫した。", "shrieks in agony.") },
                                                                   { 30, _("は痛みで絶叫した。", "shrieks in pain.") },
                                                                   { 0, _("は目に涙を浮かべながら怒った。", " angers with tear-filled eyes.") },
                                                               } },
                                                          { 0, {
                                                                   { 10, _("は弱々しく叫んだ。", "cries out feebly.") },
                                                                   { 0, _("は苦痛に満ちた表情でフラフラしている。", " staggers with expression in agony.") },
                                                               } },
                                                      } },
    };

MonsterPainDescriber::MonsterPainDescriber(PlayerType *player_ptr, const MonsterEntity *m_ptr)
    : player_ptr(player_ptr)
    , m_ptr(m_ptr)
{
}

MonsterPainDescriber::MonsterPainDescriber(PlayerType *player_ptr, MONSTER_IDX m_idx)
    : MonsterPainDescriber(player_ptr, &player_ptr->current_floor_ptr->m_list[m_idx])
{
}

/*!
 * @brief ダメージを受けたモンスターの様子を記述する
 * @param dam モンスターが受けたダメージ
 * @return std::string ダメージを受けたモンスターの様子を表す文字列。表示すべき様子が無い場合は空文字列。
 */
std::string MonsterPainDescriber::describe_normal(int dam, std::string m_name)
{
    const auto newhp = m_ptr->hp;
    const auto oldhp = newhp + dam;
    const auto percentage = std::max((newhp * 100) / oldhp, 0);

    for (const auto &[pred, table] : pain_messages) {
        if (!pred(*this->m_ptr)) {
            continue;
        }

        const auto msg = table.lower_bound(percentage)->second;
        return format("%s^%s", m_name.data(), msg);
    }

    return "";
}

/*!
 * @brief ダメージを受けたモンスターの様子を記述する（ダメージキャップ持ち）
 * @details ダメージキャップ持ちは（ダメージを受けた後のHP / ダメージを受ける前のHP)[%]だとほぼ機能しないため、判定を特殊にしている
 * @param dam モンスターが受けたダメージ
 * @return std::string ダメージを受けたモンスターの様子を表す文字列。表示すべき様子が無い場合は空文字列。
 */
std::string MonsterPainDescriber::describe_diminisher(int dam, std::string m_name)
{
    const auto oldhp = m_ptr->hp + dam;
    const auto &[damage_cap_normal, damage_cap_min] = get_damage_cap(oldhp, m_ptr->maxhp);
    const auto damage_cap = std::max(damage_cap_normal, damage_cap_min);
    const auto percentage = std::max((oldhp * 100) / m_ptr->maxhp, 0);
    const auto percentage_damage_cap = std::max((dam * 100) / damage_cap, 0);

    for (const auto &[pred, table] : pain_massages_of_diminisher) {
        if (!pred(*this->m_ptr)) {
            continue;
        }

        const auto &table2 = table.lower_bound(percentage)->second;
        const auto msg = table2.lower_bound(percentage_damage_cap)->second;

        return format("%s^%s", m_name.data(), msg);
    }

    return "";
}

/*!
 * @brief ダメージを受けたモンスターの様子を記述する
 * @param dam モンスターが受けたダメージ
 * @return std::string ダメージを受けたモンスターの様子を表す文字列。表示すべき様子が無い場合は空文字列。
 */
std::string MonsterPainDescriber::describe(int dam)
{
    const auto m_name = monster_desc(player_ptr, this->m_ptr, 0);

    if (dam == 0) {
        if (this->m_ptr->ml) {
            return format(_("%s^はダメージを受けていない。", "%s^ is unharmed."), m_name.data());
        }
        return "";
    }

    const auto &monrace = m_ptr->get_real_monrace();
    if (monrace.special_flags.has(MonsterSpecialType::DIMINISH_MAX_DAMAGE)) {
        return this->describe_diminisher(dam, m_name);
    } else {
        return this->describe_normal(dam, m_name);
    }
}
