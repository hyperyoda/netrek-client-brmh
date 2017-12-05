/*
 * newwin.c
 */
#include "copyright.h"

#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <sys/types.h>
#ifdef hpux
#include <time.h>
#else				/* hpux */
#include <sys/time.h>
#endif				/* hpux */
#include "netrek.h"

#ifndef FONT_BITMAPS
#include "bitmaps/bitmaps.h"
#include "bitmaps/oldbitmaps.h"
#endif
#include "icon.h"

#ifndef FONT_BITMAPS
#ifdef MOOBITMAPS
#include "bitmaps/moobitmaps.h"
#endif
#ifdef MOOCURSORS		/* these suck! */
#include "bitmaps/cursors.h"
#endif
#endif

#include "bitmaps/clockbitmap.h"

#if defined(DYNAMIC_BITMAPS) && !defined(FONT_BITMAPS)
#include "bitmapstuff.h"
#endif

#define SIZEOF(a)	(sizeof (a) / sizeof (*(a)))

#define BOXSIDE		(WINSIDE / 5)
#define TILESIDE	16
#define MESSAGESIZE	20
#define STATSIZE	(5 + 3 * (W_Textheight +1) + 2 + BORDER)
#define YOFF		0

void
newwin(hostmon, progname)
    char           *hostmon, *progname;
{
   int             i;

   baseWin = W_MakeWindow("netrek", 0, YOFF, WINSIDE * 2 + 1 * BORDER,
	      WINSIDE + 2 * BORDER + 2 * MESSAGESIZE, NULL, BORDER, gColor);
   W_TopWindowTitle(baseWin);
   iconWin = W_MakeWindow("netrek_icon", 0, 0, icon_width, icon_height, NULL,
			  BORDER, gColor);
   W_SetIconWindow(baseWin, iconWin);
   w = W_MakeWindow("local", -BORDER, -BORDER, WINSIDE, WINSIDE, baseWin,
		    BORDER, foreColor);
   mapw = W_MakeWindow("map", WINSIDE, -BORDER, WINSIDE, WINSIDE, baseWin,
		       BORDER, foreColor);
   tstatw = W_MakeWindow("tstat", -BORDER, WINSIDE, WINSIDE, STATSIZE, baseWin,
			 BORDER, foreColor);
   warnw = W_MakeWindow("warn", WINSIDE, WINSIDE, WINSIDE, MESSAGESIZE,
			baseWin, BORDER, foreColor);
   messagew = W_MakeWindow("message", WINSIDE, WINSIDE + BORDER + MESSAGESIZE,
			   WINSIDE, MESSAGESIZE, baseWin, BORDER, foreColor);

   planetw = W_MakeTextWindow("planet", 10, 10, 53, MAXPLANETS + 3, w, 2);
   rankw = W_MakeTextWindow("rank", 50, 300, 65, NUMRANKS + 8, w, 2);
   playerw = W_MakeTextWindow("player", 0, YOFF + WINSIDE + 2 * BORDER + 2 * MESSAGESIZE,
			      83, MAXPLAYER + 3, NULL, 2);
   helpWin = W_MakeTextWindow("help", 0, YOFF + WINSIDE + 2 * BORDER + 2 * MESSAGESIZE,
			      160, 21, NULL, BORDER);

   messwa = W_MakeScrollingWindow("review_all", WINSIDE + BORDER,
	  YOFF + WINSIDE + 3 * BORDER + 2 * MESSAGESIZE, 80, 10, 0, BORDER);
   messwt = W_MakeScrollingWindow("review_team", WINSIDE + BORDER,
      YOFF + WINSIDE + 4 * BORDER + 2 * MESSAGESIZE + 10 * W_Textheight + 8,
				  80, 5, 0, BORDER);
   messwi = W_MakeScrollingWindow("review_your", WINSIDE + BORDER,
     YOFF + WINSIDE + 5 * BORDER + 2 * MESSAGESIZE + 15 * W_Textheight + 16,
				  80, 4, 0, BORDER);
   messwk = W_MakeScrollingWindow("review_kill", WINSIDE + BORDER,
     YOFF + WINSIDE + 6 * BORDER + 2 * MESSAGESIZE + 19 * W_Textheight + 24,
				  80, 6, 0, BORDER);
   phaserwin = W_MakeScrollingWindow("review_phaser", WINSIDE + BORDER,
     YOFF + WINSIDE + 3 * BORDER + 2 * MESSAGESIZE + 15 * W_Textheight + 16,
				     80, 4, 0, BORDER);
   reviewWin = W_MakeScrollingWindow("review", WINSIDE + BORDER,
	  YOFF + WINSIDE + 3 * BORDER + 2 * MESSAGESIZE, 80, 20, 0, BORDER);

   udpWin = W_MakeMenu("UDP", WINSIDE + 10, -BORDER + 10, 40, UDP_NUMOPTS,
		       NULL, 2);

#ifdef NETSTAT
   lMeter = W_MakeWindow("lagMeter", 0, 600, lMeterWidth(), lMeterHeight(),
			 NULL, BORDER, foreColor);
#endif

#ifdef PING
   pStats = W_MakeWindow("pingStats", 0, 600, pStatsWidth(), pStatsHeight(),
			 NULL, 1, foreColor);
#endif

   for (i = 0; i < 4; i++) {
      teamWin[i] = W_MakeWindow(teamshort[1 << i], i * BOXSIDE, 400,
				BOXSIDE, BOXSIDE, w, 1, foreColor);
   }
   quitwin = W_MakeWindow("quit", 4 * BOXSIDE, 400, BOXSIDE, BOXSIDE, w, 1,
		       foreColor);
#ifdef ARMY_SLIDER
   statwin = W_MakeWindow("stats", 422, 13, 160, 110, NULL, 5, foreColor);
#else
   statwin = W_MakeWindow("stats", 422, 13, 160, 95, NULL, 5, foreColor);
#endif				/* ARMY_SLIDER */

#ifdef SCAN
   scanwin = W_MakeWindow("scanner", 422, 13, 160, 120, baseWin, 5, foreColor);
#endif				/* ATM */

   infow = 0;

   getResources(progname);

#ifdef TCURSORS
#ifdef MOOCURSORS
   DefineLocalcursor(w);
   DefineMapcursor(mapw);
#else
   W_DefineTrekCursor(baseWin);
   W_DefineTCrossCursor(w);
#endif
   W_DefineTCrossCursor(mapw);
   W_DefineUpDownCursor(messwa);
   W_DefineUpDownCursor(messwt);
   W_DefineUpDownCursor(messwi);
   W_DefineTrekCursor(helpWin);
   W_DefineUpDownCursor(reviewWin);
   W_DefineUpDownCursor(messwk);
   W_DefineUpDownCursor(phaserwin);
   W_DefineTrekCursor(playerw);
   W_DefineTrekCursor(rankw);
   W_DefineTrekCursor(statwin);
   W_DefineTrekCursor(iconWin);
   W_DefineTextCursor(messagew);
   W_DefineTrekCursor(tstatw);
   W_DefineWarningCursor(quitwin);
#ifdef SCAN
   W_DefineTrekCursor(scanwin);
#endif
   W_DefineArrowCursor(udpWin);

#ifdef NETSTAT
   W_DefineArrowCursor(lMeter);
#endif

#ifdef TCURSORS
#ifdef MOOTCURSORS
   DefineFedCursor(teamWin[0]);
   DefineRomCursor(teamWin[1]);
   DefineKliCursor(teamWin[2]);
   DefineOriCursor(teamWin[3]);
#else
   for (i = 0; i < 4; i++)
      W_DefineArrowCursor(teamWin[i]);
#endif				/* MOOBITMAPS */
#endif				/* TCURSOR */

#else
   W_DefineCursor(baseWin, 16, 16, cross_bits, crossmask_bits, 7, 7);
   W_DefineCursor(messwa, 16, 16, cross_bits, crossmask_bits, 7, 7);
   W_DefineCursor(messwt, 16, 16, cross_bits, crossmask_bits, 7, 7);
   W_DefineCursor(messwi, 16, 16, cross_bits, crossmask_bits, 7, 7);
   W_DefineCursor(helpWin, 16, 16, cross_bits, crossmask_bits, 7, 7);
   W_DefineCursor(playerw, 16, 16, cross_bits, crossmask_bits, 7, 7);
   W_DefineCursor(statwin, 16, 16, cross_bits, crossmask_bits, 7, 7);
#ifdef SCAN
   W_DefineCursor(scanwin, 16, 16, cross_bits, crossmask_bits, 7, 7);
#endif				/* ATM */
#endif

#define WARHEIGHT 2
#define WARWIDTH 20
#define WARBORDER 2

   war = W_MakeMenu("war", WINSIDE + 10, -BORDER + 10, WARWIDTH, 6, baseWin,
		    WARBORDER);

#ifdef TCURSORS
   W_DefineArrowCursor(war);
#endif

#ifdef FONT_BITMAPS
   /* store the clock bitmap */
   clockpic = W_StoreBitmap(clock_width, clock_height, clock_bits,
			    quitwin);
   /* the icon */
   icon = W_StoreBitmap(icon_width, icon_height, icon_bits, iconWin);
#else
   savebitmaps();
#endif
}

void
mapAll()
{
   initinput();
   W_MapWindow(mapw);
   W_MapWindow(tstatw);
   W_MapWindow(warnw);
   W_MapWindow(messagew);
   W_MapWindow(w);
   W_MapWindow(baseWin);

   /*
    * since we aren't mapping windows that have root as parent in x11window.c
    * (since that messes up the TransientFor feature) we have to map them
    * here. (If already mapped, W_MapWindow returns)
    */

   if (W_CheckMapped("planet"))
      W_MapWindow(planetw);
   else
      W_UnmapWindow(planetw);
   if (W_CheckMapped("rank"))
      W_MapWindow(rankw);
   else
      W_UnmapWindow(rankw);
   if (W_CheckMapped("help"))
      W_MapWindow(helpWin);
   else
      W_UnmapWindow(helpWin);
   if (W_CheckMapped("review_all"))
      W_MapWindow(messwa);
   else
      W_UnmapWindow(messwa);
   if (W_CheckMapped("review_team"))
      W_MapWindow(messwt);
   else
      W_UnmapWindow(messwt);
   if (W_CheckMapped("review_your"))
      W_MapWindow(messwi);
   else
      W_UnmapWindow(messwi);
   if (W_CheckMapped("review_kill"))
      W_MapWindow(messwk);
   else
      W_UnmapWindow(messwk);
   if (W_CheckMapped("player"))
      W_MapWindow(playerw);
   else
      W_UnmapWindow(playerw);
   if (W_CheckMapped("review"))
      W_MapWindow(reviewWin);
   else
      W_UnmapWindow(reviewWin);
   if (W_CheckMapped("UDP"))
      udpwindow();
   else
      W_UnmapWindow(udpWin);
#ifdef NETSTAT
   if (W_CheckMapped("lagMeter")) {
      netstat = 1;
      W_MapWindow(lMeter);
   } else {
      netstat = 0;
      W_UnmapWindow(lMeter);
   }
#endif
#ifdef PING
   if (W_CheckMapped("pingStats"))
      W_MapWindow(pStats);
   else
      W_UnmapWindow(pStats);
#endif

   if (W_CheckMapped("review_phaser")) {
      W_MapWindow(phaserwin);
      phaserWindow = 1;
   } else {
      W_UnmapWindow(phaserwin);
      phaserWindow = 0;
   }
}

void
updateWindows()
{
   register        i;

   W_UpdateWindow(baseWin);
   W_UpdateWindow(iconWin);
   W_UpdateWindow(w);
   W_UpdateWindow(mapw);
   W_UpdateWindow(tstatw);
   W_UpdateWindow(warnw);
   W_UpdateWindow(messagew);
   W_UpdateWindow(planetw);
   W_UpdateWindow(rankw);
   W_UpdateWindow(playerw);
   W_UpdateWindow(helpWin);
   W_UpdateWindow(messwa);
   W_UpdateWindow(messwt);
   W_UpdateWindow(messwi);
   W_UpdateWindow(messwk);
   W_UpdateWindow(phaserwin);
   W_UpdateWindow(reviewWin);
   W_UpdateWindow(udpWin);
#ifdef NETSTAT
   W_UpdateWindow(lMeter);
#endif
#ifdef PING
   W_UpdateWindow(pStats);
#endif
   for (i = 0; i < 4; i++) {
      W_UpdateWindow(teamWin[i]);
   }
   W_UpdateWindow(quitwin);
   W_UpdateWindow(statwin);
#ifdef SCAN
   W_UpdateWindow(scanwin);
#endif
   W_UpdateWindow(war);
#ifdef XTREKRC_HELP
   W_UpdateWindow(defWin);
#endif
   W_UpdateWindow(macroWin);
   W_UpdateWindow(optionWin);
}

#ifndef FONT_BITMAPS
void
savebitmaps()
{
   register int    i;
#ifdef DYNAMIC_BITMAPS

   /*
    * store the bitmaps for all the ship types, over all the * teams, over
    * all the views (ugh!)
    */
   for (i = 0; i < VIEWS; i++) {
      int             num = 0;
      /* Start FED */
      num = StdBitmapNum(FED, SCOUT);
      ship_bitmaps[num][i] = W_StoreBitmap(fed_scout_width, fed_scout_height,
					   RotateShipViews(fed_scout_bits, i, STD_SHIP_FLGS, num, fed_scout_width), w);
      num = StdBitmapNum(FED, DESTROYER);
      ship_bitmaps[num][i] = W_StoreBitmap(fed_destroyer_width, fed_destroyer_height,
					   RotateShipViews(fed_destroyer_bits, i, STD_SHIP_FLGS, num, fed_destroyer_width), w);
      num = StdBitmapNum(FED, CRUISER);
      ship_bitmaps[num][i] = W_StoreBitmap(fed_cruiser_width, fed_cruiser_height,
					   RotateShipViews(fed_cruiser_bits, i, STD_SHIP_FLGS, num, fed_cruiser_width), w);
      num = StdBitmapNum(FED, BATTLESHIP);
      ship_bitmaps[num][i] = W_StoreBitmap(fed_battleship_width, fed_battleship_height,
					   RotateShipViews(fed_battleship_bits, i, STD_SHIP_FLGS, num, fed_battleship_width), w);
      num = StdBitmapNum(FED, ASSAULT);
      ship_bitmaps[num][i] = W_StoreBitmap(fed_assault_width, fed_assault_height,
					   RotateShipViews(fed_assault_bits, i, STD_SHIP_FLGS, num, fed_assault_width), w);
#ifdef GALAXY
      num = StdBitmapNum(FED, SGALAXY);
      ship_bitmaps[num][i] = W_StoreBitmap(fed_galaxy_width, fed_galaxy_height,
					   RotateShipViews(fed_galaxy_bits, i, STD_SHIP_FLGS, num, fed_galaxy_width), w);
#endif				/* GALAXY */
      num = StdBitmapNum(FED, STARBASE);
      ship_bitmaps[num][i] = W_StoreBitmap(fed_starbase_width, fed_starbase_height,
					   RotateShipViews(fed_starbase_bits, i, STD_BASE_FLGS, num, fed_starbase_width), w);
#ifdef ATT_BITMAPS
      num = StdBitmapNum(FED, ATT);
      ship_bitmaps[num][i] = W_StoreBitmap(fed_att_width, fed_att_height,
					   RotateShipViews(fed_att_bits, i, STD_SHIP_FLGS, num, fed_att_width), w);
#else
      num = StdBitmapNum(FED, ATT);
      ship_bitmaps[num][i] = W_StoreBitmap(fed_cruiser_width, fed_cruiser_height,
					   RotateShipViews(fed_cruiser_bits, i, STD_SHIP_FLGS, num, fed_cruiser_width), w);
#endif
      /* End FED */

      /* Start ROM */
      num = StdBitmapNum(ROM, SCOUT);
      ship_bitmaps[num][i] = W_StoreBitmap(rom_scout_width, rom_scout_height,
					   RotateShipViews(rom_scout_bits, i, STD_SHIP_FLGS, num, rom_scout_width), w);
      num = StdBitmapNum(ROM, DESTROYER);
      ship_bitmaps[num][i] = W_StoreBitmap(rom_destroyer_width, rom_destroyer_height,
					   RotateShipViews(rom_destroyer_bits, i, STD_SHIP_FLGS, num, rom_destroyer_width), w);
      num = StdBitmapNum(ROM, CRUISER);
      ship_bitmaps[num][i] = W_StoreBitmap(rom_cruiser_width, rom_cruiser_height,
					   RotateShipViews(rom_cruiser_bits, i, STD_SHIP_FLGS, num, rom_cruiser_width), w);
      num = StdBitmapNum(ROM, BATTLESHIP);
      ship_bitmaps[num][i] = W_StoreBitmap(rom_battleship_width, rom_battleship_height,
					   RotateShipViews(rom_battleship_bits, i, STD_SHIP_FLGS, num, rom_battleship_width), w);
      num = StdBitmapNum(ROM, ASSAULT);
      ship_bitmaps[num][i] = W_StoreBitmap(rom_assault_width, rom_assault_height,
					   RotateShipViews(rom_assault_bits, i, STD_SHIP_FLGS, num, rom_assault_width), w);
#ifdef GALAXY
      num = StdBitmapNum(ROM, SGALAXY);
      ship_bitmaps[num][i] = W_StoreBitmap(rom_galaxy_width, rom_galaxy_height,
					   RotateShipViews(rom_galaxy_bits, i, STD_SHIP_FLGS, num, rom_galaxy_width), w);
#endif				/* GALAXY */
      num = StdBitmapNum(ROM, STARBASE);
      ship_bitmaps[num][i] = W_StoreBitmap(rom_starbase_width, rom_starbase_height,
					   RotateShipViews(rom_starbase_bits, i, STD_BASE_FLGS, num, rom_starbase_width), w);
#ifdef ATT_BITMAPS
      num = StdBitmapNum(ROM, ATT);
      ship_bitmaps[num][i] = W_StoreBitmap(rom_att_width, rom_att_height,
					   RotateShipViews(rom_att_bits, i, STD_SHIP_FLGS, num, rom_att_width), w);
#else
      num = StdBitmapNum(ROM, ATT);
      ship_bitmaps[num][i] = W_StoreBitmap(rom_cruiser_width, rom_cruiser_height,
					   RotateShipViews(rom_cruiser_bits, i, STD_SHIP_FLGS, num, rom_cruiser_width), w);
#endif
      /* End ROM */
      /* Start KLI */
      num = StdBitmapNum(KLI, SCOUT);
      ship_bitmaps[num][i] = W_StoreBitmap(kli_scout_width, kli_scout_height,
					   RotateShipViews(kli_scout_bits, i, STD_SHIP_FLGS, num, kli_scout_width), w);
      num = StdBitmapNum(KLI, DESTROYER);
      ship_bitmaps[num][i] = W_StoreBitmap(kli_destroyer_width, kli_destroyer_height,
					   RotateShipViews(kli_destroyer_bits, i, STD_SHIP_FLGS, num, kli_destroyer_width), w);
      num = StdBitmapNum(KLI, CRUISER);
      ship_bitmaps[num][i] = W_StoreBitmap(kli_cruiser_width, kli_cruiser_height,
					   RotateShipViews(kli_cruiser_bits, i, STD_SHIP_FLGS, num, kli_cruiser_width), w);
      num = StdBitmapNum(KLI, BATTLESHIP);
      ship_bitmaps[num][i] = W_StoreBitmap(kli_battleship_width, kli_battleship_height,
					   RotateShipViews(kli_battleship_bits, i, STD_SHIP_FLGS, num, kli_battleship_width), w);
      num = StdBitmapNum(KLI, ASSAULT);
      ship_bitmaps[num][i] = W_StoreBitmap(kli_assault_width, kli_assault_height,
					   RotateShipViews(kli_assault_bits, i, STD_SHIP_FLGS, num, kli_assault_width), w);
#ifdef GALAXY
      num = StdBitmapNum(KLI, SGALAXY);
      ship_bitmaps[num][i] = W_StoreBitmap(kli_galaxy_width, kli_galaxy_height,
					   RotateShipViews(kli_galaxy_bits, i, STD_SHIP_FLGS, num, kli_galaxy_width), w);
#endif				/* GALAXY */
      num = StdBitmapNum(KLI, STARBASE);
      ship_bitmaps[num][i] = W_StoreBitmap(kli_starbase_width, kli_starbase_height,
					   RotateShipViews(kli_starbase_bits, i, STD_BASE_FLGS, num, kli_starbase_width), w);
#ifdef ATT_BITMAPS
      num = StdBitmapNum(KLI, ATT);
      ship_bitmaps[num][i] = W_StoreBitmap(kli_att_width, kli_att_height,
					   RotateShipViews(kli_att_bits, i, STD_SHIP_FLGS, num, kli_att_width), w);
#else
      num = StdBitmapNum(KLI, ATT);
      ship_bitmaps[num][i] = W_StoreBitmap(kli_cruiser_width, kli_cruiser_height,
					   RotateShipViews(kli_cruiser_bits, i, STD_SHIP_FLGS, num, kli_cruiser_width), w);
#endif
      /* End KLI */
      /* Start ORI */
      num = StdBitmapNum(ORI, SCOUT);
      ship_bitmaps[num][i] = W_StoreBitmap(ori_scout_width, ori_scout_height,
					   RotateShipViews(ori_scout_bits, i, STD_SHIP_FLGS, num, ori_scout_width), w);
      num = StdBitmapNum(ORI, DESTROYER);
      ship_bitmaps[num][i] = W_StoreBitmap(ori_destroyer_width, ori_destroyer_height,
					   RotateShipViews(ori_destroyer_bits, i, STD_SHIP_FLGS, num, ori_destroyer_width), w);
      num = StdBitmapNum(ORI, CRUISER);
      ship_bitmaps[num][i] = W_StoreBitmap(ori_cruiser_width, ori_cruiser_height,
					   RotateShipViews(ori_cruiser_bits, i, STD_SHIP_FLGS, num, ori_cruiser_width), w);
      num = StdBitmapNum(ORI, BATTLESHIP);
      ship_bitmaps[num][i] = W_StoreBitmap(ori_battleship_width, ori_battleship_height,
					   RotateShipViews(ori_battleship_bits, i, STD_SHIP_FLGS, num, ori_battleship_width), w);
      num = StdBitmapNum(ORI, ASSAULT);
      ship_bitmaps[num][i] = W_StoreBitmap(ori_assault_width, ori_assault_height,
					   RotateShipViews(ori_assault_bits, i, STD_SHIP_FLGS, num, ori_assault_width), w);
#ifdef GALAXY
      num = StdBitmapNum(ORI, SGALAXY);
      ship_bitmaps[num][i] = W_StoreBitmap(ori_galaxy_width, ori_galaxy_height,
					   RotateShipViews(ori_galaxy_bits, i, STD_SHIP_FLGS, num, ori_galaxy_width), w);
#endif				/* GALAXY */
      num = StdBitmapNum(ORI, STARBASE);
      ship_bitmaps[num][i] = W_StoreBitmap(ori_starbase_width, ori_starbase_height,
					   RotateShipViews(ori_starbase_bits, i, STD_BASE_FLGS, num, ori_starbase_width), w);
#ifdef ATT_BITMAPS
      num = StdBitmapNum(ORI, ATT);
      ship_bitmaps[num][i] = W_StoreBitmap(ori_att_width, ori_att_height,
					   RotateShipViews(ori_att_bits, i, STD_SHIP_FLGS, num, ori_att_width), w);
#else
      num = StdBitmapNum(ORI, ATT);
      ship_bitmaps[num][i] = W_StoreBitmap(ori_cruiser_width, ori_cruiser_height,
					   RotateShipViews(ori_cruiser_bits, i, STD_SHIP_FLGS, num, ori_cruiser_width), w);
#endif
      /* End ORI */
      /* Start IND */
      num = StdBitmapNum(IND, SCOUT);
      ship_bitmaps[num][i] = W_StoreBitmap(ind_scout_width, ind_scout_height,
	    RotateShipViews(ind_scout_bits, i, 
	       STD_SHIP_FLGS, num, ind_scout_width), w);
      num = StdBitmapNum(IND, DESTROYER);
      ship_bitmaps[num][i] = W_StoreBitmap(ind_destroyer_width, ind_destroyer_height,
					   RotateShipViews(ind_destroyer_bits, i, STD_SHIP_FLGS, num, ind_destroyer_width), w);
      num = StdBitmapNum(IND, CRUISER);
      ship_bitmaps[num][i] = W_StoreBitmap(ind_cruiser_width, ind_cruiser_height,
					   RotateShipViews(ind_cruiser_bits, i, STD_SHIP_FLGS, num, ind_cruiser_width), w);
      num = StdBitmapNum(IND, BATTLESHIP);
      ship_bitmaps[num][i] = W_StoreBitmap(ind_battleship_width, ind_battleship_height,
					   RotateShipViews(ind_battleship_bits, i, STD_SHIP_FLGS, num, ind_battleship_width), w);
      num = StdBitmapNum(IND, ASSAULT);
      ship_bitmaps[num][i] = W_StoreBitmap(ind_assault_width, ind_assault_height,
					   RotateShipViews(ind_assault_bits, i, STD_SHIP_FLGS, num, ind_assault_width), w);
#ifdef GALAXY
      num = StdBitmapNum(IND, SGALAXY);
      ship_bitmaps[num][i] = W_StoreBitmap(ind_galaxy_width, ind_galaxy_height,
					   RotateShipViews(ind_galaxy_bits, i, STD_SHIP_FLGS, num, ind_galaxy_width), w);
#endif				/* GALAXY */
      num = StdBitmapNum(IND, STARBASE);
      ship_bitmaps[num][i] = W_StoreBitmap(ind_starbase_width, ind_starbase_height,
					   RotateShipViews(ind_starbase_bits, i, STD_BASE_FLGS, num, ind_starbase_width), w);
#ifdef ATT_BITMAPS
      num = StdBitmapNum(IND, ATT);
      ship_bitmaps[num][i] = W_StoreBitmap(ind_att_width, ind_att_height,
					   RotateShipViews(ind_att_bits, i, STD_SHIP_FLGS, num, ind_att_width), w);
#else
      num = StdBitmapNum(IND, ATT);
      ship_bitmaps[num][i] = W_StoreBitmap(ind_cruiser_width, ind_cruiser_height,
					   RotateShipViews(ind_cruiser_bits, i, STD_SHIP_FLGS, num, ind_cruiser_width), w);
#endif
      /* End IND */
   }
#else				/* DYNAMIC_BITMAPS */
   /*
    * store the bitmaps for all the ship types, over all the * teams, over
    * all the views (ugh!)
    */
   for (i = 0; i < VIEWS; i++) {
      fed_bitmaps[SCOUT][i] =
	 W_StoreBitmap(fed_scout_width, fed_scout_height,
		       fed_scout_bits[i], w);
      fed_bitmaps[DESTROYER][i] =
	 W_StoreBitmap(fed_destroyer_width, fed_destroyer_height,
		       fed_destroyer_bits[i], w);
      fed_bitmaps[CRUISER][i] =
	 W_StoreBitmap(fed_cruiser_width, fed_cruiser_height,
		       fed_cruiser_bits[i], w);
      fed_bitmaps[BATTLESHIP][i] =
	 W_StoreBitmap(fed_battleship_width, fed_battleship_height,
		       fed_battleship_bits[i], w);
      fed_bitmaps[ASSAULT][i] =
	 W_StoreBitmap(fed_assault_width, fed_assault_height,
		       fed_assault_bits[i], w);
      fed_bitmaps[STARBASE][i] =
	 W_StoreBitmap(fed_starbase_width, fed_starbase_height,
		       fed_starbase_bits[i], w);
#ifdef GALAXY
      fed_bitmaps[SGALAXY][i] =
	 W_StoreBitmap(fed_galaxy_width, fed_galaxy_width,
		       fed_galaxy_bits[i], w);
#endif				/* GALAXY */
      fed_bitmaps[ATT][i] =
	 W_StoreBitmap(fed_cruiser_width, fed_cruiser_height,
		       fed_cruiser_bits[i], w);

      kli_bitmaps[SCOUT][i] =
	 W_StoreBitmap(kli_scout_width, kli_scout_height,
		       kli_scout_bits[i], w);
      kli_bitmaps[DESTROYER][i] =
	 W_StoreBitmap(kli_destroyer_width, kli_destroyer_height,
		       kli_destroyer_bits[i], w);
      kli_bitmaps[CRUISER][i] =
	 W_StoreBitmap(kli_cruiser_width, kli_cruiser_height,
		       kli_cruiser_bits[i], w);
      kli_bitmaps[BATTLESHIP][i] =
	 W_StoreBitmap(kli_battleship_width, kli_battleship_height,
		       kli_battleship_bits[i], w);
      kli_bitmaps[ASSAULT][i] =
	 W_StoreBitmap(kli_assault_width, kli_assault_height,
		       kli_assault_bits[i], w);
      kli_bitmaps[STARBASE][i] =
	 W_StoreBitmap(kli_starbase_width, kli_starbase_height,
		       kli_starbase_bits[i], w);
#ifdef GALAXY
      kli_bitmaps[SGALAXY][i] =
	 W_StoreBitmap(kli_galaxy_width, kli_galaxy_width,
		       kli_galaxy_bits[i], w);
#endif				/* GALAXY */
      kli_bitmaps[ATT][i] =
	 W_StoreBitmap(kli_cruiser_width, kli_cruiser_height,
		       kli_cruiser_bits[i], w);

      rom_bitmaps[SCOUT][i] =
	 W_StoreBitmap(rom_scout_width, rom_scout_height,
		       rom_scout_bits[i], w);
      rom_bitmaps[DESTROYER][i] =
	 W_StoreBitmap(rom_destroyer_width, rom_destroyer_height,
		       rom_destroyer_bits[i], w);
      rom_bitmaps[CRUISER][i] =
	 W_StoreBitmap(rom_cruiser_width, rom_cruiser_height,
		       rom_cruiser_bits[i], w);
      rom_bitmaps[BATTLESHIP][i] =
	 W_StoreBitmap(rom_battleship_width, rom_battleship_height,
		       rom_battleship_bits[i], w);
      rom_bitmaps[ASSAULT][i] =
	 W_StoreBitmap(rom_assault_width, rom_assault_height,
		       rom_assault_bits[i], w);
      rom_bitmaps[STARBASE][i] =
	 W_StoreBitmap(rom_starbase_width, rom_starbase_height,
		       rom_starbase_bits[i], w);
#ifdef GALAXY
      rom_bitmaps[SGALAXY][i] =
	 W_StoreBitmap(rom_galaxy_width, rom_galaxy_width,
		       rom_galaxy_bits[i], w);
#endif				/* GALAXY */
      rom_bitmaps[ATT][i] =
	 W_StoreBitmap(rom_cruiser_width, rom_cruiser_height,
		       rom_cruiser_bits[i], w);

      ori_bitmaps[SCOUT][i] =
	 W_StoreBitmap(ori_scout_width, ori_scout_height,
		       ori_scout_bits[i], w);
      ori_bitmaps[DESTROYER][i] =
	 W_StoreBitmap(ori_destroyer_width, ori_destroyer_height,
		       ori_destroyer_bits[i], w);
      ori_bitmaps[CRUISER][i] =
	 W_StoreBitmap(ori_cruiser_width, ori_cruiser_height,
		       ori_cruiser_bits[i], w);
      ori_bitmaps[BATTLESHIP][i] =
	 W_StoreBitmap(ori_battleship_width, ori_battleship_height,
		       ori_battleship_bits[i], w);
      ori_bitmaps[ASSAULT][i] =
	 W_StoreBitmap(ori_assault_width, ori_assault_height,
		       ori_assault_bits[i], w);
      ori_bitmaps[STARBASE][i] =
	 W_StoreBitmap(ori_starbase_width, ori_starbase_height,
		       ori_starbase_bits[i], w);
#ifdef GALAXY
      ori_bitmaps[SGALAXY][i] =
	 W_StoreBitmap(ori_galaxy_width, ori_galaxy_width,
		       ori_galaxy_bits[i], w);
#endif				/* GALAXY */
      ori_bitmaps[ATT][i] =
	 W_StoreBitmap(ori_cruiser_width, ori_cruiser_height,
		       ori_cruiser_bits[i], w);

      ind_bitmaps[SCOUT][i] =
	 W_StoreBitmap(ind_scout_width, ind_scout_height,
		       ind_scout_bits[i], w);
      ind_bitmaps[DESTROYER][i] =
	 W_StoreBitmap(ind_destroyer_width, ind_destroyer_height,
		       ind_destroyer_bits[i], w);
      ind_bitmaps[CRUISER][i] =
	 W_StoreBitmap(ind_cruiser_width, ind_cruiser_height,
		       ind_cruiser_bits[i], w);
      ind_bitmaps[BATTLESHIP][i] =
	 W_StoreBitmap(ind_battleship_width, ind_battleship_height,
		       ind_battleship_bits[i], w);
      ind_bitmaps[ASSAULT][i] =
	 W_StoreBitmap(ind_assault_width, ind_assault_height,
		       ind_assault_bits[i], w);
      ind_bitmaps[STARBASE][i] =
	 W_StoreBitmap(ind_starbase_width, ind_starbase_height,
		       ind_starbase_bits[i], w);
#ifdef GALAXY
      ind_bitmaps[SGALAXY][i] =
	 W_StoreBitmap(ind_galaxy_width, ind_galaxy_height,
		       ind_galaxy_bits[i], w);
#endif				/* GALAXY */
      ind_bitmaps[ATT][i] =
	 W_StoreBitmap(ind_cruiser_width, ind_cruiser_height,
		       ind_cruiser_bits[i], w);


   }
#endif				/* DYNAMIC_BITMAPS */

   /* store the clock bitmap */
   clockpic = W_StoreBitmap(clock_width, clock_height, clock_bits,
			    quitwin);

   /* store the bitmaps for torpedo explosions  */
   for (i = 0; i < 5; i++) {
      cloud[i] = W_StoreBitmap(cloud_width, cloud_height, cloud_bits[4 - i], w);
      plasmacloud[i] = W_StoreBitmap(plasmacloud_width,
			    plasmacloud_height, plasmacloud_bits[4 - i], w);
   }

#ifdef nodef
   /* Torpedo bitmaps */
   etorp = W_StoreBitmap(etorp_width, etorp_height, etorp_bits, w);
   mtorp = W_StoreBitmap(mtorp_width, mtorp_height, mtorp_bits, w);
#endif
   eplasmatorp =
      W_StoreBitmap(eplasmatorp_width, eplasmatorp_height, eplasmatorp_bits, w);
   mplasmatorp =
      W_StoreBitmap(mplasmatorp_width, mplasmatorp_height, mplasmatorp_bits, w);

   /* planet bitmaps */
   bplanets[0] = W_StoreBitmap(planet_width, planet_height, indplanet_bits, w);
   bplanets[1] = W_StoreBitmap(planet_width, planet_height, fedplanet_bits, w);
   bplanets[2] = W_StoreBitmap(planet_width, planet_height, romplanet_bits, w);
   bplanets[3] = W_StoreBitmap(planet_width, planet_height, kliplanet_bits, w);
   bplanets[4] = W_StoreBitmap(planet_width, planet_height, oriplanet_bits, w);
   bplanets[5] = W_StoreBitmap(planet_width, planet_height, planet_bits, w);
   mbplanets[0] = W_StoreBitmap(mplanet_width, mplanet_height, indmplanet_bits, mapw);
   mbplanets[1] = W_StoreBitmap(mplanet_width, mplanet_height, fedmplanet_bits, mapw);
   mbplanets[2] = W_StoreBitmap(mplanet_width, mplanet_height, rommplanet_bits, mapw);
   mbplanets[3] = W_StoreBitmap(mplanet_width, mplanet_height, klimplanet_bits, mapw);
   mbplanets[4] = W_StoreBitmap(mplanet_width, mplanet_height, orimplanet_bits, mapw);
   mbplanets[5] = W_StoreBitmap(mplanet_width, mplanet_height, mplanet_bits, mapw);
   bplanets2[0] = bplanets[0];
   mbplanets2[0] = mbplanets[0];
   bplanets2[1] = W_StoreBitmap(planet_width, planet_height, planet001_bits, w);
   bplanets2[2] = W_StoreBitmap(planet_width, planet_height, planet010_bits, w);
   bplanets2[3] = W_StoreBitmap(planet_width, planet_height, planet011_bits, w);
   bplanets2[4] = W_StoreBitmap(planet_width, planet_height, planet100_bits, w);
   bplanets2[5] = W_StoreBitmap(planet_width, planet_height, planet101_bits, w);
   bplanets2[6] = W_StoreBitmap(planet_width, planet_height, planet110_bits, w);
   bplanets2[7] = W_StoreBitmap(planet_width, planet_height, planet111_bits, w);
   mbplanets2[1] = W_StoreBitmap(mplanet_width, mplanet_height, mplanet001_bits, mapw);
   mbplanets2[2] = W_StoreBitmap(mplanet_width, mplanet_height, mplanet010_bits, mapw);
   mbplanets2[3] = W_StoreBitmap(mplanet_width, mplanet_height, mplanet011_bits, mapw);
   mbplanets2[4] = W_StoreBitmap(mplanet_width, mplanet_height, mplanet100_bits, mapw);
   mbplanets2[5] = W_StoreBitmap(mplanet_width, mplanet_height, mplanet101_bits, mapw);
   mbplanets2[6] = W_StoreBitmap(mplanet_width, mplanet_height, mplanet110_bits, mapw);
   mbplanets2[7] = W_StoreBitmap(mplanet_width, mplanet_height, mplanet111_bits, mapw);

#ifdef MOOBITMAPS
   /* isae: My own planet bitmaps */

   bplanets[6] = W_StoreBitmap(planet_width, planet_height, myplanet000_bits, w);

   bplanets3[0] = W_StoreBitmap(planet_width, planet_height, myplanet000_bits, w);
   bplanets3[1] = W_StoreBitmap(planet_width, planet_height, myplanet001_bits, w);
   bplanets3[2] = W_StoreBitmap(planet_width, planet_height, myplanet010_bits, w);
   bplanets3[3] = W_StoreBitmap(planet_width, planet_height, myplanet011_bits, w);
   bplanets3[4] = W_StoreBitmap(planet_width, planet_height, myplanet100_bits, w);
   bplanets3[5] = W_StoreBitmap(planet_width, planet_height, myplanet101_bits, w);
   bplanets3[6] = W_StoreBitmap(planet_width, planet_height, myplanet110_bits, w);
   bplanets3[7] = W_StoreBitmap(planet_width, planet_height, myplanet111_bits, w);

   /* <isae> Added this */
   mbplanets3[0] = W_StoreBitmap(mplanet_width, mplanet_height, myindmplanet_bits, mapw);
   mbplanets3[1] = W_StoreBitmap(mplanet_width, mplanet_height, mymplanet001_bits, mapw);
   mbplanets3[2] = W_StoreBitmap(mplanet_width, mplanet_height, mymplanet010_bits, mapw);
   mbplanets3[3] = W_StoreBitmap(mplanet_width, mplanet_height, mymplanet011_bits, mapw);
   mbplanets3[4] = W_StoreBitmap(mplanet_width, mplanet_height, mplanet100_bits, mapw);
   mbplanets3[5] = W_StoreBitmap(mplanet_width, mplanet_height, mplanet101_bits, mapw);
   mbplanets3[6] = W_StoreBitmap(mplanet_width, mplanet_height, mplanet110_bits, mapw);
   mbplanets3[7] = W_StoreBitmap(mplanet_width, mplanet_height, mplanet111_bits, mapw);
#endif				/* MOOBITMAPS */


   for (i = 0; i < EX_FRAMES; i++) {
      expview[i] = W_StoreBitmap(ex_width, ex_height, ex_bits[i], w);
   }
   for (i = 0; i < SBEXPVIEWS; i++) {
      sbexpview[i] = W_StoreBitmap(sbexp_width, sbexp_height, sbexp_bits[i], w);
   }
   for (i = 0; i < SHIELD_FRAMES; i++) {
      shield[i] = W_StoreBitmap(shield_width, shield_height, shield_bits[i], w);
   }
   cloakicon = W_StoreBitmap(cloak_width, cloak_height, cloak_bits, w);
   icon = W_StoreBitmap(icon_width, icon_height, icon_bits, iconWin);
}
#endif

#ifdef HOCKEY
#ifndef FONT_BITMAPS
void
savepuckbitmap()
{
   register	i;
   if(!puck_bitmaps[0]){
      for (i = 0; i < VIEWS; i++) {
	 puck_bitmaps[i] = W_StoreBitmap(puck_width, puck_height, 
	    puck_bits[i], w);
      }
   }
}
#endif
#endif

#ifdef MOOCURSORS
/* silly cursor definition stuff from moo */

void
DefineMapcursor(window)
    W_Window        window;
{
   W_DefineCursorFromBitmap(window, mapcursor_bits, mapcursor_width, mapcursor_height,
			    mapmask_bits, mapmask_width, mapmask_height);
}

void
DefineLocalcursor(window)
    W_Window        window;
{
   W_DefineCursorFromBitmap(window, localcursor_bits, localcursor_width, localcursor_height,
			 localmask_bits, localmask_width, localmask_height);
}
#endif


#ifdef MOOTCURSORS
void
DefineFedCursor(window)
    W_Window        window;
{
#ifdef FONT_BITMAPS
   W_DefineCursorFromGlyph(window, F_playerFont(FED),
			   F_shipChar(FED, CRUISER, 0),
			   F_shipChar(FED, CRUISER, 0),
			   shipCol[remap[FED]]);
#else
   W_DefineCursorFromBitmap(window,
	   &(fed_cruiser_bits[0][0]), fed_cruiser_width, fed_cruiser_height,
	  &(fed_cruiser_bits[0][0]), fed_cruiser_width, fed_cruiser_height);
#endif
}

void
DefineRomCursor(window)
    W_Window        window;
{
#ifdef FONT_BITMAPS
   W_DefineCursorFromGlyph(window, F_playerFont(ROM),
			   F_shipChar(ROM, CRUISER, 0),
			   F_shipChar(ROM, CRUISER, 0),
			   shipCol[remap[ROM]]);
#else
   W_DefineCursorFromBitmap(window,
	     &rom_cruiser_bits[0][0], rom_cruiser_width, rom_cruiser_height,
	    &rom_cruiser_bits[0][0], rom_cruiser_width, rom_cruiser_height);
#endif
}

void
DefineKliCursor(window)
    W_Window        window;
{
#ifdef FONT_BITMAPS
   W_DefineCursorFromGlyph(window, F_playerFont(KLI),
			   F_shipChar(KLI, CRUISER, 0),
			   F_shipChar(KLI, CRUISER, 0),
			   shipCol[remap[KLI]]);
#else
   W_DefineCursorFromBitmap(window,
	   &(kli_cruiser_bits[0][0]), kli_cruiser_width, kli_cruiser_height,
	  &(kli_cruiser_bits[0][0]), kli_cruiser_width, kli_cruiser_height);
#endif
}

void
DefineOriCursor(window)
    W_Window        window;
{
#ifdef FONT_BITMAPS
   W_DefineCursorFromGlyph(window, F_playerFont(ORI),
			   F_shipChar(ORI, CRUISER, 0),
			   F_shipChar(ORI, CRUISER, 0),
			   shipCol[remap[ORI]]);
#else
   W_DefineCursorFromBitmap(window,
	   &(ori_cruiser_bits[0][0]), ori_cruiser_width, ori_cruiser_height,
	  &(ori_cruiser_bits[0][0]), ori_cruiser_width, ori_cruiser_height);
#endif
}
#endif				/* MOOTCURSORS */
