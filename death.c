/*
 * death.c
 */
#include "copyright.h"

#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#ifdef hpux
#include <time.h>
#else	/* hpux */
#include <sys/time.h>
#endif				/* hpux */
#include "netrek.h"

extern jmp_buf  env;

static char    *teamstring[9] =
{"", "and the Federation",
 "and the Romulan Empire", "",
 "and the Klingon Empire", "", "", "",
 "and the Orions"};

static char             death_mesg1[80],
			death_mesg2[80],
			death_mesg3[80];

void
show_death()

{
   register	x=50, y=80;

   if(*death_mesg1)
      W_MaskText(mapw, x, y, W_Cyan, death_mesg1, 
	 strlen(death_mesg1), W_RegularFont);
   if(*death_mesg2)
      W_MaskText(mapw, x, y+20, W_Yellow, death_mesg2, 
	 strlen(death_mesg2), W_RegularFont);
   if(*death_mesg3)
      W_MaskText(mapw, x, y+40, W_Yellow, death_mesg3, 
	 strlen(death_mesg3), W_RegularFont);
}

void
death()
{
   *death_mesg1 = *death_mesg2 = *death_mesg3 = 0;

   W_ClearWindow(mapw);
   W_ClearWindow(iconWin);

   if (oldalert != PFGREEN) {
      if (extraBorder)
	 W_ChangeBorder(w, gColor);
      W_ChangeBorder(baseWin, gColor);
      W_ChangeBorder(iconWin, gColor);
      oldalert = PFGREEN;
   }

   if (W_IsMapped(statwin)) {
      W_UnmapWindow(statwin);
      showStats = 1;
   } else {
      showStats = 0;
   }

   if (infomapped)
      destroyInfo();

   W_UnmapWindow(planetw);
   W_UnmapWindow(rankw);
   W_UnmapWindow(war);

   if (optionWin)
      optiondone();

   switch (me->p_whydead) {
   case KQUIT:
      sprintf(death_mesg1, "You have self-destructed.");
      break;
   case KTORP:
      sprintf(death_mesg1, "You were killed by a photon torpedo from %s (%c%c).",
	      players[me->p_whodead].p_name,
	      teamlet[players[me->p_whodead].p_team],
	      shipnos[me->p_whodead]);
      break;
   case KPLASMA:
      sprintf(death_mesg1, "You were killed by a plasma torpedo from %s (%c%c)",
	      players[me->p_whodead].p_name,
	      teamlet[players[me->p_whodead].p_team],
	      shipnos[me->p_whodead]);
      break;
   case KPHASER:
      sprintf(death_mesg1, "You were killed by a phaser shot from %s (%c%c)",
	      players[me->p_whodead].p_name,
	      teamlet[players[me->p_whodead].p_team],
	      shipnos[me->p_whodead]);
      break;
   case KPLANET:
      sprintf(death_mesg1, "You were killed by planetary fire from %s (%c)",
	      planets[me->p_whodead].pl_name,
	      teamlet[planets[me->p_whodead].pl_owner]);
      break;
   case KSHIP:
      sprintf(death_mesg1, "You were killed by an exploding ship formerly owned by %s (%c%c)",
	      players[me->p_whodead].p_name,
	      teamlet[players[me->p_whodead].p_team],
	      shipnos[me->p_whodead]);
      break;
   case KDAEMON:
      sprintf(death_mesg1, "You were killed by a dying daemon.");
      break;
   case KWINNER:
      sprintf(death_mesg1, "Galaxy has been conquered by %s (%c%c) %s",
	      players[me->p_whodead].p_name,
	      teamlet[players[me->p_whodead].p_team],
	      shipnos[players[me->p_whodead].p_no],
	      teamstring[players[me->p_whodead].p_team]);
      break;
   case KGHOST:
      sprintf(death_mesg1, "You were killed by a confused daemon.");
      break;
   case KGENOCIDE:
      sprintf(death_mesg1, "Your team was genocided by %s (%c%c) %s.",
	      players[me->p_whodead].p_name,
	      teamlet[players[me->p_whodead].p_team],
	      shipnos[me->p_whodead],
	      teamstring[players[me->p_whodead].p_team]);
      break;
   case KPROVIDENCE:
      sprintf(death_mesg1, "You were removed from existence by divine mercy.");
      break;
   case KOVER:
      sprintf(death_mesg1, "The a game is  over!");
      break;
   case TOURNSTART:
      sprintf(death_mesg1, "The a tournament game has begun!");
      break;
   case TOURNEND:
      sprintf(death_mesg1, "The a tournament game has ended.");
      break;
   case KBINARY:
      sprintf(death_mesg1, "Your netrek executable didn't verify correctly.");
      sprintf(death_mesg2, "(might be an old copy -- check the FAQ on rec.games.netrek.)");
      break;
   default:
      sprintf(death_mesg1, 
	 "You were killed by something unknown to this game?");
      break;
   }

   /* First we check for promotions: */
   if (promoted) {
      char	*buf = death_mesg2;
      if(*death_mesg2)
	 buf = death_mesg3;
      sprintf(buf, "Congratulations!  You have been promoted to %s",
	      ranks[mystats->st_rank].name);
      promoted = 0;
   }

   death_mesg1[79] = 0;
   death_mesg2[79] = 0;
   death_mesg3[79] = 0;

   show_death();

   longjmp(env, 0);
}
