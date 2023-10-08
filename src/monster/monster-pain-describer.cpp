#include "monster/monster-pain-describer.h"
#include "monster-race/monster-race.h"
#include "monster/monster-describer.h"
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

static const std::vector<pain_message_type> pain_messages{
    { d_char_is_any_of(",ejmvwQ"),
        {
            { 95, _("はほとんど気にとめていない。", " barely notices.") },
            { 75, _("はしり込みした。", " flinches.") },
            { 50, _("は縮こまった。", " squelches.") },
            { 35, _("は痛みに震えた。", " quivers in pain.") },
            { 20, _("は身もだえした。", " writhes about.") },
            { 10, _("は苦痛で身もだえした。", " writhes in agony.") },
            { 0, _("はぐにゃぐにゃと痙攣した。", " jerks limply.") },
        } },
    { d_char_is_any_of("l"),
        {
            { 95, _("はほとんど気にとめていない。", " barely notices.") },
            { 75, _("はしり込みした。", " flinches.") },
            { 50, _("は躊躇した。", " hesitates.") },
            { 35, _("は痛みに震えた。", " quivers in pain.") },
            { 20, _("は身もだえした。", " writhes about.") },
            { 10, _("は苦痛で身もだえした。", " writhes in agony.") },
            { 0, _("はぐにゃぐにゃと痙攣した。", " jerks limply.") },
        } },
    { d_char_is_any_of("g#+<>"),
        {
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
        return d_char_is_any_of("JMR")(monster) || !isalpha(monrace.d_char); },
        {
            { 95, _("はほとんど気にとめていない。", " barely notices.") },
            { 75, _("はシーッと鳴いた。", " hisses.") },
            { 50, _("は怒って頭を上げた。", " rears up in anger.") },
            { 35, _("は猛然と威嚇した。", " hisses furiously.") },
            { 20, _("は身もだえした。", " writhes about.") },
            { 10, _("は苦痛で身もだえした。", " writhes in agony.") },
            { 0, _("はぐにゃぐにゃと痙攣した。", " jerks limply.") },
        } },
    { d_char_is_any_of("f"),
        {
            { 95, _("は攻撃に肩をすくめた。", " shrugs off the attack.") },
            { 75, _("は吠えた。", " roars.") },
            { 50, _("は怒って吠えた。", " growls angrily.") },
            { 35, _("は痛みでシーッと鳴いた。", " hisses with pain.") },
            { 20, _("は痛みで弱々しく鳴いた。", " mewls in pain.") },
            { 10, _("は苦痛にうめいた。", " hisses in agony.") },
            { 0, _("は哀れな鳴き声を出した。", " mewls pitifully.") },
        } },
    { d_char_is_any_of("acFIKS"),
        {
            { 95, _("は攻撃を気にとめていない。", " ignores the attack.") },
            { 75, _("はキーキー鳴いた。", " chitters.") },
            { 50, _("はヨロヨロ逃げ回った。", " scuttles about.") },
            { 35, _("はうるさく鳴いた。", " twitters.") },
            { 20, _("は痛みに痙攣した。", " jerks in pain.") },
            { 10, _("は苦痛で痙攣した。", " jerks in agony.") },
            { 0, _("はピクピクひきつった。", " twitches.") },
        } },
    { d_char_is_any_of("B"),
        {
            { 95, _("はさえずった。", " chirps.") },
            { 75, _("はピーピー鳴いた。", " twitters.") },
            { 50, _("はギャーギャー鳴いた。", " squawks.") },
            { 35, _("はギャーギャー鳴きわめいた。", " chatters.") },
            { 20, _("は苦しんだ。", " jeers.") },
            { 10, _("はのたうち回った。", " flutters about.") },
            { 0, _("はキーキーと鳴き叫んだ。", " squeaks.") },
        } },
    { d_char_is_any_of("duDLUW"),
        {
            { 95, _("は攻撃を気にとめていない。", " ignores the attack.") },
            { 75, _("はしり込みした。", " flinches.") },
            { 50, _("は痛みでシーッと鳴いた。", " hisses in pain.") },
            { 35, _("は痛みでうなった。", " snarls with pain.") },
            { 20, _("は痛みに吠えた。", " roars with pain.") },
            { 10, _("は苦しげに叫んだ。", " gasps.") },
            { 0, _("は弱々しくうなった。", " snarls feebly.") },
        } },
    { d_char_is_any_of("s"),
        {
            { 95, _("は攻撃を気にとめていない。", " ignores the attack.") },
            { 75, _("は攻撃に肩をすくめた。", " shrugs off the attack.") },
            { 50, _("はカタカタと笑った。", " rattles.") },
            { 35, _("はよろめいた。", " stumbles.") },
            { 20, _("はカタカタ言った。", " rattles.") },
            { 10, _("はよろめいた。", " staggers.") },
            { 0, _("はガタガタ言った。", " clatters.") },
        } },
    { d_char_is_any_of("z"),
        {
            { 95, _("は攻撃を気にとめていない。", " ignores the attack.") },
            { 75, _("は攻撃に肩をすくめた。", " shrugs off the attack.") },
            { 50, _("はうめいた。", " groans.") },
            { 35, _("は苦しげにうめいた。", " moans.") },
            { 20, _("は躊躇した。", " hesitates.") },
            { 10, _("はうなった。", " grunts.") },
            { 0, _("はよろめいた。", " staggers.") },
        } },
    { d_char_is_any_of("G"),
        {
            { 95, _("は攻撃を気にとめていない。", " ignores the attack.") },
            { 75, _("は攻撃に肩をすくめた。", " shrugs off the attack.") },
            { 50, _("はうめいた。", " moans.") },
            { 35, _("は泣きわめいた。", " wails.") },
            { 20, _("は吠えた。", " howls.") },
            { 10, _("は弱々しくうめいた。", " moans softly.") },
            { 0, _("はかすかにうめいた。", " sighs.") },
        } },
    { d_char_is_any_of("CZ"),
        {
            { 95, _("は攻撃に肩をすくめた。", " shrugs off the attack.") },
            { 75, _("は痛みでうなった。", " snarls with pain.") },
            { 50, _("は痛みでキャンキャン吠えた。", " yelps in pain.") },
            { 35, _("は痛みで鳴きわめいた。", " howls in pain.") },
            { 20, _("は苦痛のあまり鳴きわめいた。", " howls in agony.") },
            { 10, _("は苦痛でもだえ苦しんだ。", " writhes in agony.") },
            { 0, _("は弱々しく吠えた。", " yelps feebly.") },
        } },
    { d_char_is_any_of("Xbilqrt"),
        {
            { 95, _("は攻撃を気にとめていない。", " ignores the attack.") },
            { 75, _("は痛みでうなった。", " grunts with pain.") },
            { 50, _("は痛みで叫んだ。", " squeals in pain.") },
            { 35, _("は痛みで絶叫した。", " shrieks in pain.") },
            { 20, _("は苦痛のあまり絶叫した。", " shrieks in agony.") },
            { 10, _("は苦痛でもだえ苦しんだ。", " writhes in agony.") },
            { 0, _("は弱々しく叫んだ。", " cries out feebly.") },
        } },
    { [](const MonsterEntity &) { return true; },
        {
            { 95, _("は攻撃に肩をすくめた。", " shrugs off the attack.") },
            { 75, _("は痛みでうなった。", " grunts with pain.") },
            { 50, _("は痛みで叫んだ。", " cries out in pain.") },
            { 35, _("は痛みで絶叫した。", " screams in pain.") },
            { 20, _("は苦痛のあまり絶叫した。", " screams in agony.") },
            { 10, _("は苦痛でもだえ苦しんだ。", " writhes in agony.") },
            { 0, _("は弱々しく叫んだ。", " cries out feebly.") },
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
std::string MonsterPainDescriber::describe(int dam)
{
    const auto m_name = monster_desc(player_ptr, this->m_ptr, 0);

    if (dam == 0) {
        if (this->m_ptr->ml) {
            return format(_("%s^はダメージを受けていない。", "%s^ is unharmed."), m_name.data());
        }
        return "";
    }

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
