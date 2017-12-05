/*
 * pingstats.c	(mostly taken from stats.c)
 */
#include "copyright.h"

#include <stdio.h>
#include <math.h>
#include "netrek.h"

#ifdef PING

#define	MIN(a,b)	(((a) < (b)) ? (a) : (b))

#define	BX_OFF()	((pTextWidth + 1) * W_Textwidth + S_IBORDER)
#define	BY_OFF(line)	((line) * (W_Textheight + S_IBORDER) + S_IBORDER)
#define	TX_OFF(len)	((pTextWidth - len) * W_Textwidth + S_IBORDER)
#define	TY_OFF(line)	BY_OFF(line)

/* right side labels */
#define TEXT_WIDTH		(5*W_Textwidth + 2*STAT_BORDER)
#define STAT_WIDTH		(260 + TEXT_WIDTH)
#define STAT_HEIGHT		BY_OFF(NUM_PSLIDERS)
#define STAT_BORDER		2
#define S_IBORDER		5
#define STAT_X			422
#define STAT_Y			13

#define SL_WID			\
	(STAT_WIDTH -TEXT_WIDTH - 2 * S_IBORDER - (pTextWidth + 1) * W_Textwidth)
#define SL_HEI			(W_Textheight)

#define NUM_ELS(a)		(sizeof (a) / sizeof (*(a)))
#define NUM_PSLIDERS		NUM_ELS(psliders)

typedef struct pslider {
   char           *label;
   int             min, max;
   int             green, yellow;
   int             label_length;
   int             diff;
   int            *var;
   int             lastVal;
}               PSLIDER;

typedef struct precord {
   int            *data;
   int             last_value;
}               PRECORD;

static PSLIDER   psliders[] =
{
   {"round trip time", 0, 500, 100, 200},
   {"average r.t. time", 0, 500, 100, 200},
   {"lag (st. dev.)", 0, 100, 20, 50},
   {"inc. pack loss in", 0, 50, 10, 20},
   {"inc. pack loss out", 0, 50, 10, 20},
   {"tot. pack loss in", 0, 50, 5, 10},
   {"tot. pack loss out", 0, 50, 5, 10},
};

#if __STDC__ || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

/* pingstats.c */
static void pbox P_((int filled, int x, int y, int wid, int hei, W_Color color));
static void text P_((int value, int y));

#undef P_

static int      pTextWidth = 0;
static int      pinitialized = 0;

/* externals from ping.c (didn't feel like cluttering up data.c with them) */

extern int      ping_iloss_sc;	/* inc % loss 0--100, server to client */
extern int      ping_iloss_cs;	/* inc % loss 0--100, client to server */
extern int      ping_tloss_sc;	/* total % loss 0--100, server to client */
extern int      ping_tloss_cs;	/* total % loss 0--100, client to server */
extern int      ping_lag;	/* delay in ms of last ping */
extern int      ping_av;	/* average rt */
extern int      ping_sd;	/* standard deviation */

int
pStatsHeight()
{
   return STAT_HEIGHT;
}

int
pStatsWidth()
{
   return STAT_WIDTH;
}

void
initPStats()
{
   int             i;

   if (pinitialized)
      return;
   pinitialized = 1;
   psliders[0].var = (int *) &ping_lag;
   psliders[1].var = (int *) &ping_av;
   psliders[2].var = (int *) &ping_sd;
   psliders[3].var = (int *) &ping_iloss_sc;
   psliders[4].var = (int *) &ping_iloss_cs;
   psliders[5].var = (int *) &ping_tloss_sc;
   psliders[6].var = (int *) &ping_tloss_cs;

   /* adjust */
   if (ping_av > 0) {
      psliders[0].max = MAX(ping_av * 2, 200);
      psliders[1].max = MAX(ping_av * 2, 200);
   }
   for (i = 0; i < NUM_PSLIDERS; i++) {
      psliders[i].label_length = strlen(psliders[i].label);
      pTextWidth = MAX(pTextWidth, psliders[i].label_length);
      psliders[i].diff = psliders[i].max - psliders[i].min;
      psliders[i].lastVal = 0;
   }
}

void
redrawPStats()
{
   int             i;

   W_ClearWindow(pStats);
   initPStats();
   for (i = 0; i < NUM_PSLIDERS; i++) {
      psliders[i].lastVal = 0;
   }
   for (i = 0; i < NUM_PSLIDERS; i++) {
      W_WriteText(pStats, TX_OFF(psliders[i].label_length), TY_OFF(i),
		  textColor, psliders[i].label, psliders[i].label_length,
		  W_RegularFont);
      pbox(0, BX_OFF() - 1, BY_OFF(i) - 1, SL_WID + 2, SL_HEI + 2, borderColor);
      psliders[i].lastVal = 0;
   }
   if (ping)
      updatePStats();
}

void
updatePStats()
{
   int             i, value, diff, old_x, new_x;
   W_Color         color;
   PSLIDER         *s;

   /* do the average and standard deviation calculations */
   initPStats();

   for (i = 0; i < NUM_PSLIDERS; i++) {
      s = &psliders[i];
      value = *(s->var);
      /* update decimal values at the right */
      text(*(s->var), BY_OFF(i));

      if (value < s->min)
	 value = s->min;
      else if (value > s->max)
	 value = s->max;
      if (value == s->lastVal)
	 continue;
      diff = value - s->lastVal;
      if (diff < 0) {		/* bar decreasing */
	 old_x = s->lastVal * SL_WID / s->diff;
	 new_x = value * SL_WID / s->diff;
	 pbox(1, BX_OFF() + new_x, BY_OFF(i), old_x - new_x, SL_HEI, backColor);

	 if (s->lastVal > s->green && value <= s->green)
	    pbox(1, BX_OFF(), BY_OFF(i), new_x, SL_HEI, gColor);
	 else if (s->lastVal > s->yellow && value <= s->yellow)
	    pbox(1, BX_OFF(), BY_OFF(i), new_x, SL_HEI, yColor);
      } else {			/* bar increasing */
	 if (s->lastVal <= s->yellow && value > s->yellow) {
	    color = rColor;
	    s->lastVal = 0;
	 } else if (s->lastVal <= s->green && value > s->green) {
	    color = yColor;
	    s->lastVal = 0;
	 } else if (value > s->yellow)
	    color = rColor;
	 else if (value > s->green)
	    color = yColor;
	 else
	    color = gColor;

	 old_x = s->lastVal * SL_WID / s->diff;
	 new_x = value * SL_WID / s->diff;
	 pbox(1, BX_OFF() + old_x, BY_OFF(i), new_x - old_x, SL_HEI, color);
      }
      s->lastVal = value;
   }
}

static void
pbox(filled, x, y, wid, hei, color)
    int             filled, x, y, wid, hei;
    W_Color         color;
{
   if (wid == 0)
      return;

   if (filled) {
      /* XFIX */
      W_FillArea(pStats, x, y, wid + 1, hei + 1, color);
      return;
   }
   W_MakeLine(pStats, x, y, x + wid, y, color);
   W_MakeLine(pStats, x + wid, y, x + wid, y + hei, color);
   W_MakeLine(pStats, x + wid, y + hei, x, y + hei, color);
   W_MakeLine(pStats, x, y + hei, x, y, color);
}

static void
text(value, y)
    int             value, y;
{
   extern char    *itoa();
   char            buf[6], *s = buf;

   strncpy(s, "          ", 6);

   *s++ = '(';
   s = itoa(s, value, 4, 0);
   *s++ = ')';

   W_WriteText(pStats, STAT_WIDTH - TEXT_WIDTH, y, textColor,
	       buf, 5, W_RegularFont);
}

#endif
