/*
 * data.c
 * 
 * $Log: data.c,v $
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

#include <stdio.h>
#include <sys/types.h>
#include "netrek.h"

#ifdef DYNAMIC_BITMAPS
#include "bitmapstuff.h"
#endif

struct player  *players;
struct player  *me = NULL;
struct torp    *torps;
struct plasmatorp *plasmatorps;
struct status  *status;
struct ship    *myship;
struct stats   *mystats;
struct planet  *planets;
struct phaser  *phasers;
struct message *messages;
struct mctl    *mctl;

struct memory   universe;

unsigned char	*mykeymap = NULL;
unsigned char	*keymaps[NUM_TYPES+1] = 
   {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};	/* xxx */

char		*display = NULL;
int		fastQuit=0;
int		fastQuitOk=0;
int             oldalert = 0;
int             remap[16] =
{0, 1, 2, 0, 3, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0};
int             messpend;
int             lastcount;
int             mdisplayed;
int             redrawall;
int             udcounter;
int             showTractorPressor = 1;
int             showLock = 3;
int             extraBorder = 1;
int             fillTriangle = 0;
/* udp options */
int             tryUdp = 1;
struct plupdate pl_update[MAXPLANETS];
char		buttonmap[13];
int		extended_mouse = 0;
int             lastm;
int             mapmode = 2;
int             namemode = 2;
int             showStats = 1;
int             showShields = 1;
int             warncount = 0;
int             warntimer = -1;
int             infomapped = 0;
int             keeppeace = 1;
#ifdef GATEWAY
unsigned long   netaddr = 0;	/* for blessing */
char		*serverNameRemote;
#endif

int             msgBeep = 1;	/* ATM - msg beep */
#ifdef SCAN
int             scanplayer;	/* who to scan */
int             scanmapped = 0;	/* ATM - scanners */
#endif

int             showlocal = 1;
int             showgalactic = 1;
char           *title = NULL;
char           *shipnos = "0123456789abcdefghijklmnopqrstuvwxyz";
int             sock = -1;
int             xtrekPort = -1;
int             queuePos = -1;
int             pickOk = -1;
int             lastRank = -1;
int             promoted = 0;
int             loginAccept = -1;
unsigned        localflags = 0;
int             tournMask = 15;
int             nextSocket= -1;	/* socket to use when we get ghostbusted... */
int             updatePlayer[MAXPLAYER];	/* Needs updating on player
						 * list */
char           *serverName = NULL;
char           *serverAlias = NULL;
int             loggedIn = 0;
int             reinitPlanets = 0;
int             redrawPlayer[MAXPLAYER];	/* Needs redrawing on
						 * galactic map */
int             lastUpdate[MAXPLAYER] =
{0};				/* Last update of this player */
int             timerDelay = 1000000/DEFAULT_UPDATES_PER_SECOND;
int		updates_per_second = DEFAULT_UPDATES_PER_SECOND;
int             reportKills = 1;/* report kill messages? */

int             reportKillsInReview = 1; /* report kills in review win */
int             reportAllInReview = 1; /* report all-board in review win */
int             reportTeamInReview = 1; /* report team-board in review win */
int             reportIndInReview = 1; /* report individual in review win */
int		reportPhaserInReview = 0;	/* report phaser in review */

int		phaserWindow = 0;

#ifdef LOGMESG
FILE           *logFile = NULL;	/* log message to this file */
char           *logFileName = DEFAULT_RECORDFILE;     /* Log message <isae> */
int             logMess = 0;	/* We don't log message by default */
#endif
#ifdef MOOBITMAPS
int             myPlanetBitmap = 0;	/* Use the new planet bitmaps by
					 * default --- NOT! */
#endif
int             continueTractor = 1;	/* Continous vis T/P beams by default */

int             showTractor = 1;/* show visible tractor beams */
int             commMode = 0;	/* UDP: 0=TCP only, 1=UDP updates */
int             commModeReq = 0;/* UDP: req for comm protocol change */
int             commStatus = 0;	/* UDP: used when switching protocols */
int             commSwitchTimeout = 0;	/* UDP: don't wait forever */
int             udpTotal = 1;	/* UDP: total #of packets received */
int             udpDropped = 0;	/* UDP: count of packets dropped */
int             udpRecentDropped = 0;	/* UDP: #of packets dropped recently */
int             udpSock = -1;	/* UDP: the socket */
int             udpDebug = 0;	/* UDP: debugging info on/off */
int             udpClientSend = 1;	/* UDP: send our packets using UDP? */
int             udpClientRecv = 1;	/* UDP: receive with simple UDP */
int             udpSequenceChk = 1;	/* UDP: check sequence numbers */
#ifdef GATEWAY
int             gw_serv_port, gw_port, gw_local_port;	/* UDP */
char           *gw_mach = NULL;	/* UDP */
#endif

int             debug = 0;

int             messageon = 0;
int             warp = 0;

#ifdef NBT
struct macro_list macro[MAX_MACRO];	/* NBT 2/26/93 */
int             MacroMode = 0;
int             macrocnt = 0;
#endif

#ifdef NBTDIST
/* defaults should say quasi intelligent things */
/*
 * note that all are enabled by default but some say nothing- this is to
 * allow the WTEMPED or ETEMPED message to appear if necessary
 */
struct distress_list distress[2] =
{
   {0, 101, 0, 101, 0, 101, 0, 101, -1, 40, 0, 101,
    "SHLD OUT", "%d%%shld", "Shld ok",
    "Dam. ok", "%d%%dam", "DAMAGED",
    "_", "_", "_",
    "_", "_", "_",
    "_", "%d Armies", "%d ARMS!",
    "NO GAS", "%d%%fuel", "Fuel ok",
    1, 1, 1, 1, 1, 1},
   {0, 101, 0, 66, 0, 85, 0, 101, -1, 40, 15, 101,
    "SHLD OUT", "%d%%shld", "Shld ok",
    "_", "%d%%dam", "WRP 1! %d%% dam",
    "_", "%d%%wtmp", "%d%% WTMP HIGH",
    "_", "_", "_",
    "_", "%d armies", "%d ARMS!",
    "GAS LOW %d%%", "_", "_",
    1, 1, 1, 1, 1, 1}};
#endif

#ifdef GALAXY
extern double   Sin[], Cos[];
#endif				/* GALAXY */

W_Icon          stipple=NULL, clockpic=NULL, icon=NULL;

W_Color         borderColor, backColor, textColor, myColor, warningColor,
                shipCol[5], rColor, yColor, gColor, unColor, foreColor;

int             VShieldBitmaps = 0;

#ifndef FONT_BITMAPS
W_Icon          expview[EX_FRAMES];
W_Icon          sbexpview[SBEXPVIEWS];
W_Icon          cloud[NUMDETFRAMES];
W_Icon          plasmacloud[NUMDETFRAMES];
W_Icon          etorp, mtorp;
W_Icon          eplasmatorp, mplasmatorp;
W_Icon          shield[SHIELD_FRAMES];
W_Icon		cloakicon;

#ifndef DYNAMIC_BITMAPS
W_Icon          fed_bitmaps[NUM_TYPES][VIEWS], kli_bitmaps[NUM_TYPES][VIEWS], rom_bitmaps[NUM_TYPES][VIEWS], ori_bitmaps[NUM_TYPES][VIEWS], ind_bitmaps[NUM_TYPES][VIEWS];
#else
W_Icon          ship_bitmaps[NUM_BITMAP_TYPES][VIEWS];
#endif

#ifdef HOCKEY
W_Icon		puck_bitmaps[VIEWS] = {NULL};
#endif

/* ISAE: use #define to specify the size */
W_Icon          bplanets[NUM_PLANET_BITMAPS];
W_Icon          mbplanets[NUM_PLANET_BITMAPS];
W_Icon          bplanets2[NUM_PLANET_BITMAPS2];
W_Icon          mbplanets2[NUM_PLANET_BITMAPS2];
#ifdef MOOBITMAPS
W_Icon          bplanets3[NUM_PLANET_BITMAPS2];	/* isae:  Added this */
W_Icon          mbplanets3[NUM_PLANET_BITMAPS2];	/* isae: Added this */
#endif				/* MOOBITMAPS */

#endif	/* FONT_BITMAPS */

char lastMessage[100];

char *classes[] = {"SC", "DD", "CA", "BB", "AS", "SB", "GA", "AT"};
char            teamlet[] =
{'I', 'F', 'R', 'X', 'K', 'X', 'X', 'X', 'O', 'X', 'X', 'X',
 'X', 'X', 'X', 'A'};
char           *teamshort[16] =
{"IND", "FED", "ROM", "X", "KLI", "X", "X", "X", "ORI",
 "X", "X", "X", "X", "X", "X", "ALL"};
char            pseudo[PSEUDOSIZE];
char            login[PSEUDOSIZE];
#ifdef AUTOLOGIN
char            defpasswd[PSEUDOSIZE];
int		autologin = 0;
#endif

struct ship	shipvals[NUM_TYPES];

struct rank     ranks[NUMRANKS] =
{
   {0.0, 0.0, 0.0, "Ensign"},
   {2.0, 1.0, 0.0, "Lieutenant"},
   {4.0, 2.0, 0.8, "Lt. Cmdr."},
   {8.0, 3.0, 0.8, "Commander"},
   {15.0, 4.0, 0.8, "Captain"},
   {20.0, 5.0, 0.8, "Flt. Capt."},
   {25.0, 6.0, 0.8, "Commodore"},
   {30.0, 7.0, 0.8, "Rear Adm."},
   {40.0, 8.0, 0.8, "Admiral"}};

W_Window        messagew, w, mapw, statwin, baseWin, infow, iconWin, tstatw, war,
                warnw, helpWin, teamWin[4], messwa, messwt, messwi,
                messwk, planetw, playerw, rankw, optionWin = 0, reviewWin,
		waitWin, metaWin, quitwin;
#ifdef XTREKRC_HELP
W_Window	defWin;
#endif
#ifdef SCAN
W_Window	scanw, scanwin;
#endif
W_Window        udpWin;
W_Window	phaserwin;
W_Window	motdWin;

int             sortPlayers = 1;
int             sortPlayersObs = 1;
int             teamOrder = 0;   /* DBP */

#ifdef ROTATERACE
int             rotate = 0;
int             rotate_deg = 0;
#endif

#ifdef NETSTAT
int             netstat = 0;
int             netstatfreq = 5;
W_Window        lMeter;
#endif

#ifdef PING
int		no_ping = 0;
int             ping = 0;	/* to ping or not to ping */
long            packets_sent = 0;	/* # all packets sent to server */
long            packets_received = 0;	/* # all packets received */
W_Window        pStats;
#endif

int             use_msgw = 0;   /* send last message to message window */
int             phas_msgi = 0;  /* send phaser point messages to indiv win */

#ifdef BD
int             bd = 0;
#endif

#ifdef SHORT_PACKETS
int             tryShort = 1;	/* for .xtrekrc option */
int             recv_short = 0;
int             recv_short_opt = 0;

int             recv_threshold = 0;
char            recv_threshold_s[8] =
{'0', '\0'};
int             shortversion = SHORTVERSION;  /* Which version do we use? */
#endif

int		tclock=2;

int		alt_playerlist=0;

int		new_messages = 1;

int		max_fd = 3;

#ifdef FEATURE
/* jn - SMARTMACRO */
int 		MacroNum = 0;
int		F_UseNewMacro=1;
int		F_UseSmartMacro=1;
int		F_server_feature_packets=0;
int		F_why_dead=0;
int		F_cloak_maxwarp = 0;
int		F_self_8flags = 0;
int		F_self_8flags2 = 0;
int		F_motd_bitmaps = 0;
int		F_ship_cap = 0;
W_Window	macroWin = NULL;
int		macrokey = 'X';

int 		F_gen_distress = 0;
char 		*singleMacro = NULL;
int		F_phaser_multi_send = 0;

int     F_fps;
int     F_ups;
int     F_tips = 1;
#endif /* FEATURE */

int		abbr_kmesg=0;

int		maskrecip=0;

int		dashboardStyle=0;

int		plshowstatus=0;

char		cloakChars[3] = "><";
int		showInd = 0;
int		stippleBorder = 0;

int		showPlanetOwner = 0;
int		enemyPhasers = 0;
int		newInfo = 0;

#ifdef TTS
int		tts = 0;
int		tts_len = 0;
int		tts_max_len = 40;
int		tts_width = 0;
int		tts_timer = 0;
int		tts_time = TTS_TIME;
int		tts_loc = WINSIDE/2-16;
char 		lastIn[100];
#endif

#ifdef DROP_FIX
int		drop_fix = 1;		/* TMP */
#endif

#ifdef MULTILINE_MACROS
int		multiline_enabled = 0;
#endif

int		motd_line=0;
int		motdw_line=0;

#ifdef HOCKEY
char            puck_name[PSEUDOSIZE] = PUCK_NAME;
char            puck_host[PSEUDOSIZE] = PUCK_HOST;
#endif

#ifdef PHASER_SHRINK
int		shrink_phasers =0;
int		shrink_phasers_amount = 10;
#endif

char		*defaults_file = NULL;

char    *metaserver = "metaserver.netrek.org";
int      metaport = 3521;

char		*plist = NULL;
int		list_needs_stats = 1;	/* if playerlist needs to be
					   updated for player stats */
char 	*program = NULL;

#ifdef FORKNETREK
int	waitnum = 0;
#endif
int	non_obscure = 0;
int	observ = 0;

int cloak_phases = 17;
