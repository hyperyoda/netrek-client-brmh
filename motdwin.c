/***  Pop-up motd window code.  [BDyess] 11/21/93  ***/

#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <sys/types.h>
#ifdef hpux
#include <time.h>
#else				/* hpux */
#include <sys/time.h>
#endif				/* hpux */
#ifndef SVR4
#include <strings.h>
#endif
#include <string.h>
#include "netrek.h"

extern int	MaxMotdLine;

void
motdWinEvent(key)
    int             key;
    /* handles keystrokes in the motd window */
{
   switch (key) {
   case 'f':			/* Scroll motd forward */
      motdw_line = motdw_line + LINESPERPAGE;
      if (motdw_line > MaxMotdLine) {
	 motdw_line = motdw_line - LINESPERPAGE;
	 break;
      }
      showMotd(motdWin, motdw_line);
      break;
   case 'b':			/* Scroll motd backward */
      if (motdw_line == 0)
	 break;
      motdw_line = motdw_line - LINESPERPAGE;
      if (motdw_line < 0)
	 motdw_line = 0;
      showMotd(motdWin, motdw_line);
      break;
   case 'j':
      if (motdw_line + LINESPERPAGE == MaxMotdLine)
	 break;
      motdw_line++;
      showMotd(motdWin, motdw_line);
      break;
   case 'k':
      if (!motdw_line)
	 break;
      motdw_line --;
      showMotd(motdWin, motdw_line);
      break;

   case ' ':			/* unmap window */
      showMotdWin();
      break;

   case 'r':			/* refresh */
      showMotd(motdWin, motdw_line);
      break;
   }
}

void
showMotdWin()
    /* handles map/unmap requests */
{
   if (!motdWin) {
      motdWin = W_MakeWindow(
			       "Motd" ,0, 0, WINSIDE,
			       (LINESPERPAGE + LINESSTART) * W_Textheight,
			       NULL, BORDER, foreColor);
      W_MapWindow(motdWin);
      motdw_line = 0;
      /*
      showMotd(motdWin, motdw_line);
      */
   } else if (W_IsMapped(motdWin)) {
      W_UnmapWindow(motdWin);
   } else {
      W_MapWindow(motdWin);
      motdw_line = 0;
      /*
      showMotd(motdWin, motdw_line);
      */
   }
}
