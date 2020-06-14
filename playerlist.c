
/*
 * playerlist.c
 * 
 * Borrowed from COW-lite
 *
 * Fairly substantial re-write to do variable player lists: Sept 93 DRG
 */

#include "copyright.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "netrek.h"

#ifdef RECORD
#include "recorder.h"
#endif

#define DEFAULT		"nTRNKWLr O D d "
#define NP_DEFAULT	"nTRN K     lM"

#if __STDC__ || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

/* playerlist.c */
static void make_header P_((void));
static void pline P_((struct player *j, int pos));

#undef P_

static char     header[BUFSIZ];
static int	header_len;


/*
alt_playerlist plist 

00			default
01			plist
10			np_default
11			plist
*/

/*
 * ===========================================================================
 * */

static void 
make_header()
{
   register char	*ptr, *pflist, *h = header;

   if(plist){
      /* alt_playerlist takes on different meaning here */
      if(alt_playerlist)
	 pflist = DEFAULT;
      else
	 pflist = plist;
   }
   else
      /* note that alt_playerlist is overridden if playerlist
	 has been specified */
      pflist = alt_playerlist ? NP_DEFAULT : DEFAULT;

   list_needs_stats = 0;

   ptr = pflist;
   header[0] = '\0';
   while (ptr[0] != '\0') {
      switch (ptr[0]) {
      case 'n':		/* Ship Number */
	 strcpy(h, " No");
	 h += 3;
	 break;
      case 'T':		/* Ship Type */
	 strcpy(h, " Ty");
	 h += 3;
	 break;
      case 'R':		/* Rank */
	 strcpy(h, " Rank      ");
	 h += 11;
	 break;
      case 'N':		/* Name */
	 strcpy(h, " Name            ");
	 h += 17;
	 break;
      case 'K':		/* Kills */
	 strcpy(h, " Kills");
	 h += 6;
	 break;
      case 'l':		/* Login Name */
	 strcpy(h, " Login           ");
	 h += 17;
	 break;
      case 'O':		/* Offense */
	 strcpy(h, " Offse");
	 h += 6;
	 list_needs_stats = 1;
	 break;
      case 'W':		/* Wins */
	 strcpy(h, "  Wins");
	 h += 6;
	 list_needs_stats = 1;
	 break;
      case 'D':		/* Defense */
	 strcpy(h, " Defse");
	 h += 6;
	 list_needs_stats = 1;
	 break;
      case 'L':		/* Losses */
	 strcpy(h, "  Loss");
	 h += 6;
	 list_needs_stats = 1;
	 break;
      case 'S':		/* Total Rating (stats) */
	 strcpy(h, " Stats");
	 h += 6;
	 list_needs_stats = 1;
	 break;
      case 'r':		/* Ratio */
	 strcpy(h, " Ratio");
	 h += 6;
	 list_needs_stats = 1;
	 break;
      case 'd':		/* Damage Inflicted (DI) */
	 strcpy(h, "      DI");
	 h += 8;
	 list_needs_stats = 1;
	 break;
      case ' ':		/* White Space */
	 strcpy(h, " ");
	 h += 1;
	 break;
      case 'B':		/* Bombing */
	 strcpy(h, " Bmbng");
	 h += 6;
	 list_needs_stats = 1;
	 break;
      case 'b':		/* Armies Bombed */
	 strcpy(h, " Bmbed");
	 h += 6;
	 list_needs_stats = 1;
	 break;
      case 'P':		/* Planets */
	 strcpy(h, " Plnts");
	 h += 6;
	 list_needs_stats = 1;
	 break;
      case 'p':		/* Planets Taken */
	 strcpy(h, " Plnts");
	 h += 6;
	 list_needs_stats = 1;
	 break;
      case 'M':		/* Display, Host Machine */
	 strcpy(h, " Host Machine    ");
	 h += 17;
	 break;
      case 'H':		/* Hours Played */
	 strcpy(h, " Hours ");
	 h += 7;
	 break;
      case 'k':		/* Max Kills */
	 strcpy(h, " Max K");
	 h += 6;
	 list_needs_stats = 1;
	 break;
      case 'V':		/* Kills per hour */
	 strcpy(h, "   KPH");
	 h += 6;
	 list_needs_stats = 1;
	 break;
      case 'v':		/* Deaths per hour */
	 strcpy(h, "   DPH");
	 h += 6;
	 list_needs_stats = 1;
	 break;
      default:
	 fprintf(stderr, "playerlist: %c is not a valid option.\n", 
	    ptr[0]);
	 break;
      }
      ptr++;
   }
   header_len = h - header;
}

/*
 * ===========================================================================
 * */

void
redraw_playerlist_header()
{
   W_WriteText(playerw, 0, 1, textColor, header, header_len, W_BoldFont);
}

void
playerlist()
{
   int             i;

   if (!W_IsMapped(playerw))
      return;

   /* this procedure only called once in a while, not for every player */

   make_header();
   redraw_playerlist_header();

   for (i = 0; i < MAXPLAYER; i++) {
      updatePlayer[i] = 1;
   }

   playerlist2();
}

/*
 * ===========================================================================
 * */

static void
pline(j, pos)
    struct player  *j;
    int             pos;
{
   char            	buf[BUFSIZ], *b = buf;
   register char	*ptr, *pflist;

   if(plist){
      /* alt_playerlist takes on different meaning here */
      if(alt_playerlist)
	 pflist = DEFAULT;
      else
	 pflist = plist;
   }
   else
      /* note that alt_playerlist is overridden if playerlist
	 has been specified */
      pflist = alt_playerlist ? NP_DEFAULT : DEFAULT;

#define Kills(j)	(IsStarbase(j) ? j->p_stats.st_sbkills : \
			       (j->p_stats.st_kills + j->p_stats.st_tkills))

#define Losses(j)	(IsStarbase(j) ? j->p_stats.st_sblosses : \
			       (j->p_stats.st_losses + j->p_stats.st_tlosses))

#define MaxKills(j)	(IsStarbase(j) ? j->p_stats.st_sbmaxkills :\
				j->p_stats.st_maxkills)

#define Ticks(j)	(j->p_stats.st_tticks)

   ptr = pflist;
   buf[0] = '\0';
   while (ptr[0] != '\0') {

      *b++ = ' ';
      switch (ptr[0]) {
      case 'n':		/* Ship Number */
	 if (j->p_status != PALIVE) {
	    *b++ = ' ';
	    *b++ = shipnos[j->p_no];
	 } else {
	    *b++ = teamlet[j->p_team];
	    *b++ = shipnos[j->p_no];
	 }
	 break;

      case 'T':		/* Ship Type */
	 if (j->p_status != PALIVE) {
	    *b++ = ' ';
	    *b++ = ' ';
	 } else {
	    *b++ = classes[j->p_ship.s_type][0];
	    *b++ = classes[j->p_ship.s_type][1];
	 }
	 break;
      case 'R':		/* Rank */
	 b = strcpyp_return(b, ranks[j->p_stats.st_rank].name, 10);
	 break;
      case 'N':		/* Name */
	 b = strcpyp_return(b, j->p_name, 16);
	 break;
      case 'K':		/* Kills */
	 if ((j->p_status == PALIVE) && (j->p_kills != 0.00))
	    b = itof22(b, j->p_kills);
	 else
	    b = strcpy_return(b, "     ");
	 break;
      case 'l':		/* Login Name */
	 b = strcpyp_return(b, j->p_login, 16);
	 break;
      case 'O':		/* Offense */
	 b = itof22(b, offenseRating(j));
	 break;
      case 'W':		/* Wins */
	 b = itoa(b, Kills(j), 5, 1);
	 break;
      case 'D':		/* Defense */
	 b = itof22(b, defenseRating(j));
	 break;
      case 'L':		/* Losses */
	 b = itoa(b, Losses(j), 5, 1);
	 break;
      case 'S':		/* Total Rating (stats) */
	 b = itof22(b, bombingRating(j)+planetRating(j)+offenseRating(j));
	 break;
      case 'r':		/* Ratio */
	 b = itof22(b, Losses(j) ? ((double)Kills(j)/(double)Losses(j)):
				(double) Kills(j));
	 break;
      case 'd':		/* Damage Inflicted (DI) */
	 b = itof42(b, (bombingRating(j) + planetRating(j)+offenseRating(j))
			* (j->p_stats.st_tticks / 36000.0));
	 break;
      case ' ':		/* White Space */
	 /* *b++ = ' ';  -- we get space by default on each */
	 break;
      case 'B':		/* Bombing */
	 b = itof22(b, bombingRating(j));
	 break;
      case 'b':		/* Armies Bombed */
	 b = itoa(b, j->p_stats.st_tarmsbomb + j->p_stats.st_armsbomb,
		     5, 1);
	 break;
      case 'P':		/* Planets */
	 b = itof22(b, planetRating(j));
	 break;
      case 'p':		/* Planets Taken */
	 b = itoa(b, j->p_stats.st_tplanets + j->p_stats.st_planets,
		     5, 1);
	 break;
      case 'M':		/* Display, Host Machine */
	 b = strcpyp_return(b, j->p_monitor, 16);
	 break;
      case 'H':		/* Hours Played */
	 b = itof32(b, Ticks(j) / 36000.0);
	 break;
      case 'k':		/* Max Kills  */
	 b = itof22(b, MaxKills(j));
	 break;

/* doesn't work for starbases. forget stuffing it into maxkills, send out
   new packet dammit! */

      case 'V':		/* Kills Per Hour  */
	 b = itof22(b, (double)Kills(j) / (Ticks(j)/36000.));
	 break;
      case 'v':		/* Deaths Per Hour  */
	 b = itof22(b, (double)Losses(j) / (Ticks(j)/36000.));
	 break;
      default:
	 break;
      }
      ptr++;
   }

   W_WriteText(playerw, 0, pos, playerColor(j), buf, b - buf, shipFont(j));
}

/*
 * ===========================================================================
 * */

/*
 *  DBP:  If teamOrder is set, order the players team according to:
 *        teamOrder == 1:   First
 *        teamOrder >= 2:   Last
 */

void
Sorted_playerlist3()
{
   register                     h, i;
   register int                 p = -1;
   register struct player       *j;
   static int Pos[MAXPLAYER];
   static int Order[NUMTEAM+1];   /* plus IND */
   static int firstCall=1;
   int myteam=0;

   if(firstCall) {

     /* Init team Order */
     Order[0] = 0;
     for(i=1; i < NUMTEAM+1; i++)
       Order[i] = 1 << (i-1);

     if(~(me->p_team) & ALLTEAM) {
       /* Set the team order after we have a team selected */
       firstCall = 0;
       
       /* Init Order */
       if(me->p_team == 0)
	 myteam = 0;

       for(i=1; i < NUMTEAM+1; i++) {
	 if(me->p_team == Order[i])
	   myteam = i;
       }

       
       /* Make our team either first or last in the order */
       /* Leave IND as very first though */

       if(teamOrder == 1) {                  /* First */
	 for(i=myteam; i != 1; i--)
	   Order[i] = Order[i-1];
	 Order[i] = me->p_team;
       }
       else {                                /* Last */
	 for(i=myteam; i < NUMTEAM; i++)
	   Order[i] = Order[i+1];
	 Order[i] = me->p_team;
       }
     }
   }
   else {  
     /* Check to see if we changed teams */
     if((teamOrder == 1 && me->p_team != Order[1]) ||
	(teamOrder >= 2 && me->p_team != Order[NUMTEAM])) {
       
       /* restore Order to defaults */
       Order[0] = 0;
       if(me->p_team == 0)
	 myteam = 0;

       for(i=1; i < NUMTEAM+1; i++) {
	 Order[i] = 1 << (i-1);
	 if(me->p_team == Order[i])
	   myteam = i;
       }
       
       if(teamOrder == 1) {                /* First */
	 for(i=myteam; i != 1; i--)
	   Order[i] = Order[i-1];
	 Order[i] = me->p_team;
       }
       else {                              /* Last */
	 for(i=myteam; i < NUMTEAM; i++)  
	   Order[i] = Order[i+1];
	 Order[i] = me->p_team;
       }
     }
   }
   


   for(h= 0; h < NUMTEAM+1; h++){
      for(i=0,j=players; i< MAXPLAYER; i++,j++){
         if(!j->p_status) continue;

         if(j->p_team != Order[h])
            continue;
         
         p++;
         if(!updatePlayer[i] && Pos[p] == i)
            continue;
         
         Pos[p] = i;
         updatePlayer[i] = 0;

         if (!plshowstatus && j->p_status != PALIVE)
            W_ClearArea(playerw, 0, p+2, header_len, 1);
         else
            pline(j, p+2);
      }
   }
   for(p++; p< MAXPLAYER; p++){
      if(Pos[p] != -1){
         Pos[p] = -1;
         W_ClearArea(playerw, 0, p+2, header_len, 1);
      }
   }
}


/*
 * ===========================================================================
 * */

void
Sorted_playerlist2()
{
   register                     h, i, x;
   register int                 p = -1;
   register struct player       *j;
   static int Pos[MAXPLAYER + 1];
   int torder[NUMTEAM + 1];
   
   torder[0] = NOBODY;
   torder[NUMTEAM] = me->p_team;
   for (i = 1, h = 0; i <= NUMTEAM; i++)
       if (me->p_team == (1 << (i - 1)))
	   h = 1;
       else
    	   torder[i - h] = 1 << (i - 1);
   
   for(h= 0; h <= NUMTEAM; h++){
      for(i=0,j=players; i< (sortPlayersObs ? OBSSTART : MAXPLAYER); i++,j++){
	  
         if(!j->p_status) continue;

         if (j->p_team != torder[h])
            continue;
         
         p++;
         if(!updatePlayer[i] && Pos[p] == i)
            continue;
         
         Pos[p] = i;
         updatePlayer[i] = 0;

         if (!plshowstatus && j->p_status != PALIVE)
            W_ClearArea(playerw, 0, p+2, header_len, 1);
         else
            pline(j, p+2);
      }
   }
   
    for (i = 0, j = players; i < (sortPlayersObs ? OBSSTART : MAXPLAYER); i++, j++)
    {
    	if (j->p_status == PFREE)
    	    continue;
	
	if (j->p_team != 15)
	    continue;
	
         p++;
         if(!updatePlayer[i] && Pos[p] == i)
            continue;
         
         Pos[p] = i;
         updatePlayer[i] = 0;

         if (!plshowstatus && j->p_status != PALIVE)
            W_ClearArea(playerw, 0, p+2, header_len, 1);
         else
            pline(j, p+2);
   }

   if (sortPlayersObs)
   {
      W_ClearArea(playerw, 0, ++p + 2, header_len, 1);
      Pos[p] = MAXPLAYER + 1;
	 for(i=OBSSTART,j=&players[OBSSTART]; i< MAXPLAYER; i++,j++){
            if(!j->p_status) continue;

            p++;
            if(!updatePlayer[i] && Pos[p] == i)
               continue;

            Pos[p] = i;
            updatePlayer[i] = 0;

            if (!plshowstatus && j->p_status != PALIVE)
               W_ClearArea(playerw, 0, p+2, header_len, 1);
            else
               pline(j, p+2);
	 }
   }

   for(p++; p<= MAXPLAYER; p++){
      if(Pos[p] != -1){
         Pos[p] = -1;
         W_ClearArea(playerw, 0, p+2, header_len, 1);
      }
   }
}


/*
 * ===========================================================================
 * */

void
playerlist2()
{
   register int    i;
   register struct player *j;

   if (!W_IsMapped(playerw))
      return;

   if (sortPlayers) {
     if(!teamOrder)                 /* DBP */
       Sorted_playerlist2();
     else Sorted_playerlist3();     /* DBP */
     return;
   }

   for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++) {
      if(updatePlayer[i])
	 playerlist3(i);
   }
}

void
playerlist3(i)
   
   register int            i;
{
   register struct player *j;

   updatePlayer[i] = 0;
   j = &players[i];

   if (j->p_status == PFREE || (!plshowstatus && j->p_status != PALIVE)) {
      W_ClearArea(playerw, 0, i + 2, header_len, 1);
   }
   else
      pline(j, i+2);
}

