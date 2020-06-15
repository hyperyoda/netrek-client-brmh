#ifdef BD
#include <stdio.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "packets.h"

static int      torps_b[MAXPLAYER * MAXTORP];

unsigned char   get_his_best_tcrs();
unsigned char   get_acourse();

bd_new_torp(i, t)
    int             i;
    struct torp    *t;
{
   torps_b[i] = 1;
   t->t_turns = 0;
}

bd_test_torp(i, t)
    int             i;
    struct torp    *t;
{
   if (torps_b[i])
      torps_b[i] = 0;
   else
      return;

   if (t->t_owner != me->p_no && ((t->t_war & me->p_team) ||
	     (players[t->t_owner].p_team & (me->p_hostile | me->p_swar)))) {
      unsigned char   crs;
      crs = get_his_best_tcrs(&players[t->t_owner]);
      if (angdist(crs, t->t_dir) < 6) {
	 /* XX */
	 t->t_turns = 32767;
      }
   }
}

unsigned char
get_his_best_tcrs(j)
    struct player  *j;
{
   unsigned char   crs;
   get_torp_course(me, &crs, j->p_x, j->p_y);
   return crs;
}

get_torp_course(j, crs, cx, cy)
    struct player  *j;
    unsigned char  *crs;
    int             cx, cy;
{
   double          vxa, vya, vxs, vys, vxt, vyt;
   double          dp, l, dx, dy;
   register int    x, y, mx, my, speed;
   unsigned char   dir;

   speed = j->p_speed;
   dir = j->p_dir;
   x = j->p_x;
   y = j->p_y;
   mx = cx;
   my = cy;

   vxa = (x - mx);
   vya = (y - my);

   l = (double) hypot(vxa, vya);
   if (l != 0) {
      vxa /= l;
      vya /= l;
   }
   vxs = (Cos[dir] * speed) * WARP1;
   vys = (Sin[dir] * speed) * WARP1;
   dp = vxs * vxa + vys * vya;	/* dot product of (va vs) */
   dx = vxs - dp * vxa;
   dy = vys - dp * vya;
   l = (double) hypot(dx, dy);
   dp = (double) (j->p_ship.s_torpspeed * WARP1);
   l = (dp * dp - l * l);
   if (l > 0) {
      l = sqrt(l);
      vxt = l * vxa + dx;
      vyt = l * vya + dy;
      *crs = get_acourse((int) vxt + mx, (int) vyt + my, cx, cy);
      return 1;
   }
   /* out of range? */
   return 0;
}

/* get course from (mx,my) to (x,y) */
unsigned char 
get_acourse(x, y, mx, my)
    int             x, y, mx, my;
{
   if (x == mx && y == my)
      return 0;

   return (unsigned char) (atan2((double) (x - mx),
		                       (double) (my - y)) / 3.14159 * 128.);
}

notColor(k)
    struct torp    *k;
{
   switch (players[(k)->t_owner].p_team) {
   case FED:
      return shipCol[remap[KLI]];
   case ROM:
      return shipCol[remap[ORI]];
   case KLI:
      return shipCol[remap[FED]];
   case ORI:
      return shipCol[remap[ROM]];
   default:
      return FED;
   }
}

monpoprate(pl, packet)
    struct planet  *pl;
    struct planet_spacket *packet;
{
   struct planet   new;
   static int      start, new_armies, tick;
   int             s;

   if (!start)
      start = time(NULL);

   if (((tick++) % 10) == 0) {
      s = time(NULL) - start;
      if (s) {
	 printf("rate after %d minutes: %d armies per hour.\n",
		s / 60,
		(3600 * new_armies) / s);
      }
   }
   new.pl_owner = packet->owner;
   new.pl_info = packet->info;
   new.pl_flags = ntohs(packet->flags);
   new.pl_armies = ntohl(packet->armies);

   if (new.pl_owner != pl->pl_owner || (pl->pl_armies == 0))
      return;

   if (pl->pl_armies >= new.pl_armies)
      return;

   /* FAILURE: players dropping armies */

   new_armies += new.pl_armies - pl->pl_armies;

}
#endif
