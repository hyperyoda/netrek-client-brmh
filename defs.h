#ifndef _defs_h_
#define _defs_h_
/*
 * defs.h
 * 
 * $Log: defs.h,v $
 * Revision 1.3  2000/03/04 00:52:02  ahn
 * * Upped MAXPLAYER to 36.
 * * Cosmetic changes to mkdefault.c to compile under SGI CC.
 *
 * - Dave Ahn
 *
 * Revision 1.2  2000/02/17 05:48:05  ahn
 * BRMH 2.3 from David Pinkney <dpinkney@cs.uml.edu>
 *
 * Revision 1.6  1993/10/05  16:40:38  hadley
 * checkin
 *
 * Revision 1.6  1993/10/05  16:38:08  hadley
 * checkin
 *
 */
#include "copyright.h"

#define SIGNAL(x,y)	signal(x,y)
#define PAUSE(x)	pause()

#if defined (Solaris) || defined(SVR4) || defined(SYSV)
#define MCOPY(b1,b2,l)  memcpy(b2,b1,l)
#define MZERO(b1,l)     memset(b1,0,l)
#define INDEX(s,c)      strchr(s,c)
#define RINDEX(s,c)     strrchr(s,c)
#else
#define MCOPY(b1,b2,l)  bcopy(b1,b2,l)
#define MZERO(b1,l)     bzero(b1,l)
#define INDEX(s,c)      index(s,c)
#define RINDEX(s,c)     rindex(s,c)
#endif


#ifdef NBT
#define MAX_MACRO	500
#endif

#define CLIENT_TICKS(v)	((updates_per_second * v)/10)


#ifndef MAXPLAYER
#define MAXPLAYER 36
#define OBSSTART 16
#endif
#define TESTERS 4		/* Priveledged slots for robots and game
				 * 'testers' */
#define MAXPLANETS 40
#define MAXTORP 8
#define MAXPLASMA 1
#define PKEY 128
#define WINSIDE 500		/* Size of strategic and tactical windows */
#define BORDER 4		/* border width for option windows */
#define PSEUDOSIZE 16
#define NUMRANKS 9

/* These are configuration definitions */

#define GWIDTH 100000		/* galaxy is 100000 spaces on a side */
#define WARP1 20		/* warp one will move 20 spaces per update */
#define SCALE 40		/* Window will be one pixel for 20 spaces */
#define EXPDIST 350		/* At this range a torp will explode */

#ifdef GALAXY
#define DETDIST 1600		/* At this range a player can detonate a torp */
#else
#define DETDIST 3000		/* At this range a player can detonate a torp */
#endif				/* GALAXY */

#define PHASEDIST 6000		/* At this range a player can do damage with
				 * phasers */
#define ENTORBDIST 900		/* At this range a player can orbit a planet */
#define ORBDIST 800		/* A player will orbit at this radius */
#define ORBSPEED 2		/* This is the fastest a person can go into
				 * orbit */
#define PFIREDIST 1500		/* At this range a planet will shoot at a
				 * player */
#define UPDATE 100000		/* Update time is 100000 micro-seconds */

/*
 * 6 minutes is maximum for autoquit -- anything more causes problems in the
 * server.  (?)
 */
#define AUTOQUIT 5*60		/* auto logout in 5 minutes */

#define VACANT -1		/* indicates vacant port on a starbase */
#define DOCKDIST 600
#define DOCKSPEED 2		/* If base is moving, there will be some
				 * finesse involved to dock */
#define NUMPORTS 4
#define SBFUELMIN 10000		/* If starbase's fuel is less than this, it
				 * will not refuel docked vessels */
#define TRACTDIST   6000	/* maximum effective tractor beam range */
#define TRACTEHEAT  5		/* ammount tractor beams heat engines */
#define TRACTCOST   20		/* fuel cost of activated tractor beam */


#ifdef RSA
/* string must begin with characters "RSA v" */
#define RSA_VERSION "RSA v2.0 CLIENT (standard 2)"
#endif

#define KEY_SIZE 32
#define RESERVED_SIZE 16
#define MSG_LEN 80
#define NAME_LEN 16
#define KEYMAP_LEN 96

/* These are memory sections */
#define PLAYER 1
#define MAXMESSAGE 50
#define MAXREVIEWMESSAGE 20

#define rosette(x)   ((((x) + 8) / 16) & 15)
/* #define rosette(x)   ((((x) + 256/VIEWS/2) / (256/VIEWS) + VIEWS) % VIEWS) */
/* (((x + 8) / 16 + 16)  %  16)  */

/* These are the teams */
/*
 * Note that I used bit types for these mostly for messages and war status.
 * This was probably a mistake.  It meant that Ed had to add the 'remap' area
 * to map these (which are used throughout the code as the proper team
 * variable) into a nice four team deep array for his color stuff.  Oh well.
 */
#define NOBODY 0x0
#define IND 0x0			/* independent team */
#define FED 0x1
#define ROM 0x2
#define KLI 0x4
#define ORI 0x8
#define ALLTEAM (FED|ROM|KLI|ORI)
#define MAXTEAM (ORI)
#define NUMTEAM 4


/*
 * * These are random configuration variables
 */
#define VICTORY 3		/* Number of systems needed to conquer the
				 * galaxy */
#define WARNTIME 30		/* Number of updates to have a warning on the
				 * screen */
#define MESSTIME 30		/* Number of updates to have a message on the
				 * screen */

#define TARG_PLAYER	0x1	/* Flags for gettarget */
#define TARG_PLANET	0x2
#define TARG_CLOAK	0x4	/* Include cloaked ships in search */
#define TARG_SELF	0x8
#define TARG_ENEMY      0x10
#define TARG_FRIEND     0x20

#ifndef DEFAULT_SERVER
#define DEFAULT_SERVER	"wormhole.ecst.csuchico.edu"
#endif

#define DEFAULT_PORT	2592

#define DEFAULT_UPDATES_PER_SECOND	5

/* Other stuff that Ed added */

#define ABS(a)			/* abs(a) */ (((a) < 0) ? -(a) : (a))
#undef MAX
#define MAX(a,b)		((a) > (b) ? (a) : (b))

#define myPlasmaTorp(t)		(me->p_no == (t)->pt_owner)
#define myTorp(t)		(me->p_no == (t)->t_owner)
#define friendlyPlasmaTorp(t)	((!(me->p_team & (t)->pt_war)) || (myPlasmaTorp(t)))
#define friendlyTorp(t)		((!(me->p_team & (t)->t_war)) || (myTorp(t)))
#define myPhaser(p)		(&phasers[me->p_no] == (p))
#define friendlyPhaser(p)	(me->p_team == players[(p) - phasers].p_team)
#define myPlayer(p)		(me == (p))
#define myPlanet(p)		(me->p_team == (p)->pl_owner)
#define friendlyPlayer(p)	((!(me->p_team & \
				    ((p)->p_swar | (p)->p_hostile))) && \
				    (!((p)->p_team & \
				    (me->p_swar | me->p_hostile))))
#define isAlive(p)		((p)->p_status == PALIVE)
#define friendlyPlanet(p)	((p)->pl_info & me->p_team && \
			     !((p)->pl_owner & (me->p_swar | me->p_hostile)))

#define isLockPlanet(p)		((me->p_flags & PFPLLOCK) && (me->p_planet == p->pl_no))
#define isLockPlayer(p)		((me->p_flags & PFPLOCK) && (me->p_playerl == p->p_no))

#define torpColor(t)		\
	(myTorp(t) ? myColor : shipCol[remap[players[(t)->t_owner].p_team]])
#define plasmatorpColor(t)		\
	(myPlasmaTorp(t) ? myColor : shipCol[remap[players[(t)->pt_owner].p_team]])
#define phaserColor(p)		\
	(myPhaser(p) ? myColor : shipCol[remap[players[(p) - phasers].p_team]])
/*
 * Cloaking phase (and not the cloaking flag) is the factor in determining
 * the color of the ship.  Color 0 is white (same as 'myColor' used to be).
 */
#define playerColor(p)		\
	(myPlayer(p) ? myColor : shipCol[remap[(p)->p_team]])
#define planetColor(p)		\
	(((p)->pl_info & me->p_team) ? shipCol[remap[(p)->pl_owner]] : unColor)

#ifdef EXTRAFONTS
#define planetFont(p)		\
	(myPlanet(p) ? W_MyPlanetFont : friendlyPlanet(p) ? W_FriendlyPlanetFont \
	    : W_EnemyPlanetFont)
#else
#define planetFont(p)		\
	(myPlanet(p) ? W_BoldFont : friendlyPlanet(p) ? W_UnderlineFont \
	    : W_RegularFont)
#endif

#define shipFont(p)		\
	(myPlayer(p) ? W_BoldFont : friendlyPlayer(p) ? W_UnderlineFont \
	    : W_RegularFont)
#define bombingRating(p)	\
	((p)->p_stats.st_tticks ? \
	((float) (p)->p_stats.st_tarmsbomb * status->timeprod / \
	 ((float) (p)->p_stats.st_tticks * status->armsbomb)) : 0.0)
#define planetRating(p)		\
	((p)->p_stats.st_tticks ? \
	((float) (p)->p_stats.st_tplanets * status->timeprod / \
	 ((float) (p)->p_stats.st_tticks * status->planets)) : 0.0)
#define offenseRating(p)	\
	((p)->p_stats.st_tticks ? \
	((float) (p)->p_stats.st_tkills * status->timeprod / \
	 ((float) (p)->p_stats.st_tticks * status->kills)) : 0.0)
#define defenseRating(p)	\
	((float) (p)->p_stats.st_tticks * status->losses / \
	 ((p)->p_stats.st_tlosses!=0 ? \
	  ((float) (p)->p_stats.st_tlosses * status->timeprod) : \
	  (status->timeprod)))

#ifndef FALSE
typedef enum {
   FALSE = 0, TRUE
}               boolean;
#endif

#ifndef ROTATERACE
#define sendTorpReq(dir) sendShortPacket(CP_TORP, dir)
#define sendPhaserReq(dir) sendShortPacket(CP_PHASER, dir)
#define sendDirReq(dir) sendShortPacket(CP_DIRECTION, dir)
#define sendPlasmaReq(dir) sendShortPacket(CP_PLASMA, dir)
#else
#define sendTorpReq(dir) sendShortPacket(CP_TORP, RotateDirSend(dir))
#define sendPhaserReq(dir) sendShortPacket(CP_PHASER, RotateDirSend(dir))
#define sendDirReq(dir) sendShortPacket(CP_DIRECTION, RotateDirSend(dir))
#define sendPlasmaReq(dir) sendShortPacket(CP_PLASMA, RotateDirSend(dir))
#endif				/* ROTATERACE */

#define sendSpeedReq(speed) sendShortPacket(CP_SPEED, speed)
#define sendShieldReq(state) sendShortPacket(CP_SHIELD, state)
#define sendOrbitReq(state) sendShortPacket(CP_ORBIT, state)
#define sendRepairReq(state) sendShortPacket(CP_REPAIR, state)
#define sendBeamReq(state) sendShortPacket(CP_BEAM, state)
#define sendCopilotReq(state) sendShortPacket(CP_COPILOT, state)
#define sendDetonateReq() sendShortPacket(CP_DET_TORPS, 0)
#define sendCloakReq(state) sendShortPacket(CP_CLOAK, state)
#define sendBombReq(state) sendShortPacket(CP_BOMB, state)
#define sendPractrReq() sendShortPacket(CP_PRACTR, 0)
#define sendWarReq(mask) sendShortPacket(CP_WAR, mask)
#define sendRefitReq(ship) sendShortPacket(CP_REFIT, ship)
#define sendPlaylockReq(pnum) sendShortPacket(CP_PLAYLOCK, pnum)
#define sendPlanlockReq(pnum) sendShortPacket(CP_PLANLOCK, pnum)
#define sendCoupReq() sendShortPacket(CP_COUP, 0)
#define sendQuitReq() sendShortPacket(CP_QUIT, 0)
#define sendByeReq() sendShortPacket(CP_BYE, 0)
#define sendDockingReq(state) sendShortPacket(CP_DOCKPERM, state)
#define sendResetStatsReq(verify) sendShortPacket(CP_RESETSTATS, verify)
#ifdef SCAN
#define sendScanReq(who) sendShortPacket(CP_SCAN, who)	/* ATM */
#endif				/* ATM */


/*
 * This macro allows us to time things based upon # frames / sec.
 */
#define ticks(x) ((x)*200000/timerDelay)

char           *getdefault();

/*
 * UDP control stuff
 */
#ifdef GATEWAY
#define UDP_NUMOPTS    11
#define UDP_GW         UDP_NUMOPTS-1
#else
#define UDP_NUMOPTS    10
#endif
#define UDP_CURRENT     0
#define UDP_STATUS      1
#define UDP_DROPPED     2
#define UDP_SEQUENCE    3
#define UDP_SEND	4
#define UDP_RECV	5
#define UDP_DEBUG       6
#define UDP_FORCE_RESET	7
#define UDP_UPDATE_ALL	8
#define UDP_DONE        9
#define COMM_TCP        0
#define COMM_UDP        1
#define COMM_VERIFY     2
#define COMM_UPDATE	3
#define COMM_MODE	4
#define SWITCH_TCP_OK   0
#define SWITCH_UDP_OK   1
#define SWITCH_DENIED   2
#define SWITCH_VERIFY   3
#define CONNMODE_PORT   0
#define CONNMODE_PACKET 1
#define STAT_CONNECTED  0
#define STAT_SWITCH_UDP 1
#define STAT_SWITCH_TCP 2
#define STAT_VERIFY_UDP 3
#define MODE_TCP        0
#define MODE_SIMPLE     1
#define MODE_FAT	2
#define MODE_DOUBLE     3

#define UDP_RECENT_INTR 300
#define UDP_UPDATE_WAIT	5

/* client version of UDPDIAG */
#define UDPDIAG(x)      { if (udpDebug) { printf("UDP: "); printf x; }}
#define V_UDPDIAG(x)    UDPDIAG(x)

#ifdef NETSTAT
#define		NETSTAT_NUMFIELDS	7

/* field names */
#define		NETSTAT_SWITCH		0
#define		NETSTAT_RESET		1
#define		NETSTAT_TOTAL		2
#define		NETSTAT_LOCAL		3
#define		NETSTAT_FAILURES	4
#define		NETSTAT_NFTHRESH	5
#define		NETSTAT_DONE		6

/* misc */

#define		NETSTAT_DF_NFT		2000
#define		NETSTAT_DF_NFT_S	"2000"

#endif

#ifdef SHORT_PACKETS
#define         SPK_VOFF        0	/* variable packets off */
#define         SPK_VON         1	/* variable packets on */
#define         SPK_MOFF        2	/* message packets off */
#define         SPK_MON         3	/* message packets on */
#define         SPK_M_KILLS     4	/* send kill mesgs */
#define         SPK_M_NOKILLS   5	/* don't send kill mesgs */
#define         SPK_THRESHOLD   6	/* threshold */
#define         SPK_M_WARN      7	/* warnings */
#define         SPK_M_NOWARN    8	/* no warnings */
#define SPK_SALL 9		/* only planets,kills and weapons */
#define         SPK_ALL 10	/* Full Update - SP_STATS */
#define         SPK_NUMFIELDS   6

#define         SPK_VFIELD      0
#define         SPK_MFIELD      1
#define         SPK_KFIELD      2
#define         SPK_WFIELD      3
#define         SPK_TFIELD      4
#define         SPK_DONE        5
#endif

#ifdef ROTATERACE
#define	RotateDirSend(d)	(rotate?d-rotate_deg:d)
#endif

#ifdef DEBUG
#define IFDEBUG(foo)  	foo
#else
#define IFDEBUG(foo)
#endif

#define NUM_PLANET_BITMAPS 7
#define NUM_PLANET_BITMAPS2 8

#ifdef LOGMESG
#define DEFAULT_RECORDFILE  "/tmp/Netrek.Messages.Rec"
#endif

/*
 * By default, fd_set is an array of integers large enough to allow
 * selection on the maximum number of open files.  This number is usually
 * something like 256 (although I've seen > 2000 on HPs).  Thus, fd_set
 * is usually 32 bytes long (or 256 bytes on HP), or 28 bytes longer then
 * we need since we'll only be using at most 5 live descriptors at a time
 * (stdin, stdout, stderr, sock, udpSock).  By reducing this to int, we 
 * save on stack time/space and in FD_ZERO since it no longer has to bzero()
 * 32-256 bytes of storage.
 */

/* XXX */
#ifndef __sys_types_h_
#include <sys/types.h>
#endif

#define fd_set			int

#undef FD_ZERO
#define FD_ZERO(fds)		(*fds)=0
#undef FD_SET
#define FD_SET(fd, fds)		(*fds) |= (1 << fd)
#undef FD_ISSET
#define FD_ISSET(fd, fds)	((*fds) & (1<<fd))
#undef FD_WIDTH
#define FD_WIDTH		max_fd

#ifdef FEATURE

#define MAXMACLEN       85
#define NBTM            0
#define NEWM            1
#define NEWMSPEC        2
#define NEWMMOUSE       3
#define NEWMULTIM	4

#define MACRO_ME        0
#define MACRO_PLAYER    1
#define MACRO_TEAM      2
#define MACRO_FRIEND	3
#define MACRO_ENEMY	4

#define NUM_DIST 27

#endif
#endif

#define sendServerPacket(p)     _sendServerPacket((struct player_spacket *)p)

#ifdef FONT_BITMAPS

#define F_NUMFONTS      7

#endif

#define LINESPERPAGE	31
#define LINESSTART	7

#define BITMAP_MOTD	1

#ifdef CONTROL_KEY
#   define MAXKEY 224
#else
#   define MAXKEY 96
#endif

#define KEYMAP_DEFAULT			(NUM_TYPES)

#define MAXASCII 128

#define PUCK_NAME			"Puck"
#define PUCK_HOST			"Nowhere"

#define IsStarbase(j)			(j->p_ship.s_type == STARBASE)
