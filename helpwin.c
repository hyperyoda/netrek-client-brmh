/*
 * helpwin.c copyright 1991 ERic mehlhaff Free to use, hack, etc. Just keep
 * these credits here. Use of this code may be dangerous to your health
 * and/or system. Its use is at your own risk. I assume no responsibility for
 * damages, real, potential, or imagined, resulting  from the use of it.
 * 
 * $Log: helpwin.c,v $
 * Revision 1.2  2000/02/17 05:48:05  ahn
 * BRMH 2.3 from David Pinkney <dpinkney@cs.uml.edu>
 *
 * Revision 1.6  1993/10/05  16:40:38  hadley
 * checkin
 *
 * Revision 1.6  1993/10/05  16:38:08  hadley
 * checkin
 *
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
 * this is a set of routines that makes up a multi column help window, * and
 * shows just what the keymaps current keymap representation of the * keys
 * are. *
 * 
 *  fillhelp() handles the filling in if the strings for the help window *
 * update_Help_to_Keymap() checks the keymap and sets it up in hte * help
 * window. *
 * 
 * 
 *
 * Format for each entry is follows: * first character is the hard-coded
 * character representation for the key * in the keymap.  Useful for when you
 * re-key things. This  could confuse * people who do'nt know the keymap
 * format. *
 * 
 * the next few spaces are either spaces or  keys that also do that *
 * function.  Note that if you have more than 3 keys set to do the same *
 * thing, they will not be displayed. * So, you could, I suppose map
 * everything to 'Q' and it would not * show, but that's a pretty bizarre
 * situation. *
 * 
 * 
 *
 * Bugs & Comments: * You have to be sure that helpWin is defined to be big
 * enough to handle * all the messages.  That's pretty much a trial&error
 * by-hand thing * at this point *
 * 
 */



/*
 * fills in the help window to note all keys also mapped to the *  listed
 * functions
 */
void            update_Help_to_Keymap();

#define HELPMESSAGES	(sizeof(help_message)/ sizeof(char *))
/* this is the number of help messages there are */

char           *help_message[] =
{
   "0     Set speed",
   "1     Set speed",
   "2     Set speed",
   "3     Set speed",
   "4     Set speed",
   "5     Set speed",
   "6     Set speed",
   "7     Set speed",
   "8     Set speed",
   "9     Set speed",
   ")     speed = 10",
   "!     speed = 11",
   "@     speed = 12",
   "%     speed = maximum",
   "#     speed = 1/2 maximum",
#ifdef EM
   "<     slow speed 1",
   ">     speed up 1",
#endif				/* EM */
   "k     Set course",
   "p     Fire phaser",
   "t     Fire photon torpedo",
   "f     Fire plasma torpedo",
#ifdef SCAN
   "a     Use scanning beam",
#endif				/* ATM */
   "d     detonate enemy torps",
   "D     detonate own torps",
   "L     List players",
   "P     List planets",
   "S     Status graph toggle",
   "]     Put up shields",
   "[     Put down shields",
   "u     Shield toggle",
   "s     Shield toggle",
   "i     Info on player/planet",
   "I     Extended info on player",
   "b     Bomb planet",
   "z     Beam up armies",
   "x     Beam down armies",
#ifdef EM
   "{     Cloak",
   "}     Uncloak",
#endif
   "T     Toggle tractor beam",
   "y     Toggle pressor beam",
   "_     Tractor on",
   "^     Pressor on",
   "$     Tractor/Pressor off",
   "R     Enter repair mode",
   "o     Orbit planet or dock to outpost",
   "e     Docking permission toggle",
   "r     Refit (change ship type)",
   "Q     Quit",
   "q     fast quit (no entry window)",
   "?     Message window rotate",
   "c     Cloaking device toggle",
   "C     Coup a planet",
   "l     Lock on to player/planet",
   ";     Lock on starbase/planet",
   "h     Help window toggle",
   "w     War declarations window",
   "N     Planet names toggle",
   "V     Rotate local planet display",
   "B     Rotate galactic planet display",
   "*     Send in practice robot",
   "E     Send Distress signal",
#ifdef EM
   "F     Send armies carried report",
#endif				/* EM */
   "U     Show rankings window",
   "m     Message Window Zoom",
   "+     Show UDP options window",
   "=     Update all",
#ifdef NETSTAT
   "/     Map LagMeter",
#endif
#ifdef PING
   ",     Ping stats window",
#endif
#ifdef SHORT_PACKETS
   "-     Update small",
#endif				/* SHORT_PACKETS */
   "      (space) Unmap special windows",
   ":     Re-Read netrekrc defaults",
   "&     Re-Read all netrekrc defaults",
#ifdef LOGMESG
   "M     Toggle message logging",
#endif
#ifdef nodef
   ";     Toggle planet Bitmaps",
#endif
   "|     Toggle player list sorting",
   "\\     Toggle player list",
#ifdef NBT
   "X     Enter Macro Mode",
#endif
#if !defined(CONTROL_KEY) && defined(FEATURE)
#ifdef NBT
   "X-?   Show FEATURES/MACROS/RCD window",
#endif
#endif

#if defined (CONTROL_KEY) && defined (FEATURE)
   "^s    Show FEATURES/MACROS/RCD window",
#endif

#ifdef RECORD
   "`     Toggle game recording",
#endif
   0
};

#define MAXHELP 40
/* maximum length in characters of key explanation */


void 
fillhelp()
{
   register int    i = 0, row, column;
   char            helpmessage[MAXHELP];


   /* 4 column help window. THis may be changed depending on font size */
   for (column = 0; column < 4; column++) {
      for (row = 1; row < HELPMESSAGES / 4 + 2; row++) {
	 if (help_message[i] == 0)
	    break;
	 else {
	    strcpy(helpmessage, help_message[i]);
	    update_Help_to_Keymap(helpmessage);
	    W_WriteText(helpWin, MAXHELP * column, row, textColor,
			helpmessage, strlen(helpmessage), W_RegularFont);
	    i++;
	 }
      }
      if (help_message[i] == 0)
	 break;
   }
}


/*
 * this takes the help messages and puts in the keymap, so the player can see
 * just what does  what!
 * 
 * ordinary format:       "U     Show rankings window", translatedd here to
 * "[ sE  Computer options window",
 */
void 
update_Help_to_Keymap(helpmessage)
    char            helpmessage[];
{
   int             i, num_mapped = 0;
   unsigned char   key;

#ifdef CONTROL_KEY
   if (helpmessage[0] == '^' && helpmessage[1] != ' ')
      key = helpmessage[1] + 96;
   else
      key = helpmessage[0];
#else
   key = helpmessage[0];
#endif

#ifdef CONTROL_KEY
   if (key < 32){
      return;
   }
#else
  if (key < 32 || key > 126)
  {
    return;
  }
#endif

   if (strlen(helpmessage) < 6) {
      return;
   }
   /*
    * search the keymap to see if one of its entries matches the key * for
    * this helpmessage
    */
   for (i = 0; i < MAXKEY; i++) {
      if (mykeymap[i] != key)
	 continue;
      if (i + 32 == key)
	 continue;		/* it's already there!  don't add it! */

      /* we've found a key mapped to key! */
      /* the key is i+32 */
      num_mapped++;
      if (num_mapped > 3)
	 continue;		/* we've found too many! */

      /* put the key in the string */
#ifdef CONTROL_KEY
      if (i > 127) {
	 helpmessage[1 + num_mapped++] = '^';
	 helpmessage[1 + num_mapped] = (char) i + 32 - 96;
      }
      else
	 helpmessage[1 + num_mapped] = i + 32;
#else
      helpmessage[1 + num_mapped] = i + 32;
#endif

      /* handle mappings for weird keys, i.e. space */
      if (i + 32 == ' ')
	 helpmessage[1 + num_mapped] = '_';
   }


   /* clear spaces if any area available */
   switch (num_mapped) {
   case 0:
      helpmessage[2] = ' ';
   case 1:
      helpmessage[3] = ' ';
   case 2:
      helpmessage[4] = ' ';
   case 3:
   default:
      helpmessage[5] = ' ';
   }

   return;
}
