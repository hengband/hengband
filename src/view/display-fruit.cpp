#include "view/display-fruit.h"
#include "system/angband.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"

/*!
 * @brief カジノのスロットシンボルを表示する / display fruit for dice slots
 * @param row シンボルを表示する行の上端
 * @param col シンボルを表示する行の左端
 * @param fruit 表示するシンボルID
 */
void display_fruit(int row, int col, int fruit)
{
	switch (fruit)
	{
	case 0:
		c_put_str(TERM_YELLOW, "   ####.", row, col);
		c_put_str(TERM_YELLOW, "  #    #", row + 1, col);
		c_put_str(TERM_YELLOW, " #     #", row + 2, col);
		c_put_str(TERM_YELLOW, "#      #", row + 3, col);
		c_put_str(TERM_YELLOW, "#      #", row + 4, col);
		c_put_str(TERM_YELLOW, "#     # ", row + 5, col);
		c_put_str(TERM_YELLOW, "#    #  ", row + 6, col);
		c_put_str(TERM_YELLOW, ".####   ", row + 7, col);
		prt(_(" レモン ", " Lemon  "), row + 8, col);
		break;
	case 1:
		c_put_str(TERM_ORANGE, "   ##   ", row, col);
		c_put_str(TERM_ORANGE, "  #..#  ", row + 1, col);
		c_put_str(TERM_ORANGE, " #....# ", row + 2, col);
		c_put_str(TERM_ORANGE, "#......#", row + 3, col);
		c_put_str(TERM_ORANGE, "#......#", row + 4, col);
		c_put_str(TERM_ORANGE, " #....# ", row + 5, col);
		c_put_str(TERM_ORANGE, "  #..#  ", row + 6, col);
		c_put_str(TERM_ORANGE, "   ##   ", row + 7, col);
		prt(_("オレンジ", " Orange "), row + 8, col);
		break;
	case 2:
		c_put_str(TERM_SLATE, _("   Λ   ", "   /\\   "), row, col);
		c_put_str(TERM_SLATE, _("   ||   ", "   ##   "), row + 1, col);
		c_put_str(TERM_SLATE, _("   ||   ", "   ##   "), row + 2, col);
		c_put_str(TERM_SLATE, _("   ||   ", "   ##   "), row + 3, col);
		c_put_str(TERM_SLATE, _("   ||   ", "   ##   "), row + 4, col);
		c_put_str(TERM_SLATE, _("   ||   ", "   ##   "), row + 5, col);
		c_put_str(TERM_UMBER, _(" |=亜=| ", " ###### "), row + 6, col);
		c_put_str(TERM_UMBER, _("   目   ", "   ##   "), row + 7, col);
		prt(_("   剣   ", " Sword  "), row + 8, col);
		break;
	case 3:
		c_put_str(TERM_SLATE, " ###### ", row, col);
		c_put_str(TERM_SLATE, "#      #", row + 1, col);
		c_put_str(TERM_SLATE, "# ++++ #", row + 2, col);
		c_put_str(TERM_SLATE, "# +==+ #", row + 3, col);
		c_put_str(TERM_SLATE, "#  ++  #", row + 4, col);
		c_put_str(TERM_SLATE, " #    # ", row + 5, col);
		c_put_str(TERM_SLATE, "  #  #  ", row + 6, col);
		c_put_str(TERM_SLATE, "   ##   ", row + 7, col);
		prt(_("   盾   ", " Shield "), row + 8, col);
		break;
	case 4:
		c_put_str(TERM_VIOLET, "   ##   ", row, col);
		c_put_str(TERM_VIOLET, " ###### ", row + 1, col);
		c_put_str(TERM_VIOLET, "########", row + 2, col);
		c_put_str(TERM_VIOLET, "########", row + 3, col);
		c_put_str(TERM_VIOLET, "########", row + 4, col);
		c_put_str(TERM_VIOLET, " ###### ", row + 5, col);
		c_put_str(TERM_VIOLET, "  ####  ", row + 6, col);
		c_put_str(TERM_VIOLET, "   ##   ", row + 7, col);
		prt(_(" プラム ", "  Plum  "), row + 8, col);
		break;
	case 5:
		c_put_str(TERM_RED, "      ##", row, col);
		c_put_str(TERM_RED, "   ###  ", row + 1, col);
		c_put_str(TERM_RED, "  #..#  ", row + 2, col);
		c_put_str(TERM_RED, "  #..#  ", row + 3, col);
		c_put_str(TERM_RED, " ###### ", row + 4, col);
		c_put_str(TERM_RED, "#..##..#", row + 5, col);
		c_put_str(TERM_RED, "#..##..#", row + 6, col);
		c_put_str(TERM_RED, " ##  ## ", row + 7, col);
		prt(_("チェリー", " Cherry "), row + 8, col);
		break;
	}
}
