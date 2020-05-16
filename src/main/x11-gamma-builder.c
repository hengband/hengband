/*!
 * @brief X11環境 (の中でもmaid-x11を必要とする特殊な環境)でガンマ値を調整する
 * @date 2020/05/16
 * @author Hourier
 */

#include "system/angband.h"
#include "main/x11-gamma-builder.h"

/* Table of gamma values */
byte gamma_table[256];

/* Table of ln(x/256) * 256 for x going from 0 -> 255 */
static s16b gamma_helper[256] =
{
0,-1420,-1242,-1138,-1065,-1007,-961,-921,-887,-857,-830,-806,-783,-762,-744,-726,
-710,-694,-679,-666,-652,-640,-628,-617,-606,-596,-586,-576,-567,-577,-549,-541,
-532,-525,-517,-509,-502,-495,-488,-482,-475,-469,-463,-457,-451,-455,-439,-434,
-429,-423,-418,-413,-408,-403,-398,-394,-389,-385,-380,-376,-371,-367,-363,-359,
-355,-351,-347,-343,-339,-336,-332,-328,-325,-321,-318,-314,-311,-308,-304,-301,
-298,-295,-291,-288,-285,-282,-279,-276,-273,-271,-268,-265,-262,-259,-257,-254,
-251,-248,-246,-243,-241,-238,-236,-233,-231,-228,-226,-223,-221,-219,-216,-214,
-212,-209,-207,-205,-203,-200,-198,-196,-194,-192,-190,-188,-186,-184,-182,-180,
-178,-176,-174,-172,-170,-168,-166,-164,-162,-160,-158,-156,-155,-153,-151,-149,
-147,-146,-144,-142,-140,-139,-137,-135,-134,-132,-130,-128,-127,-125,-124,-122,
-120,-119,-117,-116,-114,-112,-111,-109,-108,-106,-105,-103,-102,-100,-99,-97,
-96,-95,-93,-92,-90,-89,-87,-86,-85,-83,-82,-80,-79,-78,-76,-75,
-74,-72,-71,-70,-68,-67,-66,-65,-63,-62,-61,-59,-58,-57,-56,-54,
-53,-52,-51,-50,-48,-47,-46,-45,-44,-42,-41,-40,-39,-38,-37,-35,
-34,-33,-32,-31,-30,-29,-27,-26,-25,-24,-23,-22,-21,-20,-19,-18,
-17,-16,-14,-13,-12,-11,-10,-9,-8,-7,-6,-5,-4,-3,-2,-1
};

/*
 * Build the gamma table so that floating point isn't needed.
 *
 * Note gamma goes from 0->256.  The old value of 100 is now 128.
 */
void build_gamma_table(int gamma)
{
	gamma_table[0] = 0;
	gamma_table[255] = 255;
	for (int i = 1; i < 255; i++)
	{
		/*
		 * Initialise the Taylor series
		 *
		 * value and diff have been scaled by 256
		 */
		int n = 1;
		long value = 256 * 256;
		long diff = ((long)gamma_helper[i]) * (gamma - 256);

		while (diff)
		{
			value += diff;
			n++;


			/*
			 * Use the following identiy to calculate the gamma table.
			 * exp(x) = 1 + x + x^2/2 + x^3/(2*3) + x^4/(2*3*4) +...
			 *
			 * n is the current term number.
			 *
			 * The gamma_helper array contains a table of
			 * ln(x/256) * 256
			 * This is used because a^b = exp(b*ln(a))
			 *
			 * In this case:
			 * a is i / 256
			 * b is gamma.
			 *
			 * Note that everything is scaled by 256 for accuracy,
			 * plus another factor of 256 for the final result to
			 * be from 0-255.  Thus gamma_helper[] * gamma must be
			 * divided by 256*256 each itteration, to get back to
			 * the original power series.
			 */
			diff = (((diff / 256) * gamma_helper[i]) * (gamma - 256)) / (256 * n);
		}

		/*
		 * Store the value in the table so that the
		 * floating point pow function isn't needed .
		 */
		gamma_table[i] = ((long)(value / 256) * i) / 256;
	}
}
