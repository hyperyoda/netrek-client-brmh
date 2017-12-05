#ifndef _data_h_
#define _data_h_
/*
 * data.h
 * 
 * $Log: data.h,v $
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

#define EX_FRAMES 		5
#define SBEXPVIEWS 		7
#define NUMDETFRAMES		5	/* # frames in torp explosion */
#define ex_width        	64
#define ex_height       	64
#define sbexp_width        	80
#define sbexp_height       	80
#define cloud_width 		9
#define cloud_height 		9
#define plasmacloud_width 	13
#define plasmacloud_height 	13
#define etorp_width 		3
#define etorp_height 		3
#define eplasmatorp_width 	7
#define eplasmatorp_height 	7
#define mplasmatorp_width 	5
#define mplasmatorp_height 	5
#define mtorp_width 		3
#define mtorp_height 		3
#define crossmask_width 	16
#define crossmask_height 	16
#define planet_width 		30
#define planet_height 		30
#define mplanet_width 		16
#define mplanet_height 		16
#define shield_width 		20
#define shield_height 		20
#define cloak_width		20
#define cloak_height		20
#define icon_width 		112
#define icon_height 		80

extern struct player *players;
extern struct player *me;
extern struct torp *torps;
extern struct plasmatorp *plasmatorps;
extern struct status *status;
extern struct ship *myship;
extern struct stats *mystats;
extern struct planet *planets;
extern struct phaser *phasers;
extern struct message *messages;
extern struct mctl *mctl;
extern struct team *teams;
extern struct memory universe;
extern unsigned char   *mykeymap;
extern unsigned char   *keymaps[NUM_TYPES+1]; 
extern struct planet pdata[];

extern char	*display;

extern int	fastQuit;
extern int	fastQuitOk;

extern int      oldalert;
extern int      remap[];
extern int      udcounter;
extern char    *title;
extern struct plupdate pl_update[];
extern char     buttonmap[];
extern int	extended_mouse;
extern int      messpend;
extern int      lastcount;
extern int      mdisplayed;
extern int      redrawall;
extern int      nopilot;
extern int      watch;
extern int      selfdest;
extern int      lastm;
extern int      delay;
extern int      rdelay;
extern int      mapmode;
extern int      namemode;
extern int      showShields;
extern int      showStats;
extern int      msgBeep;	/* ATM - msg beep */
extern int      warncount;
extern int      warntimer;
extern int      infomapped;
#ifdef SCAN
extern int      scanmapped;	/* ATM - scanner stuff */
#endif				/* ATM */
extern int      mustexit;
extern int      messtime;
extern int      keeppeace;
#ifdef GATEWAY
extern unsigned long netaddr;	/* for blessing */
extern char	*serverNameRemote;
#endif

extern int      messageon;
extern int      warp;

#ifdef RSA
/* extern char testdata[]; Not needed in RSA 2.0 */
extern int      RSA_Client;
#endif

#ifdef NBT
extern struct macro_list macro[];	/* NBT 2/26/93 */
extern int      MacroMode;
extern int      macrocnt;
#endif

#ifdef NBTDIST
extern struct distress_list distress[];
#endif

#ifdef ROTATERACE
extern int      rotate;
extern int      rotate_deg;
#endif

#ifdef NETSTAT
extern int      netstat;
extern int      netstatfreq;
extern W_Window netstatWin, lMeter;
#endif

extern int      showlocal, showgalactic;
extern char    *shipnos;
extern int      sock;
extern int      xtrekPort;
extern int      queuePos;
extern int      pickOk;
extern int      lastRank;
extern int      promoted;
extern int      loginAccept;
extern unsigned localflags;
extern int      tournMask;
extern int      nextSocket;
extern int      updatePlayer[];
extern char    *serverName;
extern char    *serverAlias;
extern int      loggedIn;
extern int      reinitPlanets;
extern int      redrawPlayer[];
extern int      lastUpdate[];
extern int      timerDelay;
extern int	updates_per_second;
extern int      reportKills;

extern int      reportKillsInReview;
extern int      reportAllInReview;
extern int      reportTeamInReview;
extern int      reportIndInReview;
extern int	reportPhaserInReview;

extern int	phaserWindow;

#ifdef LOGMESG
extern FILE    *logFile;
extern char    *logFileName;
extern int      logMess;
#endif
extern int      sortPlayers;
extern int      sortPlayersObs;

extern int      teamOrder;  /* DBP */

#ifdef MOOBITMAPS
extern int      myPlanetBitmap;
#endif
extern int      continueTractor;

extern int      scanplayer;
extern int      showTractor;
extern int      commMode;	/* UDP */
extern int      commModeReq;	/* UDP */
extern int      commStatus;	/* UDP */
extern int      commSwitchTimeout;	/* UDP */
extern int      udpTotal;	/* UDP */
extern int      udpDropped;	/* UDP */
extern int      udpRecentDropped;	/* UDP */
extern int      udpSock;	/* UDP */
extern int      udpDebug;	/* UDP */
extern int      udpClientSend;	/* UDP */
extern int      udpClientRecv;	/* UDP */
extern int      udpSequenceChk;	/* UDP */
#ifdef GATEWAY
extern int      gw_serv_port, gw_port, gw_local_port;	/* UDP */
extern char    *gw_mach;	/* UDP */
#endif

extern int      showTractorPressor;
extern int      showLock;
extern int      autoKey;
extern int      extraBorder;
extern int      fillTriangle;
/* udp options */
extern int      tryUdp;

extern int      debug;

extern double   Sin[], Cos[];

extern W_Icon   stipple, clockpic, icon;

#define VIEWS 16
#ifdef GALAXY
#define NUM_TYPES 8
#else
#define NUM_TYPES 7
#endif				/* GALAXY */
extern W_Icon   expview[EX_FRAMES];
extern W_Icon   sbexpview[SBEXPVIEWS];
extern W_Icon   cloud[NUMDETFRAMES];
extern W_Icon   plasmacloud[NUMDETFRAMES];
extern W_Icon   etorp, mtorp;
extern W_Icon   eplasmatorp, mplasmatorp;
#define SHIELD_FRAMES 5
extern W_Icon   shield[SHIELD_FRAMES], cloakicon;
extern int      VShieldBitmaps;

#ifndef DYNAMIC_BITMAPS
extern W_Icon   fed_bitmaps[NUM_TYPES][VIEWS], kli_bitmaps[NUM_TYPES][VIEWS], rom_bitmaps[NUM_TYPES][VIEWS], ori_bitmaps[NUM_TYPES][VIEWS], ind_bitmaps[NUM_TYPES][VIEWS];
#else
#include "bitmapstuff.h"
extern W_Icon   ship_bitmaps[NUM_BITMAP_TYPES][VIEWS];
#endif

#ifdef HOCKEY
extern W_Icon	puck_bitmaps[VIEWS];
#endif

/* ISAE: use #define */
extern W_Icon   bplanets[NUM_PLANET_BITMAPS];
extern W_Icon   mbplanets[NUM_PLANET_BITMAPS];
extern W_Icon   bplanets2[NUM_PLANET_BITMAPS2];
extern W_Icon   mbplanets2[NUM_PLANET_BITMAPS2];

#ifdef MOOBITMAPS
extern W_Icon   bplanets3[NUM_PLANET_BITMAPS2];	/* isae: added this */
extern W_Icon   mbplanets3[NUM_PLANET_BITMAPS2];	/* isae: added this */
#endif				/* MOOBITMAPS */

extern W_Color  borderColor, backColor, textColor, myColor, warningColor,
                shipCol[5], rColor, yColor, gColor, unColor, foreColor;

extern char lastMessage[100];

extern char    *classes[];
extern char     teamlet[];
extern char    *teamshort[];
extern char     pseudo[PSEUDOSIZE];
extern char     login[PSEUDOSIZE];
#ifdef AUTOLOGIN
extern char     defpasswd[PSEUDOSIZE];
extern int	autologin;
#endif

extern struct ship	shipvals[NUM_TYPES];

extern struct rank ranks[NUMRANKS];

extern W_Window messagew, w, mapw, statwin, baseWin, infow, iconWin, tstatw, war,
                warnw, helpWin, teamWin[4], quitwin, messwa, messwt, messwi, messwk,
                planetw, rankw, playerw, optionWin, reviewWin, waitWin, metaWin;
#ifdef XTREKRC_HELP
extern W_Window	defWin;
#endif
#ifdef SCAN
extern W_Window scanw, scanwin;
#endif				/* ATM */
extern W_Window	udpWin;

extern W_Window	phaserwin;
extern W_Window motdWin;

#ifdef SHORT_PACKETS
extern W_Window spWin;
#endif

#ifdef EM
extern int      sortPlayers;
extern int      sortPlayersObs;

#endif

#ifdef CURSORFIX
#define MAXCURSORS 10
#define MESGCURSOR 1
#define TMAPCURSOR 2
#define GMAPCURSOR 2		/* note these are the same! */
#endif

#ifdef PING
extern int      no_ping;		/* to ping or not to ping */
extern int      ping;		/* to ping or not to ping */
extern long     packets_sent;	/* # all packets sent to server */
extern long     packets_received;	/* # all packets received */
extern W_Window pStats;
#endif

extern int      use_msgw;
extern int      phas_msgi;

/* tmp */
#ifdef BD
extern int      bd;
#endif

#ifdef SHORT_PACKETS
extern int      tryShort;
extern int      recv_short;
extern int	recv_short_opt;

/* OLD */
extern int      recv_mesg;
extern int      recv_kmesg;
extern int      recv_threshold;
extern char     recv_threshold_s[];
extern int      recv_warn;
extern int	shortversion;
#endif

extern int	tclock;

extern int	alt_playerlist;

extern int	new_messages;

extern int	max_fd;

#ifdef FEATURE
extern int	MacroNum;
extern int      F_UseNewMacro;
extern int      F_UseSmartMacro;
extern int      F_server_feature_packets;
extern int      F_why_dead;
extern int	F_cloak_maxwarp;
extern int	F_self_8flags;
extern int	F_self_8flags2;
extern int	F_motd_bitmaps;
extern W_Window	macroWin;
extern int	macrokey;
extern int	F_gen_distress;
extern char	*singleMacro;
extern int	F_phaser_multi_send;
extern int	F_ship_cap;
extern int  F_fps;
extern int  F_ups;
#endif

extern int	abbr_kmesg;
#endif

extern int	maskrecip;

extern int	dashboardStyle;

extern int	plshowstatus;

extern char	cloakChars[3];
extern int	showInd;
extern int	stippleBorder;

extern int	showPlanetOwner;
extern int	enemyPhasers;
extern int	newInfo;

#ifdef TTS
#define TTS_TIME	25

extern int	tts;
extern int	tts_len;
extern int	tts_max_len;
extern int	tts_width;
extern int	tts_loc;
extern int      tts_timer;
extern int      tts_time;
extern char	lastIn[100];
#endif

#ifdef DROP_FIX
extern int	drop_fix;
#endif

#ifdef MULTILINE_MACROS
extern int 	multiline_enabled;
#endif

extern int	motd_line;
extern int	motdw_line;

extern char     puck_name[PSEUDOSIZE];
extern char     puck_host[PSEUDOSIZE];

#ifdef PHASER_SHRINK
extern int	shrink_phasers;
extern int	shrink_phasers_amount;
#endif

extern char	*defaults_file;

extern char    	*metaserver;
extern int      metaport;

extern char	*plist;
extern int	list_needs_stats;
extern char	*program;

#ifdef FORKNETREK
extern int	waitnum;
#endif

extern int	non_obscure;
extern int	observ;

extern int	basetime;

extern int	torprepeat;
extern int  last_torp;

extern int  cloak_phases;
