/*
 * getname.c
 * 
 * Kevin P. Smith 09/28/88
 * 
 */
#include "copyright2.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#ifdef hpux
#include <time.h>
#else
#include <sys/time.h>
#endif
#include <errno.h>
#include <pwd.h>
#include <string.h>
#include <ctype.h>
#include "netrek.h"

static char     tempname[16];
static char     password1[16];
static char     password2[16];
static int      state;
#define ST_GETNAME 0
#define ST_GETPASS 1
#define ST_MAKEPASS1 2
#define ST_MAKEPASS2 3
#define ST_DONE 4

#ifdef AUTOLOGIN		/* COW-lite */
void
noautologin ()
{
  static char *alf = "Automatic login failed";

  autologin = 0;
  *defpasswd = *password1 = *password2 = '\0';
  W_WriteText (w, 100, 130, textColor, alf, strlen (alf),
               W_BoldFont);

}
#endif /* AUTOLOGIN */

void
getname(defname)
    char           *defname;
    /* Let person identify themselves from w */
{
   int             secondsLeft = 99;
   W_Event         event;
   unsigned char   ch;
   char            tempstr[40];
   long            lasttime;
   struct timeval  tv;
   fd_set	   mask;

   MZERO(mystats, sizeof(struct stats));
   mystats->st_tticks = 1;
   mystats->st_flags = ST_MAPMODE + ST_NAMEMODE + ST_SHOWSHIELDS +
      ST_KEEPPEACE + ST_SHOWLOCAL * 2 + ST_SHOWGLOBAL * 2;
   lasttime = time(NULL);
   tempname[0] = '\0';
   password1[0] = '\0';
   password2[0] = '\0';
   state = ST_GETNAME;

#ifdef AUTOLOGIN
   if (autologin)
        try_autologin(defname);
   if (state == ST_DONE)
         return;
#endif /*AUTOLOGIN*/

   displayStartup(defname);
   for (;;) {
      /* TSH: added select here so this doesn't busy wait (only done once
	 but it messes up performance metrics) */

      if (state == ST_DONE) {
	 W_ClearWindow(w);
	 return;
      }

      while(!W_EventsPending()){

	 tv.tv_sec = 1;
	 tv.tv_usec = 0;
	 FD_ZERO(&mask);
	 FD_SET(W_Socket(), &mask);
	 FD_SET(sock, &mask);
	 /* udp sock will not be active here */
	 select(32, &mask, 0, 0, &tv);

	 if (lasttime != time(NULL)) {
	    lasttime++;
	    secondsLeft--;
	    sprintf(tempstr, "Seconds to go: %d ", secondsLeft);
	    W_WriteText(w, 150, 400, textColor, tempstr, strlen(tempstr),
			W_BoldFont);
	    if (secondsLeft == 0) {
	       me->p_status = PFREE;
	       printf("Auto-Quit\n");
	       exit(0);
	    }
	 }

	 if(FD_ISSET(sock, &mask)){
	    readFromServer(&mask);

	    if (isServerDead()) {
	       printf("Ack!  We've been ghostbusted!\n");
	       exit(0);
	    }
	 }

	 if (!FD_ISSET(W_Socket(), &mask))
	    continue;
      }

      W_NextEvent(&event);
      if (event.Window != w)
	 continue;
      switch ((int) event.type) {
      case W_EV_EXPOSE:
	 displayStartup(defname);
	 sprintf(tempstr, "Seconds to go: %d ", secondsLeft);
	 W_WriteText(w, 150, 400, textColor, tempstr, strlen(tempstr),
		     W_BoldFont);
	 break;
      case W_EV_KEY:
	 ch = event.key;
	 if (ch == 10)
	    ch = 13;

	 switch(ch){
	    case 4:
	    case 'd' +  96:
	    case 'D' +  96:
	       if(state == ST_GETNAME && *tempname == 0)
		  exit(0);
	       break;
	    default:
	       break;
	 }
	 if (ch < 32 && ch != 21 && ch != 13 && ch != 8)
	    break;
	 switch (state) {
	 case ST_GETNAME:
	    if (ch == 13) {
	       if (*tempname == '\0') {
		  strcpy(tempname, defname);
	       }
	       loaddude();
	       displayStartup(defname);
	    } else {
	       adjustString(ch, tempname, defname);
	    }
	    break;
	 case ST_GETPASS:
	    if (ch == 13) {
	       checkpassword();
	       displayStartup(defname);
	    } else {
	       adjustString(ch, password1, defname);
	    }
	    break;
	 case ST_MAKEPASS1:
	    if (ch == 13) {
	       state = ST_MAKEPASS2;
	       displayStartup(defname);
	    } else {
	       adjustString(ch, password1, defname);
	    }
	    break;
	 case ST_MAKEPASS2:
	    if (ch == 13) {
	       makeNewGuy();
	       displayStartup(defname);
	    } else {
	       adjustString(ch, password2, defname);
	    }
	    break;
	 }
      }
   }
}

#ifdef AUTOLOGIN
void
try_autologin(defname)
char *defname;
{
   strcpy(tempname, defname);
   loaddude();
   if (state != ST_DONE) {
       strcpy(password1, defpasswd);
       checkpassword();
   }
   if (state == ST_GETNAME)
        noautologin();
}
#endif

void
loaddude()
    /*
     * Query dude.
     */
{
   if (strcmp(tempname, "Guest") == 0 || strcmp(tempname, "guest") == 0) {
      loginAccept = -1;
      sendLoginReq(tempname, "", (char *) login, 0);
      state = ST_DONE;
      me->p_stats.st_tticks = 1;/* prevent overflow */
      strcpy(me->p_name, tempname);
      while (loginAccept == -1) {
	 socketPause();
	 readFromServer(NULL);
	 if (isServerDead()) {
	    printf("Server is dead!\n");
	    exit(0);
	 }
      }
      if (loginAccept == 0) {
	 printf("Hmmm... The SOB server won't let me log in as guest!\n");
	 exit(0);
      }
      return;
   }
   /* Ask about the user */
   loginAccept = -1;
   sendLoginReq(tempname, "", login, 1);
   while (loginAccept == -1) {
      socketPause();
      readFromServer(NULL);
      if (isServerDead()) {
	 printf("Server is dead!\n");
	 exit(0);
      }
   }
   *password1 = *password2 = 0;
   if (loginAccept == 0) {
      state = ST_MAKEPASS1;
   } else {
      state = ST_GETPASS;
   }
}

void
checkpassword()
    /*
     * Check dude's password. If he is ok, move to state ST_DONE.
     */
{
   char           *s;

   sendLoginReq(tempname, password1, login, 0);
   loginAccept = -1;
   while (loginAccept == -1) {
      socketPause();
      readFromServer(NULL);
      if (isServerDead()) {
	 printf("Server is dead!\n");
	 exit(0);
      }
   }
   if (loginAccept == 0) {
      s = "Bad password!";
      W_WriteText(w, 100, 100, textColor, s, strlen(s), W_BoldFont);
      (void) W_EventsPending();
      sleep(1);
      W_ClearWindow(w);
      *tempname = 0;
      state = ST_GETNAME;
      return;
   }
   strcpy(me->p_name, tempname);

   state = ST_DONE;
}

void
makeNewGuy()
    /*
     * Make the dude with name tempname and password password1. Move to state
     * ST_DONE.
     */
{
   char           *s;

   if (strcmp(password1, password2) != 0) {
      s = "Passwords do not match";
      W_WriteText(w, 100, 120, textColor, s, strlen(s), W_BoldFont);
      (void) W_EventsPending();
      sleep(3);
      W_ClearWindow(w);
      *tempname = 0;
      state = ST_GETNAME;
      return;
   }
   /* same routine! */
   checkpassword();
}

void
adjustString(ch, str, defname)
    unsigned char   ch;
    char            *str;
    char           *defname;
{
   switch((unsigned char) ch){
      case 21:
      case 'u' + 96:
      case 'U' + 96:
	 *str = '\0';
	 if (state == ST_GETNAME)
	    displayStartup(defname);
	 break;
      
      case 8:
      case '\177':
      case 'h' + 96:
      case 'H' + 96:
	 if (strlen(str) > 0) {
	    str[strlen(str) - 1] = '\0';
	    if (state == ST_GETNAME)
	       displayStartup(defname);
	 }
	 break;
      
      default:
	 if (strlen(str) == 15)
	    return;
	 str[strlen(str) + 1] = '\0';
	 str[strlen(str)] = ch;
	 if (state == ST_GETNAME)
	    displayStartup(defname);
	 break;
   }
}

void
displayStartup(defname)
    char           *defname;
    /* Draws entry screen based upon state. */
{
   char            s[100];
   char           *t;

   if (state == ST_DONE)
      return;
   t = "Enter your name.  Use the name 'guest' to remain anonymous.";
   W_WriteText(w, 100, 30, textColor, t, strlen(t), W_BoldFont);
   t = "Type ^D (Ctrl - D) to quit.";
   W_WriteText(w, 100, 40, textColor, t, strlen(t), W_BoldFont);
   sprintf(s, "Your name (default = %s): %s               ", defname, tempname);
   W_WriteText(w, 100, 50, textColor, s, strlen(s), W_BoldFont);
   if (state == ST_GETPASS) {
      t = "Enter password: ";
      W_WriteText(w, 100, 60, textColor, t, strlen(t), W_BoldFont);
   }
   if (state > ST_GETPASS) {
      t = "You need to make a password.";
      W_WriteText(w, 100, 70, textColor, t, strlen(t), W_BoldFont);
      t = "So think of a password you can remember, and enter it.";
      W_WriteText(w, 100, 80, textColor, t, strlen(t), W_BoldFont);
      t = "What is your password? :";
      W_WriteText(w, 100, 90, textColor, t, strlen(t), W_BoldFont);
   }
   if (state == ST_MAKEPASS2) {
      t = "Enter it again to make sure you typed it right.";
      W_WriteText(w, 100, 100, textColor, t, strlen(t), W_BoldFont);
      t = "Your password? :";
      W_WriteText(w, 100, 110, textColor, t, strlen(t), W_BoldFont);
   }
}
