/*
 * dashboard.c - graphic tstatw - 6/2/93
 *
 * Modified by Eric Mehlhaff.
 *
 * copyright 1993 Lars Bernhardsson (lab@mtek.chalmers.se)
 * Free to use as long as this notice is left here.
 */

#include "copyright.h"

#include <stdio.h>
#include "netrek.h"

#ifdef RECORD
#include "recorder.h"
#endif

#define DB_NOFILL 0
#define DB_LINE 1
#define DB_FILL 2

#define DB_3DIGITS 3
#define DB_5DIGITS 5

#define DB_VARCOLOR -1		/* db_bar changes color, turns red when low */
#define DB_INVARCOLOR -2	/* db_bar changes color, turns red when hi */

extern char	*itoa();
extern char	*strcpyp_return();
extern char	*strcpy_return();

/* draws box in specified, one of several types:
** FILL -- filled
** LINE -- lined
** NOFILL -- empty box
** x,y are positions within window
** w,h are width and height of box
*/

static void 
db_box(boxwin, boxcolr, f, x, y, w, h)
   W_Window                        boxwin;
   W_Color                         boxcolr;
   int                             f;
   int                             x, y, w, h;
{
   register int                    i;
   switch (f) {
      case DB_FILL:
	 if(w < 1 || h < 2)
	    return;

	 W_FillArea(boxwin, x, y+1, w, h - 1, boxcolr);
	 break;
      case DB_LINE:
	 if (w < 1 || h < 2)
	    return;

	 for (i = 3; i < h; i += 3)
	    W_MakeLine(boxwin, x, y + i, x + w, y + i, boxcolr);

	 /*
	 W_MakeLine(boxwin, x + w, y+1, x + w, y + h-1, boxcolr);
	 */
	 break;
      case DB_NOFILL:
	 if (w == 0 || h == 0)
	    return;

	 W_MakeLine(boxwin, x, y, x + w, y, boxcolr);
	 W_MakeLine(boxwin, x + w, y, x + w, y + h, boxcolr);
	 W_MakeLine(boxwin, x + w, y + h, x, y + h, boxcolr);
	 W_MakeLine(boxwin, x, y + h, x, y, boxcolr);
	 break;
   }
}

/* similar to db_box
**  starts with a string,
**  box is drawn at x,y,
**  with height is textheight, width is a hardcoded 64 (blech)
**
*/

#define BARWIDTH	64

static void 
db_bar(fr, barwin, barcolor, lab, x, y, value, oldvalue, tmpmax, oldtmpmax, 
       max, digits)
   int				   fr;
   W_Window                        barwin;
   W_Color                         barcolor;
   char                           *lab;
   int                             x, y, value, oldvalue, tmpmax, oldtmpmax,
                                   max, digits;
{
   register int                    wt, owt, wv, owv, tw=0, tc=0;
   char                            valstr[20];
   register char		   *s = valstr;
   int				   diff, oldbarcolor = barcolor;

#ifdef BARFIX
   static char                     formatstr[15] = {"%3.3s[%03d]\0\0\0\0"};
   if (digits > 0 && digits < 10) {
      formatstr[8] = '0' + digits;
      sprintf(valstr, formatstr, lab, value);
      tc = strlen(formatstr);
      tw = W_Textwidth * tc;
      W_ClearArea(barwin, x, y, 109 + digits, 10);
   } else {
      return;
   }
#else				/* BARFIX */
   switch (digits) {
      case DB_3DIGITS:
	 if(fr){
	    s = strcpyp_return(s, lab, 3);
	    tc = 8;
	 }
	 else {
	    tc = 5;
	    x += 3*W_Textwidth;
	 }
	 tw = W_Textwidth * tc;

	 *s++ = '[';
	 s = itoa(s, value, 3, 1);
	 *s++ = ']';
	 break;
      case DB_5DIGITS:
	 if(fr){
	    s = strcpyp_return(s, lab, 3);
	    tc = 10;
	 }
	 else {
	    tc = 7;
	    x += 3*W_Textwidth;
	 }
	 tw = W_Textwidth * tc;

	 *s++ = '[';
	 s = itoa(s, value, 5, 1);
	 *s++ = ']';
	 break;
   }
#endif				/* BARFIX */

   if (max <= 0)
      max = 1;			/* avoid the floatexception */

   wt = ((BARWIDTH-1) * tmpmax)/max;
   if (wt > (BARWIDTH-1))
      wt = (BARWIDTH-1);
   owt = ((BARWIDTH-1) * oldtmpmax)/max;
   if (owt > (BARWIDTH-1))
      owt = (BARWIDTH-1);

   wv = ((BARWIDTH-1) * value)/max;
   if (wv > (BARWIDTH-1))
      wv = (BARWIDTH-1);
   owv = ((BARWIDTH-1)* oldvalue)/max;
   if(owv > (BARWIDTH-1))
      owv = (BARWIDTH-1);


   /* override bar color with value depending on how far below max
   ** and value is */
   if (barcolor == DB_VARCOLOR || barcolor == DB_INVARCOLOR) {
      int                             barv = wv,
				      oldbarv = owv;
      if (barcolor == DB_INVARCOLOR){
	 barv = (BARWIDTH-1) - wv;
	 oldbarv = (BARWIDTH-1) - owv;
      }

      if (barv > 60)
	 barcolor = W_White;
      else if (barv > 40)
	 barcolor = W_Green;
      else if (barv > 20)
	 barcolor = W_Yellow;
      else
	 barcolor = W_Red;

      if (oldbarv > 60)
	 oldbarcolor = W_White;
      else if (oldbarv > 40)
	 oldbarcolor = W_Green;
      else if (oldbarv > 20)
	 oldbarcolor = W_Yellow;
      else
	 oldbarcolor = W_Red;
   }
   W_WriteText(barwin, x, y, textColor, valstr, tc, W_RegularFont);

   x += tw;

   if(fr){
      db_box(barwin, backColor, DB_FILL, x+1, y,  BARWIDTH, 9);
      db_box(barwin, barcolor, DB_FILL, x+1, y,  wv, 9);
      db_box(barwin, barcolor, DB_LINE, x, y, wt, 9);
      db_box(barwin, barcolor, DB_NOFILL, x, y, BARWIDTH, 9);
      return;
   }

   diff = value - oldvalue;
   if(diff < 0){
      db_box(barwin, backColor, DB_FILL, x+wv+1, y, owv - wv, 9);
      if(barcolor != oldbarcolor)
	 db_box(barwin, barcolor, DB_FILL, x+1, y,  wv, 9);
      if(wt > wv)
	 db_box(barwin, barcolor, DB_LINE, x+wv, y, wt-wv, 9);
   }
   else if (diff > 0 && wv > 0){
      if(barcolor != oldbarcolor)
	 owv = 0;
      db_box(barwin, barcolor, DB_FILL, x + owv+1, y, wv - owv, 9);
   }

   if(wt > wv || owt > wv){
      diff = tmpmax - oldtmpmax;
      if(diff < 0){
	 db_box(barwin, backColor, DB_FILL, x+wt+1, y, owt - wt, 9);
	 if(barcolor != oldbarcolor)
	    db_box(barwin, barcolor, DB_LINE, x, y, wt, 9);
      }
      else if (diff > 0 && wt > 0){
	 if(barcolor != oldbarcolor)
	    owt = 0;
	 db_box(barwin, barcolor, DB_LINE, x + owt, y, wt - owt, 9);
      }
   }
   if(barcolor != oldbarcolor)
      db_box(barwin, barcolor, DB_NOFILL, x, y, BARWIDTH, 9);

}

/* draw all the relevant text information in the window */

static void 
db_flags(fr)
   int                             fr;
{
   static int                      old_tourn = -1;
   static float                    old_kills = -1.0;
   static int                      old_torp = -1;
   static unsigned int             old_flags = -1;
   char                            buf[16];
   register char		   *s = buf;
   register unsigned int	   flags = me->p_flags;
   double			   kills = nkills();

   if (fr || flags != old_flags) {
      W_Color                         flagscolor = W_White;
      buf[0] = (flags & PFSHIELD ? 'S' : ' ');
      if (flags & PFGREEN) {
	 buf[1] = 'G';
	 flagscolor = W_Green;
      } else if (flags & PFYELLOW) {
	 buf[1] = 'Y';
	 flagscolor = W_Yellow;
      } else {
	 buf[1] = 'R';
	 flagscolor = W_Red;
      }
      buf[2] = (flags & (PFPLLOCK | PFPLOCK) ? 'L' : ' ');
      buf[3] = (flags & PFREPAIR ? 'R' : ' ');
      buf[4] = (flags & PFBOMB ? 'B' : ' ');
      buf[5] = (flags & PFORBIT ? 'O' : ' ');
      if (me->p_ship.s_type == STARBASE)
	 buf[6] = (flags & PFDOCKOK ? 'D' : ' ');
      else
	 buf[6] = (flags & PFDOCK ? 'D' : ' ');
      buf[7] = (flags & PFCLOAK ? 'C' : ' ');
      buf[8] = (flags & PFWEP ? 'W' : ' ');
      buf[9] = (flags & PFENG ? 'E' : ' ');
      if (flags & PFPRESS)
	 buf[10] = 'P';
      else if (flags & PFTRACT)
	 buf[10] = 'T';
      else
	 buf[10] = ' ';
      if (flags & PFBEAMUP)
	 buf[11] = 'u';
      else if (flags & PFBEAMDOWN)
	 buf[11] = 'd';
      else
	 buf[11] = ' ';

      W_WriteText(tstatw, 2, 3, flagscolor, "Flags", 5, W_RegularFont);
      W_WriteText(tstatw, 2, 17, flagscolor, buf, 12, W_RegularFont);
      old_flags = flags;
   }
   if (fr || status->tourn != old_tourn) {
      if (status->tourn) {
	 W_WriteText(tstatw, 14, 30, textColor, "TMODE", 5, W_RegularFont);
      } else {
	 W_ClearArea(tstatw, 14, 30, 30, 10);
      }
      old_tourn = status->tourn;
   }

   if (fr || kills != old_kills){
      if (kills > 0.0) {
	 W_WriteText(tstatw, 330, 17, textColor, "Kills:", 6, W_RegularFont);
	 sprintf(buf, "%3.2f", kills);
	 W_WriteText(tstatw, 370, 17, textColor, buf, strlen(buf), W_RegularFont);
      } else {
	 W_ClearArea(tstatw, 330, 17, 96, 10);
      }
      old_kills = kills;
   }

#ifdef RECORD
   if(playback && fr) {
     strcpy(buf, "Update");
     W_WriteText(tstatw, 458, 17, textColor, buf, 6, W_RegularFont);
   }
#endif

   if (fr || me->p_ntorp != old_torp){
      if (me->p_ntorp > 0) {
	 s = strcpy_return(s, "Torps: ");
	 s = itoa(s, me->p_ntorp, 1, 1);
	 W_WriteText(tstatw, 330, 30, textColor, buf, s-buf, W_RegularFont);
      } else {
#ifdef RECORD
	if(playback)
	  W_ClearArea(tstatw, 330, 30, 70, 10);  /* Leave playback status */
	else
#endif
	 W_ClearArea(tstatw, 330, 30, 96, 10);
      }
      old_torp = me->p_ntorp;
   }
#ifdef RECORD
   if(playback && fr) {
     playback_status(buf);
     W_WriteText(tstatw, 400, 30, textColor, buf, 7, W_RegularFont);
   }
#endif
}

/* draw 'db' graphic dashboard.
** 'fr' means it needs to be entirely redrawn.
*/

void 
db_redraw_brmh(fr)
   int                             fr;
{
   static int                      old_spd = -1, old_cur_spd = -1;
   static int                      old_shl = -1, old_dam = -1;
   static int                      old_arm = -1, old_cur_arm = -1;
   static int                      old_wpn = -1, old_egn = -1;
   static int                      old_ful = -1;
   register int                    cur_max;
   double 			   kills = nkills();

   /*
   if (fr)
      W_ClearWindow(tstatw);
   */

   db_flags(fr);

   cur_max = (me->p_ship.s_maxspeed + 2) - (me->p_ship.s_maxspeed + 1) *
       ((float) me->p_damage / (float) (me->p_ship.s_maxdamage));
   if (cur_max > me->p_ship.s_maxspeed)
      cur_max = me->p_ship.s_maxspeed;
   if (cur_max < 0)
      cur_max = 0;

   if (fr || me->p_speed != old_spd || cur_max != old_cur_spd) {
      db_bar(fr, tstatw, W_White, "Spd", 90, 3,
	     me->p_speed, old_spd, cur_max, old_cur_spd, 
	     me->p_ship.s_maxspeed, DB_3DIGITS);
      old_spd = me->p_speed;
      old_cur_spd = cur_max;
   }
   if (fr || me->p_shield != old_shl) {
      db_bar(fr, tstatw, DB_VARCOLOR, "Shl", 90, 17,
	 me->p_shield, old_shl, 0, 0, me->p_ship.s_maxshield, DB_3DIGITS);
      old_shl = me->p_shield;
   }
   if (fr || me->p_damage != old_dam) {
      db_bar(fr, tstatw, DB_INVARCOLOR, "Dam", 90, 31,
	 me->p_damage, old_dam, 0, 0, me->p_ship.s_maxdamage, DB_3DIGITS);
      old_dam = me->p_damage;
   }
   if (me->p_ship.s_type == ASSAULT)
      cur_max = (((kills * 3) > me->p_ship.s_maxarmies) ?
	me->p_ship.s_maxarmies : (int) (kills * 3));
   else if (me->p_ship.s_type == STARBASE)
      cur_max = me->p_ship.s_maxarmies;
   else
      cur_max = (((kills * 2) > me->p_ship.s_maxarmies) ?
	me->p_ship.s_maxarmies : (int) (kills * 2));

   if (fr || me->p_armies != old_arm || cur_max != old_cur_arm) {
      db_bar(fr, tstatw, W_White, "Arm", 210, 3,
	 me->p_armies, old_arm, cur_max, old_cur_arm, 
	 me->p_ship.s_maxarmies, DB_3DIGITS);
      old_arm = me->p_armies;
      old_cur_arm = cur_max;
   }
   if (fr || me->p_wtemp != old_wpn) {
      db_bar(fr, tstatw, DB_INVARCOLOR, "Wpn", 210, 17,
	 me->p_wtemp / 10, old_wpn, 0, 0, 
	 me->p_ship.s_maxwpntemp / 10, 
	 DB_3DIGITS);
      old_wpn = me->p_wtemp/10;
   }
   if (fr || me->p_etemp != old_egn) {
      db_bar(fr, tstatw, DB_INVARCOLOR, "Egn", 210, 31,
        me->p_etemp / 10, old_egn, 0, 0, 
	me->p_ship.s_maxegntemp / 10, DB_3DIGITS);
      old_egn = me->p_etemp/10;
   }
   if (fr || me->p_fuel != old_ful) {
      db_bar(fr, tstatw, DB_VARCOLOR, "Ful", 330, 3,
	 me->p_fuel, old_ful, 0, 0, me->p_ship.s_maxfuel, DB_5DIGITS);
      old_ful = me->p_fuel;
   }
}
