/*
 * option.c
 */
#include "copyright.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "netrek.h"

#ifdef RECORD
#include "recorder.h"
#endif

static 
int             notdone;	/* not done flag */
static 
int		old_showgalactic;
static 
int		old_showplanetowner;

#ifdef ROTATERACE
static int      old_rotate, old_rotate_deg;
#endif

static int      lastUpdateSpeed = DEFAULT_UPDATES_PER_SECOND;

static char     newkeys[14];

#if (defined( DEBUG) || defined (BITMAP_DEBUG)) && defined(DYNAMIC_BITMAPS)
extern int      OwnBitmapNum;
#endif

char           *localmes[] =
{"Show owner on local planets",
 "Show resources on local planets",
 "Show nothing on local planets",
 "Show rabbit-ears on local planets",
 ""};

char           *namemes[] =
{"Don't show tactical planet names",
 "Show short tactical planet names",
 "Show full tactical planet names",
 ""};

char           *galacticmes[] =
{"Show owner on galactic map",
 "Show resources on galactic map",
 "Show nothing on galactic map",
 ""};

#ifdef ROTATERACE
char           *rotatemess[] =
{"Don't rotate galaxy",
 "Rotate galaxy 90 degrees",
 "Rotate galaxy 180 degrees",
 "Rotate galaxy 270 degrees",
 ""
};
#endif

char           *mapupdates[] =
{"Don't update galactic map",
 "Update galactic map rarely",
 "Update galactic map frequently",
 ""};

static char    *lockoptions[] =
{"Don't show lock icon",
 "Show lock icon on galactic map only",
 "Show lock icon on tactical map only",
 "Show lock icon on both map windows", ""};

static char    *clockoptions[] =
{"Don't show clock",
 "Show clock hh:mm",
 "Show clock hh:mm:ss",
 "" };

static char    *dashboardOptions[] =
{"Use a text dashboard",
 "Use [LAB] style dashboard",
 "Use COW style dashboard",
 "Use KRP style dashboard",
 "" };

static char *teamorderOptions[] =
{"Normal team order (be patient)",
 "List my team first",
 "List my team last",
 ""};

/* useful for options that are an int with a range */
struct int_range {
   int             min_value;	/* value is >= this */
   int             max_value;	/* value is <= this */
   int             increment;	/* a click raises/lowers this amount */
};


/*
 * Only one of op_option, op_targetwin, and op_string should be defined. If
 * op_string is defined, op_size should be too and op_text is used without a
 * "Don't" prefix. if op_range is defined, there should be a %d in op_text
 * for it, op_size will be non-useful, and the 'Don't ' prefix won't appear
 */
struct option {
   int             op_num;
   char           *op_text;	/* text to display when on */
   int            *op_option;	/* variable to test/modify (optional) */
   W_Window       *op_targetwin;/* target window to map/unmap (optional) */
   char           *op_string;	/* string to modify (optional) */
   int             op_size;	/* size of *op_string (optional) */
   char          **op_array;	/* array of strings to switch between */
   struct int_range *op_range;	/* struct definint an integer range option */
};

/* for the paged options menus */
struct option_menu {
   int             page_num;	/* page number of this menu */
   struct option_menu *Next;
   struct option  *menu;	/* pointers to arrary of options */
   int             numopt;	/* number of options in this menu page */
   int             updated;	/* 1 if options can be changed externally */
};

/* pointer to first entry in the options menu list */
static struct option_menu *FirstMenu = NULL;
static struct option_menu *CurrentMenu = NULL;	/* menu currently looked at */
int             MenuPage = 0;	/* current menu page */
int             MaxOptions = 0;	/* maximum number of options in all menu
				 * pages */
#if 0	/* not used */
static struct int_range MenuPages =
{0, 1, 1};
#endif


/* updates: use of the int range thing... */
static struct int_range updates_range =
{1, 10, 1};
/* range of menus. Will be updated when menu list is assembled */
static struct int_range Menus_Range =
{0, 1, 1};

#ifdef PHASER_SHRINK
static struct int_range shrink_phaser_range =
{5, 15, 1};
#endif

#if (defined( DEBUG) || defined (BITMAP_DEBUG)) && defined(DYNAMIC_BITMAPS)
static struct int_range bitmap_range =
{0, 50, 1};
#endif

static struct int_range enemyPhaserRange =
{0, 10, 1};


/* menus */
static
struct option   Window_Menu[] =
{
   {0, "Window Menu", &MenuPage, 0, 0, 0, NULL, &Menus_Range},
   {1, "Page %d (click to change)", &MenuPage, 0, 0, 0, NULL, &Menus_Range},
   {2, "show \"all\" message window", 0, &messwa, 0, 0, NULL, NULL},
   {4, "show \"team\" message window", 0, &messwt, 0, 0, NULL, NULL},
   {5, "show \"your\" message window", 0, &messwi, 0, 0, NULL, NULL},
   {6, "show \"kill\" message window", 0, &messwk, 0, 0, NULL, NULL},
   {7, "show \"total\" message window", 0, &reviewWin, 0, 0, NULL, NULL},
   {7, "show phaser log window", &phaserWindow, &phaserwin, 0, 0, NULL},
   {8, "show statistic window", 0, &statwin, 0, 0, NULL, NULL},
#ifdef NETSTAT
   {8, "show LagMeter ", &netstat, 0, 0, 0, NULL, NULL},
#endif
#ifdef PING
   {8, "show ping stats window", 0, &pStats, 0, 0, NULL},
#endif
   {9, "show UDP control window", 0, &udpWin, 0, 0, NULL, NULL},
   {9, "show help window", 0, &helpWin, 0, 0, NULL, NULL},
   {9, "show FEATURES/MACROS/RCDS", 0, &macroWin, 0, 0, NULL, NULL},
#ifdef XTREKRC_HELP
   {9, "show xtrekrc defaults window", 0, &defWin, 0, 0, NULL, NULL},
#endif
   {10, "done", &notdone, 0, 0, 0, NULL, NULL},
   {-1, NULL, 0, 0, 0, 0, NULL, NULL}
};

static
struct option   Features_Menu[] =
{
   {0, "Features Menu", &MenuPage, 0, 0, 0, NULL, &Menus_Range},
   {1, "Page %d (click to change)", &MenuPage, 0, 0, 0, NULL, &Menus_Range},
   {2, "", &namemode, 0, 0, 0, namemes, NULL},
/*
   {3, "show shields", &showShields, 0, 0, 0, NULL, NULL},
*/
   {4, "", &mapmode, 0, 0, 0, mapupdates, NULL},
/*
   {5, "stay peaceful when reborn", &keeppeace, 0, 0, 0, NULL, NULL},
*/
#ifdef TTS
   {5, "show team message on tactical (tts)", &tts, 0, 0, 0, NULL, NULL},
#endif
   {6, "new keymap entries: %s", 0, 0, newkeys, 13, NULL, NULL},
   {7, "", &showlocal, 0, 0, 0, localmes, NULL},
   {8, "", &showgalactic, 0, 0, 0, galacticmes, NULL},
   {9, "%d updates per second", &updates_per_second,0,0,0,0,&updates_range},
   {10, "report kill messages", &reportKills, 0, 0, 0, NULL, NULL},
   {12, "show tractor/pressor", &showTractorPressor, 0, 0, 0, NULL, NULL},
#ifdef SHORT_PACKETS
   {13, "receive short packets", &recv_short_opt, 0, 0, 0, NULL, NULL},
   {14, "receive threshold: %s", 0, 0, recv_threshold_s, 13, NULL, NULL},
#endif
   {11, "format kill messages (sp only)", &abbr_kmesg, 0, 0, 0, NULL, NULL},
   {15, "show alternate playerlist", &alt_playerlist, 0, 0, 0, NULL, NULL},
   {16,  "use new message flags", &new_messages, 0, 0, 0, NULL, NULL },
   {17, "", &dashboardStyle, 0, 0, 0, dashboardOptions, NULL},
   {18, "fix UDP dropped torps/phasers", &drop_fix, 0, 0, 0, NULL, NULL},
   {19, "use stippled border", &stippleBorder, 0, 0, 0, NULL, NULL},
   {20, "done", &notdone, 0, 0, 0, NULL, NULL},
   {-1, NULL, 0, 0, 0, 0, NULL, NULL}
};


static
struct option   SillyFeatures_Menu[] =
{
   {0, "Silly Features Menu 1", &MenuPage, 0, 0, 0, NULL, &Menus_Range},
   {1, "Page %d (click to change)", &MenuPage, 0, 0, 0, NULL, &Menus_Range},
   {11, "sort players in player window", &sortPlayers, 0, 0, 0, NULL, NULL},
   {11, "show players before observers", &sortPlayersObs, 0, 0, 0, NULL, NULL},
   {12, "show tractor/pressor beams", &showTractorPressor, 0, 0, 0, NULL, NULL},
   {12, "show continuous tractors/pressors ", &continueTractor, 0, 0, 0, NULL, NULL},
#ifdef MOOBITMAPS
   {12, "use 'Moo' planet bitmaps", &myPlanetBitmap, 0, 0, 0, NULL, NULL},
#endif
   {12, "show planet owner", &showPlanetOwner, 0, 0, 0, NULL, NULL},
   {12, "alert on extra border(s)", &extraBorder, 0, 0, 0, NULL, NULL},
#ifdef LOGMESG
   {12, "log messages", &logMess, 0, 0, 0, NULL, NULL},
#endif
#ifdef ROTATERACE
   {14, "", &rotate, 0, 0, 0, rotatemess, NULL},
#endif
   {12, "use message warp", &warp, 0, 0, 0, NULL, NULL},
#if (defined( DEBUG) || defined (BITMAP_DEBUG)) && defined(DYNAMIC_BITMAPS)
 {10, "Own bitmap number: %d", &OwnBitmapNum, 0, 0, 0, NULL, &bitmap_range},
#endif
   {12, "show shield damage", &VShieldBitmaps, 0, 0, 0, NULL, NULL},
   {12, "show last msg in msg win", &use_msgw, 0, 0, 0, NULL, NULL},
   {12, "send phaser msgs to indiv win", &phas_msgi, 0, 0, 0, NULL, NULL},
#ifdef BD
   {13, "borg torp test", &bd, 0, 0, 0, NULL, NULL},
#endif
   {17, "done", &notdone, 0, 0, 0, NULL, NULL},
   {-1, NULL, 0, 0, 0, 0, NULL, NULL}
};

static
struct option   SillyFeatures2_Menu[] =
{
   {0, "Silly Features Menu 2", &MenuPage, 0, 0, 0, NULL, &Menus_Range},
   {1, "Page %d (click to change)", &MenuPage, 0, 0, 0, NULL, &Menus_Range},
   {13, "", &showLock, 0, 0, 0, lockoptions, NULL},
   {13, "fill lock triangle", &fillTriangle, 0, 0, 0, NULL, NULL},
   {13, "use smaller info windows", &newInfo, 0, 0, 0, NULL, NULL},
   {13, "", &tclock, 0, 0, 0, clockoptions, NULL},
   {14, "enemy phaser width: %d", &enemyPhasers, 0, 0, 0, NULL, 
	&enemyPhaserRange},
   {13, "fast clear", &W_FastClear, 0, 0, 0, NULL, NULL},
#ifdef PHASER_SHRINK
   {13, "shrink phasers", &shrink_phasers, 0, 0, 0, NULL, NULL},
   {13, "Shrink phaser amount: %d", &shrink_phasers_amount, 0, 0, 0, NULL, 
							&shrink_phaser_range},
#endif
#ifdef RECORD
   {14, "Record Indiv msgs", &recordIndiv, 0, 0, 0, NULL, NULL},
#endif
#ifdef EM
   {15, "", &teamOrder, 0, 0, 0, teamorderOptions, NULL},
#endif
   {17, "done", &notdone, 0, 0, 0, NULL, NULL},
   {-1, NULL, 0, 0, 0, 0, NULL, NULL}
};

#if __STDC__ || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

/* option.c */
static void optionrefresh P_((register struct option *op));
static void AddOptMenu P_((struct option NewMenu[], int updated));
static int NumOptions P_((struct option OpMenu[]));

#undef P_


/* option menu sizes and such */
#define OPTIONBORDER	2
#define OPTIONLEN	35

/* Set up the option menus and window. */
void
optionwindow()
{
   /* Init not done flag */
   notdone = 1;

   *newkeys = '\0';
#ifdef SHORT_PACKETS
   *recv_threshold_s = 0;
#endif
   if (FirstMenu == NULL) {
      MaxOptions = InitOptionMenus();
      if (MaxOptions < 0) {
	 fprintf(stderr, "InitOptionMenus() error %d!\n", MaxOptions);
	 notdone = 0;
	 return;
      }
   }
   /* Create window big enough to hold option windows */
   if (optionWin == NULL) {

      optionWin = W_MakeMenu("option", WINSIDE + 10, -BORDER + 10, OPTIONLEN,
			     MaxOptions, baseWin, OPTIONBORDER);
#ifdef TCURSORS
      W_DefineArrowCursor(optionWin);
#endif
      CurrentMenu = FirstMenu;

      RefreshOptions();
   }
   W_ResizeMenu(optionWin, OPTIONLEN, CurrentMenu->numopt);
   /* Map window */
   W_MapWindow(optionWin);
}

/* refresh all current options */
void
RefreshOptions()
{
   int             i;
   struct option_menu *option;

   if (notdone == 0 || (option = CurrentMenu) == NULL)
      return;

   for (i = 0; i < option->numopt; i++) {
      optionrefresh(&(option->menu[i]));
   }
#ifdef nodef
   if (option->numopt < MaxOptions)
      for (i = option->numopt; i < MaxOptions; i++) {
	 OptionClear(i);
      }
#endif
}

#ifdef nodef
/* blank out option line 'i' */
void
OptionClear(i)
{
   char           *blanktext = "                                               ";
   if (optionWin && notdone)
      W_WriteText(optionWin, 0, i, textColor, blanktext, OPTIONLEN, 0);
}
#endif

/* Redraw the specified option entry */
void
optionredrawtarget(win)
    W_Window        win;
{
   register struct option *op;

#ifdef nodef
   if (notdone == 0)
      return;
#endif

   for (op = CurrentMenu->menu; op->op_text; op++) {
      if (op->op_targetwin && win == *op->op_targetwin) {
	 optionrefresh(op);
	 break;
      }
   }
}

/* Redraw the specified option option */
void
optionredrawoption(ip)
    int            *ip;
{
   register struct option *op;

   if (notdone == 0)
      return;

   for (op = CurrentMenu->menu; op->op_num >= 0; op++) {
      if (ip == op->op_option) {
	 optionrefresh(op);
	 break;
      }
   }
}

/* Refresh the option window given by the option struct */
static void
optionrefresh(op)
    register struct option *op;
{
   register int    on;
   char            buf[BUFSIZ];

   if (op == NULL || notdone == 0)
      return;

   if (op->op_string) {
      (void) sprintf(buf, op->op_text, op->op_string);
   } else if (op->op_array) {	/* Array of strings */
      strcpy(buf, op->op_array[*op->op_option]);
   } else if (op->op_range) {
      (void) sprintf(buf, op->op_text, *(op->op_option));
   } else {
      /* Either a boolean or a window */
      if (op->op_option)
	 on = *op->op_option;	/* use int for status */
      else if (op->op_targetwin)
	 on = W_IsMapped(*op->op_targetwin);	/* use window for status */
      else
	 on = 1;		/* shouldn't happen */

      if (!on)
	 strcpy(buf, "Don't ");
      else
	 buf[0] = '\0';
      strcat(buf, op->op_text);
   }

   if (islower(buf[0]))
      buf[0] = toupper(buf[0]);

   if(op->op_num == 0){       /* title */
      W_WriteText(optionWin, 0, op->op_num, W_Yellow, buf, strlen(buf), 0);
   }
   else if(op->op_num == 1){ /* "click" entry */
     W_WriteText(optionWin, 0, op->op_num, W_Green, buf, strlen(buf), 0);
   }
   else
      W_WriteText(optionWin, 0, op->op_num, textColor, buf, strlen(buf), 0);
}

/* deal with events sent to the option window */
int
optionaction(data)
    W_Event        *data;
{
   register struct option *op;
   int             i;
   register char  *cp;

   if (data->y >= CurrentMenu->numopt) {
      W_Beep();
      return (0);
   }
   if (notdone == 0)
      return (0);

   op = &(CurrentMenu->menu[data->y]);

   /* Update string; don't claim keystrokes for non-string options */
   /* deal with options with string input first */
   if (op->op_string == 0) {
      if (data->type == W_EV_KEY)
	 return (0);
   } else {
      if (data->type == W_EV_BUTTON)
	 return (0);
      switch (data->key) {

      case '\b':		/* delete character */
      case '\177':
	 cp = op->op_string;
	 i = strlen(cp);
	 if (i > 0) {
	    cp += i - 1;
	    *cp = '\0';
	 }
	 break;

      case '\027':		/* word erase */
	 cp = op->op_string;
	 i = strlen(cp);
	 /* back up over blanks */
	 while (--i >= 0 && isspace(cp[i]));
	 i++;
	 /* back up over non-blanks */
	 while (--i >= 0 && !isspace(cp[i]));
	 i++;
	 cp[i] = '\0';
	 break;

      case '\025':		/* kill line */
      case '\030':
	 op->op_string[0] = '\0';
	 break;

      default:			/* add character to the list */
	 if (data->key < 32 || data->key > 127)
	    break;
	 cp = op->op_string;
	 i = strlen(cp);
	 if (i < (op->op_size - 1) && !iscntrl(data->key)) {
	    cp += i;
	    cp[1] = '\0';
	    cp[0] = data->key;
	 } else
	    W_Beep();
	 break;
      }
   }

   /* Toggle int, if it exists */
   if (op->op_array) {
      if (data->key == W_RBUTTON) {
	 (*op->op_option)++;
	 if (*(op->op_array)[*op->op_option] == '\0') {
	    *op->op_option = 0;
	 }
      } else if (data->key == W_MBUTTON) {
	 /* set option number to zero on the middle key to ease shutoff */
	 *op->op_option = 0;
      } else if (data->key == W_LBUTTON) {
	 /* if left button, decrease option  */
	 (*op->op_option)--;
	 /* if decreased too far, set to top option */
	 if (*(op->op_option) < 0) {
	    *op->op_option = 0;
	    while (*(op->op_array)[*op->op_option] != '\0') {
	       (*op->op_option)++;
	    }
	    (*op->op_option)--;
	 }
      }
      
      
      /* Hacks for buttons with an array of options */
      
#ifdef ROTATERACE
      if (op->op_option == &rotate && rotate != old_rotate) {
	 register        i;
	 register struct planet 	*l;
	 register struct player	*j;

	 redrawall = 1;
	 reinitPlanets = 1;

	 for (i = 0, l = planets; i < MAXPLANETS; i++, l++) {
	    if (rotate) {
	       rotate_deg = -old_rotate_deg + rotate * 64;
	       rotate_coord(&l->pl_x, &l->pl_y, rotate_deg, 
			    GWIDTH / 2, GWIDTH / 2);
	       rotate_deg = rotate * 64;
	    } else {
	       rotate_deg = -old_rotate_deg;
	       rotate_coord(&l->pl_x, &l->pl_y, rotate_deg, 
			    GWIDTH / 2, GWIDTH / 2);
	       rotate_deg = 0;
	    }
	 }

	 /* we could wait for the server to do this but looks better if we 
	    do it now. */
	 for(i=0,j=players; i< MAXPLAYER; i++,j++){
	    if(j->p_status != PALIVE) continue;
	    if (rotate) {
	       rotate_deg = -old_rotate_deg + rotate * 64;
	       rotate_coord(&j->p_x, &j->p_y, rotate_deg, 
			    GWIDTH / 2, GWIDTH / 2);
	       rotate_dir(&j->p_dir, rotate_deg);

	       rotate_deg = rotate * 64;
	    } else {
	       rotate_deg = -old_rotate_deg;
	       rotate_coord(&j->p_x, &j->p_y, rotate_deg, 
			    GWIDTH / 2, GWIDTH / 2);
	       rotate_dir(&j->p_dir, rotate_deg);
	       rotate_deg = 0;
	    }
	 }
	 /* phasers/torps/etc .. wait for server */

	 old_rotate = rotate;
	 old_rotate_deg = rotate_deg;
      }
      else
#endif
      if(op->op_option == &tclock){
	 clear_clock();
	 if(tclock)
	    run_clock(1);
      }
      else if (op->op_option == &dashboardStyle){
	 W_ClearWindow(tstatw);
	 redrawTstats();
      }
      else if (op->op_option == &showgalactic){
	 if(!redrawall)
	    redrawall = 2;
      }   
    
      
    /* Hacks for toggle buttons */
    
    } else if (op->op_range) {
      if (data->key == W_RBUTTON) {
	 (*op->op_option) += op->op_range->increment;
      } else if (data->key == W_MBUTTON) {
	 (*op->op_option) = op->op_range->min_value;
      } else if (data->key == W_LBUTTON) {
	 (*op->op_option) -= op->op_range->increment;
      }
      /* wrap value around within option range */
      if (*(op->op_option) > op->op_range->max_value)
	 *(op->op_option) = op->op_range->min_value;
      if (*(op->op_option) < op->op_range->min_value)
	 *(op->op_option) = op->op_range->max_value;
   } else if (op->op_option) {
      *op->op_option = !*op->op_option;
#ifdef NETSTAT
      /* XXXXXX KLUDGE */
      if(op->op_option == &netstat){
	 if(netstat && !W_IsMapped(lMeter)){
	    ns_init(1);
	    W_MapWindow(lMeter);
	 }
	 else if(!netstat && W_IsMapped(lMeter)){
	    W_UnmapWindow(lMeter);
	 }
      }
      else
#endif
      if(op->op_option == &alt_playerlist){
	 W_ClearWindow(playerw);
	 playerlist();
      }
      else
#ifdef TTS
      if(op->op_option == &tts){
	 init_tts();
      }
#endif
      else if(op->op_option == &logMess){
	 char	buf[BUFSIZ];
	 if (logMess){
            sprintf(buf, "Message logging is ON, Captain! (%s)",
               logFileName);
            warning(buf);
         }
         else
            warning("Message logging is OFF, Captain!");
      }
      else if(op->op_option == &stippleBorder)
         oldalert = 0;
   }
   /* Map/unmap window, if it exists */
   if (op->op_targetwin) {
      if (W_IsMapped(*op->op_targetwin))
	 W_UnmapWindow(*op->op_targetwin);
      else {
#ifdef XTREKRC_HELP
	 if(op->op_targetwin == &defWin)
	    showdef();
	 else
#endif
	 if(op->op_targetwin == &macroWin)
	    showMacroWin();
	 else {
	    W_MapWindow(*op->op_targetwin);
	    if (*op->op_targetwin == udpWin)
	       udpwindow();
#ifdef PING
	    if (*op->op_targetwin == pStats)
	       redrawPStats();
#endif
	 }
      }
   }
   /* deal with possible menu change */
   if (MenuPage != CurrentMenu->page_num) {
      SetMenuPage(MenuPage);
      RefreshOptions();
   }
   if (!notdone)		/* if done, that is */
      optiondone();
   else
      optionrefresh(op);


   return (1);
}

/*
 * find the menu in the menus linked list that matches the one in the *
 * argument
 */
void
SetMenuPage(pagenum)
    int             pagenum;
{
   int             i = 1;
   if (FirstMenu != NULL)
      for (CurrentMenu = FirstMenu; CurrentMenu->Next != NULL &&
	   CurrentMenu->page_num != pagenum; i++, CurrentMenu = CurrentMenu->Next);
   W_ResizeMenu(optionWin, OPTIONLEN, CurrentMenu->numopt);
}

void
optiondone()
{
   char           *str;

   /* Unmap window */
   W_UnmapWindow(optionWin);

   /* update keymap */
   for (str = newkeys; *str != '\0'; str += 2) {
      if ((*str >= 32 && *str < 127) || *str == 'O') {
	 if (*(str + 1) == '\0')
	    break;
	 mykeymap[*str - 32] = *(str + 1);
      }
      if (*(str + 1) == '\0')
	 break;
   }
   *newkeys = '\0';

#ifdef SHORT_PACKETS
   {
      int	nt = atoi(recv_threshold_s);
      if(recv_threshold != nt){
	 recv_threshold = nt;
	 sendThreshold(recv_threshold);
      }
      *recv_threshold_s = 0;
   }
#endif

   /* optionrefresh(&(option[KEYMAP])); Not sure why this is really needed */

   sendOptionsPacket();		/* update server as to the client's options */

   if (updates_per_second != lastUpdateSpeed) {
      sendUpdatePacket(1000000 / updates_per_second);
      lastUpdateSpeed = updates_per_second;
   }
#ifdef SHORT_PACKETS
   if(recv_short != recv_short_opt){
      /* we don't set recv_short .. that's done in socket.c */
      if(recv_short_opt)
	 sendShortReq(SPK_VON);
      else
	 sendShortReq(SPK_VOFF);
   }
#endif
   
   if(old_showgalactic != showgalactic){
      old_showgalactic = showgalactic;
      if(!redrawall)
	 redrawall = 2;
   }
   if(old_showplanetowner != showPlanetOwner){
      old_showplanetowner = showPlanetOwner;
      if(!redrawall)
	 redrawall = 1;
   }
}

/* set up menus linked list */
int 
InitOptionMenus()
{
   int             i = 1;
   int             maxopts = 0;

   IFDEBUG(printf("Adding OptionMenus\n");)
   /* AddOptMenu( &OptionsMenu, 0); */
   AddOptMenu(Features_Menu, 0);
   AddOptMenu(Window_Menu, 0);
   AddOptMenu(SillyFeatures_Menu, 0);
   AddOptMenu(SillyFeatures2_Menu, 0);

   for (i = 0, CurrentMenu = FirstMenu; CurrentMenu != NULL;
	i++, CurrentMenu = CurrentMenu->Next) {
      CurrentMenu->page_num = i;/* repage the menus.. */
      if (CurrentMenu->numopt > maxopts)
	 maxopts = CurrentMenu->numopt;
   }
   CurrentMenu = FirstMenu;
   Menus_Range.max_value = i - 1;
   IFDEBUG(printf("OptionMenus Added! Maxopt = %d \n", i);)
   return maxopts;
}

static void
AddOptMenu(NewMenu, updated)
    struct option   NewMenu[];
    int             updated;
{
   struct option_menu *menuptr;
   struct option_menu *newmenu;
   int             i = 0;

   IFDEBUG(printf("AddOptMenu\n");)
   menuptr = FirstMenu;

   newmenu = (struct option_menu *) malloc(sizeof(struct option_menu));
   if (newmenu == NULL) {
      perror("Malloc Error adding a menu");
      return;
   }
   /* add to list */
   if (FirstMenu == NULL) {
      FirstMenu = newmenu;
   } else {
      for (i = 0, menuptr = FirstMenu; menuptr->Next != NULL; menuptr = menuptr->Next)
	 i++;
      menuptr->Next = newmenu;
   }
   newmenu->page_num = i;
   newmenu->Next = NULL;
   newmenu->numopt = NumOptions(NewMenu);
   newmenu->menu = NewMenu;
   newmenu->updated = updated;
   IFDEBUG(printf("Menu Added! \n", i);)
}

static int 
NumOptions(OpMenu)
    struct option   OpMenu[];
{
   int             i = 0;
   struct option  *ptr;

   ptr = &OpMenu[0];
   for (i = 0; ptr->op_num != -1 && ptr->op_option != &notdone; i++) {
      IFDEBUG(printf("Option #%d..\n", i);)
      IFDEBUG(if (ptr->op_text != NULL) printf("OP_Text:%s\n", ptr->op_text);)
      ptr = &OpMenu[i];
      ptr->op_num = i;
   }

   IFDEBUG(printf("NumOptions in this menu: %d\n", i);)
   return i;
}

/*
 * a function that could be called regularly, to deal with menus that * might
 * be updated by external events. I.e. the udp menu!
 */
void
UpdateOptions()
{
   if (notdone == 0)
      return;			/* don't update if menu isn't in use */
   if (CurrentMenu->updated)
      RefreshOptions();
}
