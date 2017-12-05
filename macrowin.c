/*
 * macrowin.c from helpwin.c
 * copyright 1993 Nick Trown
 * copyright 1991 ERic mehlhaff
 * Free to use, hack, etc. Just keep these credits here.
 * Use of this code may be dangerous to your health and/or system.
 * Its use is at your own risk.
 * I assume no responsibility for damages, real, potential, or imagined,
 * resulting  from the use of it.
 * Yeah.....what Eric said...
 */

#include <stdio.h>
#include <string.h>
#include "math.h"
#include <signal.h>
#include <sys/types.h>
#ifdef hpux
#include <time.h>
#else	/* hpux */
#include <sys/time.h>
#endif	/* hpux */
#include "netrek.h"


/*
	Fills in macro window with the macros defined in the .xtrekrc.
*/

#define MAXMACRO 70
/* maximum length in characters of key explanation */

#define MACROLEN 255
/* length of construction string since we don't know how long a macro can be */

void
showMacroWin()
{
   if(!macroWin){
      macroWin = W_MakeTextWindow("macroWin",WINSIDE,0,90,
				  feature_lines() + 3 + macrocnt + 3 +
				  rcd_lines() + 3,
				  NULL, BORDER);
      W_MapWindow(macroWin);
   }
   else if(W_IsMapped(macroWin))
      W_UnmapWindow(macroWin);
   else
      W_MapWindow(macroWin);
}

/* this actually takes care of features, macros, and rcds */
void 
fillmacro()
{
   register int                    row, i;
   char                            macromessage[MACROLEN],
				   *title;

   showFeatures();

   row = feature_lines()+3;

   /* 4 column macro window. This may be changed depending on font size */
   title = "MACROS (<macro-key> <location> <macro>):";
   W_WriteText(macroWin, 1, row, W_Yellow, title, strlen(title), 
      W_RegularFont);
   row += 2;

   for (i = 0; i < macrocnt; row++, i++) {
      if(macro[i].key > MAXASCII)
	 sprintf(macromessage, "^%c ", macro[i].key-96);
      else
	 sprintf(macromessage, " %c ", macro[i].key);
      if (macro[i].type == NEWMMOUSE) {
	 switch (macro[i].who) {
	 case MACRO_PLAYER:
	    strcat(macromessage, " PL MS ");
	    break;
	 case MACRO_TEAM:
	    strcat(macromessage, " TM MS ");
	    break;
	 default:
	    strcat(macromessage, " SELF  ");
	    break;
	 }
      } else {
	 switch (macro[i].who) {
	 case 'T':
	    strcat(macromessage, " TEAM  ");
	    break;
	 case 'A':
	    strcat(macromessage, " ALL   ");
	    break;
	 case 'F':
	    strcat(macromessage, " FED   ");
	    break;
	 case 'R':
	    strcat(macromessage, " ROM   ");
	    break;
	 case 'K':
	    strcat(macromessage, " KLI   ");
	    break;
	 case 'O':
	    strcat(macromessage, " ORI   ");
	    break;
	 case '\0':
	    strcat(macromessage, " SPEC  ");
	    break;
	 default:
	    strcat(macromessage, " ----  ");
	    break;
	 }
      }
      strcat(macromessage, macro[i].string);
      macromessage[MAXMACRO] = '\0';
      W_WriteText(macroWin, 2, row, textColor,
       macromessage, strlen(macromessage), W_RegularFont);
   }

   showRCDs(row);
}
