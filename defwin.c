#ifdef XTREKRC_HELP
/*
 * taken from helpwin.c 
 * (copyright 1991 ERic mehlhaff Free to use, hack, etc. Just keep
 *  these credits here. Use of this code may be dangerous to your health
 *  and/or system. Its use is at your own risk. I assume no responsibility for
 *  damages, real, potential, or imagined, resulting  from the use of it.)
 * 
 * $Log: defwin.c,v $
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
#ifndef SVR4
#include <strings.h>
#endif
#include <string.h>
#include <math.h>
#include <signal.h>
#include <sys/types.h>
#ifdef hpux
#include <time.h>
#else	/* hpux */
#include <sys/time.h>
#endif	/* hpux */
#include "netrek.h"

#define DEFMESSAGES	(sizeof(def_messages)/ sizeof(struct def))
/* this is the number of help messages there are */

#ifdef CONTROL_KEY
static char           *ckeymap = NULL;
#endif

#define INT_DEF		0
#define BOOL_DEF	1
#define STR_DEF		2

#define NAME_WIDTH	18
#define VAL_WIDTH	8
#define INDENT		3	
#define MAX_VLINES	38

extern int forceMono;
#ifdef RECORD
#include "recorder.h"
#endif

char	*name, *keymap, *bmap, *cloak_chars;

/* sure its a mess, but it gets the job done */

static
struct def {
   char	*name;
   int	type;
   char	*desc;
   int	*variable;

   struct {
      int	i_value;	/* if int or bool */
      char	*s_value;	/* if str */
      char	*desc;
   } values[10];
} def_messages[] = {
   {
      "clock", INT_DEF, "Clock display on bottom status bar", &tclock,
      {
	 { 0, NULL, "don't show clock" },
	 { 1, NULL, "show hour:minutes" },
	 { 2, NULL, "show hour:minutes:seconds "},
	 { 0, NULL, NULL },
      },
   },
   {
      "continueTractor", BOOL_DEF, "Show tractor/pressor continuously", 
      &continueTractor,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
   {
      "defaultShip", STR_DEF, "Default ship on button press (string)",
      NULL,
      {
	 { 0, NULL, NULL },
      },
   },
   {
      "extraAlertBorder", BOOL_DEF, "Show alert on inside local border",
      &extraBorder,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
   {
      "fillTriangle", BOOL_DEF, "Fill lock triangle",
      &fillTriangle,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      }
   },
   {
      "forcemono", BOOL_DEF, "Force monochrome display",
      &forceMono,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      }
   },
   {
      "keepPeace", BOOL_DEF, "Stay peaceful when reborn",
      &keeppeace,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
#ifdef LOGMESG
   {
      "logMessage", BOOL_DEF, "Log messages (/tmp/Netrek.Messages.Rec default)",
      &logMess,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
#endif
   {
      "name", STR_DEF, "Default name to use on login",
      (int *) &name,
      {
	 { 0, NULL, NULL },
      },
   },

  {
    "keymap", STR_DEF, "Keyboard map",
    (int *) &(keymap),
    {
      {
        0, NULL, NULL
      },
    },
  },

#ifdef CONTROL_KEY
  {
    "ckeymap", STR_DEF, "Control keyboard map",
    (int *) &(ckeymap),
    {
      {
        0, NULL, NULL
      },
    },
  },
#endif

  {
    "buttonmap", STR_DEF, "Mouse button map",
    (int *) &(bmap),
    {
      {
        0, NULL, NULL
      },
    },
  },

  {
    "cloakChars", STR_DEF, "Cloak chars for map",
    (int *) &(cloak_chars),
    {
      {
        0, NULL, NULL
      },
    },
  },

#ifdef NETSTAT
   {
      "netStatFreq", INT_DEF, "Interval in updates to calculate lag stats",
      &netstatfreq,
      {
	 { 0, NULL, NULL },
      },
   },
   {
      "netStats", BOOL_DEF, "Lag stats and lag meter display",
      &netstat,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
#endif
#ifdef MOOBITMAPS
   {
      "newPlanetBitmaps", BOOL_DEF, "MOO planet bitmaps (resource \"ticks\")",
      &myPlanetBitmap,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
#endif
   {
      "newPlayerList", BOOL_DEF, "Show alternate player list",
      &alt_playerlist,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
   {
      "phaserMsgI", BOOL_DEF, "Show phaser points messages in indiv. window",
      &phas_msgi,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
   {
      "port", INT_DEF, "Default port number if -p not specified",
      NULL,
      {
	 { 0, NULL, NULL },
      },
   },
#ifdef RECORD
   {
      "recordFile", STR_DEF, "Record file for recordGame option",
      (int *)&recordFileName,
      {
	 { 0, NULL, NULL },
      },
   },
   {
      "recordGame", BOOL_DEF, "Record game",
      &recordGame,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
#endif
   {
      "reportKills", BOOL_DEF, "Report kill messages",
      &reportKills,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
   {
      "showGalactic", INT_DEF, "Galactic planet bitmaps",
      &showgalactic,
      {
	 { 0, NULL, "show owner on galactic map" },
	 { 1, NULL, "show resources on galactic map" },
	 { 2, NULL, "show nothing on galactic map" },
	 { 0, NULL, NULL },
      },
   },
   {
      "showLocal", INT_DEF, "Local planet bitmaps",
      &showlocal,
      {
	 { 0, NULL, "show owner on local map" },
	 { 1, NULL, "show resources on local map" },
	 { 2, NULL, "show nothing on local map" },
	 { 0, NULL, NULL },
      },
   },
   {
      "showLock", INT_DEF, "Lock displayed for planets or players", 
      &showLock,
      {
	 { 0, NULL, "don't show lock" },
	 { 1, NULL, "show lock on galactic only" },
	 { 2, NULL, "show lock on tactical only" },
	 { 3, NULL, "show lock on both" },
	 { 0, NULL, NULL },
      }
   },
   {
      "showPlanetNames", BOOL_DEF, "Show planet names on galactic/local",
      &namemode,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
   {
      "showStats", BOOL_DEF, "Show ship stats window",
      &showStats,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
   {
      "showTractorPressor", BOOL_DEF, "Show (my) tractor/pressor",
      &showTractorPressor,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
#ifdef EM
   {
      "sortPlayers", BOOL_DEF, "Sort players by team",
      &sortPlayers,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
   {
      "sortPlayersObs", BOOL_DEF, "Show players before observers",
      &sortPlayersObs,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
#endif
#ifdef SHORT_PACKETS
   {
      "tryShort", BOOL_DEF, "Try SHORT-PACKETS automatically",
      &tryShort,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
#endif
   {
      "tryUdp", BOOL_DEF, "Try UDP automatically",
      &tryUdp,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
   {
      "udpClientReceive", INT_DEF, "UDP receive mode",
      &udpClientRecv,
      {
	 { 0, NULL, "TCP only" },
	 { 1, NULL, "simple UDP" },
	 { 2, NULL, "fat UDP" },
	 { 3, NULL, "double UDP (obsolete)" },
	 { 0, NULL, NULL },
      },
   },
   {
      "udpClientSend", INT_DEF, "UDP send mode",
      &udpClientSend,
      {
	 { 0, NULL, "TCP only" },
	 { 1, NULL, "simple UDP" },
	 { 2, NULL, "enforced UDP (state only)" },
	 { 3, NULL, "enforced UDP (state & weapon)" },
	 { 0, NULL, NULL },
      },
   },
   {
      "udpDebug", INT_DEF, "UDP debug mode",
      &udpDebug,
      {
	 { 0, NULL, "off" },
	 { 1, NULL, "on (connect messages only)" },
	 { 2, NULL, "on (verbose output)" },
	 { 0, NULL, NULL },
      },
   },
   {
      "udpSequenceCheck", BOOL_DEF, "UDP sequence checking",
      &udpSequenceChk,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
   {
      "updatesPerSecond", INT_DEF, "Updates per second",
      &updates_per_second,
      {
	 { 0, NULL, NULL },
      },
   },
   {
      "useMsgw", BOOL_DEF, "Send last message to send-message window",
      &use_msgw,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
   {
      "varyShields", BOOL_DEF, "Show shield damage by color",
      &VShieldBitmaps,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
   {
      "warp", BOOL_DEF, "Use pointer warp to message window",
      &warp,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
   {
      "newMesgFlags", BOOL_DEF, "Use new message flags from server",
      &new_messages,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
   {
      "shortKillMesg", BOOL_DEF, "Trim kill messages to fit 80w",
      &abbr_kmesg,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
   {
      "showPlayerStatus", BOOL_DEF, "Show player status (alive/dead) in pl",
      &plshowstatus,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
   {
      "phaserWindow", BOOL_DEF, "Show phaser-hit log",
      &phaserWindow,
      {
	 { 0, NULL, "" },
	 { 0, NULL, NULL },
      },
   },
};

char *
itos(v)

   int	v;
{
   static char	value[10];
   sprintf(value, "%d", v);
   return value;
}

char *
btoa(v)

   int	v;
{
   if(v) return "on";
   else
      return "off";
}

void 
showdef()
{
   register		i, j, x=0, y=0, xo=0, yo=0, max_desc=0,
			height=1, width=1;
   register struct def *d;
   char			*val;

   if(!defWin)
      defWin = W_MakeTextWindow("xtrekrc_help", 1, 1, 10, 10, NULL, BORDER);
   
   name = getdefault("name");
   keymap = getdefault("keymap");
#ifdef CONTROL_KEY
   ckeymap = getdefault ("ckeymap");

   /*
    * Handle leading ^G generated by make_xtrekrc () routine.
    */
   if (ckeymap && ckeymap[0] == 7 && ckeymap[1] == 7)
     ckeymap = &(ckeymap[2]);
#endif
   
   cloak_chars = cloakChars;
   bmap = getdefault("buttonmap");

   for(i=0,d=def_messages; i< DEFMESSAGES; i++, d++){
      x = xo;
      y = yo;

      W_WriteText(defWin, x, y, W_Yellow, d->name, strlen(d->name),
	 W_RegularFont);
      x += NAME_WIDTH;

      W_WriteText(defWin, x, y, textColor, d->desc, strlen(d->desc),
	 W_RegularFont);
      if(strlen(d->desc) > max_desc){
	 max_desc = strlen(d->desc);
	 width = MAX(width, x+max_desc);
      }
      y ++;
      x = xo + INDENT;

      if(d->type != STR_DEF){
	 if(!d->values[0].desc && d->variable){
	    val = itos(d->values[0].i_value);
	    W_WriteText(defWin, x, y, W_Green, val, strlen(val), 
	       W_RegularFont);
	    y++;
	 }
	 for(j=0; d->values[j].desc ; j++){
	    switch(d->type){
	       case INT_DEF:
		  val = itos(d->values[j].i_value);
		  if(d->values[j].i_value == *d->variable){
		     W_WriteText(defWin, x, y, W_Green, val, strlen(val), 
			W_RegularFont);
		     if(W_Mono()){
			W_WriteText(defWin, x+1, y, W_Green, "*", 1, 
			   W_RegularFont);
		     }
		  }
		  else
		     W_WriteText(defWin, x, y, textColor, val, strlen(val), 
			W_RegularFont);
		  x = xo + NAME_WIDTH;
		  W_WriteText(defWin, x, y, textColor, d->values[j].desc, 
		     strlen(d->values[j].desc), W_RegularFont);
		  y++;
		  x = xo + INDENT;
		  break;

	       case BOOL_DEF:
		  val = btoa(*d->variable);
		  W_WriteText(defWin, x, y, W_Green, val, strlen(val), 
		     W_RegularFont);
		  y++;
		  x = xo + INDENT;
		  break;
	       default:
		  fprintf(stderr, "Unknown type.\n");
		  break;
	    }
	 }
      }
      else if(d->variable && *((char *)d->variable)){
	 W_WriteText(defWin, x, y, W_Green, (char *) *d->variable,
					    strlen((char *)(*d->variable)),
	    			            W_RegularFont);
	 y++;
      }

      height = MAX(height, y);
      if(y > MAX_VLINES){
	 yo = 0;
	 xo += NAME_WIDTH + max_desc + 2;
	 max_desc = 0;
      }
      else{
	 yo = y + 1;
      }
   }
   if(!W_IsMapped(defWin)){
      W_ResizeTextWindow(defWin, width, height);
      W_MapWindow(defWin);
   }
}
#endif
