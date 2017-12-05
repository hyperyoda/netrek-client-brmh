/*
 * input.c
 * 
 * Modified to work as client in socket based protocol
 */
#include "copyright.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#ifdef hpux
#include <time.h>
#else				/* hpux */
#include <sys/time.h>
#endif				/* hpux */
#include <signal.h>
#include <errno.h>
#include "netrek.h"

#ifdef RECORD
#include "recorder.h"
#endif

#if __STDC__ || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

/* input.c */
static unsigned char *AddStringToKeymap P_((unsigned char *str, unsigned char *keymap, char *keymapName, int isCtrlKeymap));
static void AddStrToButtonMap P_((unsigned char *str, char *buttonmap));

int torprepeat = 0;
int last_torp = 0;

#undef P_

/*
 * Create a keymap with the default bindings.
 * 
 * If keyMap has not been allocated yet (keyMap is NULL) this will involve
 * reallocating the keymap.  Otherwise, the procedure will reset the
 * given keymap.
 * 
 * RETURN: The updated keymap.
 */

unsigned char  *
InitKeyMap(keyMap)
   unsigned char  *keyMap;
{
   register        i;
   unsigned char  *defaultMap;

   if (!keyMap) {
      keyMap = (unsigned char *) calloc(1, MAXKEY);

      if (!keyMap) {
	 fprintf(stderr, "netrek: Out of memory\n");
	 exit(1);
      }
   }
   defaultMap = keymaps[KEYMAP_DEFAULT];

   if ((!defaultMap) || (keyMap == defaultMap)) {
      /* The default keyMap starts out as a null keymap. */

      for (i = MAXKEY - 2; i >= 0; --i)
	 keyMap[i] = i + 32;

      keyMap[MAXKEY - 1] = 0;
   } else {
      /* Other keymaps start out as the current default keymap. */

      for (i = MAXKEY - 1; i >= 0; --i)
	 keyMap[i] = defaultMap[i];
   }

   return keyMap;
}


/*
 * Add the given string of keybindings to the keymap.
 * 
 * The keymap will be re-initialised if it is not already defined. If
 * isCtrlKeymap is set then `^' notation is allowed.  The variable
 * "keymapName" contains the name of the keymap (eg "ckeymap-sc") so that
 * errors can be reported nicely.
 * 
 * RETURN The updated keymap.
 */

static unsigned char *
AddStringToKeymap(str, keymap, keymapName, isCtrlKeymap)
   unsigned char   	*str, *keymap; 
   char	    		*keymapName;
   int             	isCtrlKeymap;
{
   int             ch = 0, left = 0, offset = 0, error = 0;
   unsigned char  *point;

   if (!keymap)
      keymap = InitKeyMap(keymap);


   /*
    * Use a character by character parser so that we can examine each
    * character properly and so we don't parse passed the end of the string
    * if it ends too early.
    */

   point = str;

   for (; *str != '\0'; ++str) {
      /* Read the next key  */

      if (*str == '^' && isCtrlKeymap) {
	 if (!offset) {
	    offset = 96;	/* A single ^ means control */
	    continue;		/* No new character */
	 } else {
	    ch = '^';		/* A double ^ means ^ */
	 }
      } else if (*str >= 32 && *str < MAXASCII) {
	 ch = *(str) + offset;
      } else {
	 error = 1;
	 ch = 1;
      }


      /* Have we read two keys yet?  */

      if (!left) {
	 left = ch;
      } else if (error) {
	 fprintf(stderr, "%s: Ignoring the %s entry `", defaults_file,
	    keymapName);

	 while (point <= str) {
	    putc(*point, stderr);
	    ++point;
	 }

	 fputs("'.\n\tUse 'ckeymap' to map control characters.\n", stderr);

	 left = 0;
	 point = str + 1;
      } else {
	 keymap[left - 32] = ch;
	 left = 0;
	 point = str + 1;
      }

      offset = 0;		/* Forget about ^ flag after each key is read */
   }

   if (left || offset) {
      fprintf(stderr, 
	 "%s: %s ends halfway through a key definition\n",
	      defaults_file, keymapName);
   }
   return keymap;
}


/*
 * Add a string of definitions to a buttonmap.
 */

static void
AddStrToButtonMap(str, buttonmap)
   unsigned char  	*str; 
   char		*buttonmap;
{
   int             ind, offset, error;
   char           *errString =
   "%s: buttonmap ends in the middle of a definition.\n";

   while (*str != '\0') {
      /*
       * Set the flags.
       */

      offset = 0;
      error = 0;


      /*
       * Get a number.
       */

      ind = (*str) - '0';

      if (ind > 9)
	 ind = (*str) - 'a' + 10;

      if (ind < 1 || ind > 12) {
	 fprintf(stderr, "%s: %c ignored in buttonmap\n", defaults_file, *str);
	 error = 1;		/* Allow error recovery */
      }
      if(ind > 3) extended_mouse = 1;
      /*
       * Get a key to map to.
       */

      offset = 0;

      if (*(++str) == '\0') {
	 fprintf(stderr, errString, defaults_file);
	 break;
      }
      if (*str == '^') {
	 if (*(++str) == '\0') {
	    fprintf(stderr, errString, defaults_file);
	    break;
	 }
	 if (*str != '^')
	    offset = 96;	/* ^a is ctrl-a but ^^ is ^ */
      }
      /*
       * Do the mapping if an error has not occured.
       */

      if (!error)
	 buttonmap[ind] = *str + offset;

      ++str;
   }
}


void
initkeymaps()
    /*
     * Load a new set of keymaps.
     * 
     * NOTE: The function will install a new set of keymaps, rather than
     * extending the old keymaps, if the defaults file is reloaded halfway
     * through a game.
     */
{
   unsigned char  *str;
   register        i;
   char            buf[80];


   /*
    * Create the default keymap first because the default keymap should be
    * the basis for ship specific keymaps.
    * 
    * Don't call InitKeyMap(keymaps[KEYMAP_DEFAULT]) here because it should have
    * been called already in initDefaults so that the macro defintions could
    * be assigned.  This is very nasty but what can you do.
    */

   if ((str = (unsigned char *) getdefault("keymap")) != NULL) {
      keymaps[KEYMAP_DEFAULT] =
	 AddStringToKeymap(str, keymaps[KEYMAP_DEFAULT],
			   "keymap", 0);
   }
#ifdef CONTROL_KEY
   if ((str = (unsigned char *) getdefault("ckeymap")) != NULL) {
      keymaps[KEYMAP_DEFAULT] =
	 AddStringToKeymap(str, keymaps[KEYMAP_DEFAULT],
			   "ckeymap", 1);
   }
#endif


   /*
    * Now create a keymap for each ship type that requires it.
    */

   for (i = NUM_TYPES - 1; i >= 0; --i) {
      if (keymaps[i])
	 keymaps[i] = InitKeyMap(keymaps[i]);

      sprintf(buf, "keymap-%s", classes[i]);

      if ((str = (unsigned char *) getdefault(buf)) != NULL) {
	 keymaps[i] = AddStringToKeymap(str, keymaps[i], buf, 0);
      }
#ifdef CONTROL_KEY
      sprintf(buf, "ckeymap-%s", classes[i]);

      if ((str = (unsigned char *) getdefault(buf)) != NULL) {
	 keymaps[i] = AddStringToKeymap(str, keymaps[i], buf, 1);
      }
#endif
   }


   /*
    * Now extend keymap with keymap-default and ckeymap-default.
    */

   if ((str = (unsigned char *) getdefault("keymap-default")) != NULL) {
      keymaps[KEYMAP_DEFAULT] =
	 AddStringToKeymap(str, keymaps[KEYMAP_DEFAULT],
			   "keymap-default", 0);
   }
#ifdef CONTROL_KEY
   if ((str = (unsigned char *) getdefault("ckeymap-default")) != NULL) {
      keymaps[KEYMAP_DEFAULT] =
	 AddStringToKeymap(str, keymaps[KEYMAP_DEFAULT],
			   "ckeymap-default", 1);
   }
#endif


#ifdef SHOW_DEFAULTS
   show_defaults("input-keymap", "keymap", "aabbcc",
		 "Maps new keys to old keys. Format: <new key><old key><new key><old key>...");
   show_defaults("input-keymap", "keymap-[sc,dd,ca,bb,sb]", "",
		 "Ship-specific keymap extensions (default is keymap).");
   show_defaults("input-keymap", "keymap-default", "",
	      "keymap extension used if keymap-<ship_type> is not defined");
#endif


#ifdef CONTROL_KEY		/* COWlite */
#ifdef SHOW_DEFAULTS
   show_defaults("input-keymap", "ckeymap", "^a^a^b^b^c^c",
		 "Control keys keymap. Format <new key><old key> ...\n\
Any combination of normal keys and control keys can be mapped to one\n\
another.  In other words, you can map from control key to control key\n\
control key to normal key, normal key to normal key, and normal key\n\
to control key.\n\
 \n\
New format for ckeymap is:\n\
c = any printable ascii character.\n\
^ = introduce control mapping (the key '^' not control + key.)\n\
 \n\
Each entry is a pair, like:\n\
cc              # regular format\n\
c^c             # regular->control\n\
^cc             # control->regular\n\
^c^c            # control->control");
   show_defaults("input-keymap", "ckeymap-[sc,dd,ca,bb,sb]", "",
	   "Ship-specific control-key map extensions (default is ckeymap)");
   show_defaults("input-keymap", "ckeymap-default", "",
	    "ckeymap extension used if ckeymap-<ship_type> is not defined");
#endif
#endif


   /* note: not stored on server */

   for (i = 11; i >= 0; --i)	/* Clear the current button map */
      buttonmap[i] = '\0';

   if ((str = (unsigned char *) getdefault("buttonmap")) != NULL)
      AddStrToButtonMap(str, buttonmap);


#ifdef SHOW_DEFAULTS
   show_defaults("input", "buttonmap", "1t2p3k4%5q6#7u8c9iaTbyc;",
		 "Maps buttons to key functions.  Format: <button><key>... \n\
See the help window for key descriptions.\n\
<button> is in the range 1-3 if shiftedMouse is off.  Otherwise the\n\
values are\n\
4 - shift+m1, 5 - shift+m2, 6 - shift+m3\n\
7 - ctrl+m1, 8 - ctrl+m2, 9 - ctrl+m3\n\
a - shift+ctrl+m1, b - shift+ctrl+m2, c - shift+ctrl+m3");
#endif
}

void
initinput()
{
   /* Nothing doing... */
}

void
input()
{
   fd_set          readfds;
   register
   int             xsock = W_Socket();	/* put it in a register */
   register
   int             doflush = 0;

#ifdef RECORD_DEBUG
   fprintf(RECORDFD, "input() called\n");  /* DBP */
#endif
   while (1) {

      FD_ZERO(&readfds);
      FD_SET(xsock, &readfds);
      FD_SET(sock, &readfds);
      if (udpSock >= 0)
	 FD_SET(udpSock, &readfds);

      select(max_fd, &readfds, 0, 0, 0);

      if (FD_ISSET(xsock, &readfds) || W_EventsQueued()) {
	 process_event();
	 /* NOTE: we're no longer calling XPending(), need this */
	 doflush = 1;
      }

      if (FD_ISSET(sock, &readfds) ||
	  (udpSock >= 0 && FD_ISSET(udpSock, &readfds))) {
	 intrupt(&readfds);
	 /* NOTE: we're no longer calling XPending(), need this */
	 doflush = 1;
	 if (isServerDead()) {
	    printf("Whoops!  We've been ghostbusted!\n");
	    printf("Pray for a miracle!\n");
	    /* UDP fail-safe */
	    commMode = commModeReq = COMM_TCP;
	    commSwitchTimeout = 0;
	    if (udpSock >= 0)
	       closeUdpConn();
	    if (udpWin) {
	       udprefresh(UDP_CURRENT);
	       udprefresh(UDP_STATUS);
	    }
	    connectToServer(nextSocket);
	    printf("Yea!  We've been resurrected!\n");
	 }
      }
      /* NOTE: we're no longer calling XPending(), need this */
      if (doflush) {
	 W_Flush();
	 doflush = 0;
      }
   }
}

int
process_event()
{
   W_Event         data;
   register W_Window win;

   do {
      /* we don't need a loop here technically but it won't hurt */
      if (!W_SpNextEvent(&data))
	 continue;		/* continues at loop bottom */
      win = data.Window;

      switch ((int) data.type) {
      case W_EV_KEY:
	 if (messageon) {
	    smessage(data.key);
	 }
	 /* move checks to top instead of inside keyaction() */
	 else if (win == w || win == mapw) {
	    keyaction(&data);
	 } else if (win == optionWin) {
	    optionaction(&data);
	 } else if (win == messagew || win == warnw) {
	    smessage(data.key);
	 } else if (motdWin && win == motdWin) {
	    motdWinEvent(data.key);
	 } else if ((infow && win == infow)
#ifdef SCAN
		    || win == scanwin
#endif
	    ) {
	    keyaction(&data);
	 }
	 break;
      case W_EV_BUTTON:
	 /* move checks to top instead of inside buttonaction() */
	 if (win == w || win == mapw) {
	    buttonaction(&data);
	 } else if (win == war) {
	    waraction(&data);
	 } else if (win == optionWin) {
	    optionaction(&data);
	 } else if (win == udpWin) {
	    udpaction(&data);	/* UDP */
	 } else if (win == messagew || win == warnw) {
	    if ((data.key == W_MBUTTON) && messpend)
	       smess_paste();
	 } else if ((infow && win == infow)
#ifdef SCAN
		    || win == scanwin
#endif
	    ) {
	    buttonaction(&data);
	 }
	 break;
      case W_EV_EXPOSE:
	 /* most common first */
	 if (win == mapw) {
	    setup_redraw_map(data.x, data.y,
			     data.x + data.width, data.y + data.height);
#ifdef nodef
	    if (!redrawall)
	       redrawall = 2;	/* no need to clear */
#endif
	 } else if (win == w)	/* nothing special to do here */
	    ;
	 else if (win == statwin)
	    redrawStats();
#ifdef NETSTAT
	 else if (win == lMeter)
	    redrawLMeter();
#endif
#ifdef PING
	 else if (win == pStats)
	    redrawPStats();
#endif
	 else if (win == tstatw)
	    redrawTstats();
	 else if (win == iconWin)
	    drawIcon();
	 else if (win == helpWin)
	    fillhelp();
#ifdef XTREKRC_HELP
	 else if (defWin && (win == defWin))
	    showdef();
#endif
	 else if (win == playerw)
	    playerlist();
	 else if (win == planetw)
	    planetlist();
	 else if (win == rankw)
	    ranklist();
	 else if (win == warnw)
	    W_ClearWindow(warnw);
	 else if (win == messagew)
	    smess_refresh();
	 else if (motdWin && win == motdWin)
	    motdWinEvent('r');	/* 'r' is refresh */
#ifdef FEATURE
	 else if (macroWin && win == macroWin)
	    fillmacro();
#endif
	 break;
      default:
	 break;
      }
   }
   while (W_EventsQueued());
   return 1;
}

#ifdef XTRA
void
xinput()
{
   static
   struct timeval  timeout =
   {0, 0};
   fd_set          readfds;
   int             xsock = W_Socket();

   FD_ZERO(&readfds);
   FD_SET(xsock, &readfds);
   if (select(max_fd, &readfds, 0, 0, &timeout) == 1) {
      process_event();
   }
}
#endif

void
keyaction(data)
    W_Event        *data;
{
   unsigned char   course;
   struct obtype  *gettarget(), *target;
   register W_Window win = data->Window;
   register unsigned char key = data->key;
   static int      last_det;
   register        now;

   if ((infow && win == infow)
#ifdef SCAN
       || win == scanwin
#endif
      ) {
      int             x, y;
      if (W_FindMouseInWin(&x, &y, w)) {	/* local window */
	 win = w;
	 data->x = x;
	 data->y = y;
      } else if (W_FindMouseInWin(&x, &y, mapw)) {	/* galactic window */
	 win = mapw;
	 data->x = x;
	 data->y = y;
      }
   }

#ifdef RECORD
   if(playback)
     if(playback_keyaction(data))  /* was a playback command */
       return;
#endif

#ifdef FEATURE
   if (key == macrokey)
      key = 'X';
#endif

   if (!strchr("sbogadc", key) || !(localflags & PFREFIT)) {
      if (key >= 32 && key < MAXKEY) {
          key = mykeymap[key - 32];
      }
   }
#ifdef FEATURE
   if (!MacroMode && singleMacro) {
      if (strchr(singleMacro, data->key)) {
	 MacroMode = 1;
	 if (F_UseNewMacro)
	    MacroNum = -1;
      }
   }
   if (MacroMode) {
      doMacro(data->key, data);
      return;
   }
#endif

   switch (key) {
#ifdef FEATURE
   case 'X':
#if 0				/* assume server mechanism is sufficient */
      gettimeofday(&curtp, NULL);
      if (0 /* curtp.tv_sec < lasttp.tv_sec + 6 */ ) {
	 warning("Chill out with the messages!!");
	 break;
      }
      lasttp = curtp;
#endif
      warning("In Macro Mode");
      if (F_UseNewMacro)
	 MacroNum = -1;
      MacroMode = 1;
      break;
#endif

   case '0':
   case '1':
   case '2':
   case '3':
   case '4':
   case '5':
   case '6':
   case '7':
   case '8':
   case '9':
      set_speed(key - '0');
      localflags &= ~(PFREFIT);
      break;
   case 'e':			/* Turn off docking permission, eject docked
				 * vessels. */
      if (me->p_flags & PFDOCKOK)
	 sendDockingReq(0);
      else
	 sendDockingReq(1);
      break;
   case 'r':
      localflags |= PFREFIT;
#ifdef GALAXY
      warning("s=scout, d=destroyer, c=cruiser, b=battleship, a=assault, g=galaxy, o=starbase/outpost");
#else
      warning("s=scout, d=destroyer, c=cruiser, b=battleship, a=assault, o=starbase/outpost");
#endif				/* GALAXY */
      break;
   case ')':
      set_speed(10);
      localflags &= ~(PFREFIT);
      break;
   case '!':
      set_speed(11);
      localflags &= ~(PFREFIT);
      break;
   case '@':
      set_speed(12);
      localflags &= ~(PFREFIT);
      break;
   case '#':
      set_speed(me->p_ship.s_maxspeed / 2);
      localflags &= ~(PFREFIT);
      break;
   case '%':
      set_speed(99);		/* Max speed... */
      localflags &= ~(PFREFIT);
      break;
   case '&':
      ReReadDefaults();
      break;

#ifdef EM
   case '>':
      set_speed(me->p_speed + 1);
      break;
   case '<':
      set_speed(me->p_speed - 1);
      break;
   case '{':
      cloak_on();
      break;
   case '}':
      cloak_off();
      break;
#endif				/* EM */
   case 'a':
      if (localflags & PFREFIT) {
	 do_refit(ASSAULT);
      }
#ifdef SCAN
      else {
	 if (!W_IsMapped(scanwin)) {
	    scan(win, data->x, data->y);
	 } else {
	    if (scanmapped)
	       W_UnmapWindow(scanwin);
	    scanmapped = 0;
	 }
      }
#endif				/* ATM */
      break;
#ifdef GALAXY
   case 'g':
      if (localflags & PFREFIT) {
	 do_refit(SGALAXY);
      }
      break;
#endif				/* GALAXY */
   case 'm':			/* new from galaxy -- works here too */
      message_on();
      break;

   case 'k':			/* k = set course */
      course = getcourse(win, data->x, data->y);
      set_course(course);
      me->p_flags &= ~(PFPLOCK | PFPLLOCK);
      localflags &= ~(PFREFIT);
      break;
   case 'K':            /* K = reset clock to zero */
      time(&basetime);
      break;
   case 'p':			/* p = fire phasers */
      course = getcourse(win, data->x, data->y);
      sendPhaserReq(course);
      break;
   case 't':			/* t = launch torps */
      course = getcourse(win, data->x, data->y);
      if ((now = mstime()) - last_torp >= 50) {
	 sendTorpReq(course);
	 last_torp = now;
      }
      break;
   case 'f':
      /* f = launch plasma torpedos */
      course = getcourse(win, data->x, data->y);
      sendPlasmaReq(course);
      break;
   case 'd':			/* d = detonate other torps */
      if (localflags & PFREFIT)
	 do_refit(DESTROYER);
      else if ((now = mstime()) - last_det >= 100) {
	 sendDetonateReq();
	 last_det = now;
      }
      break;
   case 'D':			/* D = detonate my torps */
      detmine();
      break;
   case '[':
      shield_down();
      break;
   case ']':
      shield_up();
      break;
   case 'u':			/* u = toggle shields */
      shield_tog();
      break;
   case 's':			/* For Serge */
      if (localflags & PFREFIT)
	 do_refit(SCOUT);
      else
	 shield_tog();
      break;
   case 'b':			/* b = bomb planet */
      if (localflags & PFREFIT)
	 do_refit(BATTLESHIP);
      else
	 bomb_planet();
      break;
   case 'z':			/* z = beam up */
      beam_up();
      break;
   case 'x':			/* x = beam down */
      beam_down();
      break;
   case 'R':			/* R = Go into repair mode */
      sendRepairReq(1);
      break;
   case 'y':
   case 'T':
      if (me->p_flags & (PFTRACT | PFPRESS)) {
	 sendTractorReq(0, me->p_no);
	 break;
      }
      target = gettarget(win, data->x, data->y, TARG_PLAYER);
      me->p_tractor = target->o_num;
      if (key == 'T') {
	 sendTractorReq(1, target->o_num);
      } else {
	 sendRepressReq(1, target->o_num);
      }
      break;

      /*
       * Alternative tractor/pressor.  These keys will nearly always be
       * remapped.
       */
   case '_':
      if (me->p_flags & (PFTRACT | PFPRESS))
	 sendTractorReq(0, me->p_no);
      target = gettarget(win, data->x, data->y, TARG_PLAYER);
      me->p_tractor = target->o_num;
      sendTractorReq(1, target->o_num);
      break;

   case '^':
      if (me->p_flags & (PFTRACT | PFPRESS))
	 sendRepressReq(0, me->p_no);
      target = gettarget(win, data->x, data->y, TARG_PLAYER);
      me->p_tractor = target->o_num;
      sendRepressReq(1, target->o_num);
      break;
   case '$':
      sendTractorReq(0, me->p_no);
      break;

   case 'o':			/* o = dock at nearby starbase or orbit
				 * nearest planet */
      if (localflags & PFREFIT) {
	 do_refit(STARBASE);
      } else {
	 sendOrbitReq(1);
      }
      break;
   case 'O':			/* O = options Window */
      if (optionWin != NULL && W_IsMapped(optionWin))
	 optiondone();
      else
	 optionwindow();
      break;
   case 'q':
      if (fastQuitOk)
	 fastQuit = 1;
   case 'Q':
      sendQuitReq();
      break;
   case 'V':
      showlocal++;
      if (showlocal == 4)
	 showlocal = 0;
      if (optionWin)
	 optionredrawoption(&showlocal);

      break;
   case 'B':
      showgalactic++;
      if (showgalactic == 3)
	 showgalactic = 0;
      if (!redrawall)
	 redrawall = 2;
      if (optionWin)
	 optionredrawoption(&showlocal);
      break;
   case '?':			/* ? = Redisplay all message windows */

      if(!W_IsMapped(reviewWin)) 
	 W_MapWindow(reviewWin);
      else
	 W_UnmapWindow(reviewWin);
	 
      if(!W_IsMapped(phaserwin)) 
	 W_MapWindow(phaserwin);
      else
	 W_UnmapWindow(phaserwin);

      if(!W_IsMapped(messwa)) 
	 W_MapWindow(messwa);
      else
	 W_UnmapWindow(messwa);

      if(!W_IsMapped(messwt)) 
	 W_MapWindow(messwt);
      else
	 W_UnmapWindow(messwt);

      if(!W_IsMapped(messwi)) 
	 W_MapWindow(messwi);
      else
	 W_UnmapWindow(messwi);

      if(!W_IsMapped(messwk)) 
	 W_MapWindow(messwk);
      else
	 W_UnmapWindow(messwk);

#if 0
      /* the logic below is badly contorted */
      if (W_IsMapped(phaserwin))
	 phaserWindow = 1;

      if (!W_IsMapped(reviewWin)) {
	 if (W_IsMapped(messwa)) {
	    W_UnmapWindow(messwa);
	    W_UnmapWindow(phaserwin);
	    W_UnmapWindow(messwt);
	    W_UnmapWindow(messwi);
	    W_UnmapWindow(messwk);
	 } else {
	    W_MapWindow(reviewWin);
	 }
      } else {
	 W_UnmapWindow(reviewWin);
	 W_MapWindow(messwa);
	 W_MapWindow(messwt);
	 W_MapWindow(messwi);
	 W_MapWindow(messwk);
	 if (phaserWindow)
	    W_MapWindow(phaserwin);
      }
#endif
      if (optionWin) {
	 optionredrawtarget(reviewWin);
	 optionredrawtarget(messwa);
	 optionredrawtarget(phaserwin);
	 optionredrawtarget(messwt);
	 optionredrawtarget(messwi);
	 optionredrawtarget(messwk);
      }
      break;
   case 'c':			/* c = cloak */
      if (localflags & PFREFIT)
	 do_refit(CRUISER);
      else
	 cloak();
      break;
   case 'C':			/* C = coups */
      sendCoupReq();
      break;
   case 'l':			/* l = lock onto */
      target = gettarget(win, data->x, data->y,
			 TARG_PLAYER | TARG_PLANET);
      if (target->o_type == PLAYERTYPE) {
	 sendPlaylockReq(target->o_num);
	 me->p_playerl = target->o_num;
	 redrawPlayer[me->p_playerl] = 1;

      } else {			/* It's a planet */
	 sendPlanlockReq(target->o_num);
	 me->p_planet = target->o_num;
	 planets[me->p_planet].pl_flags |= PLREDRAW;
      }
      break;
   case '*':
      sendPractrReq();
      break;
      /* Start of display functions */
   case ' ':			/* ' ' = clear special windows */
      W_UnmapWindow(planetw);
      W_UnmapWindow(rankw);
      if (infomapped)
	 destroyInfo();
      W_UnmapWindow(helpWin);
#ifdef XTREKRC_HELP
      if (defWin)
	 W_UnmapWindow(defWin);
#endif
      W_UnmapWindow(war);
      if (optionWin)
	 optiondone();
#ifdef SCAN
      if (scanmapped) {
	 W_UnmapWindow(scanwin);
	 scanmapped = 0;
      }
#endif
      if (udpWin)
	 udpdone();
      break;
   case 'E':			/* E = send emergency call */
#ifdef FEATURE
      rcd(generic, data);
#else
      emergency();
#endif

      break;

#ifdef FEATURE
   case 'F':			/* F = send armies carried report */
      rcd(carrying, data);
      break;
#else
#if defined(EM)
   case 'F':			/* F = send armies carried report */
      army_report();
      break;
#endif				/* EM */
#endif
   case 'L':			/* L = Player list */
      if (W_IsMapped(playerw)) {
	 W_UnmapWindow(playerw);
      } else {
	 W_MapWindow(playerw);
      }
      break;
   case 'P':			/* P = Planet list */
      if (W_IsMapped(planetw)) {
	 W_UnmapWindow(planetw);
      } else {
	 W_MapWindow(planetw);
      }
      break;
   case 'U':			/* U = Rank list */
      if (W_IsMapped(rankw)) {
	 W_UnmapWindow(rankw);
      } else {
	 W_MapWindow(rankw);
      }
      break;
   case 'S':			/* S = toggle stat mode */
      if (W_IsMapped(statwin)) {
	 W_UnmapWindow(statwin);
      } else {
	 W_MapWindow(statwin);
      }
      break;
   case 'N':			/* N = Toggle Name mode */
      namemode++;
      if (namemode > 2)
	 namemode = 0;
      if (optionWin)
	 optionredrawoption(&namemode);
      break;
   case 'i':			/* i = get information */
   case 'I':			/* I = get extended information */
      if (!infomapped)
	 inform(win, data->x, data->y, key);
      else
	 destroyInfo();
      break;
   case 'h':			/* h = Map help window */
      if (W_IsMapped(helpWin)) {
	 W_UnmapWindow(helpWin);
      } else {
#ifdef EM
	 fillhelp();
#endif				/* EM */
	 W_MapWindow(helpWin);
      }
      if (optionWin)
	 optionredrawtarget(helpWin);
      break;
   case 'w':			/* w = map war stuff */
      if (W_IsMapped(war))
	 W_UnmapWindow(war);
      else
	 warwindow();
      break;
   case '+':			/* UDP: pop up UDP control window */
      if (udpWin != NULL && W_IsMapped(udpWin))
	 udpdone();
      else {
	 char            buf[80];
	 udpwindow();
	 sprintf(buf, "UDP client version %.1f",
		 (float) UDPVERSION / 10.0);
	 warning(buf);
      }
      if (optionWin)
	 optionredrawtarget(udpWin);
      break;
   case '=':			/* UDP: request for full update */
      sendUdpReq(COMM_UPDATE);
      break;
#ifdef SHORT_PACKETS
   case '-':
      sendShortReq(SPK_SALL);
      break;
#endif
#ifdef NETSTAT
   case '/':
      netstat = !netstat;
      if (netstat && !W_IsMapped(lMeter)) {
	 ns_init(1);
	 W_MapWindow(lMeter);
      } else if (!netstat && W_IsMapped(lMeter)) {
	 W_UnmapWindow(lMeter);
      }
      break;
#endif				/* NETSTAT */

   case '\\':
      alt_playerlist = !alt_playerlist;
      W_ClearWindow(playerw);
      playerlist();
      break;

   case '|':
      sortPlayers = (!sortPlayers);
      if (sortPlayers)
	 warning("Using sorted player list, Captain!");
      else
	 warning("Using regular player list, Captain!");
      W_ClearWindow(playerw);
      playerlist();
      break;
#ifdef LOGMESG
   case 'M':
      {
	 char            buf[80];
	 logMess = !logMess;
	 if (logMess) {
	    sprintf(buf, "Message logging is ON, Captain! (%s)",
		    logFileName);
	    warning(buf);
	 } else
	    warning("Message logging is OFF, Captain!");
      }
      break;
#endif
#ifdef RECORD			/* my attempt to control the recorder during
				 * the game <isae> */
   case '`':
      recordGame = !recordGame;
      if (recordGame) {
	if(!recordFile) {
	  warning("You'll have to start the game in record mode for this option, Captain!");
	  recordGame = 0;
	}
	else warning("Recorder is ON, Captain!");
      }
      else
	 warning("Recorder is OFF, Captain!");
      break;
#endif				/* nodef */
   case ':':			/* Re-read the values from rc file -- only
				 * nifty values are affected */
      warning("Re-reading nifty configurations ...");
      readDefaults();
      break;
#ifdef nodef			/* was MOO bitmaps -- need the key for more
				 * important things */
   case ';':
      myPlanetBitmap = !myPlanetBitmap;
      if (myPlanetBitmap)
	 warning("Using NEW planet bitmaps ...");
      else
	 warning("Using OLD planet bitmaps ...");
      break;
#endif				/* MOOBITMAPS */
   case ';':
      lockPlanetOrBase(win, data->x, data->y);
      break;


#ifdef PING
   case ',':
      if (W_IsMapped(pStats)) {
	 W_UnmapWindow(pStats);
      } else {
	 W_MapWindow(pStats);
	 redrawPStats();
      }
      break;
#endif

#ifdef FEATURE
   case ('#' + 96):
      rcd(other2, data);
      break;
   case ('0' + 96):
      rcd(pop, data);
      break;
   case ('1' + 96):
      rcd(save_planet, data);
      break;
   case ('2' + 96):
      rcd(base_ogg, data);
      break;
   case ('3' + 96):
      rcd(help3, data);
      break;
   case ('4' + 96):
      rcd(help4, data);
      break;
   case ('5' + 96):
      rcd(asw, data);
      break;
   case ('6' + 96):
      rcd(asbomb, data);
      break;
   case ('7' + 96):
      rcd(doing3, data);
      break;
   case ('8' + 96):
      rcd(doing4, data);
      break;
   case ('9' + 96):
      rcd(pickup, data);
      break;
   case ('@' + 96):
      rcd(other1, data);
      break;
   case ('B' + 96):
      rcd(bombing, data);
      break;
   case ('C' + 96):
      rcd(controlling, data);
      break;
   case ('O' + 96):
      rcd(ogging, data);
      break;
   case ('T' + 96):
      rcd(take, data);
      break;
   case ('b' + 96):
      rcd(bomb, data);
      break;
   case ('c' + 96):
      rcd(space_control, data);
      break;
   case ('e' + 96):
      rcd(escorting, data);
      break;
   case ('f' + 96):
      rcd(free_beer, data);
      break;
   case ('h' + 96):
      rcd(crippled, data);
      break;
   case ('l' + 96):
      rcd(controlling, data);
      break;
   case ('m' + 96):
      rcd(bombing, data);
      break;
   case ('n' + 96):
      rcd(no_gas, data);
      break;
   case ('o' + 96):
      rcd(ogg, data);
      break;
   case ('p' + 96):
      rcd(ogging, data);
      break;
   case ('t' + 96):
      rcd(take, data);
      break;

#ifdef XTREKRC_HELP
   case ('x' + 96):
      if (W_IsMapped(defWin))
	 W_UnmapWindow(defWin);
      else
	 showdef();
      break;
#endif				/* XTREKRC_HELP */

   case ('s' + 96):
      if (W_IsMapped(macroWin))
	 W_UnmapWindow(macroWin);
      else
	 showMacroWin();
      break;
#endif				/* FEATURE */

   default:
      W_Beep();
      break;
   }
}

void
buttonaction(data)
    W_Event        *data;
{
   unsigned char   course;
   struct obtype  *gettarget();
   register W_Window win = data->Window;
   register int    key = data->key;

   if (messageon)
      message_off();		/* ATM */

   /* remap via buttonmap */

   if (KeyIsButton(key)) {
      if (buttonmap[key] != '\0') {
	 data->key = buttonmap[key];
	 keyaction(data);
	 return;
      }
   }
   /* if in subwindow, translate to to map or tactical coords */
   if ((infow && win == infow)
#ifdef SCAN
       || win == scanwin
#endif
      ) {
      int             x, y;
      if (W_FindMouseInWin(&x, &y, w)) {	/* local window */
	 win = w;
	 data->x = x;
	 data->y = y;
      } else if (W_FindMouseInWin(&x, &y, mapw)) {	/* galactic window */
	 win = mapw;
	 data->x = x;
	 data->y = y;
      } else
	 return;
   }
   switch (key) {

   case W_RBUTTON:
      course = getcourse(win, data->x, data->y);
      set_course(course);
      break;

   case W_LBUTTON:
      course = getcourse(win, data->x, data->y);
      sendTorpReq(course);
      break;

   case W_MBUTTON:
      course = getcourse(win, data->x, data->y);
      sendPhaserReq(course);
      break;

      /* SHIFTED MOUSE FUNCTIONS */

   case (W_RBUTTON + W_SHIFTBUTTON):
      set_speed(me->p_ship.s_maxspeed / 2);
      localflags &= ~(PFREFIT);
      break;

   case (W_LBUTTON + W_SHIFTBUTTON):
      set_speed(99);		/* Max speed... */
      localflags &= ~(PFREFIT);
      break;

   case (W_MBUTTON + W_SHIFTBUTTON):
      detmine();
      break;

   case (W_RBUTTON + W_CTRLBUTTON):
      if (infomapped)
	 inform(win, data->x, data->y, 'i');
      else
	 destroyInfo();
      break;

   case (W_LBUTTON + W_CTRLBUTTON):
      shield_tog();
      break;

   case (W_MBUTTON + W_CTRLBUTTON):
      cloak();
      break;

   case (W_RBUTTON + W_SHIFTBUTTON + W_CTRLBUTTON):
      lockPlanetOrBase(win, data->x, data->y);
      break;

   case (W_LBUTTON + W_SHIFTBUTTON + W_CTRLBUTTON):
      {
	 struct obtype  *target;
	 if (me->p_flags & (PFTRACT | PFPRESS))
	    sendTractorReq(0, me->p_no);
	 target = gettarget(data->Window, data->x, data->y, TARG_PLAYER);
	 me->p_tractor = target->o_num;
	 sendTractorReq(1, target->o_num);
      }
      break;

   case (W_MBUTTON + W_SHIFTBUTTON + W_CTRLBUTTON):
      {
	 struct obtype  *target;
	 if (me->p_flags & (PFTRACT | PFPRESS))
	    sendRepressReq(0, me->p_no);
	 target = gettarget(data->Window, data->x, data->y, TARG_PLAYER);
	 me->p_tractor = target->o_num;
	 sendRepressReq(1, target->o_num);
      }
      break;

   default:
      break;
   }

}

/*
 * changed from unsigned char to irint() for precise rounding (from Leonard
 * Dickens)
 */
int
getcourse(ww, x, y)
    W_Window        ww;
    int             x, y;
{
   extern double   rint();
   if (ww == mapw) {
      int             me_x, me_y;

      me_x = me->p_x * WINSIDE / GWIDTH;
      me_y = me->p_y * WINSIDE / GWIDTH;
      return (int)    rint(atan2((double) (x - me_x),
		                     (double) (me_y - y)) / 3.14159 * 128.);
   } else
      return (int)    rint(atan2((double) (x - WINSIDE / 2),
				                 (double) (WINSIDE / 2 - y))
			   / 3.14159 * 128.);
}
#ifdef SCAN
void
scan(w, x, y)
    W_Window        w;
    int             x, y;
{
}
#endif				/* ATM */
