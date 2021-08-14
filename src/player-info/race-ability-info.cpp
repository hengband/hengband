#include "player-info/race-ability-info.h"
#include "player-info/self-info-util.h"
#include "system/player-type-definition.h"

/*!
 * @brief レイシャルパワーの説明文を表示する
 * @param creature_ptr プレイヤー情報へのポインタ
 * @param self_ptr 自己分析情報へのポインタ
 * @details
 *  使用可能レベル以上を条件とする。
 */
void set_race_ability_info(player_type *creature_ptr, self_info_type *self_ptr)
{
    switch (creature_ptr->prace) {
    case player_race_type::DWARF:
        if (creature_ptr->lev >= 5)
            self_ptr->info[self_ptr->line++] = _("あなたは罠とドアと階段を感知できる。(5 MP)", "You can find traps, doors and stairs (cost 5).");

        break;
    case player_race_type::NIBELUNG:
        if (creature_ptr->lev >= 10)
            self_ptr->info[self_ptr->line++] = _("あなたは罠とドアと階段を感知できる。(5 MP)", "You can find traps, doors and stairs (cost 5).");

        break;
    case player_race_type::HOBBIT:
        if (creature_ptr->lev >= 15)
            self_ptr->info[self_ptr->line++] = _("あなたは食料を生成できる。(10 MP)", "You can produce food (cost 10).");

        break;
    case player_race_type::GNOME:
        if (creature_ptr->lev >= 5)
            self_ptr->info[self_ptr->line++] = _("あなたは範囲 10 以内にテレポートできる。(5 MP)", "You can teleport, range 10 (cost 5).");

        break;
    case player_race_type::HALF_ORC:
        if (creature_ptr->lev >= 3)
            self_ptr->info[self_ptr->line++] = _("あなたは恐怖を除去できる。(5 MP)", "You can remove fear (cost 5).");

        break;
    case player_race_type::HALF_TROLL:
        if (creature_ptr->lev >= 10)
            self_ptr->info[self_ptr->line++] = _("あなたは狂暴化することができる。(12 MP) ", "You can enter a berserk fury (cost 12).");

        break;
    case player_race_type::AMBERITE:
        if (creature_ptr->lev >= 30)
            self_ptr->info[self_ptr->line++] = _("あなたはシャドウシフトすることができる。(50 MP)", "You can Shift Shadows (cost 50).");

        if (creature_ptr->lev >= 40)
            self_ptr->info[self_ptr->line++] = _("あなたは「パターン」を心に描いて歩くことができる。(75 MP)", "You can mentally Walk the Pattern (cost 75).");

        break;
    case player_race_type::BARBARIAN:
        if (creature_ptr->lev >= 8)
            self_ptr->info[self_ptr->line++] = _("あなたは狂暴化することができる。(10 MP) ", "You can enter a berserk fury (cost 10).");

        break;
    case player_race_type::HALF_OGRE:
        if (creature_ptr->lev >= 25)
            self_ptr->info[self_ptr->line++] = _("あなたは爆発のルーンを仕掛けることができる。(35 MP)", "You can set an Explosive Rune (cost 35).");

        break;
    case player_race_type::HALF_GIANT:
        if (creature_ptr->lev >= 20)
            self_ptr->info[self_ptr->line++] = _("あなたは石の壁を壊すことができる。(10 MP)", "You can break stone walls (cost 10).");

        break;
    case player_race_type::HALF_TITAN:
        if (creature_ptr->lev >= 15)
            self_ptr->info[self_ptr->line++] = _("あなたはモンスターをスキャンすることができる。(10 MP)", "You can probe monsters (cost 10).");

        break;
    case player_race_type::CYCLOPS:
        if (creature_ptr->lev >= 20) {
            sprintf(self_ptr->plev_buf, _("あなたは %d ダメージの岩石を投げることができる。(15 MP)", "You can throw a boulder, dam. %d (cost 15)."),
                (3 * creature_ptr->lev) / 2);
            self_ptr->info[self_ptr->line++] = self_ptr->plev_buf;
        }

        break;
    case player_race_type::YEEK:
        if (creature_ptr->lev >= 15)
            self_ptr->info[self_ptr->line++] = _("あなたは恐怖を呼び起こす叫び声を発することができる。(15 MP)", "You can make a terrifying scream (cost 15).");

        break;
    case player_race_type::KLACKON:
        if (creature_ptr->lev >= 9) {
            sprintf(
                self_ptr->plev_buf, _("あなたは %d ダメージの酸を吹きかけることができる。(9 MP)", "You can spit acid, dam. %d (cost 9)."), creature_ptr->lev);
            self_ptr->info[self_ptr->line++] = self_ptr->plev_buf;
        }

        break;
    case player_race_type::KOBOLD:
        if (creature_ptr->lev >= 12) {
            sprintf(self_ptr->plev_buf, _("あなたは %d ダメージの毒矢を投げることができる。(8 MP)", "You can throw a dart of poison, dam. %d (cost 8)."),
                creature_ptr->lev);
            self_ptr->info[self_ptr->line++] = self_ptr->plev_buf;
        }

        break;
    case player_race_type::DARK_ELF:
        if (creature_ptr->lev >= 2) {
            sprintf(self_ptr->plev_buf, _("あなたは %d ダメージのマジック・ミサイルの呪文を使える。(2 MP)", "You can cast a Magic Missile, dam %d (cost 2)."),
                (3 + ((creature_ptr->lev - 1) / 5)));
            self_ptr->info[self_ptr->line++] = self_ptr->plev_buf;
        }

        break;
    case player_race_type::DRACONIAN:
        sprintf(self_ptr->plev_buf, _("あなたは %d ダメージのブレスを吐くことができる。(%d MP)", "You can breathe, dam. %d (cost %d)."), 2 * creature_ptr->lev,
            creature_ptr->lev);
        self_ptr->info[self_ptr->line++] = self_ptr->plev_buf;
        break;
    case player_race_type::MIND_FLAYER:
        if (creature_ptr->lev >= 15)
            sprintf(self_ptr->plev_buf, _("あなたは %d ダメージの精神攻撃をすることができる。(12 MP)", "You can mind blast your enemies, dam %d (cost 12)."),
                creature_ptr->lev);

        self_ptr->info[self_ptr->line++] = self_ptr->plev_buf;
        break;
    case player_race_type::IMP:
        if (creature_ptr->lev >= 30) {
            sprintf(self_ptr->plev_buf, _("あなたは %d ダメージのファイア・ボールの呪文を使える。(15 MP)", "You can cast a Fire Ball, dam. %d (cost 15)."),
                creature_ptr->lev);
            self_ptr->info[self_ptr->line++] = self_ptr->plev_buf;
            break;
        }

        if (creature_ptr->lev >= 9) {
            sprintf(self_ptr->plev_buf, _("あなたは %d ダメージのファイア・ボルトの呪文を使える。(15 MP)", "You can cast a Fire Bolt, dam. %d (cost 15)."),
                creature_ptr->lev);
            self_ptr->info[self_ptr->line++] = self_ptr->plev_buf;
        }

        break;
    case player_race_type::GOLEM:
        if (creature_ptr->lev >= 20)
            self_ptr->info[self_ptr->line++]
                = _("あなたは d20+30 ターンの間肌を石に変化させられる。(15 MP)", "You can turn your skin to stone, dur d20+30 (cost 15).");

        break;
    case player_race_type::ZOMBIE:
    case player_race_type::SKELETON:
        if (creature_ptr->lev >= 30)
            self_ptr->info[self_ptr->line++] = _("あなたは失った経験値を回復することができる。(30 MP)", "You can restore lost experience (cost 30).");

        break;
    case player_race_type::VAMPIRE:
        if (creature_ptr->lev < 2)
            break;

        sprintf(self_ptr->plev_buf, _("あなたは敵から %d HP の生命力を吸収できる。(%d MP)", "You can steal life from a foe, dam. %d (cost %d)."),
            creature_ptr->lev * 2, 1 + (creature_ptr->lev / 3));
        self_ptr->info[self_ptr->line++] = self_ptr->plev_buf;
        break;
    case player_race_type::SPECTRE:
        if (creature_ptr->lev >= 4)
            self_ptr->info[self_ptr->line++] = _("あなたは泣き叫んで敵を恐怖させることができる。(6 MP)", "You can wail to terrify your enemies (cost 6).");

        break;
    case player_race_type::SPRITE:
        if (creature_ptr->lev >= 12)
            self_ptr->info[self_ptr->line++]
                = _("あなたは敵を眠らせる魔法の粉を投げることができる。(12 MP)", "You can throw magical dust which induces sleep (cost 12).");

        break;
    case player_race_type::BALROG:
        sprintf(self_ptr->plev_buf, _("あなたは %d ダメージの地獄か火炎のブレスを吐くことができる。(%d MP)", "You can breathe nether, dam. %d (cost %d)."),
            3 * creature_ptr->lev, 10 + creature_ptr->lev / 3);
        self_ptr->info[self_ptr->line++] = self_ptr->plev_buf;
        break;
    case player_race_type::KUTAR:
        if (creature_ptr->lev >= 20)
            self_ptr->info[self_ptr->line++]
                = _("あなたは d20+30 ターンの間横に伸びることができる。(15 MP)", "You can expand horizontally, dur d20+30 (cost 15).");

        break;
    case player_race_type::ANDROID:
        if (creature_ptr->lev < 10)
            sprintf(self_ptr->plev_buf, _("あなたは %d ダメージのレイガンを撃つことができる。(7 MP)", "You can fire a ray gun with damage %d (cost 7)."),
                (creature_ptr->lev + 1) / 2);
        else if (creature_ptr->lev < 25)
            sprintf(self_ptr->plev_buf, _("あなたは %d ダメージのブラスターを撃つことができる。(13 MP)", "You can fire a blaster with damage %d (cost 13)."),
                creature_ptr->lev);
        else if (creature_ptr->lev < 35)
            sprintf(self_ptr->plev_buf, _("あなたは %d ダメージのバズーカを撃つことができる。(26 MP)", "You can fire a bazooka with damage %d (cost 26)."),
                creature_ptr->lev * 2);
        else if (creature_ptr->lev < 45)
            sprintf(self_ptr->plev_buf,
                _("あなたは %d ダメージのビームキャノンを撃つことができる。(40 MP)", "You can fire a beam cannon with damage %d (cost 40)."),
                creature_ptr->lev * 2);
        else
            sprintf(self_ptr->plev_buf, _("あなたは %d ダメージのロケットを撃つことができる。(60 MP)", "You can fire a rocket with damage %d (cost 60)."),
                creature_ptr->lev * 5);

        self_ptr->info[self_ptr->line++] = self_ptr->plev_buf;
        break;
    default:
        break;
    }
}
