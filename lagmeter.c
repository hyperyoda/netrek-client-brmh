/*
 * */
#include "copyright.h"

#include <stdio.h>
#include <ctype.h>
#include "netrek.h"

#define L_VTEXT			3	/* width of vertical text */
#define L_NB			3	/* num bars */
#define L_LENGTHTEXT		2	/* length of bar text at bottom */
#define L_VTSP			0	/* vertical spacing */
#define L_TSP			4	/* horizontal spacing */
#define L_NMARKS		11	/* number of vertical marks */
#define L_IBORDER		2	/* internal border */

#define L_WIDTH			((W_Textwidth*L_NB + L_TSP + L_NB*L_LENGTHTEXT \
					*W_Textwidth + 3*L_TSP)+2*L_IBORDER)
#define L_HEIGHT		((((W_Textheight+L_VTSP) * L_NMARKS)+\
					W_Textheight+L_VTSP)+2*L_IBORDER)
#define L_BWIDTH		(L_LENGTHTEXT * W_Textwidth)
#define L_BHEIGHT		(L_HEIGHT-(W_Textheight+L_VTSP)-2*L_IBORDER)

#ifdef NETSTAT

int
lMeterHeight()
{
   return L_HEIGHT;
}

int
lMeterWidth()
{
   return L_WIDTH;
}

void
redrawLMeter()
{
   register        i;
   char            buf[8];

   W_ClearWindow(lMeter);

   /* vertical number marks */
   for (i = 0; i < L_NMARKS; i++) {
      sprintf(buf, "%3d", (L_NMARKS - (i + 1)) * 10);
      W_WriteText(lMeter, L_IBORDER, i + (i * (W_Textheight + L_VTSP)) + L_VTSP + 2 /* X */ ,
		  textColor, buf, strlen(buf), W_RegularFont);
   }

   /* horizontal text */
   W_WriteText(lMeter, L_IBORDER + L_VTEXT * W_Textwidth + L_TSP,
	       L_HEIGHT - L_IBORDER - W_Textheight / 2 - 3, textColor, "TO", 2, W_RegularFont);

   W_WriteText(lMeter, L_IBORDER + L_VTEXT * W_Textwidth + L_BWIDTH + L_TSP + L_TSP,
	       L_HEIGHT - L_IBORDER - W_Textheight / 2 - 3, textColor, "TS", 2, W_RegularFont);

   W_WriteText(lMeter, L_IBORDER + L_VTEXT * W_Textwidth + 2*(L_BWIDTH + L_TSP) + L_TSP,
	       L_HEIGHT - L_IBORDER - W_Textheight / 2 - 3, textColor, "NF", 2, W_RegularFont);

   /* bars */
   lMeterBox(0, L_VTEXT * W_Textwidth + L_IBORDER + L_TSP,
	     L_IBORDER, L_BWIDTH, L_BHEIGHT, borderColor);
   lMeterBox(0, L_VTEXT * W_Textwidth + L_IBORDER + L_BWIDTH + L_TSP + L_TSP,
	     L_IBORDER, L_BWIDTH, L_BHEIGHT, borderColor);
   lMeterBox(0, L_VTEXT * W_Textwidth + L_IBORDER + 2 * (L_BWIDTH + L_TSP) + L_TSP,
	     L_IBORDER, L_BWIDTH, L_BHEIGHT, borderColor);

   updateLMeter();
}

void
updateLMeter()
{
   double          sd, sdl, ns_get_tstat(), ns_get_lstat();
   int             nf, h;
   W_Color         color;

   sd = ns_get_tstat();
   sdl = ns_get_lstat();
   nf = ns_get_nfailures();
   /* filled */

   lMeterBox(1, L_VTEXT * W_Textwidth + L_IBORDER + L_TSP + 1,
	     L_IBORDER + 1, L_BWIDTH - 2, L_BHEIGHT - 2, backColor);
   if (sd > 0.) {
      if (sd > 99.)
	 sd = 99.;
      color = gColor;
      if (sd > 25.)
	 color = yColor;
      if (sd > 45.)
	 color = rColor;

      h = (int) (sd * (double) L_BHEIGHT / 100.);
      lMeterBox(1, L_VTEXT * W_Textwidth + L_IBORDER + L_TSP + 1,
		L_IBORDER + L_BHEIGHT - h, L_BWIDTH - 2, h - 1, color);
   }
   lMeterBox(1, L_VTEXT * W_Textwidth + L_IBORDER + L_BWIDTH + L_TSP + L_TSP + 1,
	     L_IBORDER + 1, L_BWIDTH - 2, L_BHEIGHT - 2, backColor);
   if (sdl > 0.) {
      if (sdl > 99.)
	 sdl = 99.;
      color = gColor;
      if (sdl > 25.)
	 color = yColor;
      if (sdl > 45.)
	 color = rColor;
      h = (int) (sdl * (double) L_BHEIGHT / 100.);
      lMeterBox(1, L_VTEXT * W_Textwidth + L_IBORDER + L_BWIDTH + L_TSP + L_TSP + 1,
		L_IBORDER + L_BHEIGHT - h, L_BWIDTH - 2, h - 1, color);
   }
   lMeterBox(1, L_VTEXT * W_Textwidth + L_IBORDER + 2 * (L_BWIDTH + L_TSP) + L_TSP + 1,
	     L_IBORDER + 1, L_BWIDTH - 2, L_BHEIGHT - 2, backColor);
   if (nf > 0.) {
      if (nf > 99.)
	 nf = 99.;
      h = (int) (nf * (double) L_BHEIGHT / 100.);
      lMeterBox(1, L_VTEXT * W_Textwidth + L_IBORDER + 2 * (L_BWIDTH + L_TSP) + L_TSP + 1,
		L_IBORDER + L_BHEIGHT - h, L_BWIDTH - 1, h - 2, rColor);
   }
}

void
lMeterBox(filled, x, y, w, h, color)
    int             filled, x, y, w, h;
    W_Color         color;
{
   if (filled) {
      W_FillArea(lMeter, x, y, w + 1, h + 1, color);
      return;
   }
   W_MakeLine(lMeter, x, y, x + w, y, color);
   W_MakeLine(lMeter, x + w, y, x + w, y + h, color);
   W_MakeLine(lMeter, x + w, y + h, x, y + h, color);
   W_MakeLine(lMeter, x, y + h, x, y, color);
}

#endif
