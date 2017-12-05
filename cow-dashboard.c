
/*
 * dashboard.c - graphic tstatw - 6/2/93
 * 
 * copyright 1993 Lars Bernhardsson (lab@mtek.chalmers.se) Free to use as long
 * as this notice is left here.
 * 
 * Color by Nick Trown.
 */


/*
 * This code has been shamelessly copied from COW.1.01pl2 (after removing
 * some redundant code).		-- James Soutter 21/11/94
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

#define DB_3DIGITS 0
#define DB_5DIGITS 1

#define BAR_LENGTH 56

#if __STDC__ || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

/* cow-dashboard.c */
static char    *itoa3 P_((int val, char *result, int pad, int prec));
/*static char    *ftoa P_((float fval, char *result, int pad, int iprec, int dprec));*/
static void db_box P_((int x, int y, int w, int h, int f, int color));
static void db_bar P_((char *lab, int x, int y, int value, int tmpmax, int max, int digits, int color));
static void db_flags P_((int fr));

#undef P_

static char    *
itoa3(val, result, pad, prec)
    int             val;
    char           *result;
    int             pad;
    int             prec;
{
   int             lead_digit = 0, i, j, too_big = 1, h = 1;

   if (prec != 0)
      result[prec] = '\0';

   for (i = 100000000, j = 0; i && h <= prec; i /= 10, j++) {
      if ((9 - prec) > j && too_big)
	 continue;
      else if (h) {
	 j = 0;
	 too_big = 0;
	 h = 0;
      }
      result[j] = (val % (i * 10)) / i + '0';

      if (result[j] != '0' && !lead_digit)
	 lead_digit = 1;

      if (!lead_digit && !pad)
	 if ((result[j] = (val % (i * 10)) / i + '0') == '0')
	    result[j] = ' ';
   }

   if (val == 0)
      result[prec - 1] = '0';

   return (result);
}


static char    *
ftoa(fval, result, pad, iprec, dprec)
    float           fval;
    char           *result;
    int             pad;
    int             iprec;
    int             dprec;
{
   int             i, ival;
   float           val = fval;

   if ((iprec + dprec) != 0)
      result[iprec + dprec + 1] = '\0';

   for (i = 0; i < dprec; i++)
      val *= 10.0;

   ival = val;
   itoa3(ival, result, pad, iprec + dprec);

   for (i = (iprec + dprec); i >= iprec; i--)
      if (result[i] == ' ')
	 result[i + 1] = '0';
      else
	 result[i + 1] = result[i];

   result[iprec] = '.';

   if (fval < 1.0)
      result[iprec - 1] = '0';

   return (result);
}


static void
db_box(x, y, w, h, f, color)
    int             x, y, w, h, f, color;
{
   int             border = W_White;

   if (color == W_Red)
      border = color;

   if (w == 0 || h == 0)
      return;

   switch (f) {
   case DB_FILL:
      W_FillArea(tstatw, x, y, w + 1, h + 1, color);
      break;
   case DB_LINE:
      W_MakeLine(tstatw, x + w, y, x + w, y + h, border);
      W_MakeLine(tstatw, x + w, y + 4, x + BAR_LENGTH, y + 4, border);
      break;
   case DB_NOFILL:
      W_MakeLine(tstatw, x, y, x + w, y, border);
      W_MakeLine(tstatw, x + w, y, x + w, y + h, border);
      W_MakeLine(tstatw, x + w, y + h, x, y + h, border);
      W_MakeLine(tstatw, x, y + h, x, y, border);
      break;
   }
}

static void
db_bar(lab, x, y, value, tmpmax, max, digits, color)
    char           *lab;
    int             x, y, value, tmpmax, max, digits, color;
{
   register int    wt, wv, tw, tc;
   char            valstr[32];

   if(value < 0) value = 0;

   switch (digits) {
   case DB_3DIGITS:
      tc = 11;
      tw = W_Textwidth * tc;
      valstr[0] = lab[0];
      valstr[1] = lab[1];
      valstr[2] = '[';
      itoa3(value, &(valstr[3]), 0, 3);
      valstr[6] = '/';
      itoa3(tmpmax, &(valstr[7]), 0, 3);
      valstr[10] = ']';
      W_ClearArea(tstatw, x, y, tw + BAR_LENGTH, 10);
      break;
   case DB_5DIGITS:
      tc = 15;
      tw = W_Textwidth * tc;
      valstr[0] = lab[0];
      valstr[1] = lab[1];
      valstr[2] = '[';
      itoa3(value, &(valstr[3]), 0, 5);
      valstr[8] = '/';
      itoa3(tmpmax, &(valstr[9]), 0, 5);
      valstr[14] = ']';
      W_ClearArea(tstatw, x, y, tw + BAR_LENGTH, 10);
      break;
   default:
      tw = tc = 0;	/* just in case */
   }

   wt = (int) ((float) BAR_LENGTH * ((float) tmpmax / (float) max));
   wv = (int) ((float) BAR_LENGTH * ((float) value / (float) max));
   if (wt > BAR_LENGTH)
      wt = BAR_LENGTH;
   if (wv > BAR_LENGTH)
      wv = BAR_LENGTH;

   W_WriteText(tstatw, x, y, textColor, valstr, 3, W_RegularFont);
   W_WriteText(tstatw, x + 3 * W_Textwidth, y, textColor, (&valstr[3]), tc / 2 + 1, W_BoldFont);
   W_WriteText(tstatw, x + (tc / 2 + 1) * W_Textwidth, y, textColor, (&valstr[tc / 2 + 1]), tc / 2, W_RegularFont);

   db_box(x + tw, y, BAR_LENGTH, 9, DB_NOFILL, color);
   if (wt >= wv && wt > 0)
      db_box(x + tw, y, wt, 9, DB_LINE, color);

   if (wv > 0)
      db_box(x + tw, y, wv, 9, DB_FILL, color);
}


static void
db_flags(fr)
    int             fr;
{
   static float    old_kills = -1.0;
   static int      old_torp = -1;
   static unsigned int old_flags = -1;
   static int      old_tourn = -1;
   char            buf[16];
   double	   kills = nkills();

   if (fr || me->p_flags != old_flags) {
      buf[0] = (me->p_flags & PFSHIELD ? 'S' : ' ');
      if (me->p_flags & PFGREEN)
	 buf[1] = 'G';
      else if (me->p_flags & PFYELLOW)
	 buf[1] = 'Y';
      else
	 buf[1] = 'R';
      buf[2] = (me->p_flags & (PFPLLOCK | PFPLOCK) ? 'L' : ' ');
      buf[3] = (me->p_flags & PFREPAIR ? 'R' : ' ');
      buf[4] = (me->p_flags & PFBOMB ? 'B' : ' ');
      buf[5] = (me->p_flags & PFORBIT ? 'O' : ' ');
      if (me->p_ship.s_type == STARBASE)
	 buf[6] = (me->p_flags & PFDOCKOK ? 'D' : ' ');
      else
	 buf[6] = (me->p_flags & PFDOCK ? 'D' : ' ');
      buf[7] = (me->p_flags & PFCLOAK ? 'C' : ' ');
      buf[8] = (me->p_flags & PFWEP ? 'W' : ' ');
      buf[9] = (me->p_flags & PFENG ? 'E' : ' ');
      if (me->p_flags & PFPRESS)
	 buf[10] = 'P';
      else if (me->p_flags & PFTRACT)
	 buf[10] = 'T';
      else
	 buf[10] = ' ';
      if (me->p_flags & PFBEAMUP)
	 buf[11] = 'u';
      else if (me->p_flags & PFBEAMDOWN)
	 buf[11] = 'd';
      else
	 buf[11] = ' ';

      W_WriteText(tstatw, 2, 3, textColor, "Flags", 5, W_RegularFont);
      W_WriteText(tstatw, 2, 17, textColor, buf, 12, W_RegularFont);
      old_flags = me->p_flags;
   }
   if (fr || status->tourn != old_tourn) {
      if (status->tourn)
	 W_WriteText(tstatw, 74, 17, textColor, "T", 1, W_BoldFont);
      else
	 W_WriteText(tstatw, 74, 17, textColor, " ", 1, W_BoldFont);

      old_tourn = status->tourn;
   }
   if (fr || kills != old_kills) {
      if (kills > 0.0) {
	 W_WriteText(tstatw, 346, 17, textColor, "Kills:", 6, W_RegularFont);
	 ftoa(kills, buf, 0, 3, 2);
	 W_WriteText(tstatw, 386, 17, textColor, buf, strlen(buf), W_RegularFont);
      } else {
	 W_ClearArea(tstatw, 346, 17, 96, 10);
      }
      old_kills = kills;
   }

#ifdef RECORD
   if(playback && fr) {
     strcpy(buf, "Update");
     W_WriteText(tstatw, 458, 17,textColor, buf, 6, W_RegularFont);
   }
#endif

   if (fr || me->p_ntorp != old_torp) {
      if (me->p_ntorp > 0) {
	 W_WriteText(tstatw, 346, 30, textColor, "Torps:", 6, W_RegularFont);
	 buf[0] = me->p_ntorp % 10 + '0';
	 W_WriteText(tstatw, 386, 30, textColor, buf, 1, W_RegularFont);
      } else {
#ifdef RECORD
	if(playback)
	  W_ClearArea(tstatw, 346, 30, 54, 10);  /* Leave playback status */
	else
#endif
	 W_ClearArea(tstatw, 346, 30, 96, 10);
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

void
db_redraw_krp(fr)
    int             fr;
{
   static int      old_spd = -1, old_cur_spd = -1;
   static int      old_shl = -1, old_dam = -1;
   static int      old_arm = -1, old_cur_arm = -1;
   static int      old_wpn = -1, old_egn = -1;
   static int      old_ful = -1;
   register int    cur_max;
   register int    value;
   int             color;
   double	   kills = nkills();

   if (fr)
      W_ClearWindow(tstatw);

   db_flags(fr);

   cur_max = (me->p_ship.s_maxspeed + 2) - (me->p_ship.s_maxspeed + 1) *
      ((float) me->p_damage / (float) (me->p_ship.s_maxdamage));
   if (cur_max > me->p_ship.s_maxspeed)
      cur_max = me->p_ship.s_maxspeed;
   if (cur_max < 0)
      cur_max = 0;

   if (fr || me->p_speed != old_spd || cur_max != old_cur_spd) {
      if (me->p_speed >= me->p_ship.s_maxspeed - 2)
	 color = W_Red;
      else
	 color = W_Green;
      db_bar("Sp", 90, 3,
	     me->p_speed, cur_max, me->p_ship.s_maxspeed, DB_3DIGITS, color);
      old_spd = me->p_speed;
      old_cur_spd = cur_max;
   }
   if (fr || me->p_shield != old_shl) {
      value = (100 * me->p_shield) / me->p_ship.s_maxshield;
      if (value <= 16)
	 color = W_Red;
      else if (value <= 66)
	 color = W_Yellow;
      else
	 color = W_Green;
      db_bar("Sh", 90, 17,
	     me->p_shield, me->p_ship.s_maxshield, me->p_ship.s_maxshield,
	     DB_3DIGITS, color);
      old_shl = me->p_shield;
   }
   if (fr || me->p_damage != old_dam) {
      value = (100 * (me->p_ship.s_maxdamage - me->p_damage)) / me->p_ship.s_maxdamage;
      if (value <= 16)
	 color = W_Red;
      else if (value <= 66)
	 color = W_Yellow;
      else
	 color = W_Green;
      db_bar("Hu", 90, 31,
	     (me->p_ship.s_maxdamage - me->p_damage),
	     me->p_ship.s_maxdamage, me->p_ship.s_maxdamage,
	     DB_3DIGITS, color);
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
      value = me->p_armies;

      if (value <= 0)
	 color = W_Green;
      else
	 color = W_Red;

      db_bar("Ar", 218, 3,
	  me->p_armies, cur_max, me->p_ship.s_maxarmies, DB_3DIGITS, color);
      old_arm = me->p_armies;
      old_cur_arm = cur_max;
   }
   if (fr || me->p_wtemp != old_wpn) {
      value = (100 * me->p_wtemp) / me->p_ship.s_maxwpntemp;

      if (value <= 16)
	 color = W_Green;
      else if (value <= 66)
	 color = W_Yellow;
      else
	 color = W_Red;
      db_bar("Wt", 218, 17,
	     me->p_wtemp / 10, me->p_ship.s_maxwpntemp / 10, me->p_ship.s_maxwpntemp / 10, DB_3DIGITS
	     ,color);
      old_wpn = me->p_wtemp;
   }
   if (fr || me->p_etemp != old_egn) {
      value = (100 * me->p_etemp) / me->p_ship.s_maxegntemp;
      if (value <= 16)
	 color = W_Green;
      else if (value <= 66)
	 color = W_Yellow;
      else
	 color = W_Red;
      db_bar("Et", 218, 31,
	     me->p_etemp / 10, me->p_ship.s_maxegntemp / 10, me->p_ship.s_maxegntemp / 10, DB_3DIGITS
	     ,color);
      old_egn = me->p_etemp;
   }
   if (fr || me->p_fuel != old_ful) {
      value = ((100 * me->p_fuel) / me->p_ship.s_maxfuel);
      if (value <= 16)
	 color = W_Red;
      else if (value <= 66)
	 color = W_Yellow;
      else
	 color = W_Green;
      db_bar("Fu", 346, 3,
	     me->p_fuel, me->p_ship.s_maxfuel, me->p_ship.s_maxfuel, DB_5DIGITS, color);
      old_ful = me->p_fuel;
   }
}

void
db_redraw_COW(fr)
    int             fr;
{
   static int      old_spd = -1, old_cur_spd = -1;
   static int      old_shl = -1, old_dam = -1;
   static int      old_arm = -1, old_cur_arm = -1;
   static int      old_wpn = -1, old_egn = -1;
   static int      old_ful = -1;
   register int    cur_max;
   register int    value;
   int             color;
   double	   kills = nkills();

   if (fr)
      W_ClearWindow(tstatw);

   db_flags(fr);

   cur_max = (me->p_ship.s_maxspeed + 2) - (me->p_ship.s_maxspeed + 1) *
      ((float) me->p_damage / (float) (me->p_ship.s_maxdamage));
   if (cur_max > me->p_ship.s_maxspeed)
      cur_max = me->p_ship.s_maxspeed;
   if (cur_max < 0)
      cur_max = 0;

   if (fr || me->p_speed != old_spd || cur_max != old_cur_spd) {
      if (me->p_speed >= me->p_ship.s_maxspeed - 2)
	 color = W_Yellow;
      else
	 color = W_White;
      db_bar("Sp", 90, 3,
	     me->p_speed, cur_max, me->p_ship.s_maxspeed, DB_3DIGITS, color);
      old_spd = me->p_speed;
      old_cur_spd = cur_max;
   }
   if (fr || me->p_shield != old_shl) {
      value = (100 * me->p_shield) / me->p_ship.s_maxshield;
      if (value <= 50)
	 color = W_Red;
      else if (value < 90)
	 color = W_Yellow;
      else
	 color = W_White;
      db_bar("Sh", 90, 17,
	     me->p_ship.s_maxshield - me->p_shield, me->p_ship.s_maxshield, me->p_ship.s_maxshield, DB_3DIGITS, color);
      old_shl = me->p_shield;
   }
   if (fr || me->p_damage != old_dam) {
      value = (100 * me->p_damage) / me->p_ship.s_maxdamage;
      if (value <= 10)
	 color = W_White;
      else if (value > 50)
	 color = W_Red;
      else
	 color = W_Yellow;
      db_bar("Da", 90, 31,
	     me->p_damage, me->p_ship.s_maxdamage, me->p_ship.s_maxdamage, DB_3DIGITS, color);
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
      value = me->p_armies;

      if (value <= 3)
	 color = W_White;
      else if (value > 5)
	 color = W_Red;
      else
	 color = W_Yellow;
      db_bar("Ar", 218, 3,
	  me->p_armies, cur_max, me->p_ship.s_maxarmies, DB_3DIGITS, color);
      old_arm = me->p_armies;
      old_cur_arm = cur_max;
   }
   if (fr || me->p_wtemp != old_wpn) {
      value = (100 * me->p_wtemp) / me->p_ship.s_maxwpntemp;

      if (value > 50)
	 color = W_Red;
      else if (value <= 20)
	 color = W_White;
      else
	 color = W_Yellow;
      db_bar("Wt", 218, 17,
	     me->p_wtemp / 10, me->p_ship.s_maxwpntemp / 10, me->p_ship.s_maxwpntemp / 10, DB_3DIGITS
	     ,color);
      old_wpn = me->p_wtemp;
   }
   if (fr || me->p_etemp != old_egn) {
      value = (100 * me->p_etemp) / me->p_ship.s_maxegntemp;

      if (value <= 25)
	 color = W_White;
      else if (value < 75)
	 color = W_Yellow;
      else
	 color = W_Red;
      db_bar("Et", 218, 31,
	     me->p_etemp / 10, me->p_ship.s_maxegntemp / 10, me->p_ship.s_maxegntemp / 10, DB_3DIGITS
	     ,color);
      old_egn = me->p_etemp;
   }
   if (fr || me->p_fuel != old_ful) {
      value = ((100 * me->p_fuel) / me->p_ship.s_maxfuel);

      if (value <= 50)
	 color = W_Red;
      else if (value > 90)
	 color = W_White;
      else
	 color = W_Yellow;
      db_bar("Fu", 346, 3,
	     me->p_fuel, me->p_ship.s_maxfuel, me->p_ship.s_maxfuel, DB_5DIGITS, color);
      old_ful = me->p_fuel;
   }
}
