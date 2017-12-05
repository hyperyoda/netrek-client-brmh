/*
 * redraw.c
 */
#include "copyright.h"

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include "netrek.h"

#ifdef RECORD
#include "recorder.h"
#endif

#if __STDC__ || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

/* redraw.c */
static void get_shrink_phaser_coords P_((int *rx, int *ry, register dx, register dy, register tx, register ty, register curr_fuse, register max_fuse));

#undef P_

#define XPI     3.14159265358979323846264

static int      clearzone[4][(MAXTORP + 1) * MAXPLAYER +
		                  (MAXPLASMA + 1) * MAXPLAYER + MAXPLANETS];
static int      clearcount;

/* galactic edges, phasers (2 lines at most) */
static int      clearline[4][4 + 2 * MAXPLAYER];
static int      clearlcount;

/* tractors */
static int      cleartpline[4][2];
static int      cleartplcount;

static int      clearlmark[4];
static int      clearlmcount;

#ifndef FOR_MORONS
static int      mclearzone[6][MAXPLAYER];	/* For map window */

#else				/* FOR_MORONS */

static int      mclearzone[6][MAXPLAYER + MAXPLANETS];	/* For map window */
int             For_Morons = 0;	/* set this to 1 for defaulting to MORON mode */
short           FlashChange = 1;
static time_t   lastflash;
#define BOLD_ON		( lastread % 2)	/* whether to show flash this update */
#endif				/* FOR_MORONS */

static short    nplayers;
#ifdef FOR_MORONS
static time_t   lastread;
#endif

extern Window W_Root;

void
intrupt(readfds)
    fd_set         *readfds;
{
   time_t          time();

   udcounter++;
   if (readFromServer(readfds)) {

#ifdef XTRA
      xinput();
#endif

#ifdef FOR_MORONS
      lastread = time(NULL);
#endif
#ifndef NO_TRAP
      errcount = 0;		/* reset the signal handler error counters */
#endif
#ifdef FOR_MORONS
      if (For_Morons) {
	 if (lastread != lastflash) {
	    FlashChange = 1;
	    lastflash = lastread;
	 } else {
	    FlashChange = 0;
	 }
      }
      /* cache RealNumShips */
      (void) RealNumShips(-1);
#endif

#ifdef EM
      if (sortPlayers) {
	if(!teamOrder)               /* DBP */
	  Sorted_playerlist2();
	else Sorted_playerlist3();   /* DBP */
      }
#endif

#ifdef RECORD
      if(!playback || playback_update)
#endif
      redraw();

#ifdef RECORD
      if(playback) {
	if(playback_update) {
	  playback_update = 0;
	  /* fprintf(stderr, "reset playback_update to 0\n"); */
	}

	if(pb_mode == SCAN) {
	  /* Record our current alert status */

	  switch(pb_alert_scan) {
	  case PB_RED:
	    if(pb_alert == PB_RED && me->p_flags & PFRED) {
	      /* We started scan in red-alert, keep looking for next one */
	      ;
	    }
	    else {
	      if(me->p_flags & PFRED) {
		pb_alert = PB_RED;  /* Found our status, pause */
		pb_paused = 1;
		db_redraw(1);
	      }
	      else {
		pb_alert = PB_NONE;
	      }
	    }
	    break;
	  case PB_YELLOW:
	    if(pb_alert == PB_YELLOW && me->p_flags & PFYELLOW) {
	      /* We started scan in yellow-alert, keep looking for next one */
	      ;
	    }
	    else {
	      if(me->p_flags & PFYELLOW) {
		pb_alert = PB_YELLOW;  /* Found our status, pause */
		pb_paused = 1;
		db_redraw(1);
	      }
	      else pb_alert = PB_NONE;
	    }
	    break;
	  case PB_GREEN:
	    if(pb_alert == PB_GREEN && me->p_flags & PFGREEN) {
	      /* We started scan in green-alert, keep looking for next one */
	      ;
	    }
	    else {
	      if(me->p_flags & PFGREEN) {
		pb_alert = PB_GREEN;  /* Found our status, pause */
		pb_paused = 1;
		db_redraw(1);
	      }
	      else pb_alert = PB_NONE;
	    }
	    break;
	  case PB_DEATH:
	    if(pb_alert == PB_DEATH && me->p_status != PALIVE) {
	      /* We started scan dead, keep looking for next one */
	      ;
	    }
	    else {
	      if(me->p_status != PALIVE) {
		pb_alert = PB_DEATH;  /* Found our status, pause */
		pb_paused = 1;
		db_redraw(1);
	      }
	      else pb_alert = PB_NONE;
	    }
	    break;
	  case PB_NONE:
	  default:
	    break;
	  }  /* switch(pb_alert_scan) */
	}  /* if(pb_mode == SCAN) */
      }  /* if(playback)  */

#endif

      /* regular player list called from redraw */

#ifdef RECORD
      if(recordGame)
	recordUpdate();
#endif

   }
   if (reinitPlanets) {
      initPlanets();
      reinitPlanets = 0;
   }
#ifdef FOR_MORONS		/* my thinking is is you haven't heard from
				 * the server, no amount of pinging is going
				 * to help */
   if (lastread + 3 < time(NULL)) {
      /*
       * We haven't heard from server for awhile... Strategy:  send a useless
       * packet to "ping" server.
       */
      printf("Server might be dead, pinging with a war req!\n");
      sendWarReq(me->p_hostile);
   }
#endif
   /*
    * New: scrolling windows updated every other update instead of for every
    * message packet
    */
   if ((udcounter % 2) == 0) {
      W_FlushScrollingWindow(messwa);
      W_FlushScrollingWindow(messwt);
      W_FlushScrollingWindow(messwi);
      W_FlushScrollingWindow(messwk);
      W_FlushScrollingWindow(reviewWin);
   }
   if (me->p_status == POUTFIT) {
      death();
   }
}

void
redraw()
{
   /* erase warning line if necessary */
   if (warncount && (warntimer <= udcounter)) {
      W_ClearArea(warnw, 5, 5, W_Textwidth * warncount, W_Textheight);
      warncount = 0;
   }
   if (W_FastClear) {
      W_ClearWindow(w);
      clearcount = 0;
      clearlcount = 0;
      cleartplcount = 0;
   } else {
      while (clearcount) {
	 clearcount--;
	 W_CacheClearArea(w, clearzone[0][clearcount],
			  clearzone[1][clearcount], clearzone[2][clearcount],
			  clearzone[3][clearcount]);

      }
      while (clearlcount) {
	 clearlcount--;
	 W_CacheLine(w, clearline[0][clearlcount], clearline[1][clearlcount],
		     clearline[2][clearlcount], clearline[3][clearlcount],
		     backColor);
      }

      W_FlushClearAreaCache(w);
      W_FlushLineCaches(w);

      while (cleartplcount) {
	 cleartplcount--;

	 W_MakeTractLine(w, cleartpline[0][cleartplcount],
			 cleartpline[1][cleartplcount],
			 cleartpline[2][cleartplcount],
			 cleartpline[3][cleartplcount],
			 backColor);
      }
   }
#ifdef TTS
   if (tts && tts_timer) {
      static int      last_width;

      tts_timer--;
      if (!tts_timer) {
	 /* timed out */
	 if (!W_FastClear)
	    W_EraseTTSText(w, WINSIDE, tts_loc, last_width);
	 last_width = 0;
      } else if (tts_timer == tts_time - 1 && last_width) {
	 /* first draw -- erase previous */
	 if (!W_FastClear)
	    W_EraseTTSText(w, WINSIDE, tts_loc, last_width);
	 /* draw new */
	 W_WriteTTSText(w, WINSIDE, tts_loc, tts_width, lastIn,
			tts_len);
	 last_width = tts_width;
      } else {
	 /* regular draw */
	 W_WriteTTSText(w, WINSIDE, tts_loc, tts_width, lastIn, tts_len);
	 last_width = tts_width;
      }

   }
#endif

   local();			/* redraw local window */

#ifdef XTRA
   xinput();
#endif

   W_FlushLineCaches(w);

#ifdef RECORD
   if(playback && update_dashboard) {
     db_redraw(1);
     update_dashboard = 0;
   }
   else
#endif
   db_redraw(0);

   if (W_IsMapped(statwin))
      updateStats();

   if (mapmode)
      map();

   if (tclock)
      run_clock(0);		/* I want it where I'm looking dammit */
   
   if (torprepeat)
   {
   time_t now;
       if ((now = mstime()) - last_torp >= 50)
       {
           int x, y;
           if (W_FindMouseInWin(&x, &y, w))
           {
               sendTorpReq(getcourse(w, x, y));
               last_torp = now;
           }
           else if (W_FindMouseInWin(&x, &y, mapw))
           {
               sendTorpReq(getcourse(mapw, x, y));
               last_torp = now;
           }
       }
   }
}


#ifndef FONT_BITMAPS
W_Icon
planetmBitmap(p)
    register struct planet *p;
{
   int             i;

   if (showgalactic == 2) {
      return (mbplanets[0]);
   } else if (p->pl_info & me->p_team) {
      if (showgalactic == 1) {
	 i = 0;
	 if (p->pl_armies > 4)
	    i += 4;
	 if (p->pl_flags & PLREPAIR)
	    i += 2;
	 if (p->pl_flags & PLFUEL)
	    i += 1;
#ifdef MOOBITMAPS
	 if (myPlanetBitmap)
	    return (mbplanets3[i]);	/* <isae> new bitmaps */
	 else
#endif				/* MOOBITMAPS */
	    return (mbplanets2[i]);
      } else {
	 return (mbplanets[remap[p->pl_owner]]);
      }
   } else {
      return (mbplanets[5]);
   }
}
#endif


void
DrawLocalPlanets()
    /*
     * Draw the local planets on the tacktical map.
     * 
     * This procedure has been created to keep all the planet stuff together and
     * so that separate procedures do not have to be used to select which
     * bitmap should be used for each planet.
     */
{
   register int    my_x = me->p_x, my_y = me->p_y;
   register struct planet *l;
   int             dx, dy;
   int             view;

#ifndef FONT_BITMAPS
   int             iconNum;
   W_Icon          icon;
#endif

   view = SCALE * WINSIDE / 2;
   l = planets + MAXPLANETS;

   while (l != planets) {
      --l;

      dx = l->pl_x - my_x;
      if (dx > view || dx < -view)
	 continue;

      dy = l->pl_y - my_y;
      if (dy > view || dy < -view)
	 continue;

      dx = dx / SCALE + WINSIDE / 2;
      dy = dy / SCALE + WINSIDE / 2;


#ifdef FONT_BITMAPS

      W_MaskText(w, dx - (planet_width / 2), dy - (planet_height / 2),
		 planetColor(l), F_planetChar(l), 1, F_planetFont());

#else				/* no FONT_BITMAPS */

      if (l->pl_info & me->p_team) {
	 if (showlocal == 0)	/* show owner */
	    icon = bplanets[remap[l->pl_owner]];
	 else if (showlocal == 2)	/* show nothing */
	    icon = bplanets[0];
	 else {
	    iconNum = (l->pl_armies > 4) ? 4 : 0;

	    if (l->pl_flags & PLREPAIR)
	       iconNum += 2;
	    if (l->pl_flags & PLFUEL)
	       iconNum += 1;

	    if (showlocal == 1)	/* show resources */
	       icon = bplanets2[iconNum];
	    else		/* showlocal == 3 */
	       /* show rabbit ears */
	       icon = bplanets3[iconNum];
	 }
      } else {			/* No info */
	 icon = bplanets[5];
      }

      W_WriteBitmap(dx - (planet_width / 2), dy - (planet_height / 2),
		    icon, planetColor(l));

#endif				/* no FONT_BITMAPS */


      if (showInd && (l->pl_info & me->p_team) && (l->pl_owner == NOBODY)) {
	 W_CacheLine(w, dx - (planet_width / 2), dy - (planet_height / 2),
		  dx + (planet_width / 2 - 1), dy + (planet_height / 2 - 1),
		     W_White);
	 W_CacheLine(w, dx + (planet_width / 2 - 1), dy - (planet_height / 2),
		     dx - (planet_width / 2), dy + (planet_height / 2 - 1),
		     W_White);
      }
      clearzone[0][clearcount] = dx - (planet_width / 2);
      clearzone[1][clearcount] = dy - (planet_height / 2);
      clearzone[2][clearcount] = planet_width;
      clearzone[3][clearcount] = planet_height;
      clearcount++;

      if (namemode) {
	 int             pl_name_width;
	 int             pl_namelen;

	 if (namemode == 1)
	    /* draw a maximum of 5 characters */
	    pl_namelen = l->pl_namelen < 5 ? l->pl_namelen : 5;
	 else
	    pl_namelen = l->pl_namelen;

	 pl_name_width = W_Textwidth * pl_namelen;

	 W_MaskText(w, dx - (planet_width / 2), dy + (planet_height / 2),
		    planetColor(l), l->pl_name, pl_namelen,
		    planetFont(l));
	 if (pl_name_width <= planet_width) {
	    clearzone[3][clearcount - 1] += W_Textheight;
	 } else {
	    clearzone[0][clearcount] = dx - (planet_width / 2);
	    clearzone[1][clearcount] = dy + (planet_height / 2);
	    clearzone[2][clearcount] = pl_name_width;
	    clearzone[3][clearcount] = W_Textheight;
	    clearcount++;
	 }
      }
   }
}


/* draw out the 'tactical' map */
void
local()
{
   register int    h, i, my_x = me->p_x, my_y = me->p_y;
   register struct player *j;
   register struct torp *k;
   register struct phaser *php;
   register struct plasmatorp *pt;
   static int      tcounter = 2;
   register int    color;
   int             dx, dy;
   int             view;
   char            idbuf[10];

#if !defined(DYNAMIC_BITMAPS) && !defined(FONT_BITMAPS)
   W_Icon(*ship_bits)[VIEWS];
#endif

   if (my_x < 0)
      return;

   /*
    * Kludge to try to fix missing ID chars on tactical (short range)
    * display.
    */
   idbuf[0] = '?';
   idbuf[1] = '\0';

   DrawLocalPlanets();

   /* Draw ships */
   nplayers = 0;
   view = SCALE * WINSIDE / 2;
   for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {

      if (updatePlayer[i]) {	/* NEW */
#ifdef EM
	 if (!sortPlayers)
#endif
	    playerlist3(i);
      }
      if ((j->p_status != PALIVE) && (j->p_status != PEXPLODE))
	 continue;

      /***
      if(j->p_flags & PFOBSERV) 
	continue;	* INL *
      ***/

      /* DBP */
      /* Only draw observer's ship if it's me, I'm locked onto a player, 
	 and the player is cloaked.  For non-cloaked ship, we pick it up
	 seperately when the observed player's ship is drawn. */
      if(j->p_flags & PFOBSERV) {  /* j is an observer */
	if(j != me || !(me->p_flags & PFPLOCK)) /* I am j and player locked */
	  continue;
	else if(!((&players[me->p_playerl])->p_flags & PFCLOAK)) /* cloaked */
	  continue;
      }

      nplayers++;
      if (j->p_flags & PFCLOAK) {
	 if (j->p_cloakphase < (cloak_phases - 1)) {
	    j->p_cloakphase++;
	 }
      } else {
	 if (j->p_cloakphase) {
	    j->p_cloakphase--;
	 }
      }
      dx = j->p_x - my_x;
      if (dx > view || dx < -view) {
	 /* if he's off screen we don't draw phasers anyway */
	 phasers[j->p_no].ph_status = PHFREE;
	 continue;
      }
      dy = j->p_y - my_y;
      if (dy > view || dy < -view) {
	 /* if he's off screen we don't draw phasers anyway */
	 phasers[j->p_no].ph_status = PHFREE;
	 continue;
      }
      dx = dx / SCALE + WINSIDE / 2;
      dy = dy / SCALE + WINSIDE / 2;

      if (j->p_flags & PFCLOAK && (j->p_cloakphase == (cloak_phases - 1))) {
	 if (myPlayer(j)) {
#ifdef FONT_BITMAPS
	    W_MaskText(w, dx - (cloak_width / 2), dy - (cloak_height / 2),
		       myColor, F_cloakChar(), 1, F_cloakFont());
#else
	    W_WriteBitmap(dx - (cloak_width / 2), dy - (cloak_height / 2),
			  cloakicon, myColor);
#endif
	    clearzone[0][clearcount] = dx - (shield_width / 2);
	    clearzone[1][clearcount] = dy - (shield_height / 2);
	    clearzone[2][clearcount] = shield_width;
	    clearzone[3][clearcount] = shield_height;
	    clearcount++;

	    /* hmm... I guess this is ok now */
	    if (showShields && (me->p_flags & PFSHIELD)) {
	       int             shieldnum;
	       if (VShieldBitmaps) {
		  shieldnum = SHIELD_FRAMES * me->p_shield / me->p_ship.s_maxshield
		     ;
		  if (shieldnum >= SHIELD_FRAMES)
		     shieldnum = SHIELD_FRAMES - 1;
		  color = gColor;
		  if (shieldnum < SHIELD_FRAMES * 2 / 3) {
		     color = yColor;
		     if (shieldnum < SHIELD_FRAMES * 2 / 3) {
			color = rColor;
		     }
		  }
	       } else {
		  color = playerColor(me);
		  shieldnum = 2;
	       }
#ifdef FONT_BITMAPS
	       W_MaskText(w, dx - (shield_width / 2), dy - (shield_height / 2),
			  color, F_shieldChar(shieldnum), 1, F_shieldFont());
#else
	       W_WriteBitmap(dx - (shield_width / 2),
			dy - (shield_height / 2), shield[shieldnum], color);
#endif
	    }
	 }
	 continue;
      }
      if (j->p_status == PALIVE) {
#if !defined(DYNAMIC_BITMAPS) && !defined(FONT_BITMAPS)
	 switch (j->p_team) {
	 case FED:
	    ship_bits = fed_bitmaps;
	    break;
	 case ROM:
	    ship_bits = rom_bitmaps;
	    break;
	 case KLI:
	    ship_bits = kli_bitmaps;
	    break;
	 case ORI:
	    ship_bits = ori_bitmaps;
	    break;
	 default:
	    ship_bits = ind_bitmaps;
	    break;
	 }
#endif

	 clearzone[0][clearcount] = dx - (shield_width / 2);
	 clearzone[1][clearcount] = dy - (shield_height / 2);
	 clearzone[2][clearcount] = shield_width;
	 clearzone[3][clearcount] = shield_height;
	 /* we wait til after drawing text number to increment clearcount */

#ifdef FONT_BITMAPS
#ifdef HOCKEY
	 if (j->p_ispuck)
	    W_MaskText(w, dx - (shield_width / 2), dy - (shield_width / 2),
		     playerColor(j), F_puckChar(j->p_dir), 1, F_puckFont());
	 else
#endif
	    W_MaskText(w, dx - (shield_width / 2), dy - (shield_width / 2),
		       playerColor(j),
		       F_shipChar(j->p_team, j->p_ship.s_type, j->p_dir),
		       1, F_playerFont(j->p_team));
#else
#ifdef HOCKEY
	 if (j->p_ispuck)
	    W_WriteBitmap(dx - (j->p_ship.s_width / 2),
			  dy - (j->p_ship.s_height / 2),
			  puck_bitmaps[rosette(j->p_dir)], playerColor(j));
	 else
#endif
	    W_WriteBitmap(dx - (j->p_ship.s_width / 2),
			  dy - (j->p_ship.s_height / 2),
#ifndef DYNAMIC_BITMAPS
			  ship_bits[j->p_ship.s_type][rosette(j->p_dir)],
#else
			  ship_bitmaps[PlayerBitmap(j)][rosette(j->p_dir)],
#endif
			  playerColor(j));
#endif				/* FONT_BITMAPS */
	 if (j->p_cloakphase > 0) {
#ifdef FONT_BITMAPS
	    W_MaskText(w, dx - (cloak_height / 2), dy - (cloak_width / 2),
		       playerColor(j), F_cloakChar(), 1, F_cloakFont());
#else
	    W_WriteBitmap(dx - (cloak_width / 2),
			dy - (cloak_height / 2), cloakicon, playerColor(j));
#endif
	    clearcount++;
	    continue;
	 }
	 if (showShields && (j->p_flags & PFSHIELD)) {
	    int             shieldnum;
	    if (j == me && VShieldBitmaps) {
	       shieldnum = SHIELD_FRAMES * me->p_shield / me->p_ship.s_maxshield;
	       if (shieldnum >= SHIELD_FRAMES)
		  shieldnum = SHIELD_FRAMES - 1;
	       color = gColor;
	       if (shieldnum < SHIELD_FRAMES * 2 / 3) {
		  color = yColor;
		  if (shieldnum < SHIELD_FRAMES * 2 / 3) {
		     color = rColor;
		  }
	       }
	    } else {
	       color = playerColor(j);
	       shieldnum = 2;
	    }
#ifdef FONT_BITMAPS
	    W_MaskText(w, dx - (shield_width / 2), dy - (shield_height / 2),
		       color, F_shieldChar(shieldnum), 1, F_shieldFont());
#else
	    W_WriteBitmap(dx - (shield_width / 2),
			dy - (shield_height / 2), shield[shieldnum], color);
#endif				/* FONT_BITMAPS */
	 } {
	    if (j == me)
		sprintf(idbuf, "%d", j->p_speed);
	    else
	    {
	    	idbuf[0] = *(shipnos + j->p_no);
	    	idbuf[1] = '\0';
	    }
	    color = playerColor(j);

	    if (j == me) {
	       switch (me->p_flags & (PFGREEN | PFYELLOW | PFRED)) {
	       case PFGREEN:
		  color = gColor;
		  break;
	       case PFYELLOW:
		  color = yColor;
		  break;
	       case PFRED:
		  color = rColor;
		  break;
	       }
	    }
#ifdef HOCKEY
	    if (!j->p_ispuck)
#endif
	       W_MaskText(w, dx + (j->p_ship.s_width / 2),
			  dy - (j->p_ship.s_height / 2), color,
			  idbuf, strlen(idbuf), shipFont(j));

	    /* save a clearzone */
	    clearzone[2][clearcount] += W_Textwidth * strlen(idbuf);
	    clearcount++;
	 }
      } else if (j->p_status == PEXPLODE) {
	 int             i;

	 i = j->p_explode;
	 if (i < EX_FRAMES || (i < SBEXPVIEWS && j->p_ship.s_type == STARBASE)) {

	    if (j->p_ship.s_type == STARBASE) {
#ifdef FONT_BITMAPS
	       W_MaskText(w, dx - (sbexp_width / 2), dy - (sbexp_height / 2),
			playerColor(j), F_expSbChar(i), 1, F_explodeFont());
#else
	       W_WriteBitmap(dx - (sbexp_width / 2),
			     dy - (sbexp_height / 2), sbexpview[i],
			     playerColor(j));
#endif
	       clearzone[0][clearcount] = dx - (sbexp_width / 2);
	       clearzone[1][clearcount] = dy - (sbexp_height / 2);
	       clearzone[2][clearcount] = sbexp_width;
	       clearzone[3][clearcount] = sbexp_height;
	    } else {
#ifdef FONT_BITMAPS
	       W_MaskText(w, dx - (ex_width / 2), dy - (ex_height / 2),
			  playerColor(j), F_expChar(i), 1, F_explodeFont());
#else
	       W_WriteBitmap(dx - (ex_width / 2), dy - (ex_height / 2),
			     expview[i], playerColor(j));
#endif
	       clearzone[0][clearcount] = dx - (ex_width / 2);
	       clearzone[1][clearcount] = dy - (ex_height / 2);
	       clearzone[2][clearcount] = ex_width;
	       clearzone[3][clearcount] = ex_height;
	    }
	    clearcount++;
	    j->p_explode++;
	 }
#ifdef DROP_FIX
	 /* done exploding.  Must be off screen */
	 else if (drop_fix && j != me
	    /* special cse for critters */
		  && j->p_ship.s_type != ATT) {
	    j->p_x = -100000;
	    j->p_y = -100000;
	 }
#endif
      }
      /* Now draw his phaser (if it exists) */
      php = &phasers[j->p_no];

      if (php->ph_status != PHFREE) {
	 register int    tx, ty;
	 int             draw_ph = 1;

	 switch (php->ph_status) {
	 case PHMISS:
	    /* Here I will have to compute end coordinate */
	    tx = j->p_x + PHASEDIST * j->p_ship.s_phaserdamage / 100 * Cos[(unsigned char)php->ph_dir];
	    ty = j->p_y + PHASEDIST * j->p_ship.s_phaserdamage / 100 * Sin[(unsigned char)php->ph_dir];
	    tx = (tx - my_x) / SCALE + WINSIDE / 2;
	    ty = (ty - my_y) / SCALE + WINSIDE / 2;
#ifdef DROP_FIX
	    if (
#ifdef FEATURE
		  !F_phaser_multi_send &&
#endif
		  drop_fix) {
	       if (udcounter - php->ph_lastupdate > 2 * updates_per_second) {
		  /*
		   * dmessage("missed phaser freed", MTEAM+MVALID, me->p_no,
		   * 0);
		   */
		  php->ph_status = PHFREE;
		  draw_ph = 0;
	       }
	    }
#endif
	    break;

	 case PHHIT2:
	    tx = (php->ph_x - my_x) / SCALE + WINSIDE / 2;
	    ty = (php->ph_y - my_y) / SCALE + WINSIDE / 2;
#ifdef DROP_FIX
	    if (
#ifdef FEATURE
		  !F_phaser_multi_send &&
#endif
		  drop_fix) {
	       if (udcounter - php->ph_lastupdate > 2 * updates_per_second) {
		  /*
		   * dmessage("phhit2 phaser freed", MTEAM+MVALID, me->p_no,
		   * 0);
		   */
		  php->ph_status = PHFREE;
		  draw_ph = 0;
	       }
	    }
#endif
	    break;
	 default:
	    /* Start point is dx, dy */
	    {
	       struct player  *targ = &players[php->ph_target];

	       tx = (targ->p_x - my_x) / SCALE + WINSIDE / 2;
	       ty = (targ->p_y - my_y) / SCALE + WINSIDE / 2;

#ifdef DROP_FIX
	       if (tx < 0 || ty < 0) {
		  php->ph_status = PHFREE;
		  draw_ph = 0;
	       } else if (
#ifdef FEATURE
			    !F_phaser_multi_send &&
#endif
			    drop_fix) {
		  /*
		   * here's the heuristic -- is the phaser too long to be
		   * real?  Don't draw it (we rely on phaserdamage here)
		   */
		  if (ihypot((j->p_x - targ->p_x), (j->p_y - targ->p_y)) >
		      PHASEDIST * j->p_ship.s_phaserdamage / 100 &&
		  udcounter - php->ph_lastupdate > 2 * updates_per_second) {

		     /*
		      * dmessage("too long phaser freed", MTEAM+MVALID,
		      * me->p_no, 0);
		      */
		     php->ph_status = PHFREE;
		     draw_ph = 0;
		  }
	       }
#endif


#ifdef nodef
	       /* if this player is also phasering j, draw differently */
	       if (phasers[targ->p_no].ph_status == PHHIT &&
		   phasers[targ->p_no].ph_target == j->p_no)
		  duel = 1;
#endif
	    }
	    break;
	 }

	 if (tx == dx && ty == dy)
	    draw_ph = 0;

	 if (draw_ph) {
	    int             new_dx = dx, new_dy = dy;
	    if (php->ph_status == PHMISS || ((php->ph_fuse % 2) == 0) ||
		!friendlyPhaser(php))
	       color = shipCol[remap[j->p_team]];
	    else
	       color = foreColor;

	    /* BRM */
	    if (enemyPhasers && !friendlyPhaser(php)) {
	       register int    wx, wy, lx, ly;

#ifdef PHASER_SHRINK
	       if (shrink_phasers && php->ph_status != PHMISS) {
		  get_shrink_phaser_coords(&new_dx, &new_dy,
			     dx, dy, tx, ty, php->ph_fuse, php->ph_maxfuse);
	       }
#endif

	       wx = new_dx + enemyPhasers * Cos[(unsigned char) (php->ph_dir + 64)];
	       wy = new_dy + enemyPhasers * Sin[(unsigned char) (php->ph_dir + 64)];
	       lx = new_dx + enemyPhasers * Cos[(unsigned char) (php->ph_dir - 64)];
	       ly = new_dy + enemyPhasers * Sin[(unsigned char) (php->ph_dir - 64)];
	       W_CacheLine(w, wx, wy, tx, ty, color);

	       clearline[0][clearlcount] = wx;
	       clearline[1][clearlcount] = wy;
	       clearline[2][clearlcount] = tx;
	       clearline[3][clearlcount] = ty;
	       clearlcount++;

	       W_CacheLine(w, lx, ly, tx, ty, color);

	       clearline[0][clearlcount] = lx;
	       clearline[1][clearlcount] = ly;
	       clearline[2][clearlcount] = tx;
	       clearline[3][clearlcount] = ty;
	       clearlcount++;
	    } else {
#ifdef PHASER_SHRINK
	       if (shrink_phasers && php->ph_status != PHMISS) {
		  get_shrink_phaser_coords(&new_dx, &new_dy,
			     dx, dy, tx, ty, php->ph_fuse, php->ph_maxfuse);
		  /*
		   * if(php->ph_status == PHHIT) printf("%d, %d, %d, %d,
		   * ph_fuse %d\n", new_dx, new_dy, tx, ty, php->ph_fuse);
		   */
	       }
#endif
	       W_CacheLine(w, new_dx, new_dy, tx, ty, color);

	       clearline[0][clearlcount] = new_dx;
	       clearline[1][clearlcount] = new_dy;
	       clearline[2][clearlcount] = tx;
	       clearline[3][clearlcount] = ty;
	       clearlcount++;
	    }
	    php->ph_fuse++;
#if 0
/* TMP */
printf("fuse now at %d.\n", php->ph_fuse);
#endif
	    if (php->ph_fuse > php->ph_maxfuse + 1) {
#ifdef FEATURE
	       if (F_phaser_multi_send){
		  php->ph_status = PHFREE;
		  /* TMP */
#if 0
		  printf("freeing phaser.\n");
#endif
	       }
#endif
#ifdef PHASER_SHRINK
	       php->ph_fuse = 0;
#endif
	    }
	 }
      }
      /* ATM - show tractor/pressor beams (modified by James Collins) */
      /* showTractorPressor is a variable set by xtrekrc. */
      if (showTractorPressor) {
	 if (j == me && (j->p_flags & (PFTRACT | PFPRESS)) && j->p_status == PALIVE) {
	    double          theta;
	    unsigned char   dir;
	    int             lx[2], ly[2], px, py, target_width;

	    struct player  *tractee;
	    if (me->p_tractor < 0 || me->p_tractor >= MAXPLAYER)
	       continue;
	    tractee = &players[me->p_tractor];
	    if (tractee->p_status != PALIVE ||
		((tractee->p_flags & PFCLOAK) && (tractee->p_cloakphase == (cloak_phases - 1))))
	       continue;
	    if (tcounter >= 2) {/* continue tractor stuff */
	       if (!continueTractor)
		  tcounter--;
	       px = (players[me->p_tractor].p_x - my_x) / SCALE + WINSIDE / 2;
	       py = (players[me->p_tractor].p_y - my_y) / SCALE + WINSIDE / 2;
	       if (px == dx && py == dy)
		  continue;	/* this had better be last in for(..) */
	       theta = atan2((double) (px - dx), (double) (dy - py)) + XPI / 2.0;
	       dir = (unsigned char) (theta / XPI * 128.0);
	       if (tractee->p_flags & PFSHIELD)
		  target_width = shield_width;
	       else {
		  target_width = tractee->p_ship.s_width / 2;
	       }
	       lx[0] = px + (Cos[dir] * (target_width / 2));
	       ly[0] = py + (Sin[dir] * (target_width / 2));
	       lx[1] = px - (Cos[dir] * (target_width / 2));
	       ly[1] = py - (Sin[dir] * (target_width / 2));
	       if (j->p_flags & PFPRESS) {
		  W_MakeTractLine(w, dx, dy, lx[0], ly[0], W_Yellow);
		  W_MakeTractLine(w, dx, dy, lx[1], ly[1], W_Yellow);
	       } else {
		  W_MakeTractLine(w, dx, dy, lx[0], ly[0], W_Green);
		  W_MakeTractLine(w, dx, dy, lx[1], ly[1], W_Green);
	       }
	       cleartpline[0][cleartplcount] = dx;
	       cleartpline[1][cleartplcount] = dy;
	       cleartpline[2][cleartplcount] = lx[1];
	       cleartpline[3][cleartplcount] = ly[1];
	       cleartplcount++;
	       cleartpline[0][cleartplcount] = dx;
	       cleartpline[1][cleartplcount] = dy;
	       cleartpline[2][cleartplcount] = lx[0];
	       cleartpline[3][cleartplcount] = ly[0];
	       cleartplcount++;
	    } else
	       tcounter = 2;
	 }
      }
   }
   /* Draw torps */
   view = SCALE * WINSIDE / 2;
   for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
      if (!j->p_ntorp)
	 continue;
      for (h = 0, k = &torps[MAXTORP * i + h]; h < MAXTORP; h++, k++) {
	 if (!k->t_status)
	    continue;

#ifdef DROP_FIX
	 if (drop_fix) {
	    /* haven't heard anything on this torp in 1 seconds */
	    if (udcounter - k->t_lastupdate > 1 * updates_per_second) {
	       /*
	        * dmessage("torp freed", MTEAM+MVALID, me->p_no, 0);
	        */
	       k->t_status = TFREE;
	       j->p_ntorp--;
	       continue;
	    }
	 }
#endif

	 dx = k->t_x - my_x;
	 if (dx > view || dx < -view) {
	    /* Call any torps off screen "free" (if owned by other) */
	    if (k->t_status == TEXPLODE && j != me) {
	       k->t_status = TFREE;
	       j->p_ntorp--;
	    }
	    continue;
	 }
	 dy = k->t_y - my_y;
	 if (dy > view || dy < -view) {
	    /* Call any torps off screen "free" (if owned by other) */
	    if (k->t_status == TEXPLODE && j != me) {
	       k->t_status = TFREE;
	       j->p_ntorp--;
	    }
	    continue;
	 }
	 color = torpColor(k);
	 dx = dx / SCALE + WINSIDE / 2;
	 dy = dy / SCALE + WINSIDE / 2;
	 if (k->t_status == TEXPLODE) {
	    k->t_fuse--;
	    if (k->t_fuse <= 0) {
	       k->t_status = TFREE;
	       j->p_ntorp--;
	       continue;
	    }
	    /*
	     * if (k->t_fuse >= NUMDETFRAMES) { k->t_fuse = NUMDETFRAMES - 1;
	     * }
	     */

#ifdef FONT_BITMAPS
	    W_MaskText(w, dx - (cloud_width / 2), dy - (cloud_height / 2),
		     color, F_expTorpChar(k->t_fuse - 1), 1, F_smallFont());
#else
	    W_WriteBitmap(dx - (cloud_width / 2), dy - (cloud_height / 2),
			  cloud[k->t_fuse - 1], color);
#endif
	    clearzone[0][clearcount] = dx - (cloud_width / 2);
	    clearzone[1][clearcount] = dy - (cloud_height / 2);
	    clearzone[2][clearcount] = cloud_width;
	    clearzone[3][clearcount] = cloud_height;
	    clearcount++;
	 } else if (k->t_owner != me->p_no && ((k->t_war & me->p_team) ||
	     (players[k->t_owner].p_team & (me->p_hostile | me->p_swar)))) {
	    /*
	     * solid.  Looks strange. W_FillArea(w, dx - (etorp_width/2), dy
	     * - (etorp_height/2), etorp_width, etorp_height, torpColor(k));
	     */

#ifndef BD
	    W_CacheLine(w, dx - (etorp_width / 2), dy - (etorp_height / 2),
		    dx + (etorp_width / 2), dy + (etorp_height / 2), color);
	    W_CacheLine(w, dx + (etorp_width / 2), dy - (etorp_height / 2),
		    dx - (etorp_width / 2), dy + (etorp_height / 2), color);
#else
	    /* BORG TEST */
	    W_CacheLine(w, dx - (etorp_width / 2), dy - (etorp_height / 2),
			dx + (etorp_width / 2), dy + (etorp_height / 2),
			(k->t_turns == 32767) ? notColor(k) : torpColor(k));
	    W_CacheLine(w, dx + (etorp_width / 2), dy - (etorp_height / 2),
			dx - (etorp_width / 2), dy + (etorp_height / 2),
			(k->t_turns == 32767) ? notColor(k) : torpColor(k));
#endif

	    /*
	     * W_WriteBitmap(dx - (etorp_width/2), dy - (etorp_height/2),
	     * etorp, torpColor(k));
	     */
	    clearzone[0][clearcount] = dx - (etorp_width / 2);
	    clearzone[1][clearcount] = dy - (etorp_height / 2);
	    clearzone[2][clearcount] = etorp_width;
	    clearzone[3][clearcount] = etorp_height;
	    clearcount++;
	 } else {

	    W_CacheLine(w, dx - (mtorp_width / 2), dy, dx + (mtorp_width / 2), dy,
			color);
	    W_CacheLine(w, dx, dy - (mtorp_width / 2), dx, dy + (mtorp_width / 2),
			color);

	    /*
	     * W_WriteBitmap(dx - (mtorp_width/2), dy - (mtorp_height/2),
	     * mtorp, color);
	     */
	    clearzone[0][clearcount] = dx - (mtorp_width / 2);
	    clearzone[1][clearcount] = dy - (mtorp_height / 2);
	    clearzone[2][clearcount] = mtorp_width;
	    clearzone[3][clearcount] = mtorp_height;
	    clearcount++;
	 }
      }
   }
   /* Draw plasma torps */
   view = SCALE * WINSIDE / 2;
   for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
      if (!j->p_nplasmatorp)
	 continue;
      for (h = 0, pt = &plasmatorps[MAXPLASMA * i + h]; h < MAXPLASMA; h++, pt++) {
	 if (!pt->pt_status)
	    continue;
#ifdef DROP_FIX
	 if (drop_fix) {
	    /* haven't heard anything on this torp in 1 seconds */
	    if (udcounter - pt->pt_lastupdate > 1 * updates_per_second) {
	       /*
	        * dmessage("ptorp freed", MTEAM+MVALID, me->p_no, 0);
	        */
	       pt->pt_status = TFREE;
	       j->p_nplasmatorp--;
	       continue;
	    }
	 }
#endif

	 color = plasmatorpColor(pt);

	 dx = pt->pt_x - my_x;
	 if (dx > view || dx < -view)
	    continue;
	 dy = pt->pt_y - my_y;
	 if (dy > view || dy < -view)
	    continue;
	 dx = dx / SCALE + WINSIDE / 2;
	 dy = dy / SCALE + WINSIDE / 2;
	 if (pt->pt_status == PTEXPLODE) {
	    pt->pt_fuse--;
	    if (pt->pt_fuse <= 0) {
	       pt->pt_status = PTFREE;
#ifdef DROP_FIX
	       pt->pt_last_info_update = udcounter;	/* xx */
#endif
	       j->p_nplasmatorp--;
	       continue;
	    }
	    /*
	     * if (pt->pt_fuse >= NUMDETFRAMES) { pt->pt_fuse = NUMDETFRAMES
	     * - 1; }
	     */

#ifdef FONT_BITMAPS
	    W_MaskText(w, dx - (plasmacloud_width / 2),
		       dy - (plasmacloud_height / 2),
		       color,
		  F_expPlasmaTorpChar(pt->pt_fuse - 1), 1, F_explodeFont());
#else
	    W_WriteBitmap(dx - (plasmacloud_width / 2),
			  dy - (plasmacloud_height / 2),
			  plasmacloud[pt->pt_fuse - 1], color);
#endif
	    clearzone[0][clearcount] = dx - (plasmacloud_width / 2);
	    clearzone[1][clearcount] = dy - (plasmacloud_height / 2);
	    clearzone[2][clearcount] = plasmacloud_width;
	    clearzone[3][clearcount] = plasmacloud_height;
	    clearcount++;
	 }
	 /* needmore: if(pt->pt_war & me->p_team) */
	 else if (pt->pt_owner != me->p_no && ((pt->pt_war & me->p_team) ||
	   (players[pt->pt_owner].p_team & (me->p_hostile | me->p_swar)))) {
#ifdef FONT_BITMAPS
	    W_MaskText(w, dx - (eplasmatorp_width / 2),
		       dy - (eplasmatorp_height / 2),
		       color,
		       F_plasmaTorpEnemyChar(), 1, F_smallFont());
#else
	    W_WriteBitmap(dx - (eplasmatorp_width / 2),
			  dy - (eplasmatorp_height / 2),
			  eplasmatorp, color);
#endif
	    clearzone[0][clearcount] = dx - (eplasmatorp_width / 2);
	    clearzone[1][clearcount] = dy - (eplasmatorp_height / 2);
	    clearzone[2][clearcount] = eplasmatorp_width;
	    clearzone[3][clearcount] = eplasmatorp_height;
	    clearcount++;
	 } else {
#ifdef FONT_BITMAPS
	    W_MaskText(w, dx - (mplasmatorp_width / 2),
		       dy - (mplasmatorp_height / 2),
		       color,
		       F_plasmaTorpChar(), 1, F_smallFont());
#else
	    W_WriteBitmap(dx - (mplasmatorp_width / 2),
			  dy - (mplasmatorp_height / 2),
			  mplasmatorp, color);
#endif
	    clearzone[0][clearcount] = dx - (mplasmatorp_width / 2);
	    clearzone[1][clearcount] = dy - (mplasmatorp_height / 2);
	    clearzone[2][clearcount] = mplasmatorp_width;
	    clearzone[3][clearcount] = mplasmatorp_height;
	    clearcount++;
	 }
      }
   }
   /* Draw Edges */
   if (my_x < (WINSIDE / 2) * SCALE) {
      int             sy, ey;

      dx = (WINSIDE / 2) - (my_x) / SCALE;
      sy = (WINSIDE / 2) + (0 - my_y) / SCALE;
      ey = (WINSIDE / 2) + (GWIDTH - my_y) / SCALE;
      if (sy < 0)
	 sy = 0;
      if (ey > WINSIDE - 1)
	 ey = WINSIDE - 1;
      W_CacheLine(w, dx, sy, dx, ey, warningColor);
      /*
       * W_MakeLine(w, dx, sy, dx, ey, warningColor);
       */
      clearline[0][clearlcount] = dx;
      clearline[1][clearlcount] = sy;
      clearline[2][clearlcount] = dx;
      clearline[3][clearlcount] = ey;
      clearlcount++;
   }
   if ((GWIDTH - my_x) < (WINSIDE / 2) * SCALE) {
      int             sy, ey;

      dx = (WINSIDE / 2) + (GWIDTH - my_x) / SCALE;
      sy = (WINSIDE / 2) + (0 - my_y) / SCALE;
      ey = (WINSIDE / 2) + (GWIDTH - my_y) / SCALE;
      if (sy < 0)
	 sy = 0;
      if (ey > WINSIDE - 1)
	 ey = WINSIDE - 1;
      W_CacheLine(w, dx, sy, dx, ey, warningColor);
      /*
       * W_MakeLine(w, dx, sy, dx, ey, warningColor);
       */
      clearline[0][clearlcount] = dx;
      clearline[1][clearlcount] = sy;
      clearline[2][clearlcount] = dx;
      clearline[3][clearlcount] = ey;
      clearlcount++;
   }
   if (my_y < (WINSIDE / 2) * SCALE) {
      int             sx, ex;

      dy = (WINSIDE / 2) - (my_y) / SCALE;
      sx = (WINSIDE / 2) + (0 - my_x) / SCALE;
      ex = (WINSIDE / 2) + (GWIDTH - my_x) / SCALE;
      if (sx < 0)
	 sx = 0;
      if (ex > WINSIDE - 1)
	 ex = WINSIDE - 1;
      W_CacheLine(w, sx, dy, ex, dy, warningColor);
      /*
       * W_MakeLine(w, sx, dy, ex, dy, warningColor);
       */
      clearline[0][clearlcount] = sx;
      clearline[1][clearlcount] = dy;
      clearline[2][clearlcount] = ex;
      clearline[3][clearlcount] = dy;
      clearlcount++;
   }
   if ((GWIDTH - my_y) < (WINSIDE / 2) * SCALE) {
      int             sx, ex;

      dy = (WINSIDE / 2) + (GWIDTH - my_y) / SCALE;
      sx = (WINSIDE / 2) + (0 - my_x) / SCALE;
      ex = (WINSIDE / 2) + (GWIDTH - my_x) / SCALE;
      if (sx < 0)
	 sx = 0;
      if (ex > WINSIDE - 1)
	 ex = WINSIDE - 1;
      W_CacheLine(w, sx, dy, ex, dy, warningColor);
      /*
       * W_MakeLine(w, sx, dy, ex, dy, warningColor);
       */
      clearline[0][clearlcount] = sx;
      clearline[1][clearlcount] = dy;
      clearline[2][clearlcount] = ex;
      clearline[3][clearlcount] = dy;
      clearlcount++;
   }
   /* Change border color to signify alert status */

   if (oldalert != (me->p_flags & (PFGREEN | PFYELLOW | PFRED))) {
      oldalert = (me->p_flags & (PFGREEN | PFYELLOW | PFRED));
      switch (oldalert) {
      case PFGREEN:
	 if (extraBorder)
	    W_ChangeBorder(w, gColor);
	 W_ChangeBorder(baseWin, gColor);
	 W_ChangeBorder(iconWin, gColor);
	 break;
      case PFYELLOW:
	 if (extraBorder)
	    W_ChangeBorder(w, yColor);
	 W_ChangeBorder(baseWin, yColor);
	 W_ChangeBorder(iconWin, yColor);
	 break;
      case PFRED:
	 if (extraBorder)
	    W_ChangeBorder(w, rColor);
	 W_ChangeBorder(baseWin, rColor);
	 W_ChangeBorder(iconWin, rColor);
	 break;
      }
   }
   /* show 'lock' icon on local map (Actually an EM hack ) */
   if (showLock & 2) {
      int             tri_x = -1, tri_y = -1, facing = 0;
      int             tri_size = 4;
      view = SCALE * WINSIDE / 2;	/** just in case it's changed */
      if (me->p_flags & PFPLOCK) {
	 /* locked onto a ship */
	 j = &players[me->p_playerl];
	 if (!(j->p_flags & PFCLOAK)) {
	    dx = j->p_x - my_x;
	    dy = j->p_y - my_y;
	    if (ABS(dx) < view && ABS(dy) < view) {
	       dx = dx / SCALE + WINSIDE / 2;
	       dy = dy / SCALE + WINSIDE / 2;
	       tri_x = dx + 0;
	       tri_y = dy + 20;	/* below ship */
	       facing = 1;
	    }
	    /* printf("Drawing local triangle at %d %d\n", tri_x, tri_y); */
	 }
      } else if (me->p_flags & PFPLLOCK) {
	 /* locked onto a planet */
	 struct planet  *l = &planets[me->p_planet];
	 dx = l->pl_x - my_x;
	 dy = l->pl_y - my_y;
	 if (ABS(dx) < view && ABS(dy) < view) {
	    dx = dx / SCALE + WINSIDE / 2;
	    dy = dy / SCALE + WINSIDE / 2;
	    tri_x = dx;
	    tri_y = dy - 20;	/* below planet */
	    facing = 0;
	 }
	 /* printf("Drawing local triangle at %d %d\n", tri_x, tri_y); */
      }
      if (tri_x != -1) {
	 W_WriteTriangle(w, tri_x, tri_y, 4, facing, foreColor);
	 clearzone[0][clearcount] = tri_x - tri_size - 1;
	 clearzone[1][clearcount] = tri_y - 1 +
	    (facing ? 0 : -tri_size);
	 clearzone[2][clearcount] = tri_size * 2 + 2;
	 clearzone[3][clearcount] = tri_size + 2;
	 clearcount++;
      }
   }
}

/* map out the 'galactic' map */
void
map()
{
   register int    i;
   register struct player *j;
   register struct planet *l;
   register int    dx, dy, odx, ody;
   char plname[4];

   if (redrawall == 1) {
      W_ClearWindow(mapw);
   }
   /* Erase ships */
   else {
      for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
	 lastUpdate[i]++;
#ifdef HOCKEY
	 if (j->p_ispuck) {
	    /* always draw puck for mapmode frequently */
	    redrawPlayer[i] = 1;
	 }
#endif
	 /*
	  * Erase the guy if: redrawPlayer[i] is set and the mapmode setting
	  * allows it.
	  */
	 if (!redrawPlayer[i] || (mapmode == 1 && lastUpdate[i] < 5))
	    continue;
	 lastUpdate[i] = 0;
	 /* Clear his old image... */
	 if (mclearzone[2][i]) {
	    W_CacheClearArea(mapw, mclearzone[0][i], mclearzone[1][i],
			     mclearzone[2][i], mclearzone[3][i]);
	    /* Redraw the hole just left next update */
	    checkRedraw(mclearzone[4][i], mclearzone[5][i]);
	    mclearzone[2][i] = 0;
	 }
      }
      W_FlushClearAreaCache(mapw);
      /* this also clears planet lock triangle */
      if (clearlmcount) {
	 W_WriteTriangle(mapw, clearlmark[0], clearlmark[1], clearlmark[2],
			 clearlmark[3], backColor);
	 clearlmcount = 0;
      }
   }

   /* Draw Planets */
   for (i = 0, l = &planets[i]; i < MAXPLANETS; i++, l++) {


#if defined( FOR_MORONS )
      /* draw flashing informative text above the planets */
      if (For_Morons && FlashChange) {
	 dx = l->pl_x * WINSIDE / GWIDTH;
	 dy = l->pl_y * WINSIDE / GWIDTH;

	 if (mclearzone[2][i + MAXPLAYER]) {
	    W_ClearArea(mapw, mclearzone[0][i + MAXPLAYER],
		 mclearzone[1][i + MAXPLAYER], mclearzone[2][i + MAXPLAYER],
			mclearzone[3][i + MAXPLAYER]);
	    /* checkRedraw() -- ONly needed for ship redraws! */
	    mclearzone[2][i + MAXPLAYER] = 0;
	 }
	 if (friendlyPlanet(l)) {
	    if (me->p_kills && myPlanet(l) && l->pl_armies > 4
		&& me->p_armies < me->p_ship.s_maxarmies) {
	       W_WriteText(mapw, dx - 5 * W_Textwidth,
			   dy + (mplanet_height / 2) + W_Textheight,
			   planetColor(l), "GET ARMIES", 10,
			   BOLD_ON ? W_BoldFont : W_RegularFont);
	       mclearzone[0][i + MAXPLAYER] = dx - W_Textwidth * 5;
	       mclearzone[1][i + MAXPLAYER] = dy + (mplanet_height / 2)
		  + W_Textheight;
	       mclearzone[2][i + MAXPLAYER] = W_Textwidth * 10;
	       mclearzone[3][i + MAXPLAYER] = W_Textheight;
	    }
	 } else if (l->pl_info & me->p_team) {
	    if (l->pl_armies > 4 && RealNumShips(l->pl_owner) > 1) {
	       /* BOMB ME */
	       W_WriteText(mapw, dx - W_Textwidth * 4,
			   dy + (mplanet_height / 2) + W_Textheight,
			   planetColor(l), "BOMB HERE", 9,
			   BOLD_ON ? W_BoldFont : W_RegularFont);
	       mclearzone[0][i + MAXPLAYER] = dx - W_Textwidth * 4;
	       mclearzone[1][i + MAXPLAYER] = dy + (mplanet_height / 2)
		  + W_Textheight;
	       mclearzone[2][i + MAXPLAYER] = W_Textwidth * 9;
	       mclearzone[3][i + MAXPLAYER] = W_Textheight;
	    } else if (me->p_armies >= l->pl_armies) {
	       /* DROP_ARMIES */
	       W_WriteText(mapw, dx - W_Textwidth * 5,
			   dy + (mplanet_height / 2) + W_Textheight,
			   planetColor(l), "DROP ARMIES", 11,
			   BOLD_ON ? W_BoldFont : W_RegularFont);
	       mclearzone[0][i + MAXPLAYER] = dx - W_Textwidth * 5;
	       mclearzone[1][i + MAXPLAYER] = dy + (mplanet_height / 2)
		  + W_Textheight;
	       mclearzone[2][i + MAXPLAYER] = W_Textwidth * 11;
	       mclearzone[3][i + MAXPLAYER] = W_Textheight;
	    }
	 } else {
	    /* ORBIT ME */
	    W_WriteText(mapw, dx - W_Textwidth * 4,
			dy + (mplanet_height / 2) + W_Textheight,
			planetColor(l), "ORBIT ME", 8,
			BOLD_ON ? W_BoldFont : W_RegularFont);
	    mclearzone[0][i + MAXPLAYER] = dx - W_Textwidth * 4;
	    mclearzone[1][i + MAXPLAYER] = dy + (mplanet_height / 2)
	       + W_Textheight;
	    mclearzone[2][i + MAXPLAYER] = W_Textwidth * 8;
	    mclearzone[3][i + MAXPLAYER] = W_Textheight;
	 }
      }
#endif				/* FOR_MORONS */

      if (!(l->pl_flags & PLREDRAW) && (!redrawall))
	 continue;
      l->pl_flags &= ~PLREDRAW;	/* Turn redraw flag off! */
      dx = l->pl_x * WINSIDE / GWIDTH;
      dy = l->pl_y * WINSIDE / GWIDTH;

      /* moving planets */
      if (pl_update[l->pl_no].plu_update == 1) {

/*
printf("planet moved. (%d, %d)\n", odx - dx, ody - dy);
*/
	 odx = pl_update[l->pl_no].plu_x * WINSIDE / GWIDTH;
	 ody = pl_update[l->pl_no].plu_y * WINSIDE / GWIDTH;

	 W_ClearArea(mapw, odx - (mplanet_width / 2), ody - (mplanet_height / 2),
		     mplanet_width, mplanet_height+W_Textheight);
#if 0
	 W_WriteText(mapw, odx - (mplanet_width / 2), ody + (mplanet_height / 2),
		     backColor, l->pl_name, 3, planetFont(l));
#endif

	 pl_update[l->pl_no].plu_update = 0;
      } else if (redrawall == 2 || (l->pl_flags & PLCLEAR)) {
	 W_ClearArea(mapw, dx - (mplanet_width / 2), dy - (mplanet_height / 2),
		     mplanet_width, mplanet_height);
	 l->pl_flags &= ~PLCLEAR;
      }
#ifdef FONT_BITMAPS
      W_MaskText(mapw, dx - (mplanet_width / 2), dy - (mplanet_height / 2),
		 planetColor(l), F_planetMapChar(l), 1, F_planetMapFont());
#else
      W_WriteBitmap(dx - (mplanet_width / 2), dy - (mplanet_height / 2),
		    planetmBitmap(l), planetColor(l));
#endif
      if (l->pl_flags & PLAGRI)
    	  sprintf(plname, "%c%c%c", toupper(l->pl_name[0]), toupper(l->pl_name[1]), toupper(l->pl_name[2]));
      else
	  strncpy(plname, l->pl_name, 4);
      W_WriteText(mapw, dx - (mplanet_width / 2), dy + (mplanet_height / 2),
		  planetColor(l), plname, 3, planetFont(l));
      if (showInd && (l->pl_info & me->p_team) && (l->pl_owner == NOBODY)) {
	 W_MakeLine(mapw, dx + (mplanet_width / 2 - 1),
		    dy + (mplanet_height / 2 - 1),
		    dx - (mplanet_width / 2), dy - (mplanet_height / 2),
		    W_White);
	 W_MakeLine(mapw, dx - (mplanet_width / 2),
		    dy + (mplanet_height / 2 - 1),
		    dx + (mplanet_width / 2 - 1), dy - (mplanet_height / 2),
		    W_White);
      }
      if (showPlanetOwner && (l->pl_info & me->p_team)) {
	 static char     tmp[2];
	 tmp[0] = tolower(teamlet[l->pl_owner]);
	 W_WriteText(mapw, dx + (mplanet_width / 2) + 2, dy - 4,
		     planetColor(l), tmp, 1, planetFont(l));
      }
   }
   /* Draw ships */
   for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
      /*
       * We draw the guy if redrawall, or we just erased him. Also, we redraw
       * if we haven't drawn for 30 frames. (in case he was erased by other
       * ships).
       */
      if (lastUpdate[i] != 0 && (!redrawall) &&
      /* people are rarely motionless -- I think it's ok to reduce this */
	  lastUpdate[i] < 10)
	 continue;
      if (j->p_status != PALIVE)
	 continue;

      if(j->p_flags & PFOBSERV) continue;	/* INL */

      lastUpdate[i] = 0;
      dx = j->p_x * WINSIDE / GWIDTH;
      dy = j->p_y * WINSIDE / GWIDTH;

      if (j->p_flags & PFCLOAK) {
	 W_WriteText(mapw, dx - W_Textwidth,
		     dy - W_Textheight / 2, unColor, cloakChars,
		     (cloakChars[1] == '\0' ? 1 : 2), W_RegularFont);
      } else {
#ifdef HOCKEY
	 if (j->p_ispuck)
	    W_WriteText(mapw, dx - W_Textwidth,
			dy - W_Textheight / 2, myColor,
			"P", 1, shipFont(j));
	 else
#endif
	    W_WriteText(mapw, dx - W_Textwidth,
			dy - W_Textheight / 2, playerColor(j),
			j->p_mapchars, 2, shipFont(j));
      }

      mclearzone[0][i] = dx - W_Textwidth;
      mclearzone[1][i] = dy - W_Textheight / 2;
      mclearzone[2][i] = W_Textwidth *
#ifdef HOCKEY
	 ((j->p_ispuck && !(j->p_flags & PFCLOAK)) ? 1 : 2)
#else
	 2
#endif
	 ;

      mclearzone[3][i] = W_Textheight;
      /* Set these so we can checkRedraw() next time */
      mclearzone[4][i] = j->p_x;
      mclearzone[5][i] = j->p_y;
      redrawPlayer[i] = 0;
#if defined( FOR_MORONS)
      if (For_Morons && j != me) {
	 if (j->p_ship.s_type == STARBASE) {
	    W_WriteText(mapw, dx - W_Textwidth * 4,
			dy + W_Textheight / 2,
			playerColor(j), "STARBASE", 8,
			BOLD_ON ? W_BoldFont : W_RegularFont);
	    mclearzone[0][i] = dx - W_Textwidth * 4;
	    mclearzone[1][i] = dy - W_Textheight / 2;
	    mclearzone[2][i] = W_Textwidth * 8;
	    mclearzone[3][i] = W_Textheight * 2;
	    mclearzone[4][i] = j->p_x;
	    mclearzone[5][i] = j->p_y;
	 } else if (!friendlyPlayer(j)) {
	    if (j->p_kills >= 1.0) {
	       /* OGG ME */
	       W_WriteText(mapw, dx - W_Textwidth * 3,
			   dy + W_Textheight / 2,
			   playerColor(j), "OGG ME", 6,
			   BOLD_ON ? W_BoldFont : W_RegularFont);
	       mclearzone[0][i] = dx - W_Textwidth * 3;
	       mclearzone[1][i] = dy - W_Textheight / 2;
	       mclearzone[2][i] = W_Textwidth * 6;
	       mclearzone[3][i] = W_Textheight * 2;
	       mclearzone[4][i] = j->p_x;
	       mclearzone[5][i] = j->p_y;
	    }
	 }
      }
#endif
   }

   if ((me->p_flags & PFPLOCK) && (showLock & 1)) {
      j = &players[me->p_playerl];

      if (j->p_status == PALIVE && !(j->p_flags & PFCLOAK)) {
	 dx = j->p_x * WINSIDE / GWIDTH;
	 dy = j->p_y * WINSIDE / GWIDTH;
	 W_WriteTriangle(mapw, dx, dy + 6, 4, 1, foreColor);

	 clearlmark[0] = dx;
	 clearlmark[1] = dy + 6;
	 clearlmark[2] = 4;
	 clearlmark[3] = 1;
	 clearlmcount++;
      }
   } else if ((me->p_flags & PFPLLOCK) && (showLock & 1)) {
      struct planet  *l = &planets[me->p_planet];

      dx = l->pl_x * WINSIDE / GWIDTH;
      dy = l->pl_y * WINSIDE / GWIDTH;
      W_WriteTriangle(mapw, dx, dy - (mplanet_height) / 2 - 4, 4, 0, foreColor);

      clearlmark[0] = dx;
      clearlmark[1] = dy - (mplanet_height) / 2 - 4;
      clearlmark[2] = 4;
      clearlmark[3] = 0;
      clearlmcount++;
   }
   redrawall = 0;
}


#ifdef FOR_MORONS
/* a cached realNumShips */
int
RealNumShips(owner)
{
   int             i, num = 0;
   struct player  *p;
   static          numonteam[ALLTEAM];	/* 'cached' data */

   /* just clear cached data. Useful for when updated */
   if (owner == -1) {
      for (i = 0; i < ALLTEAM; i++)
	 numonteam[i] = -1;
      return (-1);
   }
   if (numonteam[owner] != -1)
      return (numonteam[owner]);

   for (i = 0, p = players; i < MAXPLAYER; i++, p++)
      if (p->p_status != PFREE &&
	  p->p_team == owner)
	 num++;

   numonteam[owner] = num;
   return (num);
}
#endif

/*
 * buf -- string space n   -- number w   -- field width
 * 
 * sp = 0, no space (lj) sp = 1, space    (rj) sp = 2, 0 instead of space.
 */

char           *
itoa(buf, n, w, sp)
    char           *buf;
    int             n;
    int             w, sp;
{
   register char  *s = buf;

   if (w > 4) {
      *s = '0' + n / 10000;
      if (*s != '0' || sp == 2)
	 s++;
      else if (sp == 1)
	 *s++ = ' ';
   }
   if (w > 3) {
      *s = '0' + (n % 10000) / 1000;
      if (*s != '0' || n >= 10000 || sp == 2)
	 s++;
      else if (*s == '0' && sp)
	 *s++ = ' ';
   }
   if (w > 2) {
      *s = '0' + (n % 1000) / 100;
      if (*s != '0' || n >= 1000 || sp == 2)
	 s++;
      else if (*s == '0' && sp)
	 *s++ = ' ';
   }
   if (w > 1) {
      *s = '0' + (n % 100) / 10;
      if (*s != '0' || n >= 100 || sp == 2)
	 s++;
      else if (*s == '0' && sp)
	 *s++ = ' ';
   }
   *s = '0' + (n % 10);

   return ++s;
}

/* s1 NOT NULL TERMINATED */
char           *
strcpy_return(s1, s2)
    register
    char           *s1, *s2;
{
   while (*s2)
      *s1++ = *s2++;
   return s1;
}

char           *
strcpyp_return(s1, s2, length)
    register
    char           *s1, *s2;
    register
    int             length;
{
   while (length && *s2) {
      *s1++ = *s2++;
      length--;
   }
   if (length > 0) {
      while (length--)
	 *s1++ = ' ';
   }
   return s1;
}

char           *
itof42(s, f)
    char           *s;
    double           f;
{
   *s = '0' + (int) f / 1000;
   if (*s == '0')
      *s = ' ';
   *++s = '0' + ((int) f % 1000) / 100;
   if (*s == '0' && (int) f < 1000)
      *s = ' ';
   *++s = '0' + ((int) f % 100) / 10;
   if (*s == '0' && (int) f < 100)
      *s = ' ';
   *++s = '0' + ((int) f % 10);
   *++s = '.';
   *++s = '0' + ((int) (f * 10)) % 10;
   *++s = '0' + ((int) (f * 100)) % 10;

   return ++s;
}

char           *
itof32(s, f)
    char           *s;
    double           f;
{
   *s = '0' + ((int) f % 1000) / 100;
   if (*s == '0')
      *s = ' ';
   *++s = '0' + ((int) f % 100) / 10;
   if (*s == '0' && (int) f < 100)
      *s = ' ';
   *++s = '0' + ((int) f % 10);
   *++s = '.';
   *++s = '0' + ((int) (f * 10)) % 10;
   *++s = '0' + ((int) (f * 100)) % 10;

   return ++s;
}

char           *
itof22(s, f)
    char           *s;
    double           f;
{
   register		shift = 0;

   if(f >= 100.){
      f /= 10;
      shift++;
   }

   *s = '0' + ((int) f % 100) / 10;
   if (*s == '0' && (int) f < 100)
      *s = ' ';
   *++s = '0' + ((int) f % 10);
   if(!shift)
      *++s = '.';
   *++s = '0' + ((int) (f * 10)) % 10;
   if(shift)
      *++s = '.';
   *++s = '0' + ((int) (f * 100)) % 10;

   return ++s;
}

/*
 * Why go to this trouble?  Because it seems to be a waste of X overhead to
 * redraw the entire map just because an info window -- used very frequently
 * by players -- was popped down.
 */

/* borrowed from planets.c: */
#define DETAIL 40		/* Size of redraw array */
#define SIZE	(GWIDTH/DETAIL)

#define BoxXBox(xl1, yu1, xr1, yd1, xl2, yu2, xr2, yd2) \
                                \
      (!(                       \
         ( xr1 <= xl2)  ||      \
         ( xl1 >= xr2)  ||      \
         ( yd1 <= yu2)  ||      \
         ( yu1 >= yd2)))


void
setup_redraw_map(xl, yu, xr, yd)
    register int    xl, yu, xr, yd;
{
   if (redrawall)
      return;

   if (xr - xl > WINSIDE / 2 && yd - yu > WINSIDE / 2) {
      redrawall = 2;
      return;
   } else {
      register        i, k, l, x, y;
      register struct player *j;

      xl *= (GWIDTH / WINSIDE);
      yu *= (GWIDTH / WINSIDE);
      xr *= (GWIDTH / WINSIDE);
      yd *= (GWIDTH / WINSIDE);

      for (i = xl; i < xr; i += SIZE) {
	 for (k = yu; k < yd; k += SIZE) {
	    checkRedraw(i, k);	/* borrowing from planets.c */
	 }
      }
      k = (W_Textwidth) * (GWIDTH / WINSIDE);
      l = (W_Textheight / 2) * (GWIDTH / WINSIDE);

      for (i = 0, j = players; i < MAXPLAYER; i++, j++) {
	 if (j->p_status != PALIVE)
	    continue;

	 x = j->p_x;
	 y = j->p_y;

	 if (BoxXBox(x - k, y - l, x + k, y + l, xl, yu, xr, yd)) {
	    redrawPlayer[i] = 1;
	 }
      }
   }
}

#ifdef PHASER_SHRINK
/* CAUTION: floating point intensive */
static void
get_shrink_phaser_coords(rx, ry, dx, dy, tx, ty, curr_fuse, max_fuse)
    int            *rx, *ry;
    register        dx, dy, tx, ty, curr_fuse, max_fuse;
{
   extern double   rint(), hypot();
   register int    ph_dir, range;

   /* no negative lines */
   if (curr_fuse > max_fuse)
      curr_fuse = max_fuse;

   /*
    * Calculate direction (we don't get this from the server each tick)
    */
   ph_dir = (int) rint(atan2((double) (tx - dx),
			     (double) (dy - ty)) / 3.14159 * 128.);

   /*
    * Calculate current drawing range of phaser based on distance and phaser
    * fuse
    */
   range = (5 * curr_fuse * hypot((double) dx - tx, (double) dy - ty)) /
      (shrink_phasers_amount * max_fuse);

   /* Calculate phaser origin */
   *rx = (int) (dx + range * Cos[(unsigned char) ph_dir]);
   *ry = (int) (dy + range * Sin[(unsigned char) ph_dir]);

}
#endif
