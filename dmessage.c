/*
 * dmessage.c
 * 
 * for the client of a socket based protocol.
 */
#include "copyright.h"

#include <stdio.h>
#include <string.h>
#ifndef SVR4
#include <strings.h>
#endif
#include <math.h>
#include "netrek.h"

extern struct dmacro_list *distmacro;

void
dmessage(message, flags, from, to)
    char           *message;
    unsigned char   flags, from, to;
{
   register int    len;
   W_Color         color;
   W_Window        targwin;
   int		   review = 1;

   if (!me) {
      /* not yet in game */
      return;
   }
   len = strlen(message);
   if (from == 255) {
      /* From God */
      color = textColor;
   } else {
      color = playerColor(&(players[from]));
   }

   if (new_messages) {
      int             take, destroy, team, kill, killp, killa, bomb, conq;

      take = MTEAM + MTAKE + MVALID;
      destroy = MTEAM + MDEST + MVALID;
      kill = MALL + MKILL + MVALID;
      killp = MALL + MKILLP + MVALID;
      killa = MALL + MKILLA + MVALID;
      bomb = MTEAM + MBOMB + MVALID;
      team = MTEAM + MVALID;
      conq = MALL + MCONQ + MVALID;

#ifdef FEATURE
      if (flags == (MCONFIG + MINDIV + MVALID) && from == 255) {
	 CheckFeatures(message);
#ifdef LOGMESG
	 if (logMess)
	    LogMessage(message);
#endif
	 return;
      }

      /* aha! A new type distress/macro call came in. parse it appropriately */
      if (flags == (MTEAM | MDISTR | MVALID)) {
	 struct distress dist;

	 HandleGenDistr(message, from, to, &dist);
	 len = makedistress(&dist, message, distmacro[dist.distype].macro);

	 if (len <= 0)
	    return;
	 flags ^= MDISTR;
      }
#endif

      if (flags == conq)
	 fprintf(stdout, "%s\n", message);
      if ((flags == team) || (flags == take) || (flags == destroy)) {
	 if(!reportTeamInReview) 
	    review = 0;
	 targwin = messwt;
	 W_WriteText(messwt, 0, 0, color, message, len, W_MesgFont);
      } else if ((flags == kill) || (flags == killp) || (flags == killa) ||
		 (flags == bomb)) {
	 if (reportKills)
	    W_WriteText(messwk, 0, 0, color, message, len, W_MesgFont);
	 else {
#ifdef LOGMESG
	    if (logMess)
	       LogMessage(message);
#endif
	    return;
	 }
	 /* don't send 'kill' style messages to review (total messages)
	    window if not requested */
	 if(!reportKillsInReview) 
	    review = 0;
	 targwin = messwk;
      } else if (flags & MINDIV) {
	 if(!reportIndInReview) 
	    review = 0;
	 W_WriteText(messwi, 0, 0, color, message, len, W_MesgFont);
	 targwin = messwi;
      } else {			/* if we don't know where the message belongs
				 * by this time, stick it in the all board... */
	 if(!reportAllInReview) 
	    review = 0;
	 targwin = messwa;
	 W_WriteText(messwa, 0, 0, color, message, len, W_MesgFont);
      }

      if(review)
	 W_WriteText(reviewWin, 0, 0, color, message, len, W_MesgFont);
#ifdef TTS
      if (tts && from < MAXPLAYER && players[from].p_team == me->p_team &&
	  from != me->p_no && ((flags == team) || (flags & MINDIV))) {
	 char           *s = message, *t = lastIn;
	 if (len > tts_max_len)
	    tts_len = tts_max_len;
	 else
	    tts_len = len;

	 /* RCD hack (need to add rcd) */

	 if (*s == ' ') {
	    *t++ = s[1];
	    *t++ = s[2];
	    *t++ = ':';
	    s += 9;
	    tts_len -= 6;
	 }
	 strncpy(t, s, tts_len);
	 t[tts_len - 1] = 0;
	 tts_width = W_TTSTextWidth(lastIn, tts_len);
	 tts_timer = tts_time;
      }
#endif
   } else {

      /* Kludge stuff for report kills... */
      if ((strncmp(message, "GOD->ALL", 8) == 0 &&
	   (instr(message, "was kill") ||
	    instr(message, "killed by"))) ||
	  (*message != ' ' && instr(message, "We are being attacked"))) {
	 W_WriteText(messwk, 0, 0, color, message, len, W_MesgFont);
	 if (!reportKills)
	    return;
	 if(reportKillsInReview)
	    W_WriteText(reviewWin, 0, 0, color, message, len, W_MesgFont);
	 return;
      }
#if defined(PIG_RESPONSE) || defined(FOR_MORONS)
      /* PIG CALL! */
      if (instr(message, "     ")) {
#ifdef FOR_MORONS
	 extern int      For_Morons;
	 if (For_Morons) {
	    pmessage("I'm a Moron!!", 0, MALL);
	 }
#else
	 pmessage(PIG_RESPONSE, from, MINDIV);
#endif				/* FOR_MORONS */
      }
#endif				/* PIG_RESPONSE | FOR_MORONS */

      if (flags & MTEAM) {
	 W_WriteText(messwt, 0, 0, color, message, len, W_MesgFont);
	 if(!reportTeamInReview)
	    review = 0;
	 targwin = messwt;
      } else if ((flags & MINDIV)
#ifdef nodef
		 || (flags & MGOD)
#endif
	 ) {
	 W_WriteText(messwi, 0, 0, color, message, len, W_MesgFont);
	 if(!reportIndInReview)
	    review = 0;
	 targwin = messwi;
      } else {
	 if(!reportAllInReview)
	    review = 0;
	 W_WriteText(messwa, 0, 0, color, message, len, W_MesgFont);
	 targwin = messwa;
      }

      if(review)
	 W_WriteText(reviewWin, 0, 0, color, message, len, W_MesgFont);
#ifdef TTS
      if (tts && from < MAXPLAYER && players[from].p_team == me->p_team &&
	  from != me->p_no && ((flags & MTEAM) || (flags & MINDIV))) {
	 char           *s = message, *t = lastIn;

	 if (len > tts_max_len)
            tts_len = tts_max_len;
         else
            tts_len = len;

	 /* RCD hack (need to add rcd) */
	 if (*s == ' ') {
	    *t++ = s[1];
	    *t++ = s[2];
	    *t++ = ':';
	    s += 9;
	    tts_len -= 6;
	 }
	 strncpy(t, s, tts_len);
         t[tts_len - 1] = 0;
         tts_width = W_TTSTextWidth(lastIn, tts_len);
         tts_timer = tts_time;
      }
#endif
   }
#ifdef EM
   /*
    * send warnings to warning or message window, if player doesn't have
    * messages mapped
    */
   if ((use_msgw && (targwin == messwi || targwin == messwt)) ||
       (!W_IsMapped(targwin) && !W_IsMapped(reviewWin))) {
      if (!messpend && messagew) {	/* don't collide with messages being
					 * written! */
	 W_ClearWindow(messagew);
	 W_WriteText(messagew, 5, 5, color, message, len, W_RegularFont);
      } else
	 warning(message);
   }
#endif
#ifdef LOGMESG
   if (logMess)
      LogMessage(message);
#endif
}

int
instr(string1, string2)
    char           *string1, *string2;
{
   char           *s;
   int             length;

   length = strlen(string2);
   for (s = string1; *s != 0; s++) {
      if (*s == *string2 && strncmp(s, string2, length) == 0)
	 return (1);
   }
   return (0);
}
