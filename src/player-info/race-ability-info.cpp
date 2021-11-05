﻿#include "player-info/race-ability-info.h"
#include "player-info/self-info-util.h"
#include "system/player-type-definition.h"

/*!
 * @brief レイシャルパワーの説明文を表示する
 * @param player_ptr プレイヤー情報へのポインタ
 * @param self_ptr 自己分析情報へのポインタ
 * @details
 *  使用可能レベル以上を条件とする。
 */
void set_race_ability_info(PlayerType *player_ptr, self_info_type *self_ptr)
{
    switch (player_ptr->prace) {
    case PlayerRaceType::DWARF:
        if (player_ptr->lev >= 5)
            self_ptr->info[self_ptr->line++] = _("あなたは罠とドアと階段を感知できる。(5 MP)", "You can find traps, doors and stairs (cost 5).");

        break;
    case PlayerRaceType::NIBELUNG:
        if (player_ptr->lev >= 10)
            self_ptr->info[self_ptr->line++] = _("あなたは罠とドアと階段を感知できる。(5 MP)", "You can find traps, doors and stairs (cost 5).");

        break;
    case PlayerRaceType::HOBBIT:
        if (player_ptr->lev >= 15)
            self_ptr->info[self_ptr->line++] = _("あなたは食料を生成できる。(10 MP)", "You can produce food (cost 10).");

        break;
    case PlayerRaceType::GNOME:
        if (player_ptr->lev >= 5)
            self_ptr->info[self_ptr->line++] = _("あなたは範囲 10 以内にテレポートできる。(5 MP)", "You can teleport, range 10 (cost 5).");

        break;
    case PlayerRaceType::HALF_ORC:
        if (player_ptr->lev >= 3)
            self_ptr->info[self_ptr->line++] = _("あなたは恐怖を除去できる。(5 MP)", "You can remove fear (cost 5).");

        break;
    case PlayerRaceType::HALF_TROLL:
        if (player_ptr->lev >= 10)
            self_ptr->info[self_ptr->line++] = _("あなたは狂暴化することができる。(12 MP) ", "You can enter a berserk fury (cost 12).");

        break;
    case PlayerRaceType::AMBERITE:
        if (player_ptr->lev >= 30)
            self_ptr->info[self_ptr->line++] = _("あなたはシャドウシフトすることができる。(50 MP)", "You can Shift Shadows (cost 50).");

        if (player_ptr->lev >= 40)
            self_ptr->info[self_ptr->line++] = _("あなたは「パターン」を心に描いて歩くことができる。(75 MP)", "You can mentally Walk the Pattern (cost 75).");

        break;
    case PlayerRaceType::BARBARIAN:
        if (player_ptr->lev >= 8)
            self_ptr->info[self_ptr->line++] = _("あなたは狂暴化することができる。(10 MP) ", "You can enter a berserk fury (cost 10).");

        break;
    case PlayerRaceType::HALF_OGRE:
        if (player_ptr->lev >= 25)
            self_ptr->info[self_ptr->line++] = _("あなたは爆発のルーンを仕掛けることができる。(35 MP)", "You can set an Explosive Rune (cost 35).");

        break;
    case PlayerRaceType::HALF_GIANT:
        if (player_ptr->lev >= 20)
            self_ptr->info[self_ptr->line++] = _("あなたは石の壁を壊すことができる。(10 MP)", "You can break stone walls (cost 10).");

        break;
    case PlayerRaceType::HALF_TITAN:
        if (player_ptr->lev >= 15)
            self_ptr->info[self_ptr->line++] = _("あなたはモンスターをスキャンすることができる。(10 MP)", "You can probe monsters (cost 10).");

        break;
    case PlayerRaceType::CYCLOPS:
        if (player_ptr->lev >= 20) {
            sprintf(self_ptr->plev_buf, _("あなたは %d ダメージの岩石を投げることができる。(15 MP)", "You can throw a boulder, dam. %d (cost 15)."),
                (3 * player_ptr->lev) / 2);
            self_ptr->info[self_ptr->line++] = self_ptr->plev_buf;
        }

        break;
    case PlayerRaceType::YEEK:
        if (player_ptr->lev >= 15)
            self_ptr->info[self_ptr->line++] = _("あなたは恐怖を呼び起こす叫び声を発することができる。(15 MP)", "You can make a terrifying scream (cost 15).");

        break;
    case PlayerRaceType::KLACKON:
        if (player_ptr->lev >= 9) {
            sprintf(
                self_ptr->plev_buf, _("あなたは %d ダメージの酸を吹きかけることができる。(9 MP)", "You can spit acid, dam. %d (cost 9)."), player_ptr->lev);
            self_ptr->info[self_ptr->line++] = self_ptr->plev_buf;
        }

        break;
    case PlayerRaceType::KOBOLD:
        if (player_ptr->lev >= 12) {
            sprintf(self_ptr->plev_buf, _("あなたは %d ダメージの毒矢を投げることができる。(8 MP)", "You can throw a dart of poison, dam. %d (cost 8)."),
                player_ptr->lev);
            self_ptr->info[self_ptr->line++] = self_ptr->plev_buf;
        }

        break;
    case PlayerRaceType::DARK_ELF:
        if (player_ptr->lev >= 2) {
            sprintf(self_ptr->plev_buf, _("あなたは %d ダメージのマジック・ミサイルの呪文を使える。(2 MP)", "You can cast a Magic Missile, dam %d (cost 2)."),
                (3 + ((player_ptr->lev - 1) / 5)));
            self_ptr->info[self_ptr->line++] = self_ptr->plev_buf;
        }

        break;
    case PlayerRaceType::DRACONIAN:
        sprintf(self_ptr->plev_buf, _("あなたは %d ダメージのブレスを吐くことができる。(%d MP)", "You can breathe, dam. %d (cost %d)."), 2 * player_ptr->lev,
            player_ptr->lev);
        self_ptr->info[self_ptr->line++] = self_ptr->plev_buf;
        break;
    case PlayerRaceType::MIND_FLAYER:
        if (player_ptr->lev >= 15)
            sprintf(self_ptr->plev_buf, _("あなたは %d ダメージの精神攻撃をすることができる。(12 MP)", "You can mind blast your enemies, dam %d (cost 12)."),
                player_ptr->lev);

        self_ptr->info[self_ptr->line++] = self_ptr->plev_buf;
        break;
    case PlayerRaceType::IMP:
        if (player_ptr->lev >= 30) {
            sprintf(self_ptr->plev_buf, _("あなたは %d ダメージのファイア・ボールの呪文を使える。(15 MP)", "You can cast a Fire Ball, dam. %d (cost 15)."),
                player_ptr->lev);
            self_ptr->info[self_ptr->line++] = self_ptr->plev_buf;
            break;
        }

        if (player_ptr->lev >= 9) {
            sprintf(self_ptr->plev_buf, _("あなたは %d ダメージのファイア・ボルトの呪文を使える。(15 MP)", "You can cast a Fire Bolt, dam. %d (cost 15)."),
                player_ptr->lev);
            self_ptr->info[self_ptr->line++] = self_ptr->plev_buf;
        }

        break;
    case PlayerRaceType::GOLEM:
        if (player_ptr->lev >= 20)
            self_ptr->info[self_ptr->line++]
                = _("あなたは d20+30 ターンの間肌を石に変化させられる。(15 MP)", "You can turn your skin to stone, dur d20+30 (cost 15).");

        break;
    case PlayerRaceType::ZOMBIE:
    case PlayerRaceType::SKELETON:
        if (player_ptr->lev >= 30)
            self_ptr->info[self_ptr->line++] = _("あなたは失った経験値を回復することができる。(30 MP)", "You can restore lost experience (cost 30).");

        break;
    case PlayerRaceType::VAMPIRE:
        if (player_ptr->lev < 2)
            break;

        sprintf(self_ptr->plev_buf, _("あなたは敵から %d HP の生命力を吸収できる。(%d MP)", "You can steal life from a foe, dam. %d (cost %d)."),
            player_ptr->lev * 2, 1 + (player_ptr->lev / 3));
        self_ptr->info[self_ptr->line++] = self_ptr->plev_buf;
        break;
    case PlayerRaceType::SPECTRE:
        if (player_ptr->lev >= 4)
            self_ptr->info[self_ptr->line++] = _("あなたは泣き叫んで敵を恐怖させることができる。(6 MP)", "You can wail to terrify your enemies (cost 6).");

        break;
    case PlayerRaceType::SPRITE:
        if (player_ptr->lev >= 12)
            self_ptr->info[self_ptr->line++]
                = _("あなたは敵を眠らせる魔法の粉を投げることができる。(12 MP)", "You can throw magical dust which induces sleep (cost 12).");

        break;
    case PlayerRaceType::BALROG:
        sprintf(self_ptr->plev_buf, _("あなたは %d ダメージの地獄か火炎のブレスを吐くことができる。(%d MP)", "You can breathe nether, dam. %d (cost %d)."),
            3 * player_ptr->lev, 10 + player_ptr->lev / 3);
        self_ptr->info[self_ptr->line++] = self_ptr->plev_buf;
        break;
    case PlayerRaceType::KUTAR:
        if (player_ptr->lev >= 20)
            self_ptr->info[self_ptr->line++]
                = _("あなたは d20+30 ターンの間横に伸びることができる。(15 MP)", "You can expand horizontally, dur d20+30 (cost 15).");

        break;
    case PlayerRaceType::ANDROID:
        if (player_ptr->lev < 10)
            sprintf(self_ptr->plev_buf, _("あなたは %d ダメージのレイガンを撃つことができる。(7 MP)", "You can fire a ray gun with damage %d (cost 7)."),
                (player_ptr->lev + 1) / 2);
        else if (player_ptr->lev < 25)
            sprintf(self_ptr->plev_buf, _("あなたは %d ダメージのブラスターを撃つことができる。(13 MP)", "You can fire a blaster with damage %d (cost 13)."),
                player_ptr->lev);
        else if (player_ptr->lev < 35)
            sprintf(self_ptr->plev_buf, _("あなたは %d ダメージのバズーカを撃つことができる。(26 MP)", "You can fire a bazooka with damage %d (cost 26)."),
                player_ptr->lev * 2);
        else if (player_ptr->lev < 45)
            sprintf(self_ptr->plev_buf,
                _("あなたは %d ダメージのビームキャノンを撃つことができる。(40 MP)", "You can fire a beam cannon with damage %d (cost 40)."),
                player_ptr->lev * 2);
        else
            sprintf(self_ptr->plev_buf, _("あなたは %d ダメージのロケットを撃つことができる。(60 MP)", "You can fire a rocket with damage %d (cost 60)."),
                player_ptr->lev * 5);

        self_ptr->info[self_ptr->line++] = self_ptr->plev_buf;
        break;
    default:
        break;
    }
}
