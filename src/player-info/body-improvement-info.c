#include "player-info/body-improvement-info.h"
#include "player-info/self-info-util.h"

/* todo 並び順の都合で連番を付ける。まとめても良いならまとめてしまう予定 */
void set_body_improvement_info_1(player_type *creature_ptr, self_info_type *si_ptr)
{
    if (is_blessed(creature_ptr))
        si_ptr->info[si_ptr->line++] = _("あなたは高潔さを感じている。", "You feel rightous.");

    if (is_hero(creature_ptr))
        si_ptr->info[si_ptr->line++] = _("あなたはヒーロー気分だ。", "You feel heroic.");

    if (is_shero(creature_ptr))
        si_ptr->info[si_ptr->line++] = _("あなたは戦闘狂だ。", "You are in a battle rage.");

    if (creature_ptr->protevil)
        si_ptr->info[si_ptr->line++] = _("あなたは邪悪なる存在から守られている。", "You are protected from evil.");

    if (creature_ptr->shield)
        si_ptr->info[si_ptr->line++] = _("あなたは神秘のシールドで守られている。", "You are protected by a mystic shield.");

    if (is_invuln(creature_ptr))
        si_ptr->info[si_ptr->line++] = _("あなたは現在傷つかない。", "You are temporarily invulnerable.");

    if (creature_ptr->wraith_form)
        si_ptr->info[si_ptr->line++] = _("あなたは一時的に幽体化している。", "You are temporarily incorporeal.");
}

/* todo 並び順の都合で連番を付ける。まとめても良いならまとめてしまう予定 */
void set_body_improvement_info_2(player_type *creature_ptr, self_info_type *si_ptr)
{
    if (creature_ptr->new_spells)
        si_ptr->info[si_ptr->line++] = _("あなたは呪文や祈りを学ぶことができる。", "You can learn some spells/prayers.");

    if (creature_ptr->word_recall)
        si_ptr->info[si_ptr->line++] = _("あなたはすぐに帰還するだろう。", "You will soon be recalled.");

    if (creature_ptr->alter_reality)
        si_ptr->info[si_ptr->line++] = _("あなたはすぐにこの世界を離れるだろう。", "You will soon be altered.");

    if (creature_ptr->see_infra)
        si_ptr->info[si_ptr->line++] = _("あなたの瞳は赤外線に敏感である。", "Your eyes are sensitive to infrared light.");

    if (creature_ptr->see_inv)
        si_ptr->info[si_ptr->line++] = _("あなたは透明なモンスターを見ることができる。", "You can see invisible creatures.");

    if (creature_ptr->levitation)
        si_ptr->info[si_ptr->line++] = _("あなたは飛ぶことができる。", "You can fly.");

    if (creature_ptr->free_act)
        si_ptr->info[si_ptr->line++] = _("あなたは麻痺知らずの効果を持っている。", "You have free action.");

    if (creature_ptr->regenerate)
        si_ptr->info[si_ptr->line++] = _("あなたは素早く体力を回復する。", "You regenerate quickly.");

    if (creature_ptr->slow_digest)
        si_ptr->info[si_ptr->line++] = _("あなたは食欲が少ない。", "Your appetite is small.");
}

/* todo 並び順の都合で連番を付ける。まとめても良いならまとめてしまう予定 */
void set_body_improvement_info_3(player_type *creature_ptr, self_info_type *si_ptr)
{
    if (creature_ptr->hold_exp)
        si_ptr->info[si_ptr->line++] = _("あなたは自己の経験値をしっかりと維持する。", "You have a firm hold on your experience.");

    if (creature_ptr->reflect)
        si_ptr->info[si_ptr->line++] = _("あなたは矢の呪文を反射する。", "You reflect bolt spells.");

    if (creature_ptr->sh_fire)
        si_ptr->info[si_ptr->line++] = _("あなたは炎のオーラに包まれている。", "You are surrounded with a fiery aura.");

    if (creature_ptr->sh_elec)
        si_ptr->info[si_ptr->line++] = _("あなたは電気に包まれている。", "You are surrounded with electricity.");

    if (creature_ptr->sh_cold)
        si_ptr->info[si_ptr->line++] = _("あなたは冷気のオーラに包まれている。", "You are surrounded with an aura of coldness.");

    if (creature_ptr->tim_sh_holy)
        si_ptr->info[si_ptr->line++] = _("あなたは聖なるオーラに包まれている。", "You are surrounded with a holy aura.");

    if (creature_ptr->tim_sh_touki)
        si_ptr->info[si_ptr->line++] = _("あなたは闘気のオーラに包まれている。", "You are surrounded with an energy aura.");

    if (creature_ptr->anti_magic)
        si_ptr->info[si_ptr->line++] = _("あなたは反魔法シールドに包まれている。", "You are surrounded by an anti-magic shell.");

    if (creature_ptr->anti_tele)
        si_ptr->info[si_ptr->line++] = _("あなたはテレポートできない。", "You cannot teleport.");

    if (creature_ptr->lite)
        si_ptr->info[si_ptr->line++] = _("あなたの身体は光っている。", "You are carrying a permanent light.");

    if (creature_ptr->warning)
        si_ptr->info[si_ptr->line++] = _("あなたは行動の前に危険を察知することができる。", "You will be warned before dangerous actions.");

    if (creature_ptr->dec_mana)
        si_ptr->info[si_ptr->line++] = _("あなたは少ない消費魔力で魔法を唱えることができる。", "You can cast spells with fewer mana points.");

    if (creature_ptr->easy_spell)
        si_ptr->info[si_ptr->line++] = _("あなたは低い失敗率で魔法を唱えることができる。", "Fail rate of your magic is decreased.");

    if (creature_ptr->heavy_spell)
        si_ptr->info[si_ptr->line++] = _("あなたは高い失敗率で魔法を唱えなければいけない。", "Fail rate of your magic is increased.");

    if (creature_ptr->mighty_throw)
        si_ptr->info[si_ptr->line++] = _("あなたは強く物を投げる。", "You can throw objects powerfully.");
}

/* todo 並び順の都合で連番を付ける。まとめても良いならまとめてしまう予定 */
void set_body_improvement_info_4(player_type *creature_ptr, self_info_type *si_ptr)
{
    if (creature_ptr->resist_fear)
        si_ptr->info[si_ptr->line++] = _("あなたは全く恐怖を感じない。", "You are completely fearless.");
    
    if (creature_ptr->resist_blind)
        si_ptr->info[si_ptr->line++] = _("あなたの目は盲目への耐性を持っている。", "Your eyes are resistant to blindness.");
    
    if (creature_ptr->resist_time)
        si_ptr->info[si_ptr->line++] = _("あなたは時間逆転への耐性を持っている。", "You are resistant to time.");
}
