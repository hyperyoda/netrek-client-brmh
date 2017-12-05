/*
 * enter.c
 * 
 * This version modified to work as the client in a socket based protocol.
 */
#include "copyright.h"

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <pwd.h>
#include <string.h>
#include <ctype.h>
#include "netrek.h"

/* Enter the game */

void
enter()
{
   redrawTstats();
}

/*
 * Doesn't really openmem, but it will set some stuff up...
 */
void
openmem()
{
   int             i;

   players = universe.players;
   torps = universe.torps;
   plasmatorps = universe.plasmatorps;
   status = universe.status;
   planets = universe.planets;
   phasers = universe.phasers;
   mctl = universe.mctl;
   messages = universe.messages;
   for (i = 0; i < MAXPLAYER; i++) {
      players[i].p_status = PFREE;
      players[i].p_cloakphase = 0;
      players[i].p_no = i;
      players[i].p_ntorp = 0;
      players[i].p_explode = 1;
      players[i].p_stats.st_tticks = 1;
   }
   mctl->mc_current = 0;
   status->time = 1;
   status->timeprod = 1;
   status->kills = 1;
   status->losses = 1;
   status->time = 1;
   status->planets = 1;
   status->armsbomb = 1;
   for (i = 0; i < MAXPLAYER * MAXTORP; i++) {
      torps[i].t_status = TFREE;
      torps[i].t_no = i;
      torps[i].t_owner = (i / MAXTORP);
   }
   for (i = 0; i < MAXPLAYER; i++) {
      phasers[i].ph_status = PHFREE;
   }
   for (i = 0; i < MAXPLAYER * MAXPLASMA; i++) {
      plasmatorps[i].pt_status = PTFREE;
      plasmatorps[i].pt_no = i;
      plasmatorps[i].pt_owner = (i / MAXPLASMA);
   }
   for (i = 0; i < MAXPLANETS; i++) {
      planets[i].pl_no = i;
   }
   /* initialize planet redraw for moving planets */
   for (i = 0; i < MAXPLANETS; i++) {
      pl_update[i].plu_update = -1;
   }
}

#if 0
void
drawTstats()
{
   char            buf[BUFSIZ];

   sprintf(buf, "Flags        Warp Dam Shd Torps  Kills Armies   Fuel  Wtemp Etemp");
   W_WriteText(tstatw, 50, 5, textColor, buf, strlen(buf), W_RegularFont);
   sprintf(buf,
	   "Maximum:      %2d  %3d %3d               %3d   %6d   %3d   %3d",
	   me->p_ship.s_maxspeed, me->p_ship.s_maxdamage,
	   me->p_ship.s_maxshield, me->p_ship.s_maxarmies,
	   me->p_ship.s_maxfuel, me->p_ship.s_maxwpntemp / 10,
	   me->p_ship.s_maxegntemp / 10);
   W_WriteText(tstatw, 50, 27, textColor, buf, strlen(buf), W_RegularFont);
}
#endif
