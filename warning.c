/*
 * warning.c
 */
#include "copyright.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include "netrek.h"

/*
 * * The warning in text will be printed in the warning window. * The message
 * will last WARNTIME/10 seconds unless another message * comes through and
 * overwrites it.
 */

void
warning(text)
    char           *text;
{
   warntimer = udcounter + WARNTIME;	/* set the line to be cleared */

   if (warncount > 0) {
      /* XFIX */
      W_ClearArea(warnw, 5, 5, W_Textwidth * warncount, W_Textheight);
   }
   warncount = strlen(text);
   if(!recv_short && strncmp(text, "Phaser burst", 12)==0){
      if(phaserWindow){
	 W_WriteText(phaserwin, 0, 0, textColor, text, strlen(text), W_MesgFont);
	 W_FlushScrollingWindow(phaserwin);
	 return;
      }
      if (phas_msgi){
	 W_WriteText(messwi, 0, 0, textColor, text, strlen(text), W_MesgFont);
	 return;
      }
      if(reportPhaserInReview){
         W_WriteText(reviewWin, 0, 0, textColor, text, strlen(text), W_MesgFont);
	 W_FlushScrollingWindow(reviewWin);
      }
   }
   W_WriteText(warnw, 5, 5, textColor, text, warncount, W_RegularFont);
}
