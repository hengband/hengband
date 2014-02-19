#include "angband.h"

cptr monster_name(int m_idx)
{
    static char            m_name[80];
    monster_type    *m_ptr = &m_list[m_idx];
    monster_desc(m_name, m_ptr, 0x00);
    return m_name;
}

/* 2 monster each is near by player, return true */
bool monster_near_player(int m_idx, int t_idx)
{
    monster_type    *m_ptr = &m_list[m_idx];
    monster_type    *t_ptr = &m_list[t_idx];
    return (m_ptr->cdis <= MAX_SIGHT) || (t_ptr->cdis <= MAX_SIGHT);
}

/* player can see monster, return true */
bool see_monster(int m_idx)
{
    monster_type    *m_ptr = &m_list[m_idx];
    return is_seen(m_ptr);
}

bool spell_learnable(int m_idx)
{
    monster_type    *m_ptr = &m_list[m_idx];
    /* Extract the "see-able-ness" */
    bool seen = (!p_ptr->blind && m_ptr->ml);

    bool maneable = player_has_los_bold(m_ptr->fy, m_ptr->fx);
    return (seen && maneable && !world_monster);
}

int monster_level_idx(int m_idx)
{
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);
    return rlev;
}

int spell_core(int SPELL_NUM, int hp, int y, int x, int m_idx, int SPELL_TYPE)
{
    int dam;

    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    bool learnable = spell_learnable(m_idx);

    switch (SPELL_NUM)
    {
    case RF4_ROCKET:
        dam = (hp / 4) > 800 ? 800 : (hp / 4);
        breath(y, x, m_idx, GF_ROCKET, dam, 2, FALSE, MS_ROCKET, SPELL_TYPE);
        if (SPELL_TYPE == SPELL_MON_TO_PLAYER)
        {
            update_smart_learn(m_idx, DRS_SHARD);
        }
        break;
    }
    return dam;
}

void MP_spell_RF4_SHRIEK(int m_idx)
{
    cptr m_name = monster_name(m_idx);
    disturb(1, 1);
    msg_format(_("%^sがかん高い金切り声をあげた。", "%^s makes a high pitched shriek."), m_name);
    aggravate_monsters(m_idx);
}

void MM_spell_RF4_SHRIEK(int m_idx, int t_idx)
{
    cptr m_name = monster_name(m_idx);
    cptr t_name = monster_name(t_idx);
    bool known = monster_near_player(m_idx, t_idx);
    bool see_m = see_monster(m_idx);
    if (known)
    {
        if (see_m)
        {
            msg_format(_("%^sが%sに向かって叫んだ。", "%^s shrieks at %s."), m_name, t_name);
        }
        else
        {
            mon_fight = TRUE;
        }
    }
    (void)set_monster_csleep(t_idx, 0);
}

void MP_spell_RF4_DISPEL(int m_idx)
{
    cptr m_name = monster_name(m_idx);
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."), m_name);
    else
        msg_format(_("%^sが魔力消去の呪文を念じた。", "%^s invokes a dispel magic."), m_name);

    dispel_player();
    if (p_ptr->riding) dispel_monster_status(p_ptr->riding);

    if ((p_ptr->pseikaku == SEIKAKU_COMBAT) || (inventory[INVEN_BOW].name1 == ART_CRIMSON))
        msg_print(_("やりやがったな！", ""));

    learn_spell(MS_DISPEL);
}

void MM_spell_RF4_DISPEL(int m_idx, int t_idx)
{
    cptr m_name = monster_name(m_idx);
    cptr t_name = monster_name(t_idx);
    bool known = monster_near_player(m_idx, t_idx);
    bool see_m = see_monster(m_idx);
    if (known)
    {
        if (see_m)
        {
            msg_format(_("%^sが%sに対して魔力消去の呪文を念じた。",
                         "%^s invokes a dispel magic at %s."), m_name, t_name);
        }
        else
        {
            mon_fight = TRUE;
        }
    }

    if (t_idx == p_ptr->riding) dispel_player();
    dispel_monster_status(t_idx);
}


int MP_spell_RF4_ROCKET(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    disturb(1, 1);
    if (p_ptr->blind)
        msg_format(_("%^sが何かを射った。", "%^s shoots something."), m_name);
    else
        msg_format(_("%^sがロケットを発射した。", "%^s fires a rocket."), m_name);

    return spell_core(RF4_ROCKET, m_ptr->hp, y, x, m_idx, SPELL_MON_TO_PLAYER);
}

int MM_spell_RF4_ROCKET(int y, int x, int m_idx, int t_idx)
{
    cptr m_name = monster_name(m_idx);
    cptr t_name = monster_name(t_idx);
    monster_type    *m_ptr = &m_list[m_idx];

    bool known = monster_near_player(m_idx, t_idx);
    bool see_either = see_monster(m_idx) || see_monster(t_idx);
    if (known)
    {
        if (see_either)
        {
            disturb(1, 1);

            if (p_ptr->blind)
            {
                msg_format(_("%^sが何かを射った。", "%^s shoots something."), m_name);
            }
            else
            {
                msg_format(_("%^sが%sにロケットを発射した。", "%^s fires a rocket at %s."), m_name, t_name);
            }
        }
        else
        {
            mon_fight = TRUE;
        }
    }
    return spell_core(RF4_ROCKET, m_ptr->hp, y, x, m_idx, SPELL_MON_TO_MON);
}

int spell_RF4_SHOOT(int m_idx)
{
    bool learnable = spell_learnable(m_idx);
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];

    int dam;
    disturb(1, 1);
    if (p_ptr->blind)
        msg_format(_("%^sが奇妙な音を発した。", "%^s makes a strange noise."), m_name);
    else
        msg_format(_("%^sが矢を放った。", "%^s fires an arrow."), m_name);

    dam = damroll(r_ptr->blow[0].d_dice, r_ptr->blow[0].d_side);
    bolt(m_idx, GF_ARROW, dam, MS_SHOOT);
    update_smart_learn(m_idx, DRS_REFLECT);
    return dam;
}

int spell_RF4_BREATH(int GF_TYPE, int y, int x, int m_idx)
{
    bool learnable = spell_learnable(m_idx);
    int dam, ms_type, drs_type;
    cptr type_s;
    bool smart_learn = TRUE;
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];

    switch (GF_TYPE)
    {
        case GF_ACID:
            dam = ((m_ptr->hp / 3) > 1600 ? 1600 : (m_ptr->hp / 3));
            type_s = _("酸", "acid");
            ms_type = MS_BR_ACID;
            drs_type = DRS_ACID;
            break;
        case GF_ELEC:
            dam = ((m_ptr->hp / 3) > 1600 ? 1600 : (m_ptr->hp / 3));
            type_s = _("稲妻", "lightning");
            ms_type = MS_BR_ELEC;
            drs_type = DRS_ELEC;
            break;
        case GF_FIRE:
            dam = ((m_ptr->hp / 3) > 1600 ? 1600 : (m_ptr->hp / 3));
            type_s = _("火炎", "fire");
            ms_type = MS_BR_FIRE;
            drs_type = DRS_FIRE;
            break;
        case GF_COLD:
            dam = ((m_ptr->hp / 3) > 1600 ? 1600 : (m_ptr->hp / 3));
            type_s = _("冷気", "frost");
            ms_type = MS_BR_COLD;
            drs_type = DRS_COLD;
            break;
        case GF_POIS:
            dam = ((m_ptr->hp / 3) > 800 ? 800 : (m_ptr->hp / 3));
            type_s = _("ガス", "gas");
            ms_type = MS_BR_POIS;
            drs_type = DRS_POIS;
            break;
        case GF_NETHER:
            dam = ((m_ptr->hp / 6) > 550 ? 550 : (m_ptr->hp / 6));
            type_s = _("地獄", "nether");
            ms_type = MS_BR_NETHER;
            drs_type = DRS_NETH;
            break;
        case GF_LITE:
            dam = ((m_ptr->hp / 6) > 400 ? 400 : (m_ptr->hp / 6));
            type_s = _("閃光", "light");
            ms_type = MS_BR_LITE;
            drs_type = DRS_LITE;
            break;
        case GF_DARK:
            dam = ((m_ptr->hp / 6) > 400 ? 400 : (m_ptr->hp / 6));
            type_s = _("暗黒", "darkness");
            ms_type = MS_BR_DARK;
            drs_type = DRS_DARK;
            break;
        case GF_CONFUSION:
            dam = ((m_ptr->hp / 6) > 450 ? 450 : (m_ptr->hp / 6));
            type_s = _("混乱", "confusion");
            ms_type = MS_BR_CONF;
            drs_type = DRS_CONF;
            break;
        case GF_SOUND:
            dam = ((m_ptr->hp / 6) > 450 ? 450 : (m_ptr->hp / 6));
            type_s = _("轟音", "sound");
            ms_type = MS_BR_SOUND;
            drs_type = DRS_SOUND;
            break;
        case GF_CHAOS:
            dam = ((m_ptr->hp / 6) > 600 ? 600 : (m_ptr->hp / 6));
            type_s = _("カオス", "chaos");
            ms_type = MS_BR_CHAOS;
            drs_type = DRS_CHAOS;
            break;
        case GF_DISENCHANT:
            dam = ((m_ptr->hp / 6) > 500 ? 500 : (m_ptr->hp / 6));
            type_s = _("劣化", "disenchantment");
            ms_type = MS_BR_DISEN;
            drs_type = DRS_DISEN;
            break;
        case GF_NEXUS:
            dam = ((m_ptr->hp / 3) > 250 ? 250 : (m_ptr->hp / 3));
            type_s = _("因果混乱", "nexus");
            ms_type = MS_BR_NEXUS;
            drs_type = DRS_NEXUS;
            break;
        case GF_TIME:
            dam = ((m_ptr->hp / 3) > 150 ? 150 : (m_ptr->hp / 3));
            type_s = _("時間逆転", "time");
            ms_type = MS_BR_TIME;
            smart_learn = FALSE;
            break;
        case GF_INERTIA:
            dam = ((m_ptr->hp / 6) > 200 ? 200 : (m_ptr->hp / 6));
            type_s = _("遅鈍", "inertia");
            ms_type = MS_BR_INERTIA;
            smart_learn = FALSE;
            break;
        case GF_GRAVITY:
            dam = ((m_ptr->hp / 3) > 200 ? 200 : (m_ptr->hp / 3));
            type_s = _("重力", "gravity");
            ms_type = MS_BR_GRAVITY;
            smart_learn = FALSE;
            break;
        case GF_SHARDS:
            dam = ((m_ptr->hp / 6) > 500 ? 500 : (m_ptr->hp / 6));
            type_s = _("破片", "shards");
            ms_type = MS_BR_SHARDS;
            drs_type = DRS_SHARD;
            break;
        case GF_PLASMA:
            dam = ((m_ptr->hp / 6) > 150 ? 150 : (m_ptr->hp / 6));
            type_s = _("プラズマ", "plasma");
            ms_type = MS_BR_PLASMA;
            smart_learn = FALSE;
            break;
        case GF_FORCE:
            dam = ((m_ptr->hp / 6) > 200 ? 200 : (m_ptr->hp / 6));
            type_s = _("フォース", "force");
            ms_type = MS_BR_FORCE;
            smart_learn = FALSE;
            break;
        case GF_MANA:
            dam = ((m_ptr->hp / 3) > 250 ? 250 : (m_ptr->hp / 3));
            type_s = _("魔力", "mana");
            ms_type = MS_BR_MANA;
            smart_learn = FALSE;
            break;
        case GF_NUKE:
            dam = ((m_ptr->hp / 3) > 800 ? 800 : (m_ptr->hp / 3));
            type_s = _("放射性廃棄物", "toxic waste");
            ms_type = MS_BR_NUKE;
            drs_type = DRS_POIS;
            break;
        case GF_DISINTEGRATE:
            dam = ((m_ptr->hp / 6) > 150 ? 150 : (m_ptr->hp / 6));
            type_s = _("分解", "disintegration");
            ms_type = MS_BR_DISI;
            smart_learn = FALSE;
            break;
        default:
            break;
    }

    disturb(1, 1);
    if (m_ptr->r_idx == MON_JAIAN && GF_TYPE == GF_SOUND)
    {
        msg_format(_("「ボォエ〜〜〜〜〜〜」", "'Booooeeeeee'"));
    }
    else if (m_ptr->r_idx == MON_BOTEI && GF_TYPE == GF_SHARDS)
    {
        msg_format(_("「ボ帝ビルカッター！！！」", "'Boty-Build cutter!!!'"));
    }
    else if (p_ptr->blind)
    {
        msg_format(_("%^sが何かのブレスを吐いた。", "%^s breathes."), m_name);
    }
    else
    {
        msg_format(_("%^sが%^sのブレスを吐いた。", "%^s breathes %^s."), m_name, type_s);
    }

    breath(y, x, m_idx, GF_TYPE, dam, 0, TRUE, ms_type, SPELL_MON_TO_PLAYER);
    if (smart_learn) update_smart_learn(m_idx, drs_type);
    return dam;
}

int spell_RF4_BA_CHAO(int y, int x, int m_idx)
{
    bool learnable = spell_learnable(m_idx);
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = monster_level_idx(m_idx);

    int dam;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが恐ろしげにつぶやいた。", "%^s mumbles frighteningly."), m_name);
    else
        msg_format(_("%^sが純ログルスを放った。", "%^s invokes a raw Logrus."), m_name);
    
    dam = ((r_ptr->flags2 & RF2_POWERFUL) ? (rlev * 3) : (rlev * 2)) + damroll(10, 10);

    breath(y, x, m_idx, GF_CHAOS, dam, 4, FALSE, MS_BALL_CHAOS, SPELL_MON_TO_PLAYER);
    update_smart_learn(m_idx, DRS_CHAOS);
    return dam;
}

int spell_RF4_BA_NUKE(int y, int x, int m_idx)
{
    bool learnable = spell_learnable(m_idx);
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sが放射能球を放った。", "%^s casts a ball of radiation."), m_name);
    
    dam = (rlev + damroll(10, 6)) * ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 1);

    breath(y, x, m_idx, GF_NUKE, dam, 2, FALSE, MS_BALL_NUKE, SPELL_MON_TO_PLAYER);
    update_smart_learn(m_idx, DRS_POIS);
    return dam;
}

int spell_RF5_BA_ACID(int y, int x, int m_idx)
{
    bool learnable = spell_learnable(m_idx);
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = monster_level_idx(m_idx);
    int dam, rad;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sがアシッド・ボールの呪文を唱えた。", "%^s casts an acid ball."), m_name);
    
    if (r_ptr->flags2 & RF2_POWERFUL)
    {
        rad = 4;
        dam = (rlev * 4) + 50 + damroll(10, 10);
    }
    else
    {
        rad = 2;
        dam = (randint1(rlev * 3) + 15);
    }
    breath(y, x, m_idx, GF_ACID, dam, rad, FALSE, MS_BALL_ACID, SPELL_MON_TO_PLAYER);
    update_smart_learn(m_idx, DRS_ACID);
    return dam;
}

int spell_RF5_BA_ELEC(int y, int x, int m_idx)
{
    bool learnable = spell_learnable(m_idx);
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = monster_level_idx(m_idx);
    int dam, rad;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sがサンダー・・ボールの呪文を唱えた。", "%^s casts a lightning ball."), m_name);

    if (r_ptr->flags2 & RF2_POWERFUL)
    {
        rad = 4;
        dam = (rlev * 4) + 50 + damroll(10, 10);
    }
    else
    {
        rad = 2;
        dam = (randint1(rlev * 3 / 2) + 8);
    }
    breath(y, x, m_idx, GF_ELEC, dam, rad, FALSE, MS_BALL_ELEC, SPELL_MON_TO_PLAYER);
    update_smart_learn(m_idx, DRS_ELEC);
    return dam;
}

int spell_RF5_BA_FIRE(int y, int x, int m_idx)
{
    bool learnable = spell_learnable(m_idx);
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = monster_level_idx(m_idx);
    int dam, rad;
    disturb(1, 1);

    if (m_ptr->r_idx == MON_ROLENTO)
    {
        if (p_ptr->blind)
            msg_format(_("%sが何かを投げた。", "%^s throws something."), m_name);
        else
            msg_format(_("%sは手榴弾を投げた。", "%^s throws a hand grenade."), m_name);
    }
    else
    {
        if (p_ptr->blind)
            msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
        else
            msg_format(_("%^sがファイア・ボールの呪文を唱えた。", "%^s casts a fire ball."), m_name);
    }

    if (r_ptr->flags2 & RF2_POWERFUL)
    {
        rad = 4;
        dam = (rlev * 4) + 50 + damroll(10, 10);
    }
    else
    {
        rad = 2;
        dam = (randint1(rlev * 7 / 2) + 10);
    }
    breath(y, x, m_idx, GF_FIRE, dam, rad, FALSE, MS_BALL_FIRE, SPELL_MON_TO_PLAYER);
    update_smart_learn(m_idx, DRS_FIRE);
    return dam;
}

int spell_RF5_BA_COLD(int y, int x, int m_idx)
{
    bool learnable = spell_learnable(m_idx);
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = monster_level_idx(m_idx);
    int dam, rad;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sがアイス・ボールの呪文を唱えた。", "%^s casts a frost ball."), m_name);

    if (r_ptr->flags2 & RF2_POWERFUL)
    {
        rad = 4;
        dam = (rlev * 4) + 50 + damroll(10, 10);
    }
    else
    {
        rad = 2;
        dam = (randint1(rlev * 3 / 2) + 10);
    }
    breath(y, x, m_idx, GF_COLD, dam, rad, FALSE, MS_BALL_COLD, SPELL_MON_TO_PLAYER);
    update_smart_learn(m_idx, DRS_COLD);
    return dam;
}

int spell_RF5_BA_POIS(int y, int x, int m_idx)
{
    bool learnable = spell_learnable(m_idx);
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sが悪臭雲の呪文を唱えた。", "%^s casts a stinking cloud."), m_name);

    dam = damroll(12, 2) * ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 1);
    breath(y, x, m_idx, GF_POIS, dam, 2, FALSE, MS_BALL_POIS, SPELL_MON_TO_PLAYER);
    update_smart_learn(m_idx, DRS_POIS);
    return dam;
}

int spell_RF5_BA_NETH(int y, int x, int m_idx)
{
    bool learnable = spell_learnable(m_idx);
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);
    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sが地獄球の呪文を唱えた。", "%^s casts a nether ball."), m_name);

    dam = 50 + damroll(10, 10) + (rlev * ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 1));
    breath(y, x, m_idx, GF_NETHER, dam, 2, FALSE, MS_BALL_NETHER, SPELL_MON_TO_PLAYER);
    update_smart_learn(m_idx, DRS_NETH);
    return dam;
}

int spell_RF5_BA_WATE(int y, int x, int m_idx)
{
    bool learnable = spell_learnable(m_idx);
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sが流れるような身振りをした。", "%^s gestures fluidly."), m_name);

    msg_print(_("あなたは渦巻きに飲み込まれた。", "You are engulfed in a whirlpool."));

    dam = ((r_ptr->flags2 & RF2_POWERFUL) ? randint1(rlev * 3) : randint1(rlev * 2)) + 50;
    breath(y, x, m_idx, GF_WATER, dam, 4, FALSE, MS_BALL_WATER, SPELL_MON_TO_PLAYER);
    return dam;
}

int spell_RF5_BA_MANA(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);
    if (p_ptr->blind)
        msg_format(_("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."), m_name);
    else
        msg_format(_("%^sが魔力の嵐の呪文を念じた。", "%^s invokes a mana storm."), m_name);

    dam = (rlev * 4) + 50 + damroll(10, 10);
    breath(y, x, m_idx, GF_MANA, dam, 4, FALSE, MS_BALL_MANA, SPELL_MON_TO_PLAYER);
    return dam;
}

int spell_RF5_BA_DARK(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."), m_name);
    else
        msg_format(_("%^sが暗黒の嵐の呪文を念じた。", "%^s invokes a darkness storm."), m_name);

    dam = (rlev * 4) + 50 + damroll(10, 10);
    breath(y, x, m_idx, GF_DARK, dam, 4, FALSE, MS_BALL_DARK, SPELL_MON_TO_PLAYER);
    update_smart_learn(m_idx, DRS_DARK);
    return dam;
}

int spell_RF5_DRAIN_MANA(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);

    dam = (randint1(rlev) / 2) + 1;
    breath(y, x, m_idx, GF_DRAIN_MANA, dam, 0, FALSE, MS_DRAIN_MANA, SPELL_MON_TO_PLAYER);
    update_smart_learn(m_idx, DRS_MANA);
    return dam;
}

int spell_RF5_MIND_BLAST(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    bool seen = (!p_ptr->blind && m_ptr->ml);
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);
    if (!seen)
        msg_print(_("何かがあなたの精神に念を放っているようだ。", "You feel something focusing on your mind."));
    else
        msg_format(_("%^sがあなたの瞳をじっとにらんでいる。", "%^s gazes deep into your eyes."), m_name);

    dam = damroll(7, 7);
    breath(y, x, m_idx, GF_MIND_BLAST, dam, 0, FALSE, MS_MIND_BLAST, SPELL_MON_TO_PLAYER);
    return dam;
}

int spell_RF5_BRAIN_SMASH(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    bool seen = (!p_ptr->blind && m_ptr->ml);
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);
    if (!seen)
        msg_print(_("何かがあなたの精神に念を放っているようだ。", "You feel something focusing on your mind."));
    else
        msg_format(_("%^sがあなたの瞳をじっと見ている。", "%^s looks deep into your eyes."), m_name);

    dam = damroll(12, 12);
    breath(y, x, m_idx, GF_BRAIN_SMASH, dam, 0, FALSE, MS_BRAIN_SMASH, SPELL_MON_TO_PLAYER);
    return dam;
}

int spell_RF5_CAUSE_1(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sがあなたを指さして呪った。", "%^s points at you and curses."), m_name);

    dam = damroll(3, 8);
    breath(y, x, m_idx, GF_CAUSE_1, dam, 0, FALSE, MS_CAUSE_1, SPELL_MON_TO_PLAYER);
    return dam;
}

int spell_RF5_CAUSE_2(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sがあなたを指さして恐ろしげに呪った。", "%^s points at you and curses horribly."), m_name);

    dam = damroll(8, 8);
    breath(y, x, m_idx, GF_CAUSE_2, dam, 0, FALSE, MS_CAUSE_2, SPELL_MON_TO_PLAYER);
    return dam;
}

int spell_RF5_CAUSE_3(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かを大声で叫んだ。", "%^s mumbles loudly."), m_name);
    else
        msg_format(_("%^sがあなたを指さして恐ろしげに呪文を唱えた！", "%^s points at you, incanting terribly!"), m_name);

    dam = damroll(10, 15);
    breath(y, x, m_idx, GF_CAUSE_3, dam, 0, FALSE, MS_CAUSE_3, SPELL_MON_TO_PLAYER);
    return dam;
}

int spell_RF5_CAUSE_4(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが「お前は既に死んでいる」と叫んだ。", "%^s screams the word 'DIE!'"), m_name);
    else
        msg_format(_("%^sがあなたの秘孔を突いて「お前は既に死んでいる」と叫んだ。",
        "%^s points at you, screaming the word DIE!"), m_name);

    dam = damroll(15, 15);
    breath(y, x, m_idx, GF_CAUSE_4, dam, 0, FALSE, MS_CAUSE_4, SPELL_MON_TO_PLAYER);
    return dam;
}

int spell_RF5_BO_ACID(int y, int x, int m_idx)
{
    bool learnable = spell_learnable(m_idx);
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sがアシッド・ボルトの呪文を唱えた。", "%^s casts a acid bolt."), m_name);

    dam = (damroll(7, 8) + (rlev / 3)) * ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 1);
    bolt(m_idx, GF_ACID, dam, MS_BOLT_ACID);
    update_smart_learn(m_idx, DRS_ACID);
    update_smart_learn(m_idx, DRS_REFLECT);
    return dam;
}

int spell_RF5_BO_ELEC(int y, int x, int m_idx)
{
    bool learnable = spell_learnable(m_idx);
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sがサンダー・ボルトの呪文を唱えた。", "%^s casts a lightning bolt."), m_name);

    dam = (damroll(4, 8) + (rlev / 3)) * ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 1);
    bolt(m_idx, GF_ELEC, dam, MS_BOLT_ELEC);
    update_smart_learn(m_idx, DRS_ELEC);
    update_smart_learn(m_idx, DRS_REFLECT);
    return dam;
}

int spell_RF5_BO_FIRE(int y, int x, int m_idx)
{
    bool learnable = spell_learnable(m_idx);
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sがファイア・ボルトの呪文を唱えた。", "%^s casts a fire bolt."), m_name);

    dam = (damroll(9, 8) + (rlev / 3)) * ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 1);
    bolt(m_idx, GF_FIRE, dam, MS_BOLT_FIRE);
    update_smart_learn(m_idx, DRS_FIRE);
    update_smart_learn(m_idx, DRS_REFLECT);
    return dam;
}

int spell_RF5_BO_COLD(int y, int x, int m_idx)
{
    bool learnable = spell_learnable(m_idx);
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sがアイス・ボルトの呪文を唱えた。", "%^s casts a frost bolt."), m_name);

    dam = (damroll(6, 8) + (rlev / 3)) * ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 1);
    bolt(m_idx, GF_COLD, dam, MS_BOLT_COLD);
    update_smart_learn(m_idx, DRS_COLD);
    update_smart_learn(m_idx, DRS_REFLECT);
    return dam;
}


int spell_RF5_BA_LITE(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."), m_name);
    else
        msg_format(_("%^sがスターバーストの呪文を念じた。", "%^s invokes a starburst."), m_name);

    dam = (rlev * 4) + 50 + damroll(10, 10);
    breath(y, x, m_idx, GF_LITE, dam, 4, FALSE, MS_STARBURST, SPELL_MON_TO_PLAYER);
    update_smart_learn(m_idx, DRS_LITE);
    return dam;
}


int spell_RF5_BO_NETH(int y, int x, int m_idx)
{
    bool learnable = spell_learnable(m_idx);
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sが地獄の矢の呪文を唱えた。", "%^s casts a nether bolt."), m_name);

    dam = 30 + damroll(5, 5) + (rlev * 4) / ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 3);
    bolt(m_idx, GF_NETHER, dam, MS_BOLT_NETHER);
    update_smart_learn(m_idx, DRS_NETH);
    update_smart_learn(m_idx, DRS_REFLECT);
    return dam;
}

int spell_RF5_BO_WATE(int y, int x, int m_idx)
{
    bool learnable = spell_learnable(m_idx);
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sがウォーター・ボルトの呪文を唱えた。", "%^s casts a water bolt."), m_name);

    dam = damroll(10, 10) + (rlev * 3 / ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 3));
    bolt(m_idx, GF_WATER, dam, MS_BOLT_WATER);
    update_smart_learn(m_idx, DRS_REFLECT);
    return dam;
}

int spell_RF5_BO_MANA(int y, int x, int m_idx)
{
    bool learnable = spell_learnable(m_idx);
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);
    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sが魔力の矢の呪文を唱えた。", "%^s casts a mana bolt."), m_name);

    dam = randint1(rlev * 7 / 2) + 50;
    bolt(m_idx, GF_MANA, dam, MS_BOLT_MANA);
    update_smart_learn(m_idx, DRS_REFLECT);
    return dam;
}

int spell_RF5_BO_PLAS(int y, int x, int m_idx)
{
    bool learnable = spell_learnable(m_idx);
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);
    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);

    else
        msg_format(_("%^sがプラズマ・ボルトの呪文を唱えた。", "%^s casts a plasma bolt."), m_name);

    dam = 10 + damroll(8, 7) + (rlev * 3 / ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 3));
    bolt(m_idx, GF_PLASMA, dam, MS_BOLT_PLASMA);
    update_smart_learn(m_idx, DRS_REFLECT);
    return dam;
}

int spell_RF5_BO_ICEE(int y, int x, int m_idx)
{
    bool learnable = spell_learnable(m_idx);
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);
    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sが極寒の矢の呪文を唱えた。", "%^s casts an ice bolt."), m_name);

    dam = damroll(6, 6) + (rlev * 3 / ((r_ptr->flags2 & RF2_POWERFUL) ? 2 : 3));
    bolt(m_idx, GF_ICE, dam, MS_BOLT_ICE);
    update_smart_learn(m_idx, DRS_COLD);
    update_smart_learn(m_idx, DRS_REFLECT);
    return dam;
}


int spell_RF5_MISSILE(int y, int x, int m_idx)
{
    bool learnable = spell_learnable(m_idx);
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);
    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sがマジック・ミサイルの呪文を唱えた。", "%^s casts a magic missile."), m_name);

    dam = damroll(2, 6) + (rlev / 3);
    bolt(m_idx, GF_MISSILE, dam, MS_MAGIC_MISSILE);
    update_smart_learn(m_idx, DRS_REFLECT);
    return dam;
}

void spell_RF5_SCARE(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやくと、恐ろしげな音が聞こえた。", "%^s mumbles, and you hear scary noises."), m_name);
    else
        msg_format(_("%^sが恐ろしげな幻覚を作り出した。", "%^s casts a fearful illusion."), m_name);

    if (p_ptr->resist_fear)
    {
        msg_print(_("しかし恐怖に侵されなかった。", "You refuse to be frightened."));
    }
    else if (randint0(100 + rlev / 2) < p_ptr->skill_sav)
    {
        msg_print(_("しかし恐怖に侵されなかった。", "You refuse to be frightened."));
    }
    else
    {
        (void)set_afraid(p_ptr->afraid + randint0(4) + 4);
    }
    learn_spell(MS_SCARE);
    update_smart_learn(m_idx, DRS_FEAR);
}

void spell_RF5_BLIND(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sが呪文を唱えてあなたの目をくらました！",
        "%^s casts a spell, burning your eyes!"), m_name);

    if (p_ptr->resist_blind)
    {
        msg_print(_("しかし効果がなかった！", "You are unaffected!"));
    }
    else if (randint0(100 + rlev / 2) < p_ptr->skill_sav)
    {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
    }
    else
    {
        (void)set_blind(12 + randint0(4));
    }
    learn_spell(MS_BLIND);
    update_smart_learn(m_idx, DRS_BLIND);
}

void spell_RF5_CONF(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやくと、頭を悩ます音がした。",
        "%^s mumbles, and you hear puzzling noises."), m_name);
    else
        msg_format(_("%^sが誘惑的な幻覚を作り出した。",
        "%^s creates a mesmerising illusion."), m_name);

    if (p_ptr->resist_conf)
    {
        msg_print(_("しかし幻覚にはだまされなかった。", "You disbelieve the feeble spell."));
    }
    else if (randint0(100 + rlev / 2) < p_ptr->skill_sav)
    {
        msg_print(_("しかし幻覚にはだまされなかった。", "You disbelieve the feeble spell."));
    }
    else
    {
        (void)set_confused(p_ptr->confused + randint0(4) + 4);
    }
    learn_spell(MS_CONF);
    update_smart_learn(m_idx, DRS_CONF);
}

void spell_RF5_SLOW(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    disturb(1, 1);

    msg_format(_("%^sがあなたの筋力を吸い取ろうとした！",
        "%^s drains power from your muscles!"), m_name);

    if (p_ptr->free_act)
    {
        msg_print(_("しかし効果がなかった！", "You are unaffected!"));
    }
    else if (randint0(100 + rlev / 2) < p_ptr->skill_sav)
    {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
    }
    else
    {
        (void)set_slow(p_ptr->slow + randint0(4) + 4, FALSE);
    }
    learn_spell(MS_SLOW);
    update_smart_learn(m_idx, DRS_FREE);
}

void spell_RF5_HOLD(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sがあなたの目をじっと見つめた！", "%^s stares deep into your eyes!"), m_name);

    if (p_ptr->free_act)
    {
        msg_print(_("しかし効果がなかった！", "You are unaffected!"));
    }
    else if (randint0(100 + rlev / 2) < p_ptr->skill_sav)
    {
        msg_format(_("しかし効力を跳ね返した！", "You resist the effects!"));
    }
    else
    {
        (void)set_paralyzed(p_ptr->paralyzed + randint0(4) + 4);
    }
    learn_spell(MS_SLEEP);
    update_smart_learn(m_idx, DRS_FREE);
}

void spell_RF6_HASTE(int m_idx)
{
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    disturb(1, 1);
    if (p_ptr->blind)
    {
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    }
    else
    {
        msg_format(_("%^sが自分の体に念を送った。", "%^s concentrates on %s body."), m_name);
    }

    /* Allow quick speed increases to base+10 */
    if (set_monster_fast(m_idx, MON_FAST(m_ptr) + 100))
    {
        msg_format(_("%^sの動きが速くなった。", "%^s starts moving faster."), m_name);
    }
}

int spell_RF6_HAND_DOOM(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    int dam;
    disturb(1, 1);
    msg_format(_("%^sが<破滅の手>を放った！", "%^s invokes the Hand of Doom!"), m_name);
    dam = (((s32b)((40 + randint1(20)) * (p_ptr->chp))) / 100);
    breath(y, x, m_idx, GF_HAND_DOOM, dam, 0, FALSE, MS_HAND_DOOM, SPELL_MON_TO_PLAYER);
    return dam;
}

void spell_RF6_HEAL(int m_idx)
{
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    int rlev = monster_level_idx(m_idx);
    bool seen = (!p_ptr->blind && m_ptr->ml);
    disturb(1, 1);

    /* Message */
    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sが自分の傷に集中した。", "%^s concentrates on %s wounds."), m_name);

    /* Heal some */
    m_ptr->hp += (rlev * 6);

    /* Fully healed */
    if (m_ptr->hp >= m_ptr->maxhp)
    {
        /* Fully healed */
        m_ptr->hp = m_ptr->maxhp;

        /* Message */
        if (seen)
            msg_format(_("%^sは完全に治った！", "%^s looks completely healed!"), m_name);
        else
            msg_format(_("%^sは完全に治ったようだ！", "%^s sounds completely healed!"), m_name);
    }

    /* Partially healed */
    else
    {
        /* Message */
        if (seen)
            msg_format(_("%^sは体力を回復したようだ。", "%^s looks healthier."), m_name);
        else
            msg_format(_("%^sは体力を回復したようだ。", "%^s sounds healthier."), m_name);
    }

    /* Redraw (later) if needed */
    if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
    if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);

    /* Cancel fear */
    if (MON_MONFEAR(m_ptr))
    {
        /* Cancel fear */
        (void)set_monster_monfear(m_idx, 0);

        /* Message */
        msg_format(_("%^sは勇気を取り戻した。", "%^s recovers %s courage."), m_name);
    }
}
void spell_RF6_INVULNER(int m_idx)
{
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    bool seen = (!p_ptr->blind && m_ptr->ml);
    disturb(1, 1);

    /* Message */
    if (!seen)
        msg_format(_("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."), m_name);
    else
        msg_format(_("%sは無傷の球の呪文を唱えた。", "%^s casts a Globe of Invulnerability."), m_name);

    if (!MON_INVULNER(m_ptr)) (void)set_monster_invulner(m_idx, randint1(4) + 4, FALSE);
}

void spell_RF6_BLINK(int m_idx)
{
    cptr m_name = monster_name(m_idx);
    disturb(1, 1);
    if (teleport_barrier(m_idx))
    {
        msg_format(_("魔法のバリアが%^sのテレポートを邪魔した。",
            "Magic barrier obstructs teleporting of %^s."), m_name);
    }
    else
    {
        msg_format(_("%^sが瞬時に消えた。", "%^s blinks away."), m_name);
        teleport_away(m_idx, 10, 0L);
        p_ptr->update |= (PU_MONSTERS);
    }
}

void spell_RF6_TPORT(int m_idx)
{
    cptr m_name = monster_name(m_idx);
    disturb(1, 1);
    if (teleport_barrier(m_idx))
    {
        msg_format(_("魔法のバリアが%^sのテレポートを邪魔した。",
            "Magic barrier obstructs teleporting of %^s."), m_name);
    }
    else
    {
        msg_format(_("%^sがテレポートした。", "%^s teleports away."), m_name);
        teleport_away_followable(m_idx);
    }
}

int spell_RF6_WORLD(int m_idx)
{
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    int who = 0;
    disturb(1, 1);
    if (m_ptr->r_idx == MON_DIO) who = 1;
    else if (m_ptr->r_idx == MON_WONG) who = 3;
    if (!process_the_world(randint1(2) + 2, who, TRUE)) return (FALSE);
    return who;
}

int spell_RF6_SPECIAL(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    u32b mode = 0L;
    bool direct = player_bold(y, x);
    int k, dam, count=0;

    disturb(1, 1);
    switch (m_ptr->r_idx)
    {
        case MON_OHMU:
            /* Moved to process_monster(), like multiplication */
            return -1;
        case MON_BANORLUPART:
        {
            int dummy_hp = (m_ptr->hp + 1) / 2;
            int dummy_maxhp = m_ptr->maxhp / 2;
            int dummy_y = m_ptr->fy;
            int dummy_x = m_ptr->fx;

            if (p_ptr->inside_arena || p_ptr->inside_battle || !summon_possible(m_ptr->fy, m_ptr->fx)) return -1;
            delete_monster_idx(cave[m_ptr->fy][m_ptr->fx].m_idx);
            summon_named_creature(0, dummy_y, dummy_x, MON_BANOR, mode);
            m_list[hack_m_idx_ii].hp = dummy_hp;
            m_list[hack_m_idx_ii].maxhp = dummy_maxhp;
            summon_named_creature(0, dummy_y, dummy_x, MON_LUPART, mode);
            m_list[hack_m_idx_ii].hp = dummy_hp;
            m_list[hack_m_idx_ii].maxhp = dummy_maxhp;

            msg_print(_("『バーノール・ルパート』が分裂した！","Banor=Rupart splits in two person!"));
            break;
        }

        case MON_BANOR:
        case MON_LUPART:
        {
            int dummy_hp = 0;
            int dummy_maxhp = 0;
            int dummy_y = m_ptr->fy;
            int dummy_x = m_ptr->fx;

            if (!r_info[MON_BANOR].cur_num || !r_info[MON_LUPART].cur_num) return (FALSE);
            for (k = 1; k < m_max; k++)
            {
                if (m_list[k].r_idx == MON_BANOR || m_list[k].r_idx == MON_LUPART)
                {
                    dummy_hp += m_list[k].hp;
                    dummy_maxhp += m_list[k].maxhp;
                    if (m_list[k].r_idx != m_ptr->r_idx)
                    {
                        dummy_y = m_list[k].fy;
                        dummy_x = m_list[k].fx;
                    }
                    delete_monster_idx(k);
                }
            }
            summon_named_creature(0, dummy_y, dummy_x, MON_BANORLUPART, mode);
            m_list[hack_m_idx_ii].hp = dummy_hp;
            m_list[hack_m_idx_ii].maxhp = dummy_maxhp;

            msg_print(_("『バーノール』と『ルパート』が合体した！", "Banor and Rupart combine into one!"));
            break;
        }

        case MON_ROLENTO:
            if (p_ptr->blind)
                msg_format(_("%^sが何か大量に投げた。", "%^s spreads something."), m_name);
            else
                msg_format(_("%^sは手榴弾をばらまいた。", "%^s throws some hand grenades."), m_name);

            {
                int num = 1 + randint1(3);

                for (k = 0; k < num; k++)
                {
                    count += summon_named_creature(m_idx, y, x, MON_SHURYUUDAN, mode);
                }
            }

            if (p_ptr->blind && count)
                msg_print(_("多くのものが間近にばらまかれる音がする。", "You hear many things are scattered nearby."));

            break;

        default:
        if (r_ptr->d_char == 'B')
        {
            disturb(1, 1);
            if (one_in_(3) || !direct)
            {
                msg_format(_("%^sは突然視界から消えた!", "%^s suddenly go out of your sight!"), m_name);
                teleport_away(m_idx, 10, TELEPORT_NONMAGICAL);
                p_ptr->update |= (PU_MONSTERS);
            }
            else
            {
                int get_damage = 0;
                bool fear; /* dummy */

                msg_format(_("%^sがあなたを掴んで空中から投げ落とした。",
                    "%^s holds you, and drops from the sky."), m_name);
                dam = damroll(4, 8);
                teleport_player_to(m_ptr->fy, m_ptr->fx, TELEPORT_NONMAGICAL | TELEPORT_PASSIVE);

                sound(SOUND_FALL);

                if (p_ptr->levitation)
                {
                    msg_print(_("あなたは静かに着地した。", "You float gently down to the ground."));
                }
                else
                {
                    msg_print(_("あなたは地面に叩きつけられた。", "You crashed into the ground."));
                    dam += damroll(6, 8);
                }

                /* Mega hack -- this special action deals damage to the player. Therefore the code of "eyeeye" is necessary.
                -- henkma
                */
                get_damage = take_hit(DAMAGE_NOESCAPE, dam, m_name, -1);
                if (p_ptr->tim_eyeeye && get_damage > 0 && !p_ptr->is_dead)
                {
#ifdef JP
                    msg_format("攻撃が%s自身を傷つけた！", m_name);
#else
                    char m_name_self[80];

                    /* hisself */
                    monster_desc(m_name_self, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE | MD_OBJECTIVE);

                    msg_format("The attack of %s has wounded %s!", m_name, m_name_self);
#endif
                    project(0, 0, m_ptr->fy, m_ptr->fx, get_damage, GF_MISSILE, PROJECT_KILL, -1);
                    set_tim_eyeeye(p_ptr->tim_eyeeye - 5, TRUE);
                }

                if (p_ptr->riding) mon_take_hit_mon(p_ptr->riding, dam, &fear, extract_note_dies(real_r_ptr(&m_list[p_ptr->riding])), m_idx);
            }
            break;
        }

        /* Something is wrong */
        else return -1;
    }
    return dam;
}


void spell_RF6_TELE_TO(int m_idx)
{
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    disturb(1, 1);
    msg_format(_("%^sがあなたを引き戻した。", "%^s commands you to return."), m_name);

    teleport_player_to(m_ptr->fy, m_ptr->fx, TELEPORT_PASSIVE);
    learn_spell(MS_TELE_TO);
}

void spell_RF6_TELE_AWAY(int m_idx)
{
    cptr m_name = monster_name(m_idx);
    disturb(1, 1);

    msg_format(_("%^sにテレポートさせられた。", "%^s teleports you away."), m_name);
    if ((p_ptr->pseikaku == SEIKAKU_COMBAT) || (inventory[INVEN_BOW].name1 == ART_CRIMSON))
        msg_print(_("くっそ〜", ""));

    learn_spell(MS_TELE_AWAY);
    teleport_player_away(m_idx, 100);
}

void spell_RF6_TELE_LEVEL(int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何か奇妙な言葉をつぶやいた。", "%^s mumbles strangely."), m_name);
    else
        msg_format(_("%^sがあなたの足を指さした。", "%^s gestures at your feet."), m_name);

    if (p_ptr->resist_nexus)
    {
        msg_print(_("しかし効果がなかった！", "You are unaffected!"));
    }
    else if (randint0(100 + rlev / 2) < p_ptr->skill_sav)
    {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
    }
    else
    {
        teleport_level(0);
    }
    learn_spell(MS_TELE_LEVEL);
    update_smart_learn(m_idx, DRS_NEXUS);
}

int spell_RF6_PSY_SPEAR(int m_idx)
{
    bool learnable = spell_learnable(m_idx);
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = monster_level_idx(m_idx);
    int dam;
    disturb(1, 1);
    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sが光の剣を放った。", "%^s throw a Psycho-Spear."), m_name);

    dam = (r_ptr->flags2 & RF2_POWERFUL) ? (randint1(rlev * 2) + 150) : (randint1(rlev * 3 / 2) + 100);
    beam(m_idx, GF_PSY_SPEAR, dam, MS_PSY_SPEAR);
    return dam;
}

void spell_RF6_DARKNESS(int m_idx)
{
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    bool can_use_lite_area = FALSE;

    if ((p_ptr->pclass == CLASS_NINJA) &&
        !(r_ptr->flags3 & (RF3_UNDEAD | RF3_HURT_LITE)) &&
        !(r_ptr->flags7 & RF7_DARK_MASK))
        can_use_lite_area = TRUE;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else if (can_use_lite_area)
        msg_format(_("%^sが辺りを明るく照らした。", "%^s cast a spell to light up."), m_name);
    else
        msg_format(_("%^sが暗闇の中で手を振った。", "%^s gestures in shadow."), m_name);

    if (can_use_lite_area)
    {
        (void)lite_area(0, 3);
    }
    else
    {
        learn_spell(MS_DARKNESS);
        (void)unlite_area(0, 3);
    }
}

void spell_RF6_TRAPS(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいて邪悪に微笑んだ。",
        "%^s mumbles, and then cackles evilly."), m_name);
    else
        msg_format(_("%^sが呪文を唱えて邪悪に微笑んだ。",
        "%^s casts a spell and cackles evilly."), m_name);

    learn_spell(MS_MAKE_TRAP);
    (void)trap_creation(y, x);
}

void spell_RF6_FORGET(int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    disturb(1, 1);

    msg_format(_("%^sがあなたの記憶を消去しようとしている。",
        "%^s tries to blank your mind."), m_name);

    if (randint0(100 + rlev / 2) < p_ptr->skill_sav)
    {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
    }
    else if (lose_all_info())
    {
        msg_print(_("記憶が薄れてしまった。", "Your memories fade away."));
    }
    learn_spell(MS_FORGET);
}

void spell_RF6_RAISE_DEAD(int m_idx)
{
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sが死者復活の呪文を唱えた。",
        "%^s casts a spell to revive corpses."), m_name);

    animate_dead(m_idx, m_ptr->fy, m_ptr->fx);
}

void spell_RF6_S_KIN(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = monster_level_idx(m_idx);
    int count = 0, k;
    u32b mode = 0L;
    disturb(1, 1);
    if (m_ptr->r_idx == MON_SERPENT || m_ptr->r_idx == MON_ZOMBI_SERPENT)
    {
        if (p_ptr->blind)
            msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
        else
            msg_format(_("%^sがダンジョンの主を召喚した。",
            "%^s magically summons guardians of dungeons."), m_name);
    }
    else
    {
        if (p_ptr->blind)
            msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
        else
#ifdef JP
            msg_format("%^sは魔法で%sを召喚した。",
            m_name,
            ((r_ptr->flags1) & RF1_UNIQUE ?
            "手下" : "仲間"));
#else
            msg_format("%^s magically summons %s %s.",
            m_name, m_poss,
            ((r_ptr->flags1) & RF1_UNIQUE ?
            "minions" : "kin"));
#endif
    }

    switch (m_ptr->r_idx)
    {
        case MON_MENELDOR:
        case MON_GWAIHIR:
        case MON_THORONDOR:
        {
            int num = 4 + randint1(3);
            for (k = 0; k < num; k++)
            {
                count += summon_specific(m_idx, y, x, rlev, SUMMON_EAGLES, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE));
            }
        }
        break;

        case MON_BULLGATES:
        {
            int num = 2 + randint1(3);
            for (k = 0; k < num; k++)
            {
                count += summon_named_creature(m_idx, y, x, MON_IE, mode);
            }
        }
        break;

        case MON_SERPENT:
        case MON_ZOMBI_SERPENT:
        {
            int num = 2 + randint1(3);

            if (r_info[MON_JORMUNGAND].cur_num < r_info[MON_JORMUNGAND].max_num && one_in_(6))
            {
                msg_print(_("地面から水が吹き出した！", "Water blew off from the ground!"));
                fire_ball_hide(GF_WATER_FLOW, 0, 3, 8);
            }

            for (k = 0; k < num; k++)
            {
                count += summon_specific(m_idx, y, x, rlev, SUMMON_GUARDIANS, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE));
            }
        }
        break;

        case MON_CALDARM:
        {
            int num = randint1(3);
            for (k = 0; k < num; k++)
            {
                count += summon_named_creature(m_idx, y, x, MON_LOCKE_CLONE, mode);
            }
        }
        break;

        case MON_LOUSY:
        {
            int num = 2 + randint1(3);
            for (k = 0; k < num; k++)
            {
                count += summon_specific(m_idx, y, x, rlev, SUMMON_LOUSE, PM_ALLOW_GROUP);
            }
        }
        break;

        default:
        summon_kin_type = r_ptr->d_char; /* Big hack */

        for (k = 0; k < 4; k++)
        {
            count += summon_specific(m_idx, y, x, rlev, SUMMON_KIN, PM_ALLOW_GROUP);
        }
        break;
    }
    
    if (p_ptr->blind && count)
        msg_print(_("多くのものが間近に現れた音がする。", "You hear many things appear nearby."));
}

void spell_RF6_S_CYBER(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int count = 0;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sがサイバーデーモンを召喚した！",
        "%^s magically summons Cyberdemons!"), m_name);

    count = summon_cyber(m_idx, y, x);

    if (p_ptr->blind && count)
        msg_print(_("重厚な足音が近くで聞こえる。", "You hear heavy steps nearby."));
}

void spell_RF6_S_MONSTER(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    int k, count = 0;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sが魔法で仲間を召喚した！", "%^s magically summons help!"), m_name);

    for (k = 0; k < 1; k++)
    {
        count += summon_specific(m_idx, y, x, rlev, 0, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE));
    }
    if (p_ptr->blind && count)
        msg_print(_("何かが間近に現れた音がする。", "You hear something appear nearby."));
}

void spell_RF6_S_MONSTERS(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    int k, count = 0;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sが魔法でモンスターを召喚した！", "%^s magically summons monsters!"), m_name);

    for (k = 0; k < S_NUM_6; k++)
    {
        count += summon_specific(m_idx, y, x, rlev, 0, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE));
    }

    if (p_ptr->blind && count)
        msg_print(_("多くのものが間近に現れた音がする。", "You hear many things appear nearby."));
}

void spell_RF6_S_ANT(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    int k, count = 0;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sが魔法でアリを召喚した。", "%^s magically summons ants."), m_name);

    for (k = 0; k < S_NUM_6; k++)
    {
        count += summon_specific(m_idx, y, x, rlev, SUMMON_ANT, PM_ALLOW_GROUP);
    }

    if (p_ptr->blind && count)
        msg_print(_("多くのものが間近に現れた音がする。", "You hear many things appear nearby."));
}

void spell_RF6_S_SPIDER(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    int k, count = 0;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sが魔法でクモを召喚した。", "%^s magically summons spiders."), m_name);

    for (k = 0; k < S_NUM_6; k++)
    {
        count += summon_specific(m_idx, y, x, rlev, SUMMON_SPIDER, PM_ALLOW_GROUP);
    }

    if (p_ptr->blind && count)
        msg_print(_("多くのものが間近に現れた音がする。", "You hear many things appear nearby."));
}

void spell_RF6_S_HOUND(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    int k, count = 0;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sが魔法でハウンドを召喚した。", "%^s magically summons hounds."), m_name);

    for (k = 0; k < S_NUM_4; k++)
    {
        count += summon_specific(m_idx, y, x, rlev, SUMMON_HOUND, PM_ALLOW_GROUP);
    }

    if (p_ptr->blind && count)
        msg_print(_("多くのものが間近に現れた音がする。", "You hear many things appear nearby."));
}

void spell_RF6_S_HYDRA(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    int k, count = 0;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sが魔法でヒドラを召喚した。", "%^s magically summons hydras."), m_name);

    for (k = 0; k < S_NUM_4; k++)
    {
        count += summon_specific(m_idx, y, x, rlev, SUMMON_HYDRA, PM_ALLOW_GROUP);
    }
    if (p_ptr->blind && count)
        msg_print(_("多くのものが間近に現れた音がする。", "You hear many things appear nearby."));
}

void spell_RF6_S_ANGEL(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = monster_level_idx(m_idx);
    int k, count = 0;
    int num = 1;

    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sが魔法で天使を召喚した！", "%^s magically summons an angel!"), m_name);

    if ((r_ptr->flags1 & RF1_UNIQUE) && !easy_band)
    {
        num += r_ptr->level / 40;
    }

    for (k = 0; k < num; k++)
    {
        count += summon_specific(m_idx, y, x, rlev, SUMMON_ANGEL, PM_ALLOW_GROUP);
    }

    if (count < 2)
    {
        if (p_ptr->blind && count)
            msg_print(_("何かが間近に現れた音がする。", "You hear something appear nearby."));
    }
    else
    {
        if (p_ptr->blind)
            msg_print(_("多くのものが間近に現れた音がする。", "You hear many things appear nearby."));
    }
}

void spell_RF6_S_DEMON(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    int k, count = 0;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sは魔法で混沌の宮廷から悪魔を召喚した！",
        "%^s magically summons a demon from the Courts of Chaos!"), m_name);

    for (k = 0; k < 1; k++)
    {
        count += summon_specific(m_idx, y, x, rlev, SUMMON_DEMON, PM_ALLOW_GROUP);
    }

    if (p_ptr->blind && count)
        msg_print(_("何かが間近に現れた音がする。", "You hear something appear nearby."));
}

void spell_RF6_S_UNDEAD(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    int k, count = 0;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sが魔法でアンデッドの強敵を召喚した！",
        "%^s magically summons an undead adversary!"), m_name);

    for (k = 0; k < 1; k++)
    {
        count += summon_specific(m_idx, y, x, rlev, SUMMON_UNDEAD, PM_ALLOW_GROUP);
    }

    if (p_ptr->blind && count)
        msg_print(_("何かが間近に現れた音がする。", "You hear something appear nearby."));
}

void spell_RF6_S_DRAGON(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    int k, count = 0;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sが魔法でドラゴンを召喚した！", "%^s magically summons a dragon!"), m_name);

    for (k = 0; k < 1; k++)
    {
        count += summon_specific(m_idx, y, x, rlev, SUMMON_DRAGON, PM_ALLOW_GROUP);
    }
    if (p_ptr->blind && count)
        msg_print(_("何かが間近に現れた音がする。", "You hear something appear nearby."));
}

void spell_RF6_S_HI_UNDEAD(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    monster_race    *r_ptr = &r_info[m_ptr->r_idx];
    int rlev = monster_level_idx(m_idx);
    u32b mode = 0L;
    int k, count = 0;
    disturb(1, 1);

    if (((m_ptr->r_idx == MON_MORGOTH) || (m_ptr->r_idx == MON_SAURON) || (m_ptr->r_idx == MON_ANGMAR)) && ((r_info[MON_NAZGUL].cur_num + 2) < r_info[MON_NAZGUL].max_num))
    {
        int cy = y;
        int cx = x;

        if (p_ptr->blind)
            msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
        else
            msg_format(_("%^sが魔法で幽鬼戦隊を召喚した！", "%^s magically summons rangers of Nazgul!"), m_name);

        msg_print(NULL);

        for (k = 0; k < 30; k++)
        {
            if (!summon_possible(cy, cx) || !cave_empty_bold(cy, cx))
            {
                int j;
                for (j = 100; j > 0; j--)
                {
                    scatter(&cy, &cx, y, x, 2, 0);
                    if (cave_empty_bold(cy, cx)) break;
                }
                if (!j) break;
            }
            if (!cave_empty_bold(cy, cx)) continue;

            if (summon_named_creature(m_idx, cy, cx, MON_NAZGUL, mode))
            {
                y = cy;
                x = cx;
                count++;
                if (count == 1)
                    msg_format(_("「幽鬼戦隊%d号、ナズグル・ブラック！」",
                    "A Nazgul says 'Nazgul-Rangers Number %d, Nazgul-Black!'"), count);
                else
                    msg_format(_("「同じく%d号、ナズグル・ブラック！」",
                    "Another one says 'Number %d, Nazgul-Black!'"), count);

                msg_print(NULL);
            }
        }
        msg_format(_("「%d人そろって、リングレンジャー！」",
            "They say 'The %d meets! We are the Ring-Ranger!'."), count);
        msg_print(NULL);
    }
    else
    {
        if (p_ptr->blind)
            msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
        else
            msg_format(_("%^sが魔法で強力なアンデッドを召喚した！",
            "%^s magically summons greater undead!"), m_name);

        for (k = 0; k < S_NUM_6; k++)
        {
            count += summon_specific(m_idx, y, x, rlev, SUMMON_HI_UNDEAD, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE));
        }
    }
    if (p_ptr->blind && count)
    {
        msg_print(_("間近で何か多くのものが這い回る音が聞こえる。",
            "You hear many creepy things appear nearby."));
    }
}


void spell_RF6_S_HI_DRAGON(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    int k, count = 0;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sが魔法で古代ドラゴンを召喚した！", "%^s magically summons ancient dragons!"), m_name);

    for (k = 0; k < S_NUM_4; k++)
    {
        count += summon_specific(m_idx, y, x, rlev, SUMMON_HI_DRAGON, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE));
    }
    if (p_ptr->blind && count)
    {
        msg_print(_("多くの力強いものが間近に現れた音が聞こえる。",
            "You hear many powerful things appear nearby."));
    }
}

void spell_RF6_S_AMBERITES(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    int rlev = monster_level_idx(m_idx);
    int k, count = 0;
    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sがアンバーの王族を召喚した！", "%^s magically summons Lords of Amber!"), m_name);

    for (k = 0; k < S_NUM_4; k++)
    {
        count += summon_specific(m_idx, y, x, rlev, SUMMON_AMBERITES, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE));
    }
    if (p_ptr->blind && count)
    {
        msg_print(_("不死の者が近くに現れるのが聞こえた。", "You hear immortal beings appear nearby."));
    }
}

void spell_RF6_S_UNIQUE(int y, int x, int m_idx)
{
    cptr m_name = monster_name(m_idx);
    monster_type    *m_ptr = &m_list[m_idx];
    int rlev = monster_level_idx(m_idx);
    int k, count = 0;
    bool uniques_are_summoned = FALSE;
    int non_unique_type = SUMMON_HI_UNDEAD;

    disturb(1, 1);

    if (p_ptr->blind)
        msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    else
        msg_format(_("%^sが魔法で特別な強敵を召喚した！", "%^s magically summons special opponents!"), m_name);

    for (k = 0; k < S_NUM_4; k++)
    {
        count += summon_specific(m_idx, y, x, rlev, SUMMON_UNIQUE, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE));
    }

    if (count) uniques_are_summoned = TRUE;

    if ((m_ptr->sub_align & (SUB_ALIGN_GOOD | SUB_ALIGN_EVIL)) == (SUB_ALIGN_GOOD | SUB_ALIGN_EVIL))
        non_unique_type = 0;
    else if (m_ptr->sub_align & SUB_ALIGN_GOOD)
        non_unique_type = SUMMON_ANGEL;

    for (k = count; k < S_NUM_4; k++)
    {
        count += summon_specific(m_idx, y, x, rlev, non_unique_type, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE));
    }

    if (p_ptr->blind && count)
    {
        msg_format(_("多くの%sが間近に現れた音が聞こえる。", "You hear many %s appear nearby."),
            uniques_are_summoned ? _("力強いもの", "powerful things") : _("もの", "things"));
    }
}

