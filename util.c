/*
 * util.c
 */
#include "copyright.h"

#include <stdio.h>
#include <math.h>
#include <signal.h>
#include "netrek.h"

/*
 * * Provide the angular distance between two angles.
 */
int
angdist(x, y)
    unsigned char   x, y;
{
   register unsigned char res;

   if (x > y)
      res = x - y;
   else
      res = y - x;
   if (res > 128)
      return (256 - (int) res);
   return ((int) res);
}

/*
 * * Find the object nearest mouse.  Returns a pointer to an * obtype
 * structure.  This is used for info and locking on. *
 * 
 * Because we are never interested in it, this function will * never return
 * your own ship as the target. *
 * 
 * Finally, this only works on the two main windows
 */

static struct obtype _target;

struct obtype  *
gettarget(ww, x, y, targtype)
   W_Window        ww;
   int             x, y;
   int             targtype;
{
   int g_x, g_y;
   struct obtype *gettarget2();
   struct obtype *targ;

   if (ww == mapw) {
       g_x = x * GWIDTH / WINSIDE;
       g_y = y * GWIDTH / WINSIDE;
   }
   else {
       g_x = me->p_x + ((x - WINSIDE/2) * SCALE);
       g_y = me->p_y + ((y - WINSIDE/2) * SCALE);
   }

   targ = gettarget2(g_x,g_y,targtype);

   return(targ);
}

struct obtype *
gettarget2(x, y, targtype)
int x, y;
int targtype;
{
    register int i;
    register struct player *j;
    register struct planet *k;
    register dist, closedist;	/* was double, int is big enough */
    int friendly;

    closedist = GWIDTH;
    if (targtype & TARG_PLANET) {
        for (i = 0, k = &planets[i]; i < MAXPLANETS; i++, k++) {
            dist = ihypot(x - k->pl_x, y - k->pl_y);
            if (dist < closedist) {
                _target.o_type = PLANETTYPE;
                _target.o_num = i;
                closedist = dist;
            }

        }
    }

    if (targtype & (TARG_PLAYER | TARG_FRIEND | TARG_ENEMY)) {
        for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
            if (j->p_status != PALIVE)
                continue;
            if ((j->p_flags & PFCLOAK) && (!(targtype & TARG_CLOAK)))
                continue;
            if (j == me && !(targtype & TARG_SELF))
                continue;
            friendly = friendlyPlayer(j);
            if (friendly && (targtype & TARG_ENEMY))
                continue;
            if (!friendly && (targtype & TARG_FRIEND))
                continue;

            dist = ihypot(x - j->p_x, y - j->p_y);
            if (dist < closedist) {
                _target.o_type = PLAYERTYPE;
                _target.o_num = i;
                closedist = dist;
            }
        }
    }

    if (closedist == GWIDTH) {          /* Didn't get one.  bad news */
        _target.o_type = PLAYERTYPE;
        _target.o_num = me->p_no;       /* Return myself.  Oh well... */
        return(&_target);
    }
    else {

        return(&_target);
    }
}

void
lockPlanetOrBase(ww, x, y) /* special version of gettarget, 6/1/93 LAB */
W_Window ww;
int x, y;
{
   register int i;
   register struct player *j;
   register struct planet *k;
   int g_x, g_y;
   double dist, closedist;
   register int targtyp=PLANETTYPE, targnum=0;

   if(ww == mapw) {
      g_x = x * GWIDTH / WINSIDE;
      g_y = y * GWIDTH / WINSIDE;
   } else {
      g_x = me->p_x + ((x - WINSIDE/2) * SCALE);
      g_y = me->p_y + ((y - WINSIDE/2) * SCALE);
   }
   closedist = GWIDTH;
   for (i = 0, k = &planets[i]; i < MAXPLANETS; i++, k++) {
       dist = hypot((double) (g_x - k->pl_x), (double) (g_y - k->pl_y));
       if (dist < closedist) {
           targtyp = PLANETTYPE;
           targnum = i;
           closedist = dist;
       }
   }

   for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
       if (j->p_status != PALIVE) continue;
       if (j->p_flags & PFCLOAK) continue;
       if (j == me) continue;
       if ((j->p_ship.s_type == STARBASE) && (j->p_team == me->p_team)) {
           dist = hypot((double) (g_x - j->p_x), (double) (g_y - j->p_y));
           if (dist < closedist) {
               targtyp = PLAYERTYPE;
               targnum = i;
               closedist = dist;
           }
       }

   }

   if (targtyp == PLAYERTYPE) {
       sendPlaylockReq(targnum);
       me->p_playerl = targnum;
   } else {
       sendPlanlockReq(targnum);
       me->p_planet = targnum;
   }
}

/*
 * gives approximate distance from (x1,y1) to (x2,y2)
 * with only overestimations, and then never by more
 * than (9/8) + one bit uncertainty.
 */

int 
ihypot(x1, y1)
   int x1, y1;
{
   register     x2 = 0,
                y2 = 0;

   if ((x2 -= x1) < 0) x2 = -x2;
   if ((y2 -= y1) < 0) y2 = -y2;
   return (x2 + y2 - (((x2>y2) ? y2 : x2) >> 1) );
}

int
troop_capacity()
{
   if (me->p_ship.s_type == ASSAULT)
      return (((me->p_kills * 3) > me->p_ship.s_maxarmies) ?
	      me->p_ship.s_maxarmies : (int) (me->p_kills * 3));
   else if (me->p_ship.s_type != STARBASE)
      return (((me->p_kills * 2) > me->p_ship.s_maxarmies) ?
	      me->p_ship.s_maxarmies : (int) (me->p_kills * 2));
   else
      return me->p_ship.s_maxarmies;
}

double
nkills()
{
   /* observ only set if PFOBSERV & PFPLOCK */
   return observ ? players[me->p_playerl].p_kills : me->p_kills;
}
