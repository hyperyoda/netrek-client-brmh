/*
 * This module contains the text dashboard code as well as the code general
 * to all dashboards (run_clock and db_redraw).
 */

#include "copyright.h"

#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#ifndef sgi
#include <sys/timeb.h>
#endif
#include "netrek.h"

#ifdef RECORD
#include "recorder.h"
#endif

int basetime = 0;

void
stline(flag)
    int             flag;
{
   static char     buf1[100];
   register char  *buf = buf1;
   register unsigned int flags = me->p_flags;
   double	   kills = nkills();

   *buf++ = (flags & PFSHIELD ? 'S' : ' ');
   if (me->p_flags & PFGREEN)
      *buf++ = 'G';
   else if (flags & PFYELLOW)
      *buf++ = 'Y';
   else if (flags & PFRED)
      *buf++ = 'R';
   *buf++ = (flags & (PFPLLOCK | PFPLOCK) ? 'L' : ' ');
   *buf++ = (flags & PFREPAIR ? 'R' : ' ');
   *buf++ = (flags & PFBOMB ? 'B' : ' ');
   *buf++ = (flags & PFORBIT ? 'O' : ' ');
   if (me->p_ship.s_type == STARBASE)
      *buf++ = (flags & PFDOCKOK ? 'D' : ' ');
   else
      *buf++ = (flags & PFDOCK ? 'D' : ' ');
   *buf++ = (flags & PFCLOAK ? 'C' : ' ');
   *buf++ = (flags & PFWEP ? 'W' : ' ');
   *buf++ = (flags & PFENG ? 'E' : ' ');
   if (flags & PFPRESS)
      *buf++ = 'P';
   else if (flags & PFTRACT)
      *buf++ = 'T';
   else
      *buf++ = ' ';
   if (flags & PFBEAMUP)
      *buf++ = 'u';
   else if (flags & PFBEAMDOWN)
      *buf++ = 'd';
   else
      *buf++ = ' ';
   *buf++ = (status->tourn) ? 't' : ' ';
   *buf++ = ' ';
   *buf = '0' + ((me->p_speed % 100) / 10);
   if (*buf == '0')
      *buf++ = ' ';
   else
      buf++;
   *buf++ = '0' + (me->p_speed % 10);	/* speed */
   *buf++ = ' ';
   *buf++ = ' ';
   *buf = '0' + (me->p_damage / 100);
   if (*buf == '0')
      *buf++ = ' ';
   else
      buf++;
   *buf = '0' + ((me->p_damage % 100) / 10);
   if ((*buf == '0') && (me->p_damage < 100))
      *buf++ = ' ';
   else
      buf++;
   *buf++ = '0' + (me->p_damage % 10);
   *buf++ = ' ';
   *buf = '0' + (me->p_shield / 100);
   if (*buf == '0')
      *buf++ = ' ';
   else
      buf++;
   *buf = '0' + ((me->p_shield % 100) / 10);
   if ((*buf == '0') && (me->p_shield < 100))
      *buf++ = ' ';
   else
      buf++;
   *buf++ = '0' + (me->p_shield % 10);
   *buf++ = ' ';
   *buf++ = ' ';
   *buf = '0' + ((me->p_ntorp % 100) / 10);
   if (*buf == '0')
      *buf++ = ' ';
   else
      buf++;
   *buf++ = '0' + (me->p_ntorp % 10);
   *buf++ = ' ';
   *buf++ = ' ';
   *buf++ = ' ';

   if(kills > 99.99){
      *buf++ = '0' + ((int) (kills / 100));
      *buf++ = '0' + ((int) (((int)kills % 100) / 10));
   }
   else if (kills > 9.99){
      *buf++ = ' ';
      *buf++ = '0' + ((int) (kills / 10));
   }
   else{
      *buf++ = ' ';
      *buf++ = ' ';
   }

   *buf++ = '0' + (((int) kills) % 10);
   *buf++ = '.';
   *buf++ = '0' + (((int) (kills * 10)) % 10);
   *buf++ = '0' + (((int) (kills * 100)) % 10);
   *buf++ = ' ';
   *buf++ = ' ';
   *buf++ = ' ';
   *buf++ = ' ';
   *buf = '0' + ((me->p_armies % 100) / 10);
   if (*buf == '0')
      *buf++ = ' ';
   else
      buf++;
   *buf++ = '0' + (me->p_armies % 10);
   *buf++ = ' ';
   *buf++ = ' ';

   *buf = '0' + (me->p_fuel / 100000);
   if (*buf == '0')
      *buf++ = ' ';
   else
      buf++;
   *buf = '0' + ((me->p_fuel % 100000) / 10000);
   if ((*buf == '0') && (me->p_fuel < 100000))
      *buf++ = ' ';
   else
      buf++;
   *buf = '0' + ((me->p_fuel % 10000) / 1000);
   if ((*buf == '0') && (me->p_fuel < 10000))
      *buf++ = ' ';
   else
      buf++;
   *buf = '0' + ((me->p_fuel % 1000) / 100);
   if ((*buf == '0') && (me->p_fuel < 1000))
      *buf++ = ' ';
   else
      buf++;
   *buf = '0' + ((me->p_fuel % 100) / 10);
   if ((*buf == '0') && (me->p_fuel < 100))
      *buf++ = ' ';
   else
      buf++;
   *buf++ = '0' + (me->p_fuel % 10);
   *buf++ = ' ';
   *buf++ = ' ';
   *buf++ = ' ';

   *buf = '0' + ((me->p_wtemp / 10) / 100);
   if (*buf == '0')
      *buf++ = ' ';
   else
      buf++;
   *buf = '0' + (((me->p_wtemp / 10) % 100) / 10);
   if ((*buf == '0') && (me->p_wtemp < 1000))
      *buf++ = ' ';
   else
      buf++;
   *buf++ = '0' + ((me->p_wtemp / 10) % 10);

   *buf++ = ' ';
   *buf++ = ' ';
   *buf++ = ' ';

   *buf = '0' + ((me->p_etemp / 10) / 100);
   if (*buf == '0')
      *buf++ = ' ';
   else
      buf++;
   *buf = '0' + (((me->p_etemp / 10) % 100) / 10);
   if ((*buf == '0') && (me->p_etemp < 1000))
      *buf++ = ' ';
   else
      buf++;
   *buf++ = '0' + ((me->p_etemp / 10) % 10);

#ifdef RECORD
   if(playback && redraw) {
     *buf++ = ' ';
     *buf++ = ' ';
     *buf++ = ' ';
     *buf++ = ' ';
     strcpy(buf, "Update");
     W_WriteText(tstatw, 50, 5 + W_Textheight + 1, textColor, buf1, 74,
		 W_RegularFont);
   }
   else
#endif   

   W_WriteText(tstatw, 50, 5 + W_Textheight + 1, textColor, buf1, 64,
	       W_RegularFont);
}


/* update stat window record for max speed, army capacity */
void
updateMaxStats(redraw)
    int             redraw;
{
   char            buf[BUFSIZ];
   static int      lastdamage = -1;
   static int      lastkills = -1;
   static int      lastship = -1;
   int             maxspeed;
   int             troop_capacity;
   double	   kills = nkills();
   int             mykills = (int) (10. * kills);

   /* don't really need a update if nothing's changed! */

#ifdef RECORD_DEBUG
   /* fprintf(RECORDFD, "updateMaxStats, redraw=%d...", redraw); */
#endif

   if (!redraw && lastkills == mykills && lastship == me->p_ship.s_type &&
       lastdamage == me->p_damage)
     {
#ifdef RECORD_DEBUG
       /* fprintf(RECORDFD, "blowing function off.\n"); */
#endif
      return;
     }

   lastkills = mykills;
   lastdamage = me->p_damage;
   lastship = me->p_ship.s_type;

   if (me->p_ship.s_type == ASSAULT)
      troop_capacity = (((kills * 3) > me->p_ship.s_maxarmies) ?
			me->p_ship.s_maxarmies : (int) (kills * 3));
   else if (me->p_ship.s_type != STARBASE)
      troop_capacity = (((kills * 2) > me->p_ship.s_maxarmies) ?
			me->p_ship.s_maxarmies : (int) (kills * 2));
   else
      troop_capacity = me->p_ship.s_maxarmies;

   maxspeed = (me->p_ship.s_maxspeed + 2) -
      (me->p_ship.s_maxspeed + 1) *
      ((float) me->p_damage / (float) (me->p_ship.s_maxdamage));
   if (maxspeed > me->p_ship.s_maxspeed)
      maxspeed = me->p_ship.s_maxspeed;
   if (maxspeed < 0)
      maxspeed = 0;

#ifdef RECORD
   if(playback) {
     if(redraw) {
       strcpy(buf, "Flags        Warp Dam Shd Torps  Kills Armies   Fuel  Wtemp Etemp  ");
       playback_status(&buf[67]);
     }
     else strcpy(buf, "Flags        Warp Dam Shd Torps  Kills Armies   Fuel  Wtemp Etemp  ");
   }
   else
#endif
   if (tclock)
      strcpy(buf, "Flags        Warp Dam Shd Torps  Kills Armies   Fuel  Wtemp Etemp  Time");
   else
      strcpy(buf, "Flags        Warp Dam Shd Torps  Kills Armies   Fuel  Wtemp Etemp");

   W_WriteText(tstatw, 50, 5, textColor, buf, strlen(buf), W_RegularFont);
#ifdef NO_SPRINTF
#define SPACE	"                                                    "
   {
      extern char    *itoa();
      register char  *s = buf;
      strncpy(s, "Maximum:   ", 11);
      s += 11;
      s = itoa(s, maxspeed, 2, 1);
      *s++ = '/';
      s = itoa(s, me->p_ship.s_maxspeed, 2, 1);
      *s++ = ' ';
      *s++ = ' ';
      s = itoa(s, me->p_ship.s_maxdamage, 3, 1);
      *s++ = ' ';
      s = itoa(s, me->p_ship.s_maxshield, 3, 1);
      strncpy(s, SPACE, 14);
      s += 14;
      s = itoa(s, troop_capacity, 2, 1);
      *s++ = '/';
      s = itoa(s, me->p_ship.s_maxarmies, 2, 1);
      *s++ = ' ';
      *s++ = ' ';
      *s++ = ' ';
      s = itoa(s, me->p_ship.s_maxfuel, 6, 1);
      *s++ = ' ';
      *s++ = ' ';
      *s++ = ' ';
      s = itoa(s, me->p_ship.s_maxwpntemp / 10, 3, 1);
      *s++ = ' ';
      *s++ = ' ';
      *s++ = ' ';
      s = itoa(s, me->p_ship.s_maxegntemp / 10, 3, 1);
      *s++ = '\0';
   }
#else
   sprintf(buf,
	"Maximum:   %2d/%2d  %3d %3d              %2d/%2d  %6d   %3d   %3d",
	   maxspeed, me->p_ship.s_maxspeed,
	   me->p_ship.s_maxdamage, me->p_ship.s_maxshield,
	   troop_capacity, me->p_ship.s_maxarmies,
	   me->p_ship.s_maxfuel, me->p_ship.s_maxwpntemp / 10,
	   me->p_ship.s_maxegntemp / 10);
#endif
   W_WriteText(tstatw, 50, 5 + 2 * (W_Textheight + 1), textColor, buf,
	       strlen(buf), W_RegularFont);

#ifdef RECORD_DEBUG
   /* fprintf(RECORDFD, "function executed\n"); */
#endif


}



void
run_clock(update)
    int             update;
{
   char            timebuf[10];
   static int      lasttime;
   long            curtime;
   struct tm      *tm;

#ifdef RECORD
   if(playback) {
     playback_clock();
     return;
   }
#endif

   time(&curtime);
   if (tclock == 1) {
      if (curtime / 60 == lasttime / 60 && !update)
	 return;
   } else {			/* tclock == 2 */
      if (curtime == lasttime && !update)
	 return;
   }
   if (basetime)
   {
       int ct1, ct2;
       curtime = curtime - basetime;
       ct1 = curtime / 60;
       ct2 = curtime - ct1 * 60;
       sprintf(timebuf, "%5d:%02d", ct1, ct2);
   }
   else
   {
       lasttime = curtime;
       tm = localtime(&curtime);
       sprintf(timebuf, "%2d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
   }

   W_WriteText(tstatw, 50 + (66 * W_Textwidth), 27, W_Yellow, timebuf,
	       (tclock == 2) ? 8 : 5,
	       W_BoldFont);
}

void
clear_clock()
{
   W_WriteText(tstatw, 50 + (66 * W_Textwidth), 27, W_Yellow,
	       "       ", 8, W_BoldFont);
}


void
db_redraw(fr)
    int             fr;
{

#ifdef RECORD_DEBUG
  /* fprintf(RECORDFD, "db_redraw(%d)\n", fr); */
#endif

   switch (dashboardStyle) {
   case 0:
      stline(fr);
      updateMaxStats(fr);	/* Update the max stats <isae> */
      break;

   case 1:
      db_redraw_brmh(fr);
      break;

   case 2:
      db_redraw_COW(fr);
      break;

   case 3:
      db_redraw_krp(fr);
      break;

   default:
      break;
   }
}
