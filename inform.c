/*
 * inform.c
 */
#include "copyright.h"

#include <stdio.h>
#include <math.h>
#include <signal.h>
#include "netrek.h"

/* Display information about the nearest object to mouse */

/*
 * * When the player asks for info, this routine finds the object * nearest
 * the mouse, either player or planet, and pop up a window * with the desired
 * information in it. *
 * 
 * We intentionally provide less information than is actually * available.
 * Keeps the fog of war up. *
 * 
 * There is a different sized window for each type player/planet * and we take
 * care to keep it from extending beyond the main * window boundaries.
 */

static W_Window	infow_local, infow_map;

void
inform(ww, x, y, key)
    W_Window        ww;
    int             x, y;
    char            key;
{
   char            buf[BUFSIZ];
   int             line = 0;
   register struct player *j;
   register struct planet *k;
   int             mx, my;
   struct obtype  *gettarget(), *target;
   int             windowWidth, windowHeight;
   int		   lines;

   /* make these once, reuse */
   if(!infow_local)
      infow_local = W_MakeWindow("info", 0, 0, 1, 1, w, 2, foreColor);
   if(!infow_map)
      infow_map = W_MakeWindow("info", 0, 0, 1, 1, mapw, 2, foreColor);
   
   if(ww == mapw)
      infow = infow_map;
   else
      infow = infow_local;

   W_ClearWindow(infow);

   mx = x;
   my = y;
   infomapped = 1;
   if (key == 'i') {
      target = gettarget(ww, x, y, TARG_PLAYER | TARG_PLANET);
   } else {
      target = gettarget(ww, x, y, TARG_PLAYER | TARG_SELF);
   }

   /*
    * This is pretty lame.  We make a graphics window for the info window so
    * we can accurately space the thing to barely fit into the galactic map
    * or whatever.
    */

   windowWidth = W_WindowWidth(ww);
   windowHeight = W_WindowHeight(ww);
   if (target->o_type == PLAYERTYPE) {
      if(key == 'i'){
	 lines = newInfo ? 5 : 8;
	 /* Too close to the edge? */
	 if (mx + 23 * W_Textwidth + 2 > windowWidth)
	    mx = windowWidth - 23 * W_Textwidth - 2;
	    if (my + lines * W_Textheight + 2 > windowHeight)
	       my = windowHeight - lines * W_Textheight - 2;

	 j = &players[target->o_num];

	 W_ResizeWindow(infow, 23*W_Textwidth, lines*W_Textheight);
	 W_ReposWindow(infow, mx, my);
	 W_MapWindow(infow);
	 (void) sprintf(buf, "%s (%c%c):", 
	    j->p_name, teamlet[j->p_team], shipnos[j->p_no]);
	 W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), buf, strlen(buf), shipFont(j));

	 if(!newInfo){
	    (void) sprintf(buf, "Login   %s", j->p_login);
	    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), buf, strlen(buf), shipFont(j));
	    (void) sprintf(buf, "Display %s", j->p_monitor);
	    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), buf, strlen(buf), shipFont(j));
	 }
	 else {
	    int	ly;
	    sprintf(buf, "%s @ %s", j->p_login, j->p_monitor);
	    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), buf, strlen(buf), shipFont(j));

	    ly = W_Textheight * line + W_Textheight/2;
	    W_MakeLine(infow, 0, ly,
			      23*W_Textwidth, ly, foreColor);
	    W_MakeLine(infow, 10*W_Textwidth, ly,
			      10*W_Textwidth, W_Textheight*lines, foreColor);
	    line++;
	 }

	 if(!newInfo){
	    (void) sprintf(buf, "Speed   %-d", j->p_speed);
	    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), buf, strlen(buf),
		     W_RegularFont);

	    (void) sprintf(buf, "Kills   %-4.2f", j->p_kills);
	    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), buf, strlen(buf),
		     W_RegularFont);
	 }
	 else {
	    (void) sprintf(buf, "Speed %-d", j->p_speed);
	    W_WriteText(infow, W_Textwidth, W_Textheight * line, playerColor(j), buf, strlen(buf),
		     W_RegularFont);

	    (void) sprintf(buf, "Kills %-4.2f", j->p_kills);
	    W_WriteText(infow, 11*W_Textwidth, W_Textheight * line++, playerColor(j), buf, strlen(buf),
		     W_RegularFont);
	 }

	 if(!newInfo){
	    (void) sprintf(buf, "Dist    %d", 
	       ihypot(me->p_x-j->p_x, me->p_y-j->p_y));
	    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), buf, strlen(buf),
		     W_RegularFont);
	 (void) sprintf(buf, "S-Class %s", classes[j->p_ship.s_type]);
	 W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), buf, strlen(buf),
		     W_RegularFont);

	 if (j->p_swar & me->p_team)
	    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), "WAR", 3,
			W_RegularFont);
	 else if (j->p_hostile & me->p_team)
	    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), "HOSTILE", 7,
			W_RegularFont);
	 else
	    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), "PEACEFUL", 8,
			W_RegularFont);
	 }
	 else {
	    char *wars;
	    (void) sprintf(buf, "Ship  %s", classes[j->p_ship.s_type]);
	    W_WriteText(infow, W_Textwidth, W_Textheight * line, playerColor(j), buf, strlen(buf),
			W_RegularFont);
	    if(j->p_swar & me->p_team)
	       wars = "WAR";
	    else if(j->p_hostile & me->p_team)
	       wars = "HOSTILE";
	    else
	       wars = "PEACEFUL";
	    W_WriteText(infow, 11*W_Textwidth, W_Textheight * line++, playerColor(j), wars, strlen(wars),
			W_RegularFont);
	 }
      } else {			/* New information window! */
	 if (mx + 24 * W_Textwidth + 2 > windowWidth)
	    mx = windowWidth - 24 * W_Textwidth - 2;
	 if (my + 10 * W_Textheight + 2 > windowHeight)
	    my = windowHeight - 10 * W_Textheight - 2;

	 j = &players[target->o_num];

	 W_ResizeWindow(infow, 24*W_Textwidth, 10*W_Textheight);
	 W_ReposWindow(infow, mx, my);
	 W_MapWindow(infow);

	 (void) sprintf (buf, "%s (%c%c):", j->p_name, teamlet[j->p_team], shipnos[j->p_no]);
	 W_WriteText (infow, W_Textwidth, W_Textheight * line++, playerColor (j), buf, strlen (buf), shipFont (j));
	 (void) sprintf (buf, "Login   %-s", j->p_login);
	 W_WriteText (infow, W_Textwidth, W_Textheight * line++, playerColor (j), buf, strlen (buf),
                       W_RegularFont);
	 (void) sprintf (buf, "Display %-s", j->p_monitor);
	 W_WriteText (infow, W_Textwidth, W_Textheight * line++, playerColor (j), buf, strlen (buf),
                       W_RegularFont);
	 strcpy (buf, "        Rating    Total");
	 W_WriteText (infow, W_Textwidth, W_Textheight * line++, playerColor (j), buf, strlen (buf),
                       W_RegularFont);
	 sprintf (buf, "Bombing: %5.2f  %5d", bombingRating (j),
                   j->p_stats.st_armsbomb + j->p_stats.st_tarmsbomb);
	 W_WriteText (infow, W_Textwidth, W_Textheight * line++, playerColor (j), buf, strlen (buf),
                       W_RegularFont);
	 sprintf (buf, "Planets: %5.2f  %5d", planetRating (j),
                   j->p_stats.st_planets + j->p_stats.st_tplanets);
	 W_WriteText (infow, W_Textwidth, W_Textheight * line++, playerColor (j), buf, strlen (buf),
                       W_RegularFont);
	 sprintf (buf, "Offense: %5.2f  %5d", offenseRating (j),
                   j->p_stats.st_kills + j->p_stats.st_tkills);
	 W_WriteText (infow, W_Textwidth, W_Textheight * line++, playerColor (j), buf, strlen (buf),
                       W_RegularFont);
	 sprintf (buf, "Defense: %5.2f  %5d", defenseRating (j),
                   j->p_stats.st_losses + j->p_stats.st_tlosses);
	 W_WriteText (infow, W_Textwidth, W_Textheight * line++, playerColor (j), buf, strlen (buf),
                       W_RegularFont);
	 if (j->p_ship.s_type == STARBASE) {
	    sprintf (buf, "  Maxkills: %6.2f", j->p_stats.st_sbmaxkills);
	 }
         else {
	    sprintf (buf, "  Maxkills: %6.2f", j->p_stats.st_maxkills);
	 }
	 W_WriteText (infow, W_Textwidth, W_Textheight * line++, playerColor (j), buf, strlen (buf),
                       W_RegularFont);
	 sprintf (buf, "  Hours:    %6.2f",
                   (float) j->p_stats.st_tticks / 36000.0);
	 W_WriteText (infow, W_Textwidth, W_Textheight * line++, playerColor (j), buf, strlen (buf),
                       W_RegularFont);
      }
   } else {			/* Planet */
      /* Too close to the edge? */
      if (mx + 20 * W_Textwidth + 2 > windowWidth)
	 mx = windowWidth - 25 * W_Textwidth - 2;
      if (my + 3 * W_Textheight + 2 > windowHeight)
	 my = windowHeight - 3 * W_Textheight - 2;

      k = &planets[target->o_num];

      W_ResizeWindow(infow, 25*W_Textwidth, 3*W_Textheight);
      W_ReposWindow(infow, mx, my);
      W_MapWindow(infow);
      if (k->pl_info & me->p_team) {
	 (void) sprintf(buf, "%s (%c)", k->pl_name, teamlet[k->pl_owner]);
	 W_WriteText(infow, W_Textwidth, W_Textheight * line++, planetColor(k), buf, strlen(buf),
		     planetFont(k));
	 (void) sprintf(buf, "Armies %d", k->pl_armies);
	 W_WriteText(infow, W_Textwidth, W_Textheight * line++, planetColor(k), buf, strlen(buf),
		     W_RegularFont);
	 (void) sprintf(buf, "%s %s %s %c%c%c%c",
			(k->pl_flags & PLREPAIR ? "REPAIR" : "      "),
			(k->pl_flags & PLFUEL ? "FUEL" : "    "),
			(k->pl_flags & PLAGRI ? "AGRI" : "    "),
			(k->pl_info & FED ? 'F' : ' '),
			(k->pl_info & ROM ? 'R' : ' '),
			(k->pl_info & KLI ? 'K' : ' '),
			(k->pl_info & ORI ? 'O' : ' '));
	 W_WriteText(infow, W_Textwidth, W_Textheight * line++, planetColor(k), buf, strlen(buf),
		     W_RegularFont);
      } else {
	 (void) sprintf(buf, "%s", k->pl_name);
	 W_WriteText(infow, W_Textwidth, W_Textheight * line++, planetColor(k), buf, strlen(buf),
		     W_RegularFont);
	 (void) sprintf(buf, "No other info");
	 W_WriteText(infow, W_Textwidth, W_Textheight * line++, planetColor(k), buf, strlen(buf),
		     W_RegularFont);
      }
   }
}


void
destroyInfo()
{
   W_UnmapWindow(infow);
   infomapped = 0;
   infow = NULL;
}
