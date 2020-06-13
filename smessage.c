/*
 * smessage.c
 * 
 * $Log: smessage.c,v $
 * Revision 1.2  2000/02/17 05:48:05  ahn
 * BRMH 2.3 from David Pinkney <dpinkney@cs.uml.edu>
 *
 * Revision 1.6  1993/10/05  16:40:38  hadley
 * checkin
 *
 * Revision 1.6  1993/10/05  16:38:08  hadley
 * checkin
 * Revision 1.3  1993/03/03  19:32:02  jmn Mehlhaff's
 * version
 * 
 * Revision 1.2  1993/02/02  21:26:08  jmn Made Distress calls quasi-intelligent
 * - will say things like FUEL OUT or WARP 1! depending on conditions when
 * distress given. Levels for FUEL OUT and SHIELDS OUT and stuff like that
 * are arbitrarily hard-coded- this should probably be stuck in .xtrekrc
 * somewhere but I couldn't be bothered. jmn
 * 
 * 
 */
#include "copyright.h"

#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <ctype.h>
#include <X11/XKBlib.h>
#include "netrek.h"

#define ADDRLEN 10
#define MAX_MESSAGE_LEN		79

/* XFIX */
#define BLANKCHAR(col, n) W_ClearArea(messagew, 5+W_Textwidth*(col), 5, \
    W_Textwidth * (n), W_Textheight);
#define DRAWCURSOR(col) W_WriteText(messagew, 5+W_Textwidth*(col), 5, \
    textColor, &cursor, 1, W_RegularFont);

static int      lcount;
static char     sbuf[MAX_MESSAGE_LEN+1];
static char     cursor = '_';
static char     addr, *addr_str;

char           *getaddr(), *getaddr2();

void
smessage(ichar)
    unsigned char 	ichar;
{
   register int    i;
   char           *getaddr();
   char            twochar[2];

   if (messpend == 0) {
      messpend = 1;

      /* clear out the message window, in case mesages went there ! */
      W_ClearWindow(messagew);

      if (mdisplayed) {
	 BLANKCHAR(0, lastcount);
	 mdisplayed = 0;
      }
      /* Put the proper recipient in the window */
      switch(ichar){
	 case 't':
	 case 'T':
	 addr = teamlet[me->p_team];
	 break;
      default:
	 addr = ichar;
      }
      addr_str = getaddr(addr);
      if (addr_str == 0) {
	 /* print error message */
	 messpend = 0;
	 message_off();
	 return;
      }
      W_WriteText(messagew, 5, 5, textColor, addr_str, strlen(addr_str),
		  W_RegularFont);
      lcount = ADDRLEN;
      DRAWCURSOR(ADDRLEN);
      return;
   }

   switch (ichar){	/* why & ~0x80? */
#ifdef CONTROL_KEY
   case 'h'+96:
   case 'H'+96:
#endif
   case '\b':			/* character erase */
   case '\177':
      if (--lcount < ADDRLEN) {
	 lcount = ADDRLEN;
	 break;
      }
      BLANKCHAR(lcount + 1, 1);
      DRAWCURSOR(lcount);
      break;

   case '\027':		/* word erase */
      i = 0;
      /* back up over blanks */
      while (--lcount >= ADDRLEN &&
	     isspace((unsigned char) sbuf[lcount - ADDRLEN] & ~(0x80)))
	 i++;
      lcount++;
      /* back up over non-blanks */
      while (--lcount >= ADDRLEN &&
	     !isspace((unsigned char) sbuf[lcount - ADDRLEN] & ~(0x80)))
	 i++;
      lcount++;

      if (i > 0) {
	 BLANKCHAR(lcount, i + 1);
	 DRAWCURSOR(lcount);
      }
      break;

#ifdef CONTROL_KEY
   case 'u'+96:
   case 'U'+96:
#endif
   case '\025':		/* kill line */
   case '\030':
      if (lcount > ADDRLEN) {
	 BLANKCHAR(ADDRLEN, lcount - ADDRLEN + 1);
	 lcount = ADDRLEN;
	 DRAWCURSOR(ADDRLEN);
      }
      break;

#ifdef CONTROL_KEY
   case '['+96:
#endif
   case '\033':		/* abort message */
      BLANKCHAR(0, lcount + 1);
      mdisplayed = 0;
      messpend = 0;
      message_off();
      break;

#ifdef CONTROL_KEY
   case 'm'+96:
   case 'M'+96:
   case 'j'+96:
   case 'J'+96:
#endif
   case '\n':
   case '\r':			/* send message */
      sbuf[lcount - ADDRLEN] = '\0';
      messpend = 0;
      switch (addr) {
      case 'A':
	 pmessage(sbuf, 0, MALL);
	 break;
      case 'F':
	 pmessage(sbuf, FED, MTEAM);
	 break;
      case 'R':
	 pmessage(sbuf, ROM, MTEAM);
	 break;
      case 'K':
	 pmessage(sbuf, KLI, MTEAM);
	 break;
      case 'O':
	 pmessage(sbuf, ORI, MTEAM);
	 break;
#ifdef GODMESSAGE
      case 'G':
	 pmessage(sbuf, 255, MGOD);
	 break;
#endif
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
          if (players[addr - '0'].p_status == PFREE)
            {
              warning ("That player left the game. message not sent.");
              return;
            }
          pmessage (sbuf, addr - '0', MINDIV);
          break;
      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f':
      case 'g':
      case 'h':
      case 'i':
      case 'j':
      case 'k':
      case 'l':
      case 'm':
      case 'n':
      case 'o':
      case 'p':
      case 'q':
      case 'r':
      case 's':
      case 't':
      case 'u':
      case 'v':
      case 'w':
      case 'x':
      case 'y':
      case 'z':
          if (players[addr - 'a' + 10].p_status == PFREE)
            {
              warning ("That player left the game. message not sent.");
              return;
            }
          pmessage (sbuf, addr - 'a' + 10, MINDIV);
	 break;
      default:
	 warning("Not legal recipient");
      }
      BLANKCHAR(0, lcount + 1);
      mdisplayed = 0;
      lcount = 0;
      break;

   default:			/* add character */
      if (lcount >= MAX_MESSAGE_LEN) {
	 W_Beep();
	 break;
      }
      if (iscntrl((unsigned char) ichar & ~(0x80)))
	 break;
      twochar[0] = ichar;
      twochar[1] = cursor;
      W_WriteText(messagew, 5 + W_Textwidth * lcount, 5, textColor,
		  twochar, 2, W_RegularFont);
      sbuf[(lcount++) - ADDRLEN] = ichar;
      break;
   }
}

/* paste into smessage window */
void
smess_paste()
{
   register	i, nl=0;
   int		len, toowide;
   char		*s, *m = s = W_FetchBuffer(&len);

   if(!len)
      return;
   
   for(i=0; i< len; i++,s++){

      /* use the first character as the destination address */
      if(!messpend){
	 addr = *s;
	 smessage(*s);
	 if(!messpend){	/* invalid address, abort paste */
	    break;
	 }
	 continue;
      }
      /* otherwise 'addr' already contains a valid destination */
      toowide = lcount >= MAX_MESSAGE_LEN;

      if(toowide || *s == '\n' || *s == '\r'){
	 /* break up lines greater then MAX_MESSAGE_LEN */
	 nl ++;
	 if(nl == 5 && i < len-1){
	   warning("Paste message truncated to 5 lines.");
	   smessage('\n');
	   break;
	 }
	 else if(i < len -1){
	    /* more coming, put original address */
	    smessage('\n');
	    smessage(addr);
	    if(!messpend){	/* invalid address (player left game)
				   abort paste */
	       break;
	    }
	    if(!toowide)
	       continue;
	 }
      }
      smessage(*s);
   }
   W_FreeBuffer(m);
}

/* refresh the smessage window */
void
smess_refresh()
{
   if (messpend == 1) {
      W_WriteText(messagew, 5, 5, textColor, addr_str, strlen(addr_str),
		  W_RegularFont);
      W_WriteText(messagew, 5 + W_Textwidth * ADDRLEN, 5, textColor,
		  sbuf, lcount - ADDRLEN , W_RegularFont);
      W_WriteText(messagew, 5 + W_Textwidth * lcount, 5, textColor,
		  &cursor, 1, W_RegularFont);
   } else {
      /* no pending message, handle by clearing */
      W_ClearWindow(messagew);
   }
}

void
pmessage(str, recip, group)
    char           *str;
    int             recip;
    int             group;
{
   sendMessage(str, group, recip);
   sprintf(lastMessage, "%s  %s", getaddr2(group, recip), str);
   lastMessage[79] = 0;

   if ((group == MTEAM && recip != me->p_team) ||
       (group == MINDIV && recip != me->p_no) ||
       (recip == 255)){
      /* it won't be echoed by the server, show it here */
      dmessage(lastMessage, group, me->p_no, recip);
   }

   message_off();
}

char           *
getaddr(who)
    char            who;
{
   switch (who) {
   case 'A':
      return (getaddr2(MALL, 0));
   case 'F':
      return (getaddr2(MTEAM, FED));
   case 'R':
      return (getaddr2(MTEAM, ROM));
   case 'K':
      return (getaddr2(MTEAM, KLI));
   case 'O':
      return (getaddr2(MTEAM, ORI));
#ifdef GODMESSAGE
   case 'G':
      return (getaddr2(MGOD, 0));
#endif
   case '0':
   case '1':
   case '2':
   case '3':
   case '4':
   case '5':
   case '6':
   case '7':
   case '8':
   case '9':
      if (players[who - '0'].p_status == PFREE)
        {
          warning ("Slot is not alive.");
          return 0;
        }
      return (getaddr2 (MINDIV, who - '0'));
      break;
   case 'a':
   case 'b':
   case 'c':
   case 'd':
   case 'e':
   case 'f':
   case 'g':
   case 'h':
   case 'i':
   case 'j':
   case 'k':
   case 'l':
   case 'm':
   case 'n':
   case 'o':
   case 'p':
   case 'q':
   case 'r':
   case 's':
   case 't':
   case 'u':
   case 'v':
   case 'w':
   case 'x':
   case 'y':
   case 'z':
      if (who - 'a' + 10 > MAXPLAYER) {
	 warning("Player is not in game");
	 return (0);
      }
      if (players[who - 'a' + 10].p_status == PFREE)
        {
          warning ("Slot is not alive.");
          return 0;
      }
      return (getaddr2 (MINDIV, who - 'a' + 10));
      break;
   default:
      warning("Not legal recipient");
      return (0);
   }
}

char           *
getaddr2(flags, recip)
    int             flags;
    int             recip;
{
   static char     addrmesg[ADDRLEN];

   (void) sprintf(addrmesg, " %c%c->", teamlet[me->p_team], shipnos[me->p_no]);
   switch (flags) {
   case MALL:
      (void) sprintf(&addrmesg[5], "ALL");
      break;
   case MTEAM:
      if(maskrecip){
	 (void) sprintf(&addrmesg[5], "???");
	 maskrecip = 0;
      }
      else
	 (void) sprintf(&addrmesg[5], teamshort[recip]);
      break;
   case MINDIV:
      if(maskrecip){
	 (void) sprintf(&addrmesg[5], "?? ");
	 maskrecip = 0;
      }
      else
	 (void) sprintf(&addrmesg[5], "%c%c ",
			teamlet[players[recip].p_team], shipnos[recip]);
      break;
#ifdef GODMESSAGE
   case MGOD:
      (void) sprintf(&addrmesg[5], "GOD");
      break;
#endif
   }
   return (addrmesg);
}


/* Send an emergency signal out to everyone. */
void
emergency()
{
#ifdef NBTDIST
   char            ebuf[200];

   int             index = 0;
   char            temp[50];
   int             dam, shld, arms, wtmp, etmp, fuel;

   /*
    * Most of this is thanks to Nick Trown - since this can do old style
    * distresses we trash the old style code and keep it only within RCS
    */
   if (myship->s_type == STARBASE) {	/* Do SB distress */
      index = 1;
      sprintf(ebuf, "Help! %c%c(Base!):",
	      teamlet[me->p_team], shipnos[me->p_no]);
   } else
      sprintf(ebuf, "Help! %c%c:",	/* normal distress */
	      teamlet[me->p_team], shipnos[me->p_no]);
   dam = (100 * me->p_damage) / me->p_ship.s_maxdamage;
   shld = (100 * me->p_shield) / me->p_ship.s_maxshield;
   arms = me->p_armies;
   wtmp = (100 * me->p_wtemp) / me->p_ship.s_maxwpntemp;
   etmp = (100 * me->p_etemp) / me->p_ship.s_maxegntemp;
   fuel = ((100 * me->p_fuel) / me->p_ship.s_maxfuel);

   /*
    * Let's try to be a little smart about this. Usually a player will only
    * distress if there is something wrong. So check for them first.
    */

   /*
    * DEBUGGING STUFF printf ("dam:%d shld:%d arms:%d wtmp:%d etmp:%d
    * fuel:%d\n",dam,shld,arms,wtmp,etmp,fuel); printf ("MIN:dam:%d shld:%d
    * arms:%d wtmp:%d etmp:%d
    * fuel:%d\n",distress[index].min_dam,distress[index].min_shld,distress[ind
    * ex].min_arms,distress[index].min_wtmp,distress[index].min_etmp,distress[
    * index].min_fuel); printf ("MAX:dam:%d shld:%d arms:%d wtmp:%d etmp:%d
    * fuel:%d\n",distress[index].max_dam,distress[index].max_shld,distress[ind
    * ex].max_arms,distress[index].max_wtmp,distress[index].max_etmp,distress[
    * index].max_fuel);
    */

   if (distress[index].dam_on) {
      if (dam < distress[index].min_dam)
	 sprintf(temp, distress[index].low_dam, dam);
      else if (dam < distress[index].max_dam)
	 sprintf(temp, distress[index].mid_dam, dam);
      else
	 sprintf(temp, distress[index].high_dam, dam);
      if (temp[0] != '_')
	 sprintf(ebuf, "%s %s,", ebuf, temp);
   }
   if (distress[index].shld_on) {
      if (shld < distress[index].min_shld)
	 sprintf(temp, distress[index].low_shld, shld);
      else if (shld < distress[index].max_shld)
	 sprintf(temp, distress[index].mid_shld, shld);
      else
	 sprintf(temp, distress[index].high_shld, shld);
      if (temp[0] != '_')
	 sprintf(ebuf, "%s %s,", ebuf, temp);
   }
   if (distress[index].etmp_on) {
      if (me->p_flags & PFENG)
	 sprintf(temp, "ETEMP!");
      else if (etmp < distress[index].min_etmp)
	 sprintf(temp, distress[index].low_etmp, etmp);
      else if (etmp < distress[index].max_etmp)
	 sprintf(temp, distress[index].mid_etmp, etmp);
      else
	 sprintf(temp, distress[index].high_etmp, etmp);
      if (temp[0] != '_')
	 sprintf(ebuf, "%s %s,", ebuf, temp);
   }
   if (distress[index].fuel_on) {
      if (fuel < distress[index].min_wtmp)
	 sprintf(temp, distress[index].low_fuel, fuel);
      else if (fuel < distress[index].max_fuel)
	 sprintf(temp, distress[index].mid_fuel, fuel);
      else
	 sprintf(temp, distress[index].high_fuel, fuel);
      if (temp[0] != '_')
	 sprintf(ebuf, "%s %s,", ebuf, temp);
   }
   if (distress[index].wtmp_on) {
      if (me->p_flags & PFWEP)
	 sprintf(temp, "WTEMPED!");
      else if (wtmp < distress[index].min_wtmp)
	 sprintf(temp, distress[index].low_wtmp, wtmp);
      else if (wtmp < distress[index].max_wtmp)
	 sprintf(temp, distress[index].mid_wtmp, wtmp);
      else
	 sprintf(temp, distress[index].high_wtmp, wtmp);
      if (temp[0] != '_')
	 sprintf(ebuf, "%s %s,", ebuf, temp);
   }
   if (distress[index].arms_on) {
      if (arms < distress[index].min_arms)
	 sprintf(temp, distress[index].low_arms, arms);
      else if (arms < distress[index].max_arms)
	 sprintf(temp, distress[index].mid_arms, arms);
      else
	 sprintf(temp, distress[index].high_arms, arms);
      if (temp[0] != '_')
	 sprintf(ebuf, "%s %s,", ebuf, temp);
   }
   ebuf[strlen(ebuf) - 1] = '\0';	/* get rid of last ',' */

#else
   char ebuf[80];
   char wtempstring[40];

   if (myship->s_type==STARBASE) {
      if (me->p_flags & PFWEP) {
	 strcpy(wtempstring, "WTEMPED!");
      } else {
	 strcpy(wtempstring, "!");
      }
      sprintf(ebuf,
"HELP! %c%c (BASE):  %d%% dmg, %d%% shld, %d men, %d%% Wtmp %s",
            teamlet[me->p_team], shipnos[me->p_no],
            (100*me->p_damage)/me->p_ship.s_maxdamage,
            (100*me->p_shield)/me->p_ship.s_maxshield,
            me->p_armies,
            (100*me->p_wtemp)/me->p_ship.s_maxwpntemp,
            wtempstring);
   } else {
      if (me->p_flags & (PFENG|PFWEP)) {
	 if (me->p_flags & PFENG) {
	    strcpy(wtempstring, " & ETEMPED!");
	    sprintf(ebuf,
          "HELP! %c%c: %d%% dmg, %d%% shld, %d%%fuel, %d men, ETEMPED!!",
                  teamlet[me->p_team], shipnos[me->p_no],
                  (100*me->p_damage)/me->p_ship.s_maxdamage,
                  (100*me->p_shield)/me->p_ship.s_maxshield,
                  ((100 * me->p_fuel)/me->p_ship.s_maxfuel),
                  me->p_armies ); } else {
	    strcpy(wtempstring, "!"); }
	    if (me->p_flags & PFWEP) {
	    sprintf(ebuf,
          "HELP! %c%c: %d%% dmg, %d%% shld, %d%%fuel, %d men, WTEMPED!%s",
                  teamlet[me->p_team], shipnos[me->p_no],
                  (100*me->p_damage)/me->p_ship.s_maxdamage,
                  (100*me->p_shield)/me->p_ship.s_maxshield,
                  ((100 * me->p_fuel)/me->p_ship.s_maxfuel),
                  me->p_armies, wtempstring ); }
      } else {
	 sprintf(ebuf,
	 "Distress Call from %c%c:  %d%% dmg, %d%% shld, %d%% fuel, %d armies!",
	       teamlet[me->p_team], shipnos[me->p_no],
	       (100*me->p_damage)/me->p_ship.s_maxdamage,
	       (100*me->p_shield)/me->p_ship.s_maxshield,
	       ((100 * me->p_fuel)/me->p_ship.s_maxfuel),
	       me->p_armies );
      }
   }
#endif
   pmessage(ebuf, me->p_team, MTEAM);
}

#ifdef EM
/* function to send a message to team saying how many armies are carried */
void
army_report()
{
   char            mbuf[80];
   char            arstring[40];

   switch (me->p_armies) {
   case 1:
      strcpy(arstring, "one measly army");
      break;
   case 0:
      strcpy(arstring, "NO armies");
      break;
   default:
      sprintf(arstring, "%d armies", me->p_armies);
   }
   if (myship->s_type == STARBASE) {
      sprintf(mbuf, "Your Starbase(%c%c) is carrying %s!",
	      teamlet[me->p_team], shipnos[me->p_no], arstring);
   } else {
      sprintf(mbuf, "I have %s on board!", arstring);
   }
   pmessage(mbuf, me->p_team, MTEAM);
}
#endif				/* EM */

void
message_on()
{
   if (warp) {
      W_WarpPointer(messagew);
   }
#ifdef TCURSORS
   else {
      messageon = 1;
      W_DefineTextCursor(w);
      W_DefineTextCursor(mapw);
   }
#endif				/* TCURSORS */
}

void
message_off()
{
   if (warp) {
      W_WarpPointer(NULL);
   }
#ifdef TCURSORS
   else {
      messageon = 0;
#ifdef MOOCURSORS
      DefineLocalcursor(w);
      DefineMapcursor(mapw);
#else
      W_DefineTCrossCursor(w);
      W_DefineTCrossCursor(mapw);
#endif				/* MOOCURSORS */
   }
#endif				/* TCURSORS */
}
