/*
 * interface.c
 * 
 * This file will include all the interfaces between the input routines and the
 * daemon.  They should be useful for writing robots and the like
 */
#include "copyright.h"

#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include <sys/time.h>
#ifndef sgi
#include <sys/timeb.h>
#endif
#include <signal.h>
#include "netrek.h"

void
set_speed(speed)
    int             speed;
{
   sendSpeedReq(speed);
}

void
set_course(dir)
    unsigned char   dir;
{
   sendDirReq(dir);
}

void
shield_up()
{
   if (!(me->p_flags & PFSHIELD)) {
      sendShieldReq(1);
   }
}

void
shield_down()
{
   if (me->p_flags & PFSHIELD) {
      sendShieldReq(0);
   }
}

void
shield_tog()
{
   if (me->p_flags & PFSHIELD) {
      sendShieldReq(0);
   } else {
      sendShieldReq(1);
   }
}

void
bomb_planet()
{
   if (!(me->p_flags & PFBOMB)) {
      sendBombReq(1);
   }
}

void
beam_up()
{
   if (!(me->p_flags & PFBEAMUP)) {
      sendBeamReq(1);		/* 1 means up... */
   }
}

void
beam_down()
{
   if (!(me->p_flags & PFBEAMDOWN)) {
      sendBeamReq(2);		/* 2 means down... */
   }
}

void
repair()
{
   if (!(me->p_flags & PFREPAIR)) {
      sendRepairReq(1);
   }
}

void
repair_off()
{
   if (me->p_flags & PFREPAIR) {
      sendRepairReq(0);
   }
}

void
repeat_message()
{
   if (++lastm == MAXMESSAGE)
      lastm = 0;
}

void
cloak()
{
   if (me->p_flags & PFCLOAK) {
      sendCloakReq(0);
   } else {
      sendCloakReq(1);
   }
}

void
cloak_on()
{
   if (!(me->p_flags & PFCLOAK)) {
      sendCloakReq(1);
   }
}

void
cloak_off()
{
   if (me->p_flags & PFCLOAK) {
      sendCloakReq(0);
   }
}

int
mstime()
{
   static
   struct timeval  tv_base;
   struct timeval  tv;

   if (!tv_base.tv_sec) {
      gettimeofday(&tv_base, NULL);
      return 0;
   }
   gettimeofday(&tv, NULL);
   return (tv.tv_sec - tv_base.tv_sec) * 1000 +
      (tv.tv_usec - tv_base.tv_usec) / 1000;
}

int
msetime()
{
   struct timeval  tv;
   gettimeofday(&tv, NULL);
   return (tv.tv_sec - 732737182) * 1000 + tv.tv_usec / 1000;
}

#ifdef nodefGALAXY_BORG_FEATURES/* borg alert! */
/* robot features */

unsigned char 
our_calc(x, y)
    int             x, y;
{
   return (atan2((double) (x), (double) (y)) / 3.14159 * 128.);
}

void
plasma_phaser(pt)
    register struct plasmatorp *pt;
{
   int             myphrange;
   int             x, y;

   x = (pt->pt_x - me->p_x);
   y = (me->p_y - pt->pt_y);
   myphrange = PHASEDIST * me->p_ship.s_phaserdamage / 100;
   if (pt->pt_status != PTMOVE)
      return;
   if (!(pt->pt_war & me->p_team) && !(me->p_hostile & pt->pt_team))
      return;
   if (abs(pt->pt_x - me->p_x) > myphrange)
      return;
   if (abs(pt->pt_y - me->p_y) > myphrange)
      return;
   if (hypot((float) pt->pt_x - me->p_x, (float) pt->pt_y - me->p_y) >
       myphrange)
      return;
   if (!(me->p_flags & PFCLOAK) && !(me->p_flags & PFREPAIR)) {
      warning(" --- Firing auto-plasma phaser --- ");
      sendPhaserReq(our_calc(x, y));
   }
}

void
get_enemy(data)
    W_Event        *data;
{
   int             myphrange;
   register int    i;
   register struct player *j;
   int             g_x, g_y;
   double          dist, closedist;
   unsigned char   course;

   closedist = GWIDTH;
   myphrange = PHASEDIST * me->p_ship.s_phaserdamage / 100;
   if (data->Window == mapw) {
      g_x = data->x * (GWIDTH / WINSIDE);
      g_y = data->y * (GWIDTH / WINSIDE);
   } else {
      g_x = me->p_x + ((data->x - WINSIDE / 2) * SCALE);
      g_y = me->p_y + ((data->y - WINSIDE / 2) * SCALE);
   }

   for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
      if ((j->p_status != PALIVE) || (j == me))
	 continue;
      if (!(j->p_swar & me->p_team) && !(me->p_hostile & j->p_team))
	 continue;
      if (abs(j->p_x - me->p_x) > myphrange)
	 continue;
      if (abs(j->p_y - me->p_y) > myphrange)
	 continue;
      dist = hypot((double) (g_x - j->p_x), (double) (g_y - j->p_y));
      if (dist > myphrange)
	 continue;
      if (dist < closedist) {
	 data->x = j->p_x;
	 data->y = j->p_y;
	 closedist = dist;
      }
   }

   if (closedist == GWIDTH)
      course = getcourse(data->Window, data->x, data->y);
   else
      course = our_calc((data->x - me->p_x), (me->p_y - data->y));

   sendPhaserReq(course);
}
#endif
