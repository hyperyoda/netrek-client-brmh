/*
 * findslot.c
 * 
 * Kevin Smith 03/23/88
 * 
 */
#include "copyright2.h"

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <pwd.h>
#include <string.h>
#include <ctype.h>
#ifdef hpux
#include <time.h>
#else                           /* hpux */
#include <sys/time.h>
#endif
#include "netrek.h"

#ifdef EM
#define WAITMOTD
#endif

#define WAITWIDTH 180
#define WAITHEIGHT 60
#define WAITTITLE 15		/* height of title for wait window */
#define WAITICONHEIGHT 50
#define WAITICONWIDTH  50

extern int	newMotdStuff;	/* from newwwin.c */

#if __STDC__ || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

/* findslot.c */
static void destroyWait P_((void));
static void mapWaitWin P_((void));
static void mapWaitQuit P_((void));
static void mapWaitCount P_((unsigned int count));
static void mapMotdButtonWin P_((void));
static void mapWaitIcon P_((unsigned int count));

#undef P_

/* special case for in game but waitQ window still up (non-obscure mode).
   jkelley's suggestion: change window to reflect game entry instead of
   obscuring whatever you're working on (another game, for example) */

static int		   in = 0;
static int		   toggle;

static W_Window		   qwin, countWin, motdButtonWin, waitIcon;

static void
destroyWait()

{
   if(motdWin)
      W_UnmapWindow(motdWin);

   /* xx: tvtwm may crash if destroy followed by map
      too quickly */
   W_UnmapWindow(waitWin);
   W_UnmapWindow(waitIcon);

   W_Flush();
   W_DestroyWindow(waitWin);
   waitWin = NULL;
   W_DestroyWindow(waitIcon);
}

int
findslot()
{
   int             oldcount = -1;
   int		   seconds = 99, lasttime=0, now;
   W_Event         event;
   struct timeval  tv;
   fd_set          mask;
   char		   wait_title[32];

   /* Wait for some kind of indication about in/not in */
   while (queuePos == -1) {
      socketPause();
      if (isServerDead()) {
	 printf("Augh!  Ghostbusted!\n");
	 exit(0);
      }
      readFromServer(NULL);
      if (me != NULL) {
	 /* We are in! */
	 return (me->p_no);
      }
   }
#ifdef FORKNETREK
   if(waitnum)
      sprintf(wait_title, "wait%d", waitnum);
   else
#endif
      strcpy(wait_title, "wait");

   /* We have to wait.  Make appropriate windows, etc... */
   waitWin = W_MakeWindow(wait_title, 0, 0, WAITWIDTH, WAITHEIGHT, NULL, 2, 
      foreColor);
   W_TopWindowTitle(waitWin);

   countWin = W_MakeWindow("count", WAITWIDTH / 3, WAITTITLE, WAITWIDTH / 3,
			   WAITHEIGHT - WAITTITLE - 1, waitWin, 1, foreColor);
   qwin = W_MakeWindow("waitquit", 0, WAITTITLE, WAITWIDTH / 3, 
			   WAITHEIGHT - WAITTITLE - 1, waitWin, 1, foreColor);
   motdButtonWin = W_MakeWindow("motd_select", 2 * WAITWIDTH / 3 -1, WAITTITLE,
      WAITWIDTH, WAITHEIGHT - WAITTITLE - 1, waitWin, 1, foreColor);
   waitIcon = W_MakeWindow("wait_icon", 0, 0, WAITICONWIDTH, WAITICONHEIGHT,
			   NULL, BORDER, foreColor);
   W_SetIconWindow(waitWin, waitIcon);

   W_MapWindow(waitWin);
   W_MapWindow(countWin);
   W_MapWindow(qwin);
   W_MapWindow(motdButtonWin);
   W_Flush();

   for (;;) {
      /* improve the response of the waitQ window -- don't wait
         1 second before doing anything */
      while (!W_EventsPending()) {
	 tv.tv_sec = 1;
	 tv.tv_usec = 0;
	 FD_ZERO(&mask);
	 FD_SET(W_Socket(), &mask);
	 FD_SET(sock, &mask);
	 select(32, &mask, 0, 0, &tv);

	 if(in){
	    /* we're actually in the game but showing modified window
	       instead of login window */
	    if(lasttime != (now=time(NULL))){
	       lasttime = now;
	       seconds--;
	       if(seconds == 0){
		  me->p_status = PFREE;
		  printf("Auto-Quit\n");
		  exit(0);
	       }

	       if(toggle){
		  W_ChangeBackground(countWin, W_White);
		  /*
		  W_ChangeBorder(countWin, W_Black);
		  */
	       }
	       else {
		  W_ChangeBackground(countWin, W_Black);
		  /*
		  W_ChangeBorder(countWin, W_White);
		  */
	       }
	       toggle = !toggle;

	       mapWaitCount(seconds);
	       mapWaitIcon(seconds);
	       mapWaitWin();
	    }
	 }

	 if(FD_ISSET(sock, &mask)){
	    readFromServer(NULL);
	    if (isServerDead()) {
	       printf("We've been ghostbusted!\n");
	       exit(0);
	    }
	    if(motdWin && newMotdStuff)
	       showMotd(motdWin, motdw_line);
	    if (!in && queuePos != oldcount) {
	       mapWaitCount(queuePos);
	       mapWaitIcon(queuePos);
	       oldcount = queuePos;
	    }
	    if (me != NULL) {
	       
	       W_Beep();
	       W_Beep();
	       W_Beep();

	       if(non_obscure){
		  in = 1;
		  mapWaitWin();
		  mapWaitQuit();
		  mapWaitCount(seconds);
		  mapMotdButtonWin();
	       }
	       else {
		  destroyWait();
		  return (me->p_no);
	       }
	    }
	 }
      }
      if(!W_EventsPending())
	 continue;

      W_NextEvent(&event);
      switch ((int) event.type) {

      case W_EV_KEY:
	 if (motdWin && event.Window == motdWin) {
	    motdWinEvent(event.key);
	 } else if (event.Window == motdButtonWin) {
	    showMotdWin();
	 }
	 break;

      case W_EV_BUTTON:
	 if (event.Window == qwin) {
	    printf("OK, bye!\n");
	    exit(0);
	 } else if (event.Window == motdButtonWin) {
	    showMotdWin();
	 } else if (event.Window == waitIcon) {
	    mapWaitIcon(queuePos);
	 } else if (in && event.Window == countWin) {
	    destroyWait();
	    return (me->p_no);
	 }
	 break;

      case W_EV_EXPOSE:
	 if (event.Window == waitWin) {
	    mapWaitWin();
	 } else if (event.Window == qwin) {
	    mapWaitQuit();
	 } else if (event.Window == countWin) {
	    mapWaitCount(in ? seconds : queuePos);
	 } else if (motdWin && event.Window == motdWin) {
	    showMotd(motdWin, motdw_line);
	 } else if (event.Window == motdButtonWin) {
	    mapMotdButtonWin();
	 } else if (event.Window == waitIcon) {
	    mapWaitIcon(queuePos);
	 }
	 break;
      default:
	 break;
      }
   }
}

static void
mapWaitWin()
{
   char           *s;
   W_Color	color;

   if(in){
      s = "Netrek:   GAME  READY!";
      color = W_White;
   }
   else{
      s = "Netrek:  Game is full.";
      color = W_White;
   }

   W_WriteText(waitWin, 15, 5, color, s, strlen(s), W_RegularFont);
}

static void
mapWaitQuit()
{
   char           *s = "Quit";
   W_Color	  color;

   if(in)
      color = textColor;
   else
      color = textColor;

   W_WriteText(qwin, 15, 15, color, s, strlen(s), W_RegularFont);
}

static void
mapWaitCount(count)
    unsigned int    count;
{
   char           *s;
   char           *t;
   char            buf[10];
   register int    len;
   W_Color	   color;

   if(in){
      s = "ENTER";
      t = "GAME";
      color = toggle ? W_White : W_Black;
   }
   else {
      s = "Wait";
      t = "Queue";
      color = textColor;
   }

   W_WriteText(countWin, 15, 5, color, s, strlen(s), W_RegularFont);
   W_WriteText(countWin, 20, 15, color, t, strlen(t), W_RegularFont);
   sprintf(buf, "%d  ", count);
   len = /* strlen(buf) */ 3;
   if (count == -1)
      strcpy(buf, "?");
   W_WriteText(countWin, WAITWIDTH / 6 - len * W_Textwidth / 2, 25,
	       color, buf, len, W_RegularFont);
}

static void
mapMotdButtonWin()
{
   char         *s = "MOTD";
   W_Color	color;

   if(in){
      color = textColor;
   }
   else
      color = textColor;

   W_WriteText(motdButtonWin, 15, 15, color, s, strlen(s), W_RegularFont);
}

static void
mapWaitIcon(count)
    unsigned int    count;
{
   char            buf[5];
   int             len;

   sprintf(buf, "%d", count);
   len = strlen(buf);
   W_WriteText(waitIcon, WAITICONWIDTH / 2 - 10, W_Textheight, textColor,
	       buf, len,
	       W_BigFont);
}

