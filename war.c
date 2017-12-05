/*
 * war.c
 */
#include "copyright.h"

#include <stdio.h>
#include <math.h>
#include <signal.h>
#include "netrek.h"

static int      newhostile;

/* Set up the war window and map it */
static char    *feds = "FED - ";
static char    *roms = "ROM - ";
static char    *klis = "KLI - ";
static char    *oris = "ORI - ";
static char    *gos = "  Re-program";
static char    *exs = "  Exit - no change";
static char    *peaces = "Peace";
static char    *hostiles = "Hostile";
static char    *wars = "War";

void
warwindow()
{
   W_MapWindow(war);
   newhostile = me->p_hostile;
   warrefresh();
}

void
warrefresh()
{
   fillwin(0, feds, newhostile, me->p_swar, FED);
   fillwin(1, roms, newhostile, me->p_swar, ROM);
   fillwin(2, klis, newhostile, me->p_swar, KLI);
   fillwin(3, oris, newhostile, me->p_swar, ORI);
   W_WriteText(war, 0, 4, textColor, gos, strlen(gos), 0);
   W_WriteText(war, 0, 5, textColor, exs, strlen(exs), 0);
}

void
fillwin(menunum, string, hostile, warbits, team)
    int             menunum;
    char           *string;
    int             hostile, warbits;
    int             team;
{
   char            buf[80];

   if (team & warbits) {
      (void) sprintf(buf, "  %s%s", string, wars);
      W_WriteText(war, 0, menunum, rColor, buf, strlen(buf), 0);
   } else if (team & hostile) {
      (void) sprintf(buf, "  %s%s", string, hostiles);
      W_WriteText(war, 0, menunum, yColor, buf, strlen(buf), 0);
   } else {
      (void) sprintf(buf, "  %s%s", string, peaces);
      W_WriteText(war, 0, menunum, gColor, buf, strlen(buf), 0);
   }
}

void
waraction(data)
    W_Event        *data;
{
   int             enemyteam = 0;

   if (data->y == 4) {
      W_UnmapWindow(war);
      sendWarReq(newhostile);
      redrawall = 1;		/* jch -- redraw planet names in diff font */
      return;
   }
   if (data->y == 5) {
      W_UnmapWindow(war);
      return;
   }
   if (data->y == 0)
      enemyteam = FED;
   if (data->y == 1)
      enemyteam = ROM;
   if (data->y == 2)
      enemyteam = KLI;
   if (data->y == 3)
      enemyteam = ORI;

   if (me->p_swar & enemyteam) {
      warning("You are already at war!");
      W_Beep();
   } else {
      if (me->p_team == enemyteam) {
	 warning("It would never work ... your crew would have you in the brig in no time.");
      } else {
	 newhostile ^= enemyteam;
      }
   }
   warrefresh();
}
