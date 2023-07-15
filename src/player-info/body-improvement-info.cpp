#include "player-info/body-improvement-info.h"
#include "player-info/self-info-util.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "system/player-type-definition.h"

/*!< @todo 並び順の都合で連番を付ける。まとめても良いならまとめてしまう予定 */
void set_body_improvement_info_1(PlayerType *player_ptr, self_info_type *self_ptr)
{
    if (is_blessed(player_ptr)) {
        self_ptr->info[self_ptr->line++] = _("あなたは高潔さを感じている。", "You feel rightous.");
    }

    if (is_hero(player_ptr)) {
        self_ptr->info[self_ptr->line++] = _("あなたはヒーロー気分だ。", "You feel heroic.");
    }

    if (is_shero(player_ptr)) {
        self_ptr->info[self_ptr->line++] = _("あなたは戦闘狂だ。", "You are in a battle rage.");
    }

    if (player_ptr->protevil) {
        self_ptr->info[self_ptr->line++] = _("あなたは邪悪なる存在から守られている。", "You are protected from evil.");
    }

    if (player_ptr->shield) {
        self_ptr->info[self_ptr->line++] = _("あなたは神秘のシールドで守られている。", "You are protected by a mystic shield.");
    }

    if (is_invuln(player_ptr)) {
        self_ptr->info[self_ptr->line++] = _("あなたは現在傷つかない。", "You are temporarily invulnerable.");
    }

    if (player_ptr->wraith_form) {
        self_ptr->info[self_ptr->line++] = _("あなたは一時的に幽体化している。", "You are temporarily incorporeal.");
    }
}

/*!< @todo 並び順の都合で連番を付ける。まとめても良いならまとめてしまう予定 */
void set_body_improvement_info_2(PlayerType *player_ptr, self_info_type *self_ptr)
{
    if (player_ptr->new_spells) {
        self_ptr->info[self_ptr->line++] = _("あなたは呪文や祈りを学ぶことができる。", "You can learn some spells/prayers.");
    }

    if (player_ptr->word_recall) {
        self_ptr->info[self_ptr->line++] = _("あなたはすぐに帰還するだろう。", "You will soon be recalled.");
    }

    if (player_ptr->alter_reality) {
        self_ptr->info[self_ptr->line++] = _("あなたはすぐにこの世界を離れるだろう。", "You will soon be altered.");
    }

    if (player_ptr->see_infra) {
        self_ptr->info[self_ptr->line++] = _("あなたの瞳は赤外線に敏感である。", "Your eyes are sensitive to infrared light.");
    }

    if (player_ptr->see_inv) {
        self_ptr->info[self_ptr->line++] = _("あなたは透明なモンスターを見ることができる。", "You can see invisible creatures.");
    }

    if (player_ptr->levitation) {
        self_ptr->info[self_ptr->line++] = _("あなたは飛ぶことができる。", "You can fly.");
    }

    if (player_ptr->free_act) {
        self_ptr->info[self_ptr->line++] = _("あなたは麻痺知らずの効果を持っている。", "You have free action.");
    }

    if (player_ptr->regenerate) {
        self_ptr->info[self_ptr->line++] = _("あなたは素早く体力を回復する。", "You regenerate quickly.");
    }

    if (player_ptr->slow_digest) {
        self_ptr->info[self_ptr->line++] = _("あなたは食欲が少ない。", "Your appetite is small.");
    }
}

/*!< @todo 並び順の都合で連番を付ける。まとめても良いならまとめてしまう予定 */
void set_body_improvement_info_3(PlayerType *player_ptr, self_info_type *self_ptr)
{
    if (player_ptr->hold_exp) {
        self_ptr->info[self_ptr->line++] = _("あなたは自己の経験値をしっかりと維持する。", "You have a firm hold on your experience.");
    }

    if (has_reflect(player_ptr)) {
        self_ptr->info[self_ptr->line++] = _("あなたは矢の呪文を反射する。", "You reflect bolt spells.");
    }

    if (has_resist_curse(player_ptr)) {
        self_ptr->info[self_ptr->line++] = _("あなたはより強く呪いに抵抗できる。", "You can resist curses powerfully.");
    }

    if (has_sh_fire(player_ptr)) {
        self_ptr->info[self_ptr->line++] = _("あなたは炎のオーラに包まれている。", "You are surrounded with a fiery aura.");
    }

    if (get_player_flags(player_ptr, TR_SELF_FIRE)) {
        self_ptr->info[self_ptr->line++] = _("あなたは身を焼く炎に包まれている。", "You are being damaged with fire.");
    }

    if (has_sh_elec(player_ptr)) {
        self_ptr->info[self_ptr->line++] = _("あなたは電気のオーラに包まれている。", "You are surrounded with an electricity aura.");
    }

    if (get_player_flags(player_ptr, TR_SELF_ELEC)) {
        self_ptr->info[self_ptr->line++] = _("あなたは身を焦がす電撃に包まれている。", "You are being damaged with electricity.");
    }

    if (has_sh_cold(player_ptr)) {
        self_ptr->info[self_ptr->line++] = _("あなたは冷気のオーラに包まれている。", "You are surrounded with an aura of coldness.");
    }

    if (get_player_flags(player_ptr, TR_SELF_COLD)) {
        self_ptr->info[self_ptr->line++] = _("あなたは身も凍る冷気に包まれている。", "You are being damaged with coldness.");
    }

    if (player_ptr->tim_sh_holy) {
        self_ptr->info[self_ptr->line++] = _("あなたは聖なるオーラに包まれている。", "You are surrounded with a holy aura.");
    }

    if (player_ptr->tim_sh_touki) {
        self_ptr->info[self_ptr->line++] = _("あなたは闘気のオーラに包まれている。", "You are surrounded with an energy aura.");
    }

    if (player_ptr->anti_magic) {
        self_ptr->info[self_ptr->line++] = _("あなたは反魔法シールドに包まれている。", "You are surrounded by an anti-magic shell.");
    }

    if (player_ptr->anti_tele) {
        self_ptr->info[self_ptr->line++] = _("あなたはテレポートできない。", "You cannot teleport.");
    }

    if (player_ptr->lite) {
        self_ptr->info[self_ptr->line++] = _("あなたの身体は光っている。", "You are carrying a permanent light.");
    }

    if (player_ptr->warning) {
        self_ptr->info[self_ptr->line++] = _("あなたは行動の前に危険を察知することができる。", "You will be warned before dangerous actions.");
    }

    if (player_ptr->dec_mana) {
        self_ptr->info[self_ptr->line++] = _("あなたは少ない消費魔力で魔法を唱えることができる。", "You can cast spells with fewer mana points.");
    }

    if (player_ptr->easy_spell) {
        self_ptr->info[self_ptr->line++] = _("あなたは低い失敗率で魔法を唱えることができる。", "Your magic fails less often.");
    }

    if (player_ptr->heavy_spell) {
        self_ptr->info[self_ptr->line++] = _("あなたは高い失敗率で魔法を唱えなければいけない。", "Your magic fails more often.");
    }

    if (player_ptr->mighty_throw) {
        self_ptr->info[self_ptr->line++] = _("あなたは強く物を投げる。", "You can throw objects powerfully.");
    }
}

/*!< @todo 並び順の都合で連番を付ける。まとめても良いならまとめてしまう予定 */
void set_body_improvement_info_4(PlayerType *player_ptr, self_info_type *self_ptr)
{
    if (has_resist_fear(player_ptr)) {
        self_ptr->info[self_ptr->line++] = _("あなたは全く恐怖を感じない。", "You are completely fearless.");
    }

    if (has_resist_blind(player_ptr)) {
        self_ptr->info[self_ptr->line++] = _("あなたの目は盲目への耐性を持っている。", "Your eyes are resistant to blindness.");
    }

    if (has_resist_time(player_ptr)) {
        self_ptr->info[self_ptr->line++] = _("あなたは時間逆転への耐性を持っている。", "You are resistant to time.");
    }
}
