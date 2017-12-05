#ifdef SHORT_PACKETS
#include "copyright2.h"
#include <stdio.h>
#include <sys/types.h>
#include <math.h>
#include <errno.h>
#include "netrek.h"

#include "sp.h"
#include "wtext.h"		/* here are all warnings */

/* SP_S_WARNING vari texte */
static char           *s_texte[256];   /* Better with a malloc scheme */
static char            no_memory[] = {"Not enough memory for warning string!"};

static
char *whydeadmess[] = { "", "", "[photon]", "[phaser]", "", "[explos]",
      "", "", "", "", "", "[plasma]", "[detted]", "[chain]",
      "[TEAM]", "", "[detted]", "[chain]", "[?]"};
#define MAX_WHYDEAD 	(sizeof(whydeadmess)/sizeof(char *))
/* For INL Server */
static
char           *shiptype[NUM_TYPES] =
{"SC", "DD", "CA", "BB", "AS", "SB", "GA", "AT"};

#if __STDC__ || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

static void killmesg P_((struct warning_s_spacket *packet, char *buf, int shipt, int karg3, int karg4, int karg5));

#undef P_

void
handleSWarning(packet)
    struct warning_s_spacket *packet;
{
   char            buf[256];
   register char   *bp = buf;
   register struct player *target;
   register int    damage;
   register unsigned char	w, ar1, ar2;

   static int      arg3, arg4;	/* Here are the arguments for warnings with
				 * more than 2 arguments */
   static int      karg3, karg4, karg5;

   /* with a little work we can eliminate identical warning message floods 
      for phaser-recharge, 8-torp-limit, beaming up/down, bombing, etc. */

   static int	   last_udc, lw, lar1,lar2,lar3,lar4;

#define MESG_IDENT_INTERVAL	(w == lw && udcounter - last_udc < 5)

   w = packet->whichmessage;
   ar1 = packet->argument;
   ar2 = packet->argument2;

   switch (w) {
   case TEXTE:			/* damage used as tmp var */
      if(MESG_IDENT_INTERVAL){
	 if(ar1 == lar1 && ar2 == lar2){
	    return;
	 }
      }

      damage = (unsigned char) ar1;
      damage |= (unsigned char) ar2 << 8;

      if (damage >= 0 && damage < NUMWTEXTS)
	 warning(w_texts[damage]);

      lar1 = ar1;
      lar2 = ar2;
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   case PHASER_HIT_TEXT:
      last_udc = udcounter;
      lw = packet->whichmessage;

      target = &players[(unsigned char) ar1 & 0x3f];
      damage = (unsigned char) ar2;
      if ((unsigned char) ar1 & 64)
	 damage |= 256;
      if ((unsigned char) ar1 & 128)
	 damage |= 512;

      if(phaserWindow){
	 if(W_WindowWidth(phaserwin) <= 13){
	    bp = itoa(bp, damage, 3, 0);
	    bp = strcpy_return(bp, " pts on ");
	    bp = strcpyp_return(bp, target->p_mapchars, 2);
	    W_WriteText(phaserwin, 0, 0, textColor, buf, bp-buf, W_MesgFont);
	    W_FlushScrollingWindow(phaserwin);
	    if(reportPhaserInReview){
	       W_WriteText(reviewWin, 0, 0, textColor, buf, bp-buf, W_MesgFont);
	       W_FlushScrollingWindow(reviewWin);
	    }
	    break;
	 }
	 else if(W_WindowWidth(phaserwin) <= 32){
	    bp = itoa(bp, damage, 3, 0);
	    bp = strcpy_return(bp, " pts on ");
	    bp = strcpy_return(bp, target->p_name);
	    *bp ++ = ' ';
	    *bp ++ = '(';
	    bp = strcpyp_return(bp, target->p_mapchars, 2);
	    *bp ++ = ')';
	    W_WriteText(phaserwin, 0, 0, textColor, buf, bp-buf, W_MesgFont);
	    W_FlushScrollingWindow(phaserwin);
	    if(reportPhaserInReview){
	       W_WriteText(reviewWin, 0, 0, textColor, buf, bp-buf, W_MesgFont);
	       W_FlushScrollingWindow(reviewWin);
	    }
	    break;
	 }
      }
      bp = strcpy_return(bp, "Phaser burst hit ");
      bp = strcpy_return(bp, target->p_name);
      *bp ++ = ' ';
      *bp ++ = '(';
      bp = strcpyp_return(bp, target->p_mapchars, 2);
      *bp ++ = ')';
      bp = strcpy_return(bp, " for ");
      bp = itoa(bp, damage, 3, 0);
      bp = strcpy_return(bp, " points.");
      *bp = 0;
      if(phaserWindow){
	 W_WriteText(phaserwin, 0, 0, textColor, buf, bp-buf, W_MesgFont);
	 W_FlushScrollingWindow(phaserwin);
      }
      else{
	 if(phas_msgi)
	    W_WriteText(messwi, 0, 0, textColor, buf, bp-buf, W_MesgFont);
	 else
	    warning(buf);
      }
      if(reportPhaserInReview){
	 W_WriteText(reviewWin, 0, 0, textColor, buf, bp-buf, W_MesgFont);
	 W_FlushScrollingWindow(reviewWin);
      }
      break;
   case BOMB_INEFFECTIVE:
      if(MESG_IDENT_INTERVAL){
	 if(ar1 == lar1){
	    return;
	 }
      }
      bp = strcpy_return(bp, "Weapons Officer: Bombing is ineffective for ");
      bp = itoa(bp, (int)ar1, 3, 0);
      bp = strcpy_return(bp, " armies.");
      *bp = 0;
      warning(buf);
      lar1 = ar1;
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   case BOMB_TEXT:
      if(MESG_IDENT_INTERVAL){
	 if(ar1 == lar1 && ar2 == lar2){
	    return;
	 }
      }
      bp = strcpy_return(bp, "Bombing ");
      bp = strcpy_return(bp, planets[(unsigned char)ar1].pl_name);
      bp = strcpy_return(bp, ".  Sensors read ");
      bp = itoa(bp, (unsigned int)ar2, 3, 0);
      bp = strcpy_return(bp, " armies left.");
      *bp = 0;
      warning(buf);
      lar1 = ar1;
      lar2 = ar2;
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   case BEAMUP_TEXT:
      if(MESG_IDENT_INTERVAL){
	 if(ar1 == lar1){
	    return;
	 }
      }
      sprintf(buf, "%s: Too few armies to beam up",
	      planets[(unsigned char) ar1].pl_name);
      warning(buf);
      lar1 = ar1;
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   case BEAMUP2_TEXT:
      if(MESG_IDENT_INTERVAL){
	 if(ar1 == lar1 && ar2 == lar2){
	    return;
	 }
      }
      sprintf(buf, "Beaming up.  (%d/%d)", (unsigned char) ar1, 
		(unsigned char) ar2);
      warning(buf);
      lar1 = ar1;
      lar2 = ar2;
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   case BEAMUPSTARBASE_TEXT:
      if(MESG_IDENT_INTERVAL){
	 if(ar1 == lar1){
	    return;
	 }
      }
      sprintf(buf, "Starbase %s: Too few armies to beam up",
	      players[ar1].p_name);
      warning(buf);
      lar1 = ar1;
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   case BEAMDOWNSTARBASE_TEXT:
      if(MESG_IDENT_INTERVAL){
	 if(ar1 == lar1){
	    return;
	 }
      }
      sprintf(buf, "No more armies to beam down to Starbase %s.",
	      players[ar1].p_name);
      warning(buf);
      lar1 = ar1;
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   case BEAMDOWNPLANET_TEXT:
      if(MESG_IDENT_INTERVAL){
	 if(ar1 == lar1){
	    return;
	 }
      }
      sprintf(buf, "No more armies to beam down to %s.",
	      planets[(unsigned char) ar1].pl_name);
      warning(buf);
      lar1 = ar1;
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   case SBREPORT:
      if(MESG_IDENT_INTERVAL){
	 if(ar1 == lar1){
	    return;
	 }
      }
      sprintf(buf, "Transporter Room:  Starbase %s reports all troop bunkers are full!",
	      players[ar1].p_name);
      warning(buf);
      lar1 = ar1;
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   case ONEARG_TEXT:
      if(MESG_IDENT_INTERVAL){
	 if(ar1 == lar1){
	    return;
	 }
      }
      if (ar1 < NUMVARITEXTS) {

	 sprintf(buf, vari_texts[ar1], (unsigned char) ar2);
	 warning(buf);
      }
      lar1 = ar1;
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   case BEAM_D_PLANET_TEXT:
      if(MESG_IDENT_INTERVAL){
	 if(ar1 == lar1 && ar2 == lar2 && arg3 == lar3 && arg4 == lar4){
	    return;
	 }
      }
      sprintf(buf, "Beaming down.  (%d/%d) %s has %d armies left",

	      arg3,
	      arg4,
	      planets[(unsigned char) ar1].pl_name, ar2);
      warning(buf);
      lar1 = ar1;
      lar2 = ar2;
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   case ARGUMENTS:
      arg3 = lar3 = (unsigned char) ar1;
      arg4 = lar4 = (unsigned char) ar2;
      break;
   case BEAM_U_TEXT:
      if(MESG_IDENT_INTERVAL){
	 if(ar1 == lar1 && ar2 == lar2 && arg3 == lar3 && arg4 == lar4){
	    return;
	 }
      }
      sprintf(buf, "Transfering ground units.  (%d/%d) Starbase %s has %d armies left",
	      (unsigned char) arg3, (unsigned char) arg4, players[ar1].p_name, (unsigned char) ar2);
      warning(buf);
      lar1 = ar1;
      lar2 = ar2;
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   case LOCKPLANET_TEXT:
      if(MESG_IDENT_INTERVAL){
	 if(ar1 == lar1){
	    return;
	 }
      }
      sprintf(buf, "Locking onto %s", planets[(unsigned char) ar1].pl_name);
      warning(buf);
      lar1 = ar1;
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   case LOCKPLAYER_TEXT:
      if(MESG_IDENT_INTERVAL){
	 if(ar1 == lar1){
	    return;
	 }
      }
      sprintf(buf, "Locking onto %s", players[(unsigned char) ar1].p_name);
      warning(buf);
      lar1 = ar1;
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;

   case SBRANK_TEXT:
      sprintf(buf, "You need a rank of %s or higher to command a starbase!", ranks[ar1].name);
      warning(buf);
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   case SBDOCKREFUSE_TEXT:
      sprintf(buf, "Starbase %s refusing us docking permission captain.",
	      players[ar1].p_name);
      warning(buf);
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   case SBDOCKDENIED_TEXT:
      sprintf(buf, "Starbase %s: Permission to dock denied, all ports currently occupied.", players[ar1].p_name);
      warning(buf);
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   case SBLOCKSTRANGER:
      if(MESG_IDENT_INTERVAL){
	 if(ar1 == lar1){
	    return;
	 }
      }
      sprintf(buf, "Locking onto %s (%c%c)",
	      players[ar1].p_name,
	      teamlet[players[ar1].p_team],
	      shipnos[players[ar1].p_no]);
      warning(buf);
      lar1 = ar1;
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   case SBLOCKMYTEAM:
      if(MESG_IDENT_INTERVAL){
	 if(ar1 == lar1){
	    return;
	 }
      }
      sprintf(buf, "Locking onto %s (%c%c) (docking is %s)",
	      players[ar1].p_name,
	      teamlet[players[ar1].p_team],
	      shipnos[players[ar1].p_no],
	      (players[ar1].p_flags & PFDOCKOK) ? "enabled" : "disabled");
      warning(buf);
      lar1 = ar1;
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   case DMKILL:
      killmesg(packet, buf, 1, karg3, karg4, karg5);
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   case KILLARGS:
      karg3 = (unsigned char) ar1;
      karg4 = (unsigned char) ar2;
      break;
   case KILLARGS2:
      karg5 = (unsigned char) ar1;
      if(karg5 < 0 || karg5 > MAX_WHYDEAD-1) karg5 = MAX_WHYDEAD-1;
      break;
   case DMKILLP:
      {
	 struct mesg_spacket msg;
	 (void) sprintf(msg.mesg, "GOD->ALL %s (%c%c) killed by %s (%c)",
			players[ar1].p_name,
			teamlet[players[ar1].p_team],
			shipnos[ar1],
			planets[(unsigned char) ar2].pl_name,
	      teamlet[planets[(unsigned char) ar2].pl_owner]);
	 msg.type = SP_MESSAGE;
	 msg.mesg[79] = '\0';
	 msg.m_flags = MALL | MVALID | MKILLP;
	 msg.m_recpt = 0;
	 msg.m_from = 255;
	 handleMessage(&msg);
      }
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   case DMBOMB:
      {
	 struct mesg_spacket msg;
	 char            buf1[80];
	 (void) sprintf(buf, "%-3s->%-3s", 
	    planets[(unsigned char) ar2].pl_name, 
	    teamshort[planets[(unsigned char) ar2].pl_owner]);
	 (void) sprintf(buf1, "We are being attacked by %s %c%c who is %d%% damaged.",
			players[ar1].p_name,
			teamlet[players[ar1].p_team],
			shipnos[ar1],
			arg3);
	 (void) sprintf(msg.mesg, "%s %s", buf, buf1);
	 msg.type = SP_MESSAGE;
	 msg.mesg[79] = '\0';
	 msg.m_flags = MTEAM | MVALID | MBOMB;
	 msg.m_recpt = planets[(unsigned char) ar2].pl_owner;
	 msg.m_from = 255;
	 handleMessage(&msg);
      }
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   case DMDEST:
      {
	 struct mesg_spacket msg;
	 char            buf1[80];
	 (void) sprintf(buf, "%s destroyed by %s (%c%c)",
			planets[(unsigned char) ar1].pl_name,
			players[ar2].p_name,
			teamlet[players[ar2].p_team],
			shipnos[(unsigned char) ar2]);
	 (void) sprintf(buf1, "%-3s->%-3s",
			planets[(unsigned char) ar1].pl_name, teamshort[planets[(unsigned char) ar1].pl_owner]);
	 (void) sprintf(msg.mesg, "%s %s", buf1, buf);
	 msg.type = SP_MESSAGE;
	 msg.mesg[79] = '\0';
	 msg.m_flags = MTEAM | MVALID | MDEST;
	 msg.m_recpt = planets[(unsigned char) ar1].pl_owner;
	 msg.m_from = 255;
	 handleMessage(&msg);
      }
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   case DMTAKE:
      {
	 struct mesg_spacket msg;
	 char            buf1[80];
	 (void) sprintf(buf, "%s taken over by %s (%c%c)",
			planets[(unsigned char) ar1].pl_name,
			players[ar2].p_name,
			teamlet[players[ar2].p_team],
			shipnos[ar2]);
	 (void) sprintf(buf1, "%-3s->%-3s",
			planets[(unsigned char) ar1].pl_name, teamshort[players[ar2].p_team]);
	 (void) sprintf(msg.mesg, "%s %s", buf1, buf);
	 msg.type = SP_MESSAGE;
	 msg.mesg[79] = '\0';
	 msg.m_flags = MTEAM | MVALID | MTAKE;
	 msg.m_recpt = players[ar2].p_team;
	 msg.m_from = 255;
	 handleMessage(&msg);
      }
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   case DGHOSTKILL:
      {
	 struct mesg_spacket msg;
	 ushort          damage;
	 damage = (unsigned char) karg3;
	 damage |= (unsigned char) (karg4 & 0xff) << 8;
	 (void) sprintf(msg.mesg, "GOD->ALL %s (%c%c) was kill %0.2f for the GhostBusters",
			players[(unsigned char) ar1].p_name, teamlet[players[(unsigned char) ar1].p_team],
			shipnos[(unsigned char) ar1],
			(float) damage / 100.0);
	 msg.type = SP_MESSAGE;
	 msg.mesg[79] = '\0';
	 msg.m_flags = MALL | MVALID;
	 msg.m_recpt = 0;
	 msg.m_from = 255;
	 handleMessage(&msg);
      }
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
      /* INL Daemon Mesages */
   case INLDMKILLP:
      {
	 struct mesg_spacket msg;
	 buf[0] = 0;
	 if (arg3) {		/* Armies */
	    sprintf(buf, "+%d", arg3);
	 }
	 (void) sprintf(msg.mesg, "GOD->ALL %s(%s) (%c%c%s) killed by %s (%c)",
			players[(unsigned char) ar1].p_name,
	  shiptype[players[(unsigned char) ar1].p_ship.s_type],
		  teamlet[players[(unsigned char) ar1].p_team],
			shipnos[(unsigned char) ar1],
			buf,
			planets[(unsigned char) ar2].pl_name,
	      teamlet[planets[(unsigned char) ar2].pl_owner]);
	 msg.type = SP_MESSAGE;
	 msg.mesg[79] = '\0';
	 msg.m_flags = MALL | MVALID | MKILLP;
	 msg.m_recpt = 0;
	 msg.m_from = 255;
	 handleMessage(&msg);
      }
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   case INLDMKILL:
      killmesg(packet, buf, 1, karg3, karg4, karg5);
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;

   case INLDRESUME:
      {
	 struct mesg_spacket msg;
	 sprintf(msg.mesg, " Game will resume in %d seconds", 
	    (unsigned char)ar1);
	    msg.m_flags=MALL|MVALID;
	    msg.type=SP_MESSAGE;
	    msg.mesg[79]='\0';
	    msg.m_recpt=0;
	    msg.m_from=255;
	    handleMessage(&msg);
      }
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;

   case INLDTEXTE:
      if(MESG_IDENT_INTERVAL){
	 if(ar1 == lar1){
	    return;
	 }
      }
      if (ar1 < NUMDAEMONTEXTS) {
	 struct mesg_spacket msg;
	 strcpy(msg.mesg, daemon_texts[(unsigned char) ar1]);
	 msg.m_flags = MALL | MVALID;
	 msg.type = SP_MESSAGE;
	 msg.mesg[79] = '\0';
	 msg.m_recpt = 0;
	 msg.m_from = 255;
	 handleMessage(&msg);
      }
      lar1 = ar1;
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;

   case STEXTE:
      if(MESG_IDENT_INTERVAL){
	 if(ar1 == lar1){
	    return;
	 }
      }
      warning(s_texte[(unsigned char) ar1]);
      lar1 = ar1;
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;

   case SHORT_WARNING:
      {
	 struct warning_spacket *warn = (struct warning_spacket *) packet;
	 warning(warn->mesg);
      }
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;

   case STEXTE_STRING:
      {
	 struct warning_spacket *warn = (struct warning_spacket *) packet;
	 warning(warn->mesg);
	 s_texte[(unsigned char) warn->pad2] = (char *)malloc(warn->pad3 - 4);
	 if (s_texte[(unsigned char) warn->pad2] == NULL) {
	    s_texte[(unsigned char) warn->pad2] = no_memory;
	    warning("Could not add warning! (No memory!)");
	 } else
	    strcpy(s_texte[(unsigned char) warn->pad2], warn->mesg);
      }
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   default:
      warning("Unknown short message!");
      fprintf(stderr, "Unknown short message: %d\n", packet->whichmessage);
      last_udc = udcounter;
      lw = packet->whichmessage;
      break;
   }
}

static void
killmesg(packet, buf, shipt, karg3, karg4, karg5)

   struct warning_s_spacket 	*packet;
   char				*buf;
   int				shipt;
   int				karg3,karg4,karg5;
{
   register char	*bp = buf;
   struct mesg_spacket 	msg;
   int             	killer, victim, armies, damage;
   float           	kills;

   victim = (unsigned char) packet->argument & 0x3f;
   killer = (unsigned char) packet->argument2 & 0x3f;
   /* that's only a temp */
   damage = (unsigned char) karg3;
   damage |= (karg4 & 127) << 8;
   kills = damage / 100.0;
   armies = (((unsigned char) packet->argument >> 6) | ((unsigned char) packet->argument2 & 192) >> 4);
   if (karg4 & 128)
      armies |= 16;

   bp = strcpy_return(bp, "GOD->ALL ");
   if(abbr_kmesg)
      bp = strcpyp_return(bp, players[victim].p_name, 13);
   else
      bp = strcpy_return(bp, players[victim].p_name);
   if(shipt){
      *bp++ = ' ';
      *bp++ = '[';
      bp = strcpy_return(bp, shiptype[players[victim].p_ship.s_type]);
      *bp++ = ']';
   }

   *bp++ = ' ';
   *bp++ = '(';
   *bp++ = teamlet[players[victim].p_team];
   *bp++ = shipnos[victim];
   if(armies){
      *bp++ = '+';
      if(abbr_kmesg){
	 char	tbuf[10];
	 char	*s = tbuf;
	 s = itoa(s, armies, 2, 0);
	 *s ++ = ')';
	 *s ++ = ' ';
	 bp = strcpyp_return(bp, tbuf, 3);
      }
      else{
	 bp = itoa(bp, armies, 3, 0);
	 bp = strcpy_return(bp, " armies)");
      }
      msg.m_flags = MALL | MVALID | MKILLA;
   }
   else{
      msg.m_flags = MALL | MVALID | MKILL;
      if(abbr_kmesg){
	 bp = strcpy_return(bp, ")   ");
      }
      else
	 *bp++ = ')';
   }

   if(abbr_kmesg){
      *bp ++ = ':';
   }
   else
      bp = strcpy_return(bp, " was kill ");
   bp = itof32(bp, kills);
   bp = strcpy_return(bp, " for ");

   if(abbr_kmesg)
      bp = strcpyp_return(bp, players[killer].p_name, 13);
   else
      bp = strcpy_return(bp, players[killer].p_name);
   if(shipt){
      *bp++ = ' ';
      *bp++ = '[';
      bp = strcpy_return(bp, shiptype[players[killer].p_ship.s_type]);
      *bp++ = ']';
   }
   *bp++ = ' ';
   *bp++ = '(';
   *bp++ = teamlet[players[killer].p_team];
   *bp++ = shipnos[killer];
   *bp++ = ')';
   *bp = 0;
#ifdef FEATURE
   if(F_why_dead){
      *bp ++ = ' ';
      bp = strcpy_return(bp, whydeadmess[karg5]);
      *bp = 0;
      karg5 = 0;
   }
#endif

   msg.type = SP_MESSAGE;
   strncpy(msg.mesg, buf, 79);
   msg.mesg[79] = '\0';
   msg.m_recpt = 0;
   msg.m_from = 255;
   handleMessage(&msg);
}
#endif
