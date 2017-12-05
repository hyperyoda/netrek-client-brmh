/*
 * entrywin.c
 * 
 * The original newwin.c split into two sections this one handling all the motd
 * and entry window stuff
 * 
 */
#include "copyright.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <sys/types.h>
#ifdef hpux
#include <time.h>
#else				/* hpux */
#include <sys/time.h>
#endif				/* hpux */
#include "netrek.h"

#include "bitmaps/clockbitmap.h"

#ifdef RECORD
#include "recorder.h"
#endif

struct list {
   char            bold;
   struct list    *next;
   char           *data;
};

static struct list *motddata = NULL;    /* pointer to first bit of motddata */
static struct list *valuedata = NULL;    /* pointer to server status info */
static int      first = 1;

#if __STDC__ || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

static int lineOnPage P_((int l, int line));
static void showPics P_((W_Window win, int l));
static void showValues P_((void));

#undef P_

#define NRHEADERS       4
static struct piclist 	*motdPics = NULL;
int			newMotdStuff = 0; /* set to 1 when new motd packets arrive */
int             	MaxMotdLine = 0;


#ifdef EM
/*
 * if a motd line from the server is this, the client will junk all motd *
 * data it currently has.  New data may be received
 */
#define MOTDCLEARLINE  "\033\030CLEAR_MOTD"
#endif

#define SIZEOF(a)	(sizeof (a) / sizeof (*(a)))

#define BOXSIDE		(WINSIDE / 5)
#define TILESIDE	16
#define MESSAGESIZE	20
#define STATSIZE	(MESSAGESIZE * 2 + BORDER)
#define YOFF		0

#define stipple_width 16
#define stipple_height 16
static char     stipple_bits[] =
{
   0x01, 0x01, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08,
   0x10, 0x10, 0x20, 0x20, 0x40, 0x40, 0x80, 0x80,
   0x01, 0x01, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08,
   0x10, 0x10, 0x20, 0x20, 0x40, 0x40, 0x80, 0x80};

/* This routine throws up an entry window for the player. */

void
entrywindow(team, s_type)
    int            *team, *s_type;
{
   int             typeok = 0, i = 0;
   W_Event         event;
   int             lastplayercount[4];
   int             okayMask, lastOkayMask;
   int             resetting = 0;
   int             tiled = 0;
   long            lasttime = -1, now;
   int             spareTime = 240;	/* Allow them an extra 240 seconds,
					 * as long as they are active */
   long		   startTime, elapsed = 0;
   struct timeval  tv;
   fd_set          mask;
#ifdef AUTOLOGIN
   static int	   first = 1;
#endif
   int		   draw_motd = 1;

   /* The following allows quick choosing of teams */

   if (fastQuit) {
      if (me->p_flags & PFSELFDEST) {
	 *team = -1;
	 return;
      } else
	 fastQuit = 0;
   }
   lastOkayMask = okayMask = tournMask;

   for (i = 0; i < 4; i++) {
      if (okayMask & (1 << i)) {
	 tiled = 0;
      } else {
	 tiled = 1;
      }

      if (tiled) {
	 W_TileWindow(teamWin[i], stipple);
      } else {
	 W_UnTileWindow(teamWin[i]);
      }
      W_MapWindow(teamWin[i]);
      lastplayercount[i] = -1;	/* force redraw first time through */
   }
   W_MapWindow(quitwin);

   *team = -1;
   startTime = mstime();

/*
   if (me->p_whydead != KWINNER && me->p_whydead != KGENOCIDE)
*/
#ifdef AUTOLOGIN
   if(first && autologin){
      first = 0;	/* expose will draw motd */
      draw_motd = 0;
   }
#endif

   if (W_IsMapped(playerw))
      playerlist();

   do {
      while (!W_EventsPending()) {

#ifdef PING
	 /*
	  * changes here so that client reads from server whenever available
	  * instead of waiting 1 second each time.  This has no affect on
	  * anything since we time(0) autoquit anyway
	  */
#endif
	 tv.tv_sec = 1;
	 tv.tv_usec = 0;

	 FD_ZERO(&mask);
	 FD_SET(W_Socket(), &mask);
	 FD_SET(sock, &mask);
	 if (udpSock >= 0)
	    FD_SET(udpSock, &mask);
	 select(32, &mask, 0, 0, &tv);

	 if (FD_ISSET(sock, &mask) ||
	     (udpSock >= 0 && FD_ISSET(udpSock, &mask))) {
	    readFromServer(&mask);
	 }
	 now = mstime();
	 elapsed = now - startTime;
	 if (elapsed > AUTOQUIT * 1000) {
	    printf("Auto-Quit.\n");
	    *team = 4;
	    break;
	 }

	 if(newMotdStuff || draw_motd){
	    draw_motd = 0;
	    showMotd(w, motd_line);
	    showValues();
	 }

	 if (lasttime != now/1000){

	    /* this info is updated only once per second anyway */
	    if (W_IsMapped(playerw)) {
#ifdef EM
	      if (sortPlayers) {
		if(!teamOrder)                 /* DBP */
		  Sorted_playerlist2();  
		else Sorted_playerlist3();     /* DBP */
	      }
	       else
#endif
		  playerlist2();
	    }

	    showTimeLeft(elapsed/1000, AUTOQUIT);
	    lasttime = now/1000;
	 }

	 okayMask = tournMask;

	 for (i = 0; i < 4; i++) {
	    if ((okayMask ^ lastOkayMask) & (1 << i)) {
	       if (okayMask & (1 << i)) {
		  W_UnTileWindow(teamWin[i]);
	       } else {
		  W_TileWindow(teamWin[i], stipple);
	       }
	       lastplayercount[i] = -1;	/* force update */
	    }
	    redrawTeam(teamWin[i], i, &lastplayercount[i]);
	 }
	 lastOkayMask = okayMask;
      }

      if (*team == 4)
	 break;
      
      /* an event happened -- reset timer, process event */

      now = mstime();

      if((now - startTime)/1000 <= spareTime){
	 spareTime -= (now - startTime)/1000;
	 startTime = now;
      } else {
	 startTime += spareTime * 1000;
	 spareTime = 0;
      }

#if 0
      if (!W_EventsPending())
	 continue;
#endif

      W_NextEvent(&event);
      typeok = 1;

      switch ((int) event.type) {
      case W_EV_KEY:
	 switch (event.key) {
	 case 's':
	    *s_type = SCOUT;
	    break;
	 case 'd':
	    *s_type = DESTROYER;
	    break;
	 case 'c':
	    *s_type = CRUISER;
	    break;
	 case 'b':
	    *s_type = BATTLESHIP;
	    break;
#ifdef GALAXY
	 case 'g':
	    *s_type = SGALAXY;
	    break;
#endif				/* GALAXY */
	 case 'X':
	    *s_type = ATT;
	    break;
	 case 'a':
	    *s_type = ASSAULT;
	    break;
	 case 'o':
	    *s_type = STARBASE;
	    break;
	 default:
	    typeok = 0;
	    break;
	 }
	 if(motdWin && event.Window == motdWin)
	    motdWinEvent(event.key);
	 else if (event.Window == w) {
	    switch (event.key) {
	    case 'y':
	       if (resetting) {
		  sendResetStatsReq('Y');
		  warning("OK, your stats have been reset.");
		  resetting = 0;
	       }
	       break;
	    case 'n':
	       if (resetting) {
		  warning("I didn't think so.");
		  resetting = 0;
	       }
	       break;
	    case 'R':
	       warning("Are you sure you want to reset your stats?");
	       resetting = 1;
	       break;
	    case 'f':		/* Scroll motd forward */
               motd_line = motd_line + LINESPERPAGE;
               if (motd_line > MaxMotdLine) {
                  motd_line = motd_line - LINESPERPAGE;
                  break;
               }
               showMotd(w, motd_line);
	       break;
	    case 'b':		/* Scroll motd backward */
               if (motd_line == 0)
                  break;
               motd_line = motd_line - LINESPERPAGE;
               if (motd_line < 0)
                  motd_line = 0;
               showMotd(w, motd_line);
               break;
	    case 'j':
	       if(motd_line + LINESPERPAGE == MaxMotdLine)
		  break;
	       motd_line++;
               showMotd(w, motd_line);
	       break;
	    case 'k':
	       if(!motd_line)
		  break;
	       motd_line--;
               showMotd(w, motd_line);
	       break;
	    case 'M':
	       showMotdWin();
	       break;

#ifdef EM
	    case 'S':
	       SaveMotd();
	       break;
#endif
	    }
	 }
	 /* No break, we just fall through */
      case W_EV_BUTTON:
	 if (typeok == 0)
	    break;
	 for (i = 0; i < 4; i++)
	    if (event.Window == teamWin[i]) {
	       *team = i;
	       break;
	    }
	 if (event.Window == quitwin /* new */ &&
	     event.type == W_EV_BUTTON) {
	    *team = 4;
	    break;
	 }
	 if (*team != -1 && !teamRequest(*team, *s_type)) {
	    *team = -1;
	 }

         if (udpWin && event.Window == udpWin){
            udpaction(&event);   /* UDP */
         }

	 break;
      case W_EV_EXPOSE:
	 for (i = 0; i < 4; i++)
	    if (event.Window == teamWin[i]) {
	       lastplayercount[i] = -1;	/* force update */
	       redrawTeam(teamWin[i], i, &lastplayercount[i]);
	       break;
	    }
	 if (event.Window == quitwin){
	    redrawQuit();
	    showTimeLeft(elapsed/1000, AUTOQUIT);
	 }
	 else if (event.Window == tstatw)
	    redrawTstats();
	 else if (event.Window == iconWin)
	    drawIcon();
	 else if (event.Window == w){
	    showMotd(w, motd_line);
	 }
	 else if(event.Window == mapw){
	    show_death();	/* keep death message */
	    showValues();
	 }
	 else if (event.Window == helpWin)
	    fillhelp();
#ifdef XTREKRC_HELP
	 else if (defWin && (event.Window == defWin))
	    showdef();
#endif
	 else if (event.Window == playerw)
	    playerlist();
	 else if (event.Window == warnw)
	    W_ClearWindow(warnw);
	 else if (event.Window == messagew)
	    smess_refresh();
	 else if (motdWin && event.Window == motdWin)
	    showMotd(motdWin, motdw_line);
	 break;
      }
   } while (*team < 0);
   if (event.Window != quitwin)
      warning("Welcome aboard Captain!");

   torprepeat = 0;
   
   if (*team == 4) {
      *team = -1;
      return;
   }
   for (i = 0; i < 4; i++)
      W_UnmapWindow(teamWin[i]);
   W_UnmapWindow(quitwin);
}

/* Attempt to pick specified team & ship */
int
teamRequest(team, ship)
    int             team, ship;
{
   int             lastTime;

   pickOk = -1;
   sendTeamReq(team, ship);
   lastTime = time(NULL);
   while (pickOk == -1) {
      if (lastTime + 3 < time(NULL)) {
	 sendTeamReq(team, ship);
      }
      socketPause();
      readFromServer(NULL);
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
	 pickOk = 0;
	 break;
      }
   }

#ifdef RECORD
   teamReq = team;
#endif

   return (pickOk);
}

int
numShips(owner)
    int             owner;
{
   int             i, num = 0;
   struct player  *p;

   for (i = 0, p = players; i < MAXPLAYER; i++, p++)
      if (p->p_status == PALIVE && p->p_team == owner)
	 num++;
   return (num);
}

#ifdef unused
realNumShips(owner)
{
   int             i, num = 0;
   struct player  *p;

   for (i = 0, p = players; i < MAXPLAYER; i++, p++)
      if (p->p_status != PFREE &&
	  p->p_team == owner)
	 num++;
   return (num);
}

deadTeam(owner)
    int             owner;
    /* The team is dead if it has no planets and cannot coup it's home planet */
{
   int             i, num = 0;
   struct planet  *p;

   if (planets[remap[owner] * 10 - 10].pl_couptime == 0)
      return (0);
   for (i = 0, p = planets; i < MAXPLANETS; i++, p++) {
      if (p->pl_owner & owner) {
	 num++;
      }
   }
   if (num != 0)
      return (0);
   return (1);
}
#endif

static char    *AUTHOR[] =
{
   "",
   "---  XtrekII Release Version 6.1 ---",
   "By Chris Guthrie, Ed James,",
   "Scott Silvey, and Kevin Smith",
   "Amdahl UDP Revision",	/* UDP */
};

int
checkBold(line)
    /* Determine if that line should be highlighted on sign-on screen */
    /* Which is done when it is the players own score being displayed */
    char           *line;
{
   char           *s, *t;
   int             i;
   int             end = 0;

   if (strlen(line) < 60)
      return (0);
   if (me == NULL)
      return (0);

   s = line + 4;
   t = me->p_name;


   for (i = 0; i < 16; i++) {
      if (!end) {
         if (*t == '\0')
            end = 1;
         else if (*t != *s)
            return (0);
      }
      if (end) {
         if (*s != ' ')
            return (0);
      }
      s++;
      t++;
   }
   return (1);
}

void
showMotd(motdwin, atline)

    W_Window        motdwin;
    int		    atline;
{
   FILE           	*fopen();
   int             	i, length, top, center;
   register struct list *data;
   int             	count;

   if(!motdwin) return;
   newMotdStuff = 0;
   W_ClearWindow(motdwin);
   for (i = 0; i < SIZEOF(AUTHOR); i++) {
      length = strlen(AUTHOR[i]);
      center = WINSIDE / 2 - (length * W_Textwidth) / 2;
      W_WriteText(motdwin, center, i * W_Textheight, textColor, AUTHOR[i],
                  length, W_RegularFont);
   }
   top = LINESSTART;

   if (first) {
      first = 0;
      data = motddata;
      while (data != NULL) {
         data->bold = checkBold(data->data);
         data = data->next;
      }
   }
   data = motddata;
   for (i = 0; i < atline; i++) {
      if (data == NULL) {
         atline = 0;
         data = motddata;
         break;
      }
      data = data->next;
   }
   count = LINESPERPAGE;
   for (i = top; i < 50; i++) {
      if (data == NULL)
         break;
      if (!strcmp(data->data, STATUS_TOKEN)) /* ATM */
         break;
      if (data->bold) {
         W_WriteText(motdwin, 20, i * W_Textheight, textColor, data->data,
                     strlen(data->data), W_BoldFont);
      } else {
         W_WriteText(motdwin, 20, i * W_Textheight, textColor, data->data,
                     strlen(data->data), W_RegularFont);
      }
      data = data->next;
      count--;
      if (count <= 0)
         break;
   }
   showPics(motdwin, atline);
}

static int
lineOnPage(l, line)

   int	l, line;
{
   return l >= line && l <= line + LINESPERPAGE;
}

static void
showPics(win, l)
    W_Window        	win;
    int			l;
{
   struct piclist *temp;
   register	   x,y;

   temp = motdPics;

   while (temp != NULL) {
      if (lineOnPage(temp->line, l)){
	    
	 y = ((temp->line+1 - l) + LINESSTART) * W_Textheight
	     + temp->y + W_Textheight/2;
	 x = temp->x;

	 if (temp->thepic){
	    W_WriteWinBitmap(win, x, y, temp->thepic, 
	       temp->color);
	 }
	 else {
	    W_MakeLine(win, x, y,
	     x + temp->width - 1, y + temp->height - 1, W_Grey);
	    W_MakeLine(win, x, y + temp->height - 1,
		       x + temp->width - 1, y, W_Grey);
	    W_MakeLine(win, x, y,
		       x + temp->width - 1, y, W_Grey);
	    W_MakeLine(win, x, y,
		       x, y + temp->height - 1, W_Grey);
	    W_MakeLine(win, x, y + temp->height - 1,
	     x + temp->width - 1, y + temp->height - 1, W_Grey);
	    W_MakeLine(win, x + temp->width - 1, y + temp->height - 1,
		       x + temp->width - 1, y, W_Grey);
	 }
      }
      temp = temp->next;
   }
}


/*
 * ATM: show the current values of the .sysdef parameters.
 */

static void
showValues()

{
   int             i;
   static char    *msg = "OPTIONS SET WHEN YOU STARTED WERE:";
   register struct list *data = valuedata;

   if(!data) return;

   data = data->next;	/* skip the status token */

   W_WriteText(mapw, 20, 14 * W_Textheight, textColor, msg,
               strlen(msg), W_RegularFont);
   for (i = 16; i < 50; i += 2) {
      if (data == NULL)
         break;
      if (data->data[0] == '+') /* quick boldface hack */
         W_WriteText(mapw, 20, i * W_Textheight, textColor, data->data + 1,
                     strlen(data->data) - 1, W_BoldFont);
      else
         W_WriteText(mapw, 20, i * W_Textheight, textColor, data->data,
                     strlen(data->data), W_RegularFont);
      data = data->next;
   }
}

static struct piclist **motd_buftail = &motdPics, *currpic;

void
ClearMotd()
{
   struct piclist *temppic;
   struct list    *temp, *temp2;

   while (motdPics) {
      temppic = motdPics;
      motdPics = temppic->next;
      if (temppic->thepic)
	 W_FreeBitmap(temppic->thepic);
      free(temppic);
   }
   motd_buftail = &motdPics;
   currpic = NULL;

   temp = motddata;             /* start of motd information */
   while (temp != NULL) {
      temp2 = temp;
      temp = temp->next;
      free(temp2->data);
      free(temp2);
   }

   first = 1;                   /* so that it'll check bold next time around */
}

void
newMotdPic(index, x, y, l, color)
   
   int	index, x,y,l, color;
{
   struct piclist *temp, *i;

   if (lineOnPage(l, motd_line)){
      newMotdStuff = 1;		/* set flag for event loop */
   }

   temp = (*motd_buftail) = 
      (struct piclist *) malloc(sizeof(struct piclist));
   temp->next = NULL;
   temp->index = index;
   temp->x = x;
   temp->y = y;
   temp->line = l;
   temp->color = color;

   temp->width = 100;
   temp->height = 100;
   temp->thepic = NULL;

   for(i=motdPics; i != temp; i=i->next){
      /* bitmap sharing */
      if(i->index == index){
	 temp->width = i->width;
	 temp->height = i->height;
	 temp->thepic = i->thepic;
      }
   }

   currpic = temp;
   motd_buftail = &(temp->next);
}

void
newMotdPicBitmap(size, width, height, bits)

   int	size;
   int	width, height;
   char	*bits;
{
   currpic->width = width;
   currpic->height = height;
   currpic->thepic = bits ? 
      W_StoreBitmap(width, height, bits, motdWin) : 0;
}

void
newMotdLine(line)
    char           *line;
{
   static struct list **temp = &motddata;
   static int      statmode = 0;/* ATM */

   newMotdStuff = 1;

   if (!statmode && !strcmp(line, STATUS_TOKEN))
      statmode = 1;
   if (!statmode)
      MaxMotdLine++;            /* ATM - don't show on left */
   (*temp) = (struct list *) malloc(sizeof(struct list));
   if ((*temp) == NULL) {       /* malloc error checking -- 10/30/92 EM */
      printf("Warning:  Couldn't malloc space for a new motd line!");
      return;
   }
#ifdef EM
   /* Motd clearing code */
   if (strcmp(line, MOTDCLEARLINE) == 0) {
      ClearMotd();
      return;
   }
#endif

   (*temp)->next = NULL;
   (*temp)->data = malloc(strlen(line) + 1);
   strcpy((*temp)->data, line);

   if(!valuedata && (strcmp((*temp)->data, STATUS_TOKEN) == 0))
      valuedata = *temp;

   temp = &((*temp)->next);
}

/* ARGSUSED */
void
getResources(prog)
    char           *prog;
{
   getTiles();
}

void
getTiles()
{
   if (!stipple)
      stipple = W_StoreBitmap(stipple_width, stipple_height, stipple_bits, w);
}

void
redrawTeam(win, teamNo, lastnum)
    W_Window        win;
    int             teamNo;
    int            *lastnum;
{
   char            buf[BUFSIZ];
   static char    *teams[] =
   {"Federation", "Romulan", "Klingon", "Orion"};
   int             num = numShips(1 << teamNo);

   /* Only redraw if number of players has changed */
   if (*lastnum == num)
      return;

   W_ClearWindow(win);
   W_WriteText(win, 5, 5, shipCol[teamNo + 1], teams[teamNo],
	       strlen(teams[teamNo]), W_RegularFont);
   (void) sprintf(buf, "%d", num);
   W_MaskText(win, 35, 46, shipCol[teamNo + 1], buf, strlen(buf),
	      W_BigFont);
   *lastnum = num;
}

void
redrawQuit()
{
   W_WriteText(quitwin, 5, 5, textColor, "Quit xtrek", 10, W_RegularFont);
}

void
drawIcon()
{
   W_WriteBitmap(0, 0, icon, W_White);
}

#define CLOCK_WID	(BOXSIDE * 9 / 10)
#define CLOCK_HEI	(BOXSIDE * 2 / 3)
#define CLOCK_BDR	0
#define CLOCK_X		(BOXSIDE / 2 - CLOCK_WID / 2)
#define CLOCK_Y		(BOXSIDE / 2 - CLOCK_HEI / 2)

#define PI		3.141592654

void
showTimeLeft(time, max)
    int             time, max;
{
   char            buf[BUFSIZ], *cp;
   int             cx, cy, ex, ey, tx, ty;

   if ((max - time) < 10 && time & 1) {
      W_Beep();
   }
   /* XFIX */
   W_ClearArea(quitwin, CLOCK_X, CLOCK_Y, CLOCK_WID, CLOCK_HEI);

   cx = CLOCK_X + CLOCK_WID / 2;
   cy = CLOCK_Y + (CLOCK_HEI - W_Textheight) / 2;
   ex = cx - clock_width / 2;
   ey = cy - clock_height / 2;
   W_WriteBitmap(ex, ey, clockpic, foreColor);

   ex = cx - clock_width * sin(2 * PI * time / max) / 2;
   ey = cy - clock_height * cos(2 * PI * time / max) / 2;
   W_MakeLine(quitwin, cx, cy, ex, ey, foreColor);

   sprintf(buf, "%d", max - time);
   tx = cx - W_Textwidth * strlen(buf) / 2;
   ty = cy - W_Textheight / 2;
   W_WriteText(quitwin, tx, ty, textColor, buf, strlen(buf), W_RegularFont);

   cp = "Auto Quit";
   tx = CLOCK_X + cx - W_Textwidth * strlen(cp) / 2;
   ty = CLOCK_Y + CLOCK_HEI - W_Textheight;
   W_WriteText(quitwin, tx, ty, textColor, cp, strlen(cp), W_RegularFont);
}

void
do_refit(type)
    int             type;
{
   sendRefitReq(type);
   localflags &= ~PFREFIT;
}

#ifdef EM
#define DEFAULT_SAVEFILE  "netrek_motd"
/* default file to save motd into */

/* save all the motd data into a save file! */
void
SaveMotd()
{
   FILE           *fp;
   struct list    *mtd_ptr = NULL;
   char           *nameptr;
   time_t          time_clock;
   char		   buf[BUFSIZ],
		   buf2[BUFSIZ];

   if ((nameptr = getdefault("motd.savefile")) == NULL){
      sprintf(buf, "%s.%s", DEFAULT_SAVEFILE, serverName);
      nameptr = buf;
   }
#ifdef SHOW_DEFAULTS
   show_defaults("string", "motd.savefile", DEFAULT_SAVEFILE,
	      "Name of file to save motd in if 'S' typed in entry window.");
#endif

   fp = fopen(nameptr, "a");

   if (fp == NULL) {
      perror(nameptr);
      return;
   }
   time_clock = time(NULL);

   if (fprintf(fp, "Motd data captured at %s\n", ctime(&time_clock)) < 0) {
      perror(nameptr);
      fclose(fp);
      return;
   }
   for (mtd_ptr = motddata; mtd_ptr != NULL; mtd_ptr = mtd_ptr->next) {
      if (fprintf(fp, "%s\n", mtd_ptr->data) < 0) {
         fprintf(stderr, "Error writing motd!\n");
         perror("fprintf error");
         fclose(fp);
      }
   }
   sprintf(buf2, "Netrek motd saved in %s\n", nameptr);
   warning(buf2);
   fclose(fp);
}
#endif
