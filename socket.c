/*
 * Socket.c
 * 
 * Kevin P. Smith 1/29/89 UDP stuff v1.0 by Andy McFadden  Feb-Apr 1992
 * 
 * UDP protocol v1.0
 * 
 * Routines to allow connection to the xtrek server.
 */
#include "copyright2.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <math.h>
#include <errno.h>
#include "netrek.h"

#ifdef SHORT_PACKETS
#include "sp.h"
#endif

#ifdef nodef
#define INCLUDE_SCAN		/* include Amdahl scanning beams */
#define INCLUDE_VISTRACT	/* include visible tractor beams */
#endif

#ifdef GATEWAY
/*
 * (these values are now defined in "main.c":) char *gw_mach        =
 * "charon";     |client gateway; strcmp(serverName) int   gw_serv_port   =
 * 5000;         |what to tell the server to use int   gw_port        = 5001;
 * |where we will contact gw int   gw_local_port  = 5100;         |where we
 * expect gw to contact us
 * 
 * The client binds to "5100" and sends "5000" to the server (TCP).  The server
 * sees that and sends a UDP packet to gw on port udp5000, which passes it
 * through to port udp5100 on the client.  The client-gw gets the server's
 * host and port from recvfrom.  (The client can't use the same method since
 * these sockets are one-way only, so it connect()s to gw_port (udp5001) on
 * the gateway machine regardless of what the server sends.)
 * 
 * So all we need in .gwrc is: udp 5000 5001 tde.uts 5100
 * 
 * assuming the client is on tde.uts.  Note that a UDP declaration will work for
 * ANY server, but you need one per player, and the client has to have the
 * port numbers in advance.
 * 
 * If we're using a standard server, we're set.  If we're running through a
 * gatewayed server, we have to do some unpleasant work on the server side...
 */
#endif

#ifdef NEED_EXIT
#ifdef sgi
void            exit(int status);
#endif				/* sgi */
#endif				/* NEED_EXIT */

#ifdef RECORD
#include "recorder.h"
#endif RECORD

#ifdef SHORT_PACKETS
static char     numofbits[256] =
{0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1,
 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 1,
 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2,
 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1,
 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2,
 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2,
 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3,
 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1,
 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2,
 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2,
 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3,
 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 2,
 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3,
 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3,
 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4,
 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};

static int      vtsize[9] =
{4, 8, 8, 12, 12, 16, 20, 20, 24};	/* How big is the torppacket */
#ifdef unused
static int      vtdata[9] =
{0, 3, 5, 7, 9, 12, 14, 16, 18};/* How big is Torpdata */
#endif
static int      vtisize[9] =
{4, 7, 9, 11, 13, 16, 18, 20, 22};	/* 4 byte Header + torpdata */

int             spwinside = 500;/* WINSIDE from Server */
#define SPWINSIDE 500		/* To make it safe */
long            spgwidth = GWIDTH;	/* GWITHT from Server */

static
int             my_x, my_y;	/* for rotation we need to keep track of our
				 * real coordinates */
#endif				/* SHORT_PACKETS */

#ifdef PACKET_LOG
int             ALL_BYTES = 0;	/* To log all bytes */
#endif

#if __STDC__ || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

/* socket.c */
static void handleTorp P_((struct torp_spacket * packet));
static void handleVTorp P_((unsigned char *sbuf));
static void handleTorpInfo P_((struct torp_info_spacket * packet));
static void handleStatus P_((struct status_spacket * packet));
static void handleSelf P_((struct you_spacket * packet));
static void handleSelfShort P_((struct youshort_spacket * packet));
static void handleSelfShip P_((struct youss_spacket * packet));
static void handlePlayer P_((struct player_spacket * packet));
static void handleVPlayer P_((unsigned char *sbuf));
static void handleWarning P_((struct warning_spacket * packet));
static void handlePlanet P_((struct planet_spacket * packet));
static void handlePhaser P_((struct phaser_spacket * packet));
static void handleSMessage P_((struct mesg_s_spacket * packet));
static void handleQueue P_((struct queue_spacket * packet));
static void handlePickok P_((struct pickok_spacket * packet));
static void handleLogin P_((struct login_spacket * packet));
static void handlePlasmaInfo P_((struct plasma_info_spacket * packet));
static void handlePlasma P_((struct plasma_spacket * packet));
static void handleFlags P_((struct flags_spacket * packet));
static void handleKills P_((struct kills_spacket * packet));
static void handlePStatus P_((struct pstatus_spacket * packet));
static void handleMotd P_((struct motd_spacket * packet));
static void handleMotdPic P_((struct motd_pic_spacket * packet));
static void handleBitmap P_((struct bitmap_spacket * packet));
static void handleMask P_((struct mask_spacket * packet));
static void handleBadVersion P_((struct badversion_spacket * packet));
static void handleHostile P_((struct hostile_spacket * packet));
static void handlePlyrLogin P_((struct plyr_login_spacket * packet));
static void handleStats P_((struct stats_spacket * packet));
static void handlePlyrInfo P_((struct plyr_info_spacket * packet));
static void handlePlanetLoc P_((struct planet_loc_spacket * packet));
static void handleReserved P_((struct reserved_spacket * packet));
static void handleFeature P_((struct feature_spacket * packet));
static void handleUdpReply P_((struct udp_reply_spacket * packet));
static void handleSequence P_((struct sequence_spacket * packet));
static void handleShortReply P_((struct shortreply_spacket * packet));
static void handleVTorpInfo P_((unsigned char *sbuf));
static void handleVPlanet P_((unsigned char *sbuf));
/* S_P2 */
static void handleVKills P_((unsigned char *sbuf));
static void handleVPhaser P_((unsigned char *sbuf));
static void handle_s_Stats P_((struct stats_s_spacket * packet));
static void handleShipCap P_((struct ship_cap_spacket *packet));

static void new_flags P_((unsigned int data, int which));

static void processBitmap P_((struct bitmap_spacket * packet, int size, char *bits));
static int sock_read P_((int sock, char *buff, int size));

#undef P_

struct packet_handler handlers[] =
{
   {0, NULL},			/* record 0 */
   {sizeof(struct mesg_spacket), handleMessage},	/* SP_MESSAGE */
   {sizeof(struct plyr_info_spacket), handlePlyrInfo},	/* SP_PLAYER_INFO */
   {sizeof(struct kills_spacket), handleKills},	/* SP_KILLS */
   {sizeof(struct player_spacket), handlePlayer},	/* SP_PLAYER */
   {sizeof(struct torp_info_spacket), handleTorpInfo},	/* SP_TORP_INFO */
   {sizeof(struct torp_spacket), handleTorp},	/* SP_TORP */
   {sizeof(struct phaser_spacket), handlePhaser},	/* SP_PHASER */
   {sizeof(struct plasma_info_spacket), handlePlasmaInfo},	/* SP_PLASMA_INFO */
   {sizeof(struct plasma_spacket), handlePlasma},	/* SP_PLASMA */
   {sizeof(struct warning_spacket), handleWarning},	/* SP_WARNING */
   {sizeof(struct motd_spacket), handleMotd},	/* SP_MOTD */
   {sizeof(struct you_spacket), handleSelf},	/* SP_YOU */
   {sizeof(struct queue_spacket), handleQueue},	/* SP_QUEUE */
   {sizeof(struct status_spacket), handleStatus},	/* SP_STATUS */
   {sizeof(struct planet_spacket), handlePlanet},	/* SP_PLANET */
   {sizeof(struct pickok_spacket), handlePickok},	/* SP_PICKOK */
   {sizeof(struct login_spacket), handleLogin},	/* SP_LOGIN */
   {sizeof(struct flags_spacket), handleFlags},	/* SP_FLAGS */
   {sizeof(struct mask_spacket), handleMask},	/* SP_MASK */
   {sizeof(struct pstatus_spacket), handlePStatus},	/* SP_PSTATUS */
   {sizeof(struct badversion_spacket), handleBadVersion},	/* SP_BADVERSION */
   {sizeof(struct hostile_spacket), handleHostile},	/* SP_HOSTILE */
   {sizeof(struct stats_spacket), handleStats},	/* SP_STATS */
   {sizeof(struct plyr_login_spacket), handlePlyrLogin},	/* SP_PL_LOGIN */
   {sizeof(struct reserved_spacket), handleReserved},	/* SP_RESERVED */
   {sizeof(struct planet_loc_spacket), handlePlanetLoc},	/* SP_PLANET_LOC */
#ifdef HANDLE_SCAN
   {sizeof(struct scan_spacket), handleScan},	/* SP_SCAN (ATM) */
#else
   {0, exit},			/* won't be called */
#endif
   {sizeof(struct udp_reply_spacket), handleUdpReply},	/* SP_UDP_STAT */
   {sizeof(struct sequence_spacket), handleSequence},	/* SP_SEQUENCE */
   {sizeof(struct sc_sequence_spacket), handleSequence},	/* SP_SC_SEQUENCE */
   {0, exit},			/* #31, and exit won't really be called */
   {sizeof(struct motd_pic_spacket), handleMotdPic},
   {0, exit},			/* 33 */
   {0, exit},			/* 34 */
   {0, exit},			/* 35 */
   {0, exit},			/* 36 */
   {0, exit},			/* 37 */
   {0, exit},			/* 38 */
   {sizeof(struct ship_cap_spacket), handleShipCap},     /* SP_SHIP_CAP */
#ifdef SHORT_PACKETS
   {sizeof(struct shortreply_spacket), handleShortReply},	/* SP_S_REPLY */
   {-1, handleSMessage},	/* SP_S_MESSAGE */
   {-1 /* sizeof(struct warning_s_spacket) */ , handleSWarning},	/* SP_S_WARNING */
   {sizeof(struct youshort_spacket), handleSelfShort},	/* SP_S_YOU */
   {sizeof(struct youss_spacket), handleSelfShip},	/* SP_S_YOU_SS */
   {-1, /* variable */ handleVPlayer},	/* SP_S_PLAYER */
#else
   {0, exit},			/* 40 */
   {0, exit},			/* 41 */
   {0, exit},			/* 42 */
   {0, exit},			/* 43 */
   {0, exit},			/* 44 */
   {0, exit},			/* 45 */
#endif
#ifdef PING
   {sizeof(struct ping_spacket), handlePing},	/* SP_PING */
#else
   {0, exit},
#endif
#ifdef SHORT_PACKETS
   {-1, /* variable */ handleVTorp},	/* SP_S_TORP */
   {-1, handleVTorpInfo},	/* SP_S_TORP_INFO */
   {20, handleVTorp},		/* SP_S_8_TORP */
   {-1, handleVPlanet},		/* SP_S_PLANET */
#endif
   {0, exit},			/* 51 */
   {0, exit},			/* 52 */
   {0, exit},			/* 53 */
   {0, exit},			/* 54 */
   {0, exit},			/* 55 */
#ifdef SHORT_PACKETS		/* S_P2 */
   {0, exit},			/* SP_S_SEQUENCE not yet implemented */
   {-1, handleVPhaser},		/* SP_S_PHASER */
   {-1, handleVKills},		/* SP_S_KILLS */
   {sizeof(struct stats_s_spacket), handle_s_Stats},	/* SP_S_STATS */
#else
   {0, exit},			/* 56 */
   {0, exit},			/* 57 */
   {0, exit},			/* 58 */
   {0, exit},			/* 59 */
#endif
#ifdef FEATURE
   {sizeof(struct feature_spacket), handleFeature},	/* SP_FEATURE */
#endif
   {sizeof(struct bitmap_spacket), handleBitmap},	/* SP_BITMAP */
};

int             sizes[] =
{
   0,				/* record 0 */
   sizeof(struct mesg_cpacket),	/* CP_MESSAGE */
   sizeof(struct speed_cpacket),/* CP_SPEED */
   sizeof(struct dir_cpacket),	/* CP_DIRECTION */
   sizeof(struct phaser_cpacket),	/* CP_PHASER */
   sizeof(struct plasma_cpacket),	/* CP_PLASMA */
   sizeof(struct torp_cpacket),	/* CP_TORP */
   sizeof(struct quit_cpacket),	/* CP_QUIT */
   sizeof(struct login_cpacket),/* CP_LOGIN */
   sizeof(struct outfit_cpacket),	/* CP_OUTFIT */
   sizeof(struct war_cpacket),	/* CP_WAR */
   sizeof(struct practr_cpacket),	/* CP_PRACTR */
   sizeof(struct shield_cpacket),	/* CP_SHIELD */
   sizeof(struct repair_cpacket),	/* CP_REPAIR */
   sizeof(struct orbit_cpacket),/* CP_ORBIT */
   sizeof(struct planlock_cpacket),	/* CP_PLANLOCK */
   sizeof(struct playlock_cpacket),	/* CP_PLAYLOCK */
   sizeof(struct bomb_cpacket),	/* CP_BOMB */
   sizeof(struct beam_cpacket),	/* CP_BEAM */
   sizeof(struct cloak_cpacket),/* CP_CLOAK */
   sizeof(struct det_torps_cpacket),	/* CP_DET_TORPS */
   sizeof(struct det_mytorp_cpacket),	/* CP_DET_MYTORP */
   sizeof(struct copilot_cpacket),	/* CP_COPILOT */
   sizeof(struct refit_cpacket),/* CP_REFIT */
   sizeof(struct tractor_cpacket),	/* CP_TRACTOR */
   sizeof(struct repress_cpacket),	/* CP_REPRESS */
   sizeof(struct coup_cpacket),	/* CP_COUP */
   sizeof(struct socket_cpacket),	/* CP_SOCKET */
   sizeof(struct options_cpacket),	/* CP_OPTIONS */
   sizeof(struct bye_cpacket),	/* CP_BYE */
   sizeof(struct dockperm_cpacket),	/* CP_DOCKPERM */
   sizeof(struct updates_cpacket),	/* CP_UPDATES */
   sizeof(struct resetstats_cpacket),	/* CP_RESETSTATS */
   sizeof(struct reserved_cpacket),	/* CP_RESERVED */
#ifdef INCLUDE_SCAN
   sizeof(struct scan_cpacket),	/* CP_SCAN (ATM) */
#else
   0,
#endif
   sizeof(struct udp_req_cpacket),	/* CP_UDP_REQ */
   sizeof(struct sequence_cpacket),	/* CP_SEQUENCE */
   0,				/* 37 */
   0,				/* 38 */
   0,				/* 39 */
   0,				/* 40 */
   0,				/* 41 */
#ifdef PING
   sizeof(struct ping_cpacket),	/* CP_PING_RESPONSE */
#else
   0,
#endif
#ifdef SHORT_PACKETS
   sizeof(struct shortreq_cpacket),	/* CP_S_REQ */
   sizeof(struct threshold_cpacket),	/* CP_S_THRS */
   -1,				/* CP_S_MESSAGE */
#endif
   0,				/* 46 */
   0,				/* 47 */
   0,				/* 48 */
   0,				/* 49 */
   0,				/* 50 */
   0,				/* 51 */
   0,				/* 52 */
   0,				/* 53 */
   0,				/* 54 */
   0,				/* 55 */
   0,				/* 56 */
   0,				/* 57 */
   0,				/* 58 */
   0,				/* 59 */
#ifdef FEATURE
   sizeof(struct feature_cpacket),
#endif
};

#define NUM_PACKETS (sizeof(handlers) / sizeof(handlers[0]) - 1)
#define NUM_SIZES (sizeof(sizes) / sizeof(sizes[0]) - 1)


#ifdef PACKET_LOG
/*
 * stuff useful for logging server packets to see how much bandwidth * netrek
 * is really using
 */
int             log_packets = 0;/* whether or not to be logging packets */
int             packet_log[NUM_PACKETS];	/* number of packets logged */
int             outpacket_log[NUM_SIZES];
#endif				/* PACKET_LOG */

int             serverDead = 0;

#define BUFSIZE	2048
char            buf[BUFSIZE];

static int      udpLocalPort = 0;
static int      udpServerPort = 0;
static long     serveraddr = 0;
static long     sequence = 0;
static int      drop_flag = 0;
static int      chan = -1;	/* tells sequence checker where packet is
				 * from */
static short    fSpeed, fDirection, fShield, fOrbit, fRepair, fBeamup, fBeamdown,
                fCloak, fBomb, fDockperm, fPhaser, fPlasma, fPlayLock,
                fPlanLock, fTractor, fRepress;
/* reset all the "force command" variables */
void
resetForce()
{
   fSpeed = fDirection = fShield = fOrbit = fRepair = fBeamup = fBeamdown =
   fCloak = fBomb = fDockperm = fPhaser = fPlasma = fPlayLock = fPlanLock =
   fTractor = fRepress = -1;
}

/*
 * If something we want to happen hasn't yet, send it again.
 * 
 * The low byte is the request, the high byte is a max count.  When the max
 * count reaches zero, the client stops trying.  Checking is done with a
 * macro for speed & clarity.
 */
#define FCHECK_FLAGS(flag, force, const) {                      \
        if (force > 0) {                                        \
            if (((me->p_flags & flag) && 1) ^ ((force & 0xff) && 1)) {  \
                speedReq.type = const;                          \
                speedReq.speed = (force & 0xff);                \
                sendServerPacket(&speedReq);                    \
                V_UDPDIAG(("Forced %d:%d\n", const, force & 0xff));     \
                force -= 0x100;                                 \
                if (force < 0x100) force = -1;  /* give up */   \
            } else                                              \
                force = -1;                                     \
        }                                                       \
}
#define FCHECK_VAL(value, force, const) {                       \
        if (force > 0) {                                        \
            if ((value) != (force & 0xff)) {                    \
                speedReq.type = const;                          \
                speedReq.speed = (force & 0xff);                \
                sendServerPacket(&speedReq);                    \
                V_UDPDIAG(("Forced %d:%d\n", const, force & 0xff));     \
                force -= 0x100;                                 \
                if (force < 0x100) force = -1;  /* give up */   \
            } else                                              \
                force = -1;                                     \
        }                                                       \
}
#define FCHECK_TRACT(flag, force, const) {                      \
        if (force > 0) {                                        \
            if (((me->p_flags & flag) && 1) ^ ((force & 0xff) && 1)) {  \
                tractorReq.type = const;                        \
                tractorReq.state = ((force & 0xff) >= 0x40);    \
                tractorReq.pnum = (force & 0xff) & (~0x40);     \
                sendServerPacket(&tractorReq);                  \
                V_UDPDIAG(("Forced %d:%d/%d\n", const,          \
                        tractorReq.state, tractorReq.pnum));    \
                force -= 0x100;                                 \
                if (force < 0x100) force = -1;  /* give up */   \
            } else                                              \
                force = -1;                                     \
        }                                                       \
}

void
checkForce()
{
   struct speed_cpacket speedReq;
   struct tractor_cpacket tractorReq;

   FCHECK_VAL(me->p_speed, fSpeed, CP_SPEED);	/* almost always repeats */

#ifdef nodef
   FCHECK_VAL(me->p_dir, fDirection, CP_DIRECTION);	/* (ditto) */
#endif

   FCHECK_FLAGS(PFSHIELD, fShield, CP_SHIELD);

   FCHECK_FLAGS(PFORBIT, fOrbit, CP_ORBIT);

   FCHECK_FLAGS(PFREPAIR, fRepair, CP_REPAIR);

   FCHECK_FLAGS(PFBEAMUP, fBeamup, CP_BEAM);

   if (!(me->p_flags & PFBEAMUP))
      FCHECK_FLAGS(PFBEAMDOWN, fBeamdown, CP_BEAM);

   FCHECK_FLAGS(PFCLOAK, fCloak, CP_CLOAK);

   if (!(me->p_flags & PFBEAMDOWN))
      FCHECK_FLAGS(PFBOMB, fBomb, CP_BOMB);

   FCHECK_FLAGS(PFDOCKOK, fDockperm, CP_DOCKPERM);
   FCHECK_VAL(phasers[me->p_no].ph_status, fPhaser, CP_PHASER);	/* bug: dir 0 */
   FCHECK_VAL(plasmatorps[me->p_no].pt_status, fPlasma, CP_PLASMA);	/* (ditto) */

   FCHECK_FLAGS(PFPLOCK, fPlayLock, CP_PLAYLOCK);
   FCHECK_FLAGS(PFPLLOCK, fPlanLock, CP_PLANLOCK);

   FCHECK_TRACT(PFPRESS, fRepress, CP_REPRESS);
   FCHECK_TRACT(PFTRACT, fTractor, CP_TRACTOR);
}

void
connectToServer(port)
    int             port;
{
   int             s;
   struct sockaddr_in addr;
   struct sockaddr_in naddr;
   int             len;
   fd_set          readfds;
   struct timeval  timeout;
   struct hostent *hp;
   int             optval;

   serverDead = 0;
   if (sock != -1) {
      shutdown(sock, 2);
      close(sock);
      sock = -1;
   }
#ifdef nodef
   sleep(3);			/* I think this is necessary for some unknown
				 * reason */
#endif

   printf("Waiting for connection (port %d). \n", port);

   if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      printf("I can't create a socket\n");
      exit(2);
   }
   /* allow local address resuse */
   optval = 1;
   if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &optval, sizeof(int))
       < 0) {
      perror("setsockopt");
   }
                   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = INADDR_ANY;
   addr.sin_port = htons(port);

   if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
#ifdef nodef
      sleep(10);
      if (bind(s, &addr, sizeof(addr)) < 0) {
	 sleep(10);
	 if (bind(s, &addr, sizeof(addr)) < 0) {
	    printf("I can't bind to port!\n");
	    exit(3);
	 }
      }
#endif
      perror("bind");		/* NEW */
      exit(1);
   }
   if (listen(s, 1) < 0)
      perror("listen");

   len = sizeof(naddr);

 tryagain:
   timeout.tv_sec = 240;	/* four minutes */
   timeout.tv_usec = 0;
   FD_ZERO(&readfds);
   FD_SET(s, &readfds);

   if (s >= max_fd)
      max_fd = s + 1;

   if (select(max_fd, &readfds, NULL, NULL, &timeout) == 0) {
      printf("Well, I think the server died!\n");
      exit(0);
   }
   sock = accept(s, (struct sockaddr *)&naddr, &len);

   if (sock == -1) {
      goto tryagain;
   }
   if (sock >= max_fd)
      max_fd = sock + 1;

   printf("Got connection.\n");

   close(s);
   pickSocket(port);		/* new socket != port */


   /*
    * This is necessary; it tries to determine who the caller is, and set
    * "serverName" and "serveraddr" appropriately.
    */
   len = sizeof(struct sockaddr_in);
   if (getpeername(sock, (struct sockaddr *) & addr, &len) < 0) {
      perror("unable to get peername");
      serverName = "nowhere";
   } else {
      serveraddr = addr.sin_addr.s_addr;
      hp = gethostbyaddr((char *) &addr.sin_addr.s_addr, sizeof(long), AF_INET);
      if (hp != NULL) {
	 serverName = (char *) malloc(strlen(hp->h_name) + 1);
	 strcpy(serverName, hp->h_name);
      } else {
	 serverName = (char *) malloc(strlen(inet_ntoa(addr.sin_addr)) + 1);
	 strcpy(serverName, inet_ntoa(addr.sin_addr));
      }
   }
   printf("Connection from server %s (0x%x)\n", serverName,
	  (unsigned int) serveraddr);

}

#ifdef nodef
void
set_tcp_opts(s)
    int             s;
{
   int             optval = 1;
   struct protoent *ent;

   ent = getprotobyname("TCP");
   if (!ent) {
      fprintf(stderr, "TCP protocol not found.\n");
      return;
   }
   if (setsockopt(s, ent->p_proto, TCP_NODELAY, &optval, sizeof(int)) < 0)
                      perror("setsockopt");
}

void
set_udp_opts(s)
    int             s;
{
   int             optval = BUFSIZ;
   struct protoent *ent;
   ent = getprotobyname("UDP");
   if (!ent) {
      fprintf(stderr, "UDP protocol not found.\n");
      return;
   }
   if (setsockopt(s, ent->p_proto, SO_RCVBUF, &optval, sizeof(int)) < 0)
                      perror("setsockopt");
}
#endif

void
callServer(port, server)
    int             port;
    char           *server;
{
   int             s;
   struct sockaddr_in addr;
   struct hostent *hp;

   serverDead = 0;

   printf("Calling %s on port %d.\n", server, port);

   if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      printf("I can't create a socket\n");
      exit(0);
   }
   addr.sin_family = AF_INET;
   addr.sin_port = htons(port);

   if ((addr.sin_addr.s_addr = inet_addr(server)) == -1) {
      if ((hp = gethostbyname(server)) == NULL) {
	 printf("Who is %s?\n", server);
	 exit(0);
      } else {
	 addr.sin_addr.s_addr = *(long *) hp->h_addr;
      }
   }
   serveraddr = addr.sin_addr.s_addr;

   if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      fprintf(stderr, "%s not listening!\n", server);
      exit(0);
   }
   printf("Got connection.\n");

   sock = s;

   if (sock >= max_fd)
      max_fd = sock + 1;

#ifdef TREKHOPD
   /*
    * We use a different scheme from gw: we tell the server who we want to
    * connect to and what our local UDP port is; it picks its own UDP ports
    * and tells us what they are.  This is MUCH more flexible and avoids a
    * number of problems, and may come in handy if the blessed scheme
    * changes.  It's also slightly more work.
    */
   {
      extern int      port_req, use_trekhopd, serv_port;
      extern char    *host_req;
      struct mesg_cpacket msg;
      struct mesg_spacket reply;
      int             n, count, *ip;
      char           *buf;

      if (use_trekhopd) {
	 msg.type = SP_MESSAGE;
	 msg.group = msg.indiv = msg.pad1 = 0;
	 ip = (int *) msg.mesg;
	 *(ip++) = htons(port_req);
	 *(ip++) = htons(gw_local_port);
	 strncpy(msg.mesg + 8, login, 8);
	 strcpy(msg.mesg + 16, host_req);
	 if (gwrite(s, (char *) &msg, sizeof(struct mesg_cpacket)) < 0) {
	    fprintf(stderr, "trekhopd init failure\n");
	    exit(1);
	 }
	                 printf("--- trekhopd request sent, awaiting reply\n");
	 /* now block waiting for reply */
	 count = sizeof(struct mesg_spacket);
	 for (buf = (char *) &reply; count; buf += n, count -= n) {
	    if ((n = sock_read(s, buf, count)) <= 0) {
	       perror("trekhopd read");
	       exit(1);
	    }
	 }

	 if (reply.type != SP_MESSAGE) {
	    fprintf(stderr, "Got bogus reply from trekhopd (%d)\n",
		    reply.type);
	    exit(1);
	 }
	 ip = (int *) reply.mesg;
	 gw_serv_port = ntohl(*ip++);
	 gw_port = ntohl(*ip++);
	 serv_port = ntohl(*ip++);
	 printf("--- trekhopd reply received\n");

	 printf("ports = %d/%d, %d\n", gw_serv_port, gw_port, serv_port);
      }
   }
#endif				/* TREKHOPD */

#ifdef RECORD
   if(recordGame)
     startRecorder();
#endif

   pickSocket(port);		/* new socket != port */

}

int
isServerDead()
{
   return (serverDead);
}

void
socketPause()
{
   struct timeval  timeout;
   fd_set          readfds;

#ifdef RECORD
   if(playback)
     return;
#endif

   timeout.tv_sec = 1;
   timeout.tv_usec = 0;

   FD_ZERO(&readfds);
   FD_SET(sock, &readfds);
   if (udpSock >= 0)		/* new */
      FD_SET(udpSock, &readfds);

   select(max_fd, &readfds, 0, 0, &timeout);
}

int
readFromServer(readfds)
    fd_set         *readfds;
{
   int             retval = 0;

#ifdef RECORD
   if(playback) 
     {
       if(!me) {
	 while(!me)        /* read up to the handleSelf[short] to satisfy */
	   doRead(sock);   /* findslot() */
       }
       else if(loginAccept < 1) {
	 while(loginAccept < 1)
	   doRead(sock);
	 /* there always seem to be 2 handleLogin packets, grab another */
	 /* doRead(sock);  loginAccept is >=1 on the 2nd one, no need    */
       }
       else if(pickOk < 1) { /* read up to the handlePickok to simulate */
	 while(pickOk < 1)   /* entrywin () */
	   doRead(sock);
       }
       else {
	 while(!playback_update)
	   doRead(sock);
       }
       
       return 1;
     }
#endif

   if (serverDead)
      return (0);

   if (!readfds) {
      struct timeval  timeout;
      fd_set          mask;

      readfds = &mask;

      /*
       * tryagain:
       */
      FD_ZERO(readfds);
      FD_SET(sock, readfds);
      if (udpSock >= 0)
	 FD_SET(udpSock, readfds);
      timeout.tv_sec = 0;
      timeout.tv_usec = 0;
      if ((select(max_fd, readfds, 0, 0, &timeout)) == 0) {
	 dotimers();
	 return 0;
      }
   }
   if (udpSock >= 0 && FD_ISSET(udpSock, readfds)) {
      /* WAS V_ */
      UDPDIAG(("Activity on UDP socket\n"));
      chan = udpSock;
      if (commStatus == STAT_VERIFY_UDP) {
	 warning("UDP connection established");
	 sequence = 0;		/* reset sequence #s */
	 resetForce();

	 if (udpDebug)
	    printUdpInfo();
	 UDPDIAG(("UDP connection established.\n"));

	 commMode = COMM_UDP;
	 commStatus = STAT_CONNECTED;
	 commSwitchTimeout = 0;
	 if (udpClientRecv != MODE_SIMPLE)
	    sendUdpReq(COMM_MODE + udpClientRecv);
	 if (udpWin) {
	    udprefresh(UDP_CURRENT);
	    udprefresh(UDP_STATUS);
	 }
      }
      retval += doRead(udpSock);
   }
   /* Read info from the xtrek server */
   if (FD_ISSET(sock, readfds)) {
      chan = sock;
      if (commMode == COMM_TCP)
	 drop_flag = 0;		/* just in case */
      retval += doRead(sock);
   }
   dotimers();
   return (retval != 0);	/* convert to 1/0 */
}

void
dotimers()
{
   /* if switching comm mode, decrement timeout counter */
   if (commSwitchTimeout > 0) {
      if (!(--commSwitchTimeout)) {
	 /*
	  * timed out; could be initial request to non-UDP server (which
	  * won't be answered), or the verify packet got lost en route to the
	  * server.  Could also be a request for TCP which timed out (weird),
	  * in which case we just reset anyway.
	  */
	 commModeReq = commMode = COMM_TCP;
	 commStatus = STAT_CONNECTED;
	 if (udpSock >= 0)
	    closeUdpConn();
	 if (udpWin) {
	    udprefresh(UDP_CURRENT);
	    udprefresh(UDP_STATUS);
	 }
	 warning("Timed out waiting for UDP response from server");
	 UDPDIAG(("Timed out waiting for UDP response from server\n"));
      }
   }
   /* if we're in a UDP "force" mode, check to see if we need to do something */
   if (udpClientSend > 1 && commMode == COMM_UDP)
      checkForce();
}

int
doRead(asock)
    int             asock;
{

   struct timeval  timeout;
   fd_set          readfds;
   char           *bufptr;
   int             size;
   int             count;
   int             temp;

   timeout.tv_sec = 0;
   timeout.tv_usec = 0;

   count = sock_read(asock, buf, BUFSIZE - /* space for packet frag */ BUFSIZE / 4);
#ifdef NETSTAT
   if (netstat &&
       (asock == udpSock ||
	commMode != COMM_UDP ||
	udpClientRecv == MODE_TCP)) {
      ns_record_update(count);
   }
#endif

   if (debug)
      printf("R- %8d: %d b %s\n",
	     mstime(), count, asock == udpSock ? "UDP" : "TCP");

   if (count <= 0) {
      if (asock == udpSock) {
	 if (errno == ECONNREFUSED) {
	    struct sockaddr_in addr;

	    UDPDIAG(("asock=%d, sock=%d, udpSock=%d, errno=%d\n",
		     asock, sock, udpSock, errno));
	    UDPDIAG(("count=%d\n", count));
	    UDPDIAG(("Hiccup(%d)!  Reconnecting\n", errno));
	    addr.sin_addr.s_addr = serveraddr;
	    addr.sin_port = htons(udpServerPort);
	    addr.sin_family = AF_INET;
	    if (connect(udpSock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
	       perror("connect");
	       UDPDIAG(("Unable to reconnect\n"));
	       /* and fall through to disconnect */
	    } else {
	       UDPDIAG(("Reconnect successful\n"));
	       return (0);
	    }
	 }
	 UDPDIAG(("*** UDP disconnected (res=%d, err=%d)\n",
		  count, errno));
	 warning("UDP link severed");
	 printUdpInfo();
	 closeUdpConn();
	 commMode = commModeReq = COMM_TCP;
	 commStatus = STAT_CONNECTED;
	 if (udpWin) {
	    udprefresh(UDP_STATUS);
	    udprefresh(UDP_CURRENT);
	 }
	 return (0);
      }
      printf("1) Got read() of %d. Server dead\n", count);
      perror("1) read()");
      serverDead = 1;
      return (0);
   }
   bufptr = buf;
   while (bufptr < buf + count) {
#ifdef RECORD
     if(playback && *bufptr == RECORD_UPDATE)
       size = 4;
     else
#endif
   {
      if (*bufptr < 1 || *bufptr > NUM_PACKETS || handlers[(int) *bufptr].size == 0) {
	 fprintf(stderr, "Unknown packet type: %d\n", *bufptr);
#ifndef CORRUPTED_PACKETS
	 printf("count: %d, bufptr at %d,  Content:\n", count,
		bufptr - buf);
	 for (i = 0; i < count; i++) {
	    printf("0x%x, ", (unsigned int) buf[i]);
	 }
#endif
	 return (0);
      }
      size = handlers[(int) *bufptr].size;
   }  /* playback */

      if (size == -1) {		/* variable packet */
	 switch (*bufptr) {
#ifdef SHORT_PACKETS
	 case SP_S_MESSAGE:
#ifdef RECORD
	   if(playback)   
	     /* reading 4 bytes at a time from playback file, need to read the
	      fifth to get the correct size for this packet*/
	     count += sock_read(asock, buf+count, 1);
#endif
	    size = ((unsigned char) bufptr[4]);	/* IMPORTANT  Changed */
	    break;
	 case SP_S_WARNING:
	    if ((unsigned char) bufptr[1] == STEXTE_STRING ||
		(unsigned char) bufptr[1] == SHORT_WARNING) {
	       size = ((unsigned char) bufptr[3]);
	    } else
	       size = 4;	/* Normal Packet */
	    break;
	 case SP_S_PLAYER:
	    if ((unsigned char) bufptr[1] & (unsigned char) 128) {	/* Small +extended
									 * Header */
	       size = ((unsigned char) (bufptr[1] & 0x3f) * 4) + 4;
	    } else if ((unsigned char) bufptr[1] & 64) {	/* Small Header */
	       if (shortversion == SHORTVERSION)	/* S_P2 */
		  size = ((unsigned char) (bufptr[1] & 0x3f) * 4) + 4 + (bufptr[2] * 4);
	       else
		  size = ((unsigned char) (bufptr[1] & 0x3f) * 4) + 4;
	    } else {		/* Big Header */
	       size = ((unsigned char) bufptr[1] * 4 + 12);
	    }
	    break;
	 case SP_S_TORP:
	    size = vtsize[(int) numofbits[(int) (unsigned char) bufptr[1]]];
	    break;
	 case SP_S_TORP_INFO:
	    size = (vtisize[(int) numofbits[(int) (unsigned char) bufptr[1]]] + numofbits[(unsigned char) bufptr[3]]);
	    break;
	 case SP_S_PLANET:
	    size = ((unsigned char) bufptr[1] * VPLANET_SIZE) + 2;
	    break;
	 case SP_S_PHASER:	/* S_P2 */
	    switch ((unsigned char) bufptr[1] & 0x0f) {
	    case PHFREE:
	    case PHHIT:
	    case PHMISS:
	       size = 4;
	       break;
	    case PHHIT2:
	       size = 8;
	       break;
	    default:
	       size = sizeof(struct phaser_s_spacket);
	       break;
	    }
	    break;

	 case SP_S_KILLS:	/* S_P2 */
	    size = ((unsigned char) bufptr[1] * 2) + 2;
	    break;
#endif				/* SHORT_PACKETS */
	    break;
	 default:
	    fprintf(stderr, "Unknown variable packet\n");
	    /*
	     * exit(1);
	     */
	    return 0;
	    break;
	 }
	 if ((size % 4) != 0) {
	    size += (4 - (size % 4));
	 }
	 if (size <= 0) {
	    fprintf(stderr, "Bad short-packet size value (%d) on packet %d\n",
		    size, *bufptr);
	    return 0;
	 }
      }
      /*
       * printf("enter at %d, (final: %d)\n", size, count+(buf-bufptr));
       */

      while (size > count + (buf - bufptr)) {
	 /*
	  * printf("size: %d, (final: %d)\n", size, count+(buf-bufptr));
	  */
	 /*
	  * We wait for up to ten seconds for rest of packet. If we don't get
	  * it, we assume the server died.
	  */
	 /*
	  * tryagain1:
	  */
	 /*
	  * printf("frag\n");
	  */
#ifdef RECORD
	if(!playback)
#endif
      {
	 timeout.tv_sec = 20;
	 timeout.tv_usec = 0;
	 FD_ZERO(&readfds);
	 FD_SET(asock, &readfds);
	 /* readfds=1<<asock; */
	 if ((temp = select(max_fd, &readfds, 0, 0, &timeout)) == 0) {
	    printf("Packet fragment.  Server must be dead\n");
	    serverDead = 1;
	    return (0);
	 }
      }  /* playback */

	 temp = sock_read(asock, buf + count, size - (count + (buf - bufptr)));
	 /*
	  * printf("read %d, trying for %d\n", temp, size -
	  * (count+(buf-bufptr)));
	  */
	 if (debug)
	    printf("R- %8d: %d b %s (frag)\n",
		   mstime(), temp, asock == udpSock ? "UDP" : "TCP");
	 count += temp;
	 if (temp <= 0) {
	    printf("2) Got read() of %d.  Server is dead\n", temp);
	    serverDead = 1;
	    return (0);
	 }
      }


#ifdef RECORD

      if(playback) {
	packets_recorded++;    /* packets read if playback */
#ifdef RECORD_DEBUG
	fprintf(RECORDFD, "Read packet %3d, size %3d\n", *bufptr, size);
#endif
      }

      if(playback && (*bufptr == RECORD_UPDATE)) {
	playback_update++;
	updates_recorded++;
	me->p_tractor = bufptr[1];
	if(me->p_flags & PFPLOCK)
	  me->p_playerl = bufptr[2];
	else me->p_planet = bufptr[2];
      }
      else 
#endif
   {
      if (handlers[(int) *bufptr].handler != NULL) {
	 if (asock != udpSock ||
	     (!drop_flag || *bufptr == SP_SEQUENCE || *bufptr == SP_SC_SEQUENCE)) {
	    if (debug)
	       printf("T- %3d %d\n", *bufptr, size);
#ifdef RECORD
	    if(recordGame) {
	      if(RECORDPACKET(*bufptr))
		recordPacket(bufptr, size);
#ifdef RECORD_DEBUG
	      else fprintf(RECORDFD, "Skip packet %3d\n",
			   (int)(*bufptr));
#endif
	    }
#endif
	    
#ifdef PACKET_LOG
	    if (log_packets)
	      (void) Log_Packet((char) (*bufptr), size);
#endif
#ifdef PING
	    if (asock == udpSock)
	      packets_received++;
#endif

	    (*(handlers[(int) *bufptr].handler)) (bufptr);
	 } else {
	    if (debug) {
	       if (drop_flag)
		 fprintf(stderr, "%d bytes dropped.\n", size);
	    }
	    UDPDIAG(("Ignored type %d\n", *bufptr));
	 }
      }
      else {
	fprintf(stderr, "Handler for packet %d not installed...\n", *bufptr);
      }
   }  /* playback- not a RECORD_UPDATE */
      bufptr += size;

#ifdef nodef

#ifdef RECORD
      if(playback) {
	count = BUFSIZE;
      }
      else
#endif
    {
      if (bufptr > buf + BUFSIZ) {
	 MCOPY(buf + BUFSIZ, buf, BUFSIZ);
	 if (count == BUFSIZ * 2) {
	  tryagain2:
	    FD_ZERO(&readfds);
	    FD_SET(asock, &readfds);
	    /* readfds = 1<<asock; */
	    if ((temp = select(max_fd, &readfds, 0, 0, &timeout)) > 0) {
	       temp = sock_read(asock, buf + BUFSIZ, BUFSIZ);
#ifdef RECORD_DEBUG
	       if(recordGame || playback)
		 fprintf(RECORDFD, "In this other doRead frag area...\n");
#endif
	       if (debug)
		  printf("R- %8d: %d b %s (frag2)\n",
			 mstime(), temp, asock == udpSock ? "UDP" : "TCP");
	       count = BUFSIZ + temp;
	       if (temp <= 0) {
		  printf("3) Got read() of %d.  Server is dead\n", temp);
		  serverDead = 1;
		  return (0);
	       }
	    } else {
	       count = BUFSIZ;
	    }
	 } else {
	    count -= BUFSIZ;
	 }
	 bufptr -= BUFSIZ;
      }
    }  /* playback */
#endif
   }
   return (1);
}


static void
handleTorp(packet)
    struct torp_spacket *packet;
{
   struct torp    *thetorp;

#ifdef CORRUPTED_PACKETS
   if (ntohs(packet->tnum) < 0 || ntohs(packet->tnum) >= MAXPLAYER * MAXTORP) {
      fprintf(stderr, "handleTorp: bad index %d\n", ntohs(packet->tnum));
      return;
   }
#endif


   thetorp = &torps[ntohs(packet->tnum)];

#ifdef DROP_FIX
   if (drop_fix)
      thetorp->t_lastupdate = udcounter;
#endif

   thetorp->t_x = ntohl(packet->x);
   thetorp->t_y = ntohl(packet->y);
   thetorp->t_dir = packet->dir;

#ifdef ROTATERACE
   if (rotate) {
      rotate_coord(&thetorp->t_x, &thetorp->t_y, rotate_deg,
		   GWIDTH / 2, GWIDTH / 2);
      rotate_dir(&thetorp->t_dir, rotate_deg);
   }
#endif
#ifdef BD
   if (bd)
      bd_test_torp(ntohs(packet->tnum), thetorp);
   else
      t->t_turns = 0;
#endif
}

#ifdef SHORT_PACKETS

static void
handleVTorp(sbuf)
    unsigned char  *sbuf;
{
   unsigned char  *which, *data;
   unsigned char   bitset;
   struct torp    *thetorp;
   int             dx, dy;
   int             shiftvar;

   int             i;
   register int    shift = 0;	/* How many torps are extracted (for shifting
				 * ) */
   register struct player *j;

   /* now we must find the data ... :-) */
   if (sbuf[0] == SP_S_8_TORP) {/* MAX packet */
      bitset = 0xff;
      which = &sbuf[1];
      data = &sbuf[2];
   } else {			/* Normal Packet */
      bitset = sbuf[1];
      which = &sbuf[2];
      data = &sbuf[3];
   }

#ifdef CORRUPTED_PACKETS
   /* we probably should do something clever here - jmn */
   if (*which * 8 >= MAXPLAYER * MAXTORP) {
      fprintf(stderr, "handleVTorp: bad index %d\n", *which * 8);
      return;
   }
#endif

   thetorp = &torps[((unsigned char) *which * 8)];
   for (shift = 0, i = 0; /* (unsigned char)bitset != 0 */ i < 8;
	i++,
	thetorp++,
	bitset >>= 1) {

#ifdef DROP_FIX
      if (drop_fix)
	 thetorp->t_lastupdate = udcounter;
#endif

      if (bitset & 01) {
	 dx = (*data >> shift);
	 data++;
	 shiftvar = (unsigned char) *data;	/* to silence gcc */
	 shiftvar <<= (8 - shift);
	 dx |= (shiftvar & 511);
	 shift++;
	 dy = (*data >> shift);
	 data++;
	 shiftvar = (unsigned char) *data;	/* to silence gcc */
	 shiftvar <<= (8 - shift);
	 dy |= (shiftvar & 511);
	 shift++;
	 if (shift == 8) {
	    shift = 0;
	    data++;
	 }
	 /*
	  * This is necessary because TFREE/TMOVE is now encoded in the
	  * bitset
	  */

	 if (thetorp->t_status == TFREE) {
	    thetorp->t_status = TMOVE;	/* guess */
	    j = &players[(int) thetorp->t_owner];
	    j->p_ntorp++;
	 }
	 /* NEW */
	 else if (thetorp->t_owner == me->p_no &&
	    thetorp->t_status == TEXPLODE)
	    thetorp->t_status = TMOVE;
	 /* Check if torp is visible */
	 if (dx > SPWINSIDE || dy > SPWINSIDE) {
	    thetorp->t_x = -100000;	/* Not visible */
	    thetorp->t_y = -100000;
	 } else {		/* visible */
	    thetorp->t_x = my_x + ((dx - SPWINSIDE / 2) * SCALE);
	    thetorp->t_y = my_y + ((dy - SPWINSIDE / 2) * SCALE);
#ifdef ROTATERACE
	    if (rotate) {
	       rotate_coord(&thetorp->t_x, &thetorp->t_y, rotate_deg,
			    GWIDTH / 2, GWIDTH / 2);
	    }
#endif
	 }
      }
      /* if */
      else {			/* We got a TFREE */
	 /* tsh */
	 if (thetorp->t_status &&
	     (thetorp->t_status != TEXPLODE)) {
	    players[(int) thetorp->t_owner].p_ntorp--;
	    thetorp->t_status = TFREE;	/* That's no guess */
	 }
      }
   }				/* for */
}
#endif

static void
handleTorpInfo(packet)
    struct torp_info_spacket *packet;
{
   struct torp    *thetorp;
   register struct player *j;

#ifdef CORRUPTED_PACKETS
   if (ntohs(packet->tnum) < 0 || ntohs(packet->tnum) >= MAXPLAYER * MAXTORP) {
      fprintf(stderr, "handleTorpInfo: bad index %d\n", ntohs(packet->tnum));
      return;
   }
#endif

   thetorp = &torps[ntohs((int) packet->tnum)];

#ifdef DROP_FIX
   if (drop_fix)
      thetorp->t_lastupdate = udcounter;
#endif

   if (packet->status == TEXPLODE && thetorp->t_status == TFREE) {
      /* FAT: redundant explosion; don't update p_ntorp */
      /*
       * printf("texplode ignored\n");
       */
      return;
   }
   if (thetorp->t_status == TFREE && packet->status) {
      j = &players[(int) thetorp->t_owner];
      j->p_ntorp++;
#ifdef nodef
      if (players[(int) thetorp->t_owner].p_status == PEXPLODE)
	 fprintf(stderr, "TORP STARTED WHEN PLAYER EXPLODED\n");
#endif
      /* BORG TEST */
#ifdef BD
      if (bd)
	 bd_new_torp(ntohs(packet->tnum), thetorp);
      else
	 t->t_turns = 0;
#endif
   }
   if (thetorp->t_status && packet->status == TFREE) {
      players[thetorp->t_owner].p_ntorp--;
   }
   thetorp->t_war = packet->war;

   if (packet->status != thetorp->t_status) {
      /* FAT: prevent explosion reset */
      thetorp->t_status = packet->status;
      if (thetorp->t_status == TEXPLODE) {
	 thetorp->t_fuse = NUMDETFRAMES;
      }
   }
}

static void
handleStatus(packet)
    struct status_spacket *packet;
{
   status->tourn = packet->tourn;
   status->armsbomb = ntohl(packet->armsbomb);
   status->planets = ntohl(packet->planets);
   status->kills = ntohl(packet->kills);
   status->losses = ntohl(packet->losses);
   status->time = ntohl(packet->time);
   status->timeprod = ntohl(packet->timeprod);

   if (debug) {
      printf("SERVER STATS:\n");
      printf("\ttime     : %d\n", status->time / (60 * 60 * 10));
      printf("\tkills    : %d\n", status->kills);
      printf("\tlosses   : %d\n", status->losses);
      printf("\tplanets  : %d\n", status->planets);
      printf("\tarmsbomb : %d\n", status->armsbomb);
   }
}

static void
handleSelf(packet)
    struct you_spacket *packet;
{
#ifdef CORRUPTED_PACKETS
   if (packet->pnum < 0 || packet->pnum >= MAXPLAYER) {
      fprintf(stderr, "handleSelf: bad index %d\n", packet->pnum);
      return;
   }
#endif

#ifdef RECORD_DEBUG
   if(recordGame || playback)
     fprintf(RECORDFD, "handleSelf packet, me set to player %d\n", 
	     packet->pnum);
#endif

   me = &players[packet->pnum];
   myship = &(me->p_ship);
   mystats = &(me->p_stats);
   me->p_hostile = packet->hostile;
   me->p_swar = packet->swar;
   me->p_armies = packet->armies;

#ifdef nodef
   if (((me->p_flags & PFGREEN) && (ntohl(packet->flags) & PFRED)) ||
       ((me->p_flags & PFRED) && (ntohl(packet->flags) & PFGREEN)))
      printf("green/red transition\n");
#endif

   me->p_flags = ntohl(packet->flags);

   if (me->p_flags & PFPLOCK) {
      redrawPlayer[me->p_playerl] = 1;
      observ = me->p_flags & PFOBSERV;
   }
   else observ = 0;

   me->p_damage = ntohl(packet->damage);
   me->p_shield = ntohl(packet->shield);
   me->p_fuel = ntohl(packet->fuel);
   me->p_etemp = ntohs(packet->etemp);
   me->p_wtemp = ntohs(packet->wtemp);
   me->p_whydead = ntohs(packet->whydead);
   me->p_whodead = ntohs(packet->whodead);

#ifdef RECORD_DEBUG
   if(recordGame || playback)
     fprintf(RECORDFD, "handleSelf: observ=%d, fuel=%d\n", observ, me->p_fuel);
#endif


#ifdef INCLUDE_VISTRACT
   if (packet->tractor & 0x40)
      me->p_tractor = (short) packet->tractor & (~0x40);	/* ATM - visible
								 * tractors */
#ifdef nodef			/* tmp */
   else
      me->p_tractor = -1;
#endif				/* nodef */
#endif
}

#ifdef SHORT_PACKETS
static void
handleSelfShort(packet)
    struct youshort_spacket *packet;
{

#ifdef RECORD_DEBUG
  if(recordGame || playback)
    fprintf(RECORDFD, "handleSelfShort: setting me to pnum %d\n", 
	    packet->pnum);
#endif

   me = &players[packet->pnum];
   myship = &(me->p_ship);
   mystats = &(me->p_stats);
   me->p_hostile = packet->hostile;
   me->p_swar = packet->swar;
   me->p_armies = packet->armies;
   me->p_flags = ntohl(packet->flags);
   me->p_whydead = packet->whydead;
   me->p_whodead = packet->whodead;

   if (me->p_flags & PFPLOCK) {
      redrawPlayer[me->p_playerl] = 1;
      observ = me->p_flags & PFOBSERV;
   }
   else observ = 0;

#ifdef RECORD_DEBUG
   if(recordGame || playback)
     fprintf(RECORDFD, "handleSelfShort: observ = %d, me->p_playerl=%d\n", 
	   observ, me->p_playerl);
#endif

}

static void
handleSelfShip(packet)
    struct youss_spacket *packet;
{
   if (!me)
      return;			/* wait.. */

   me->p_damage = ntohs(packet->damage);
   me->p_shield = ntohs(packet->shield);
   me->p_fuel = ntohs(packet->fuel);
   me->p_etemp = ntohs(packet->etemp);
   me->p_wtemp = ntohs(packet->wtemp);

#ifdef RECORD_DEBUG
   if(recordGame || playback)
     fprintf(RECORDFD, "handleSelfShip, fuel=%d\n", me->p_fuel);
#endif

#ifdef FEATURE
   if (F_self_8flags)
      me->p_flags = (me->p_flags & 0xffffff00) | (unsigned char) packet->flags8;
   else if (F_self_8flags2) {
      unsigned int    new_flags = me->p_flags & ~(PFSHIELD | PFREPAIR | PFCLOAK |
						PFGREEN | PFYELLOW | PFRED |
						  PFTRACT | PFPRESS);
      new_flags |= ((packet->flags8 & PFSHIELD) |
		    (packet->flags8 & PFREPAIR) |
		    ((packet->flags8 & (PFCLOAK << 2)) >> 2) |
		    ((packet->flags8 & (PFGREEN << 7)) >> 7) |
		    ((packet->flags8 & (PFYELLOW << 7)) >> 7) |
		    ((packet->flags8 & (PFRED << 7)) >> 7) |
		    ((packet->flags8 & (PFTRACT << 15)) >> 15) |
		    ((packet->flags8 & (PFPRESS << 15)) >> 15));

      me->p_flags = new_flags;
   }
#endif
}
#endif

static void
handlePlayer(packet)
    struct player_spacket *packet;
{
   register struct player *pl;
#ifdef CORRUPTED_PACKETS
   if (packet->pnum < 0 || packet->pnum >= MAXPLAYER) {
      fprintf(stderr, "handlePlayer: bad index %d\n", packet->pnum);
      return;
   }
#endif

   pl = &players[packet->pnum];

#ifdef DROP_FIX
   if (drop_fix)
      pl->p_lastupdate = udcounter;
#endif

   pl->p_dir = packet->dir;
   pl->p_speed = packet->speed;
#ifdef FEATURE
   if (F_cloak_maxwarp && pl != me) {
      if (pl->p_speed == 0xf)
	 pl->p_flags |= PFCLOAK;
      else if (pl->p_flags & PFCLOAK)
	 pl->p_flags &= ~PFCLOAK;
   }
#endif
   pl->p_x = ntohl(packet->x);
   pl->p_y = ntohl(packet->y);
   redrawPlayer[(int) packet->pnum] = 1;

#ifdef ROTATERACE
   if (rotate) {
      rotate_coord(&pl->p_x, &pl->p_y, rotate_deg, GWIDTH / 2, GWIDTH / 2);
      rotate_dir(&pl->p_dir, rotate_deg);
   }
#endif
}

#ifdef SHORT_PACKETS
static void
handleVPlayer(sbuf)
    unsigned char  *sbuf;
{
   register int    x, y, i, numofplayers, pl_no, save;
   register struct player *pl;
   numofplayers = (unsigned char) sbuf[1] & 0x3f;

#ifdef CORRUPTED_PACKETS
   /*
    * should do something clever here - jmn if(pl_no < 0 || pl_no >=
    * MAXPLAYER){ return; }
    */
#endif

#if MAXPLAYER > 32
   if (sbuf[1] & (unsigned char) 128) {	/* Short Header + Extended */
      sbuf += 4;
      for (i = 0; i < numofplayers; i++) {
	 pl_no = ((unsigned char) *sbuf & 0x1f) + 32;
	 if (pl_no >= MAXPLAYER)
	    continue;		/* a little error check */
	 save = (unsigned char) *sbuf;
	 sbuf++;
	 pl = &players[pl_no];
	 pl->p_speed = (unsigned char) *sbuf & 15;	/* SPEED */
#ifdef FEATURE
	 if (F_cloak_maxwarp && pl != me) {
	    if (pl->p_speed == 0xf)
	       pl->p_flags |= PFCLOAK;
	    else if (pl->p_flags & PFCLOAK)
	       pl->p_flags &= ~PFCLOAK;
	 }
#endif
	 pl->p_dir = (unsigned char) (*sbuf >> 4) * 16;	/* realDIR */
	 sbuf++;
	 x = (unsigned char) *sbuf++;
	 y = (unsigned char) *sbuf++;	/* The lower 8 Bits are saved */
	 /* Now we must preprocess the coordinates */
	 if ((unsigned char) save & 64)
	    x |= 256;
	 if ((unsigned char) save & 128)
	    y |= 256;
	 /* Now test if it's galactic or local coord */
	 if (save & 32) {	/* It's galactic */

	    redrawPlayer[pl_no] = 1;
	    if (x == 501 || y == 501) {
	       x = -500;
	       y = -500;
	    }
	    pl->p_x = x * (GWIDTH / SPWINSIDE);
	    pl->p_y = y * (GWIDTH / SPWINSIDE);
#ifdef DROP_FIX
	    if (drop_fix)
	       pl->p_lastupdate = udcounter;
#endif
#ifdef ROTATERACE
	    if (rotate) {
	       rotate_coord(&pl->p_x, &pl->p_y,
			    rotate_deg, GWIDTH / 2, GWIDTH / 2);
	       rotate_dir(&pl->p_dir, rotate_deg);
	    }
#endif
	 } else {		/* Local */
	    redrawPlayer[pl->p_no] = 1;
	    pl->p_x = my_x + ((x - SPWINSIDE / 2) * SCALE);
	    pl->p_y = my_y + ((y - SPWINSIDE / 2) * SCALE);
#ifdef DROP_FIX
	    if (drop_fix) {
	       pl->p_lastupdate = udcounter;
	    }
#endif
#ifdef ROTATERACE
	    if (rotate) {
	       rotate_coord(&pl->p_x, &pl->p_y,
			    rotate_deg, GWIDTH / 2, GWIDTH / 2);
	       rotate_dir(&pl->p_dir, rotate_deg);
	    }
#endif
	 }
      }				/* for */
   }
   /* if */
   else
#endif				/* MAXPLAYER > 32 */
   if (sbuf[1] & 64) {		/* Short Header  */
      if (shortversion == SHORTVERSION) {	/* flags S_P2 */
	 if (sbuf[2] == 2) {
	    unsigned int   *tmp = (unsigned int *) &sbuf[4];
	    new_flags(ntohl(*tmp), sbuf[3]);
	    tmp++;
	    new_flags(ntohl(*tmp), 0);
	    sbuf += 8;
	 } else if (sbuf[2] == 1) {
	    unsigned int   *tmp = (unsigned int *) &sbuf[4];
	    new_flags(ntohl(*tmp), sbuf[3]);
	    sbuf += 4;
	 }
      }
      sbuf += 4;
      for (i = 0; i < numofplayers; i++) {
	 pl_no = ((unsigned char) *sbuf & 0x1f);
	 if (pl_no >= MAXPLAYER)
	    continue;
	 save = (unsigned char) *sbuf;
	 sbuf++;
	 pl = &players[pl_no];
	 pl->p_speed = (unsigned char) *sbuf & 15;	/* SPEED */
#ifdef FEATURE
	 if (F_cloak_maxwarp && pl != me) {
	    if (pl->p_speed == 0xf)
	       pl->p_flags |= PFCLOAK;
	    else if (pl->p_flags & PFCLOAK)
	       pl->p_flags &= ~PFCLOAK;
	 }
#endif
	 pl->p_dir = (unsigned char) (*sbuf >> 4) * 16;	/* realDIR */
	 sbuf++;
	 x = (unsigned char) *sbuf++;
	 y = (unsigned char) *sbuf++;	/* The lower 8 Bits are saved */
	 /* Now we must preprocess the coordinates */
	 if ((unsigned char) save & 64)
	    x |= 256;
	 if ((unsigned char) save & 128)
	    y |= 256;
	 /* Now test if it's galactic or local coord */
	 if (save & 32) {	/* It's galactic */
	    redrawPlayer[pl->p_no] = 1;
	    if (x == 501 || y == 501) {
	       x = -500;
	       y = -500;
	    }
	    pl->p_x = x * (GWIDTH / SPWINSIDE);
	    pl->p_y = y * (GWIDTH / SPWINSIDE);
#ifdef DROP_FIX
	    if (drop_fix)
	       pl->p_lastupdate = udcounter;
#endif
#ifdef ROTATERACE
	    if (rotate) {
	       rotate_coord(&pl->p_x, &pl->p_y,
			    rotate_deg, GWIDTH / 2, GWIDTH / 2);
	       rotate_dir(&pl->p_dir, rotate_deg);
	    }
#endif
	 } else {		/* Local */
	    redrawPlayer[pl->p_no] = 1;
	    pl->p_x = my_x + ((x - SPWINSIDE / 2) * SCALE);
	    pl->p_y = my_y + ((y - SPWINSIDE / 2) * SCALE);
#ifdef DROP_FIX
	    if (drop_fix) {
	       pl->p_lastupdate = udcounter;
	    }
#endif
#ifdef ROTATERACE
	    if (rotate) {
	       rotate_coord(&pl->p_x, &pl->p_y,
			    rotate_deg, GWIDTH / 2, GWIDTH / 2);
	       rotate_dir(&pl->p_dir, rotate_deg);
	    }
#endif
	 }
      }				/* for */
   }
   /* 2. if */
   else {			/* Big Packet */
      struct player_s_spacket *packet = (struct player_s_spacket *) sbuf;
      pl = &players[me->p_no];
      pl->p_dir = (unsigned char) packet->dir;
      pl->p_speed = packet->speed;
#ifdef FEATURE
      if (F_cloak_maxwarp && pl != me) {
	 if (pl->p_speed == 0xf)
	    pl->p_flags |= PFCLOAK;
	 else if (pl->p_flags & PFCLOAK)
	    pl->p_flags &= ~PFCLOAK;
      }
#endif
      if (shortversion == SHORTVERSION) {	/* S_P2 */
	 struct player_s2_spacket *pa2 = (struct player_s2_spacket *) sbuf;
	 pl->p_x = my_x = SCALE * ntohs(pa2->x);
	 pl->p_y = my_y = SCALE * ntohs(pa2->y);
	 new_flags(ntohl(pa2->flags), 0);
      } else {			/* OLDSHORTVERSION */
	 pl->p_x = my_x = ntohl(packet->x);
	 pl->p_y = my_y = ntohl(packet->y);
      }
#ifdef DROP_FIX
      if (drop_fix)
	 pl->p_lastupdate = udcounter;
#endif
#ifdef ROTATERACE
      if (rotate) {
	 rotate_coord(&pl->p_x, &pl->p_y,
		      rotate_deg, GWIDTH / 2, GWIDTH / 2);
	 rotate_dir(&pl->p_dir, rotate_deg);
      }
#endif
      redrawPlayer[me->p_no] = 1;

      if (sbuf[1] == 0)
	 return;
      sbuf += 12;		/* Now the small packets */
      for (i = 0; i < numofplayers; i++) {
	 pl_no = ((unsigned char) *sbuf & 0x1f);
	 if (pl_no >= MAXPLAYER)
	    continue;
	 save = (unsigned char) *sbuf;
	 sbuf++;
	 pl = &players[pl_no];
	 pl->p_speed = (unsigned char) *sbuf & 15;	/* SPEED */
#ifdef FEATURE
	 if (F_cloak_maxwarp && pl != me) {
	    if (pl->p_speed == 0xf)
	       pl->p_flags |= PFCLOAK;
	    else if (pl->p_flags & PFCLOAK)
	       pl->p_flags &= ~PFCLOAK;
	 }
#endif
	 pl->p_dir = (unsigned char) (*sbuf >> 4) * 16;	/* realDIR */
	 sbuf++;
	 x = (unsigned char) *sbuf++;
	 y = (unsigned char) *sbuf++;	/* The lower 8 Bits are saved */
	 /* Now we must preprocess the coordinates */
	 if ((unsigned char) save & 64)
	    x |= 256;
	 if ((unsigned char) save & 128)
	    y |= 256;
	 /* Now test if it's galactic or local coord */
	 if (save & 32) {	/* It's galactic */
	    redrawPlayer[pl_no] = 1;
	    if (x == 501 || y == 501) {
	       x = -500;
	       y = -500;
	    }
	    pl->p_x = x * (GWIDTH / SPWINSIDE);
	    pl->p_y = y * (GWIDTH / SPWINSIDE);
#ifdef DROP_FIX
	    if (drop_fix)
	       pl->p_lastupdate = udcounter;
#endif
#ifdef ROTATERACE
	    if (rotate) {
	       rotate_coord(&pl->p_x, &pl->p_y,
			    rotate_deg, GWIDTH / 2, GWIDTH / 2);
	       rotate_dir(&pl->p_dir, rotate_deg);
	    }
#endif
	 } else {		/* Local */
	    redrawPlayer[pl_no] = 1;
	    pl->p_x = my_x + (x - SPWINSIDE / 2) * SCALE;
	    pl->p_y = my_y + (y - SPWINSIDE / 2) * SCALE;
#ifdef DROP_FIX
	    if (drop_fix)
	       pl->p_lastupdate = udcounter;
#endif
#ifdef ROTATERACE
	    if (rotate) {
	       rotate_coord(&pl->p_x, &pl->p_y,
			    rotate_deg, GWIDTH / 2, GWIDTH / 2);
	       rotate_dir(&pl->p_dir, rotate_deg);
	    }
#endif
	 }
      }				/* for */
   }
}

#endif				/* SHORT_PACKETS */

static void
handleWarning(packet)
    struct warning_spacket *packet;
{
   packet->mesg[sizeof(packet->mesg) - 1] = '\0';	/* guarantee null
							 * termination */
   warning(packet->mesg);
}

void
sendShortPacket(type, state)
    char            type, state;
{
   struct speed_cpacket speedReq;

   speedReq.type = type;
   speedReq.speed = state;
   sendServerPacket((struct player_spacket *) & speedReq);

   /* if we're sending in UDP mode, be prepared to force it */
   if (commMode == COMM_UDP && udpClientSend >= 2) {
      switch (type) {
      case CP_SPEED:
	 fSpeed = state | 0x100;
	 break;
      case CP_DIRECTION:
	 fDirection = state | 0x100;
	 break;
      case CP_SHIELD:
	 /* stop force -tsh */
	 fBeamup = fBeamdown = fBomb = 0;
	 fShield = state | 0xa00;
	 break;
      case CP_ORBIT:
	 /* stop force -tsh */
	 fPlayLock = fPlanLock = fSpeed = 0;
	 fOrbit = state | 0xa00;
	 break;
      case CP_REPAIR:
	 /* stop force -tsh */
	 fSpeed = fBeamup = fBeamdown = fBomb = 0;
	 fRepair = state | 0xa00;
	 break;
      case CP_CLOAK:
	 fCloak = state | 0xa00;
	 break;
      case CP_BOMB:
	 /* stop force -tsh */
	 fBeamup = fBeamdown = fPlayLock = fPlanLock = fSpeed = fShield = 0;
	 fBomb = state | 0xa00;
	 break;
      case CP_DOCKPERM:
	 fDockperm = state | 0xa00;
	 break;
      case CP_PLAYLOCK:
	 fSpeed = 0;
	 fPlayLock = state | 0xa00;
	 break;
      case CP_PLANLOCK:
	 fSpeed = 0;
	 /*
	  * XX: this avoids the case where lock never generates a PFPLLOCK --
	  * instead just orbits.  Sideaffect, won't force if you're orbiting
	  * and lock on another planet
	  */
	 if (!(me->p_flags & PFORBIT))
	    fPlanLock = state | 0xa00;
	 break;
      case CP_BEAM:
	 if (state == 1) {
	    /* stop force -tsh */
	    fRepair = fBomb = fBeamdown = fPlayLock = fPlanLock = fSpeed =
	       fShield = 0;
	    fBeamup = 1 | 0x500;
	 } else {
	    /* stop force -tsh */
	    fRepair = fBomb = fBeamup = fPlayLock = fPlanLock = fSpeed =
	       fShield = 0;
	    fBeamdown = 2 | 0x500;
	 }
	 break;
      }

      /* force weapons too? */
      if (udpClientSend >= 3) {
	 switch (type) {
	 case CP_PHASER:
	    fPhaser = state | 0x100;
	    break;
	 case CP_PLASMA:
	    fPlasma = state | 0x100;
	    break;
	 }
      }
   }
}

#define sendServerPacket(p)	_sendServerPacket((struct player_spacket *)p)
void
_sendServerPacket(packet)
    /* Pick a random type for the packet */
    struct player_spacket *packet;
{
   int             size;

   if (serverDead)
      return;
   if (packet->type < 1 || packet->type > NUM_SIZES || sizes[(int) packet->type] == 0) {
      printf("Attempt to send strange packet %d!\n", packet->type);
      return;
   }
   size = sizes[(int) packet->type];
#ifdef PACKET_LOG
   if (log_packets)
      Log_OPacket(packet->type, size);
#endif
   if (commMode == COMM_UDP) {
      /* for now, just sent everything via TCP */
   }
   if (commMode == COMM_TCP || !udpClientSend) {
      /* special case for verify packet */
      if (packet->type == CP_UDP_REQ) {
	 if (((struct udp_req_cpacket *) packet)->request == COMM_VERIFY)
	    goto send_udp;
      }
      /*
       * business as usual (or player has turned off UDP transmission)
       */
      if (gwrite(sock, (char *) packet, size) != size) {
	 printf("gwrite failed.  Server must be dead\n");
	 serverDead = 1;
      }
   } else {
      /*
       * UDP stuff
       */
      switch (packet->type) {
      case CP_SPEED:
      case CP_DIRECTION:
      case CP_PHASER:
      case CP_PLASMA:
      case CP_TORP:
      case CP_QUIT:
      case CP_PRACTR:
      case CP_SHIELD:
      case CP_REPAIR:
      case CP_ORBIT:
      case CP_PLANLOCK:
      case CP_PLAYLOCK:
      case CP_BOMB:
      case CP_BEAM:
      case CP_CLOAK:
      case CP_DET_TORPS:
      case CP_DET_MYTORP:
      case CP_REFIT:
      case CP_TRACTOR:
      case CP_REPRESS:
      case CP_COUP:
      case CP_DOCKPERM:
#ifdef INCLUDE_SCAN
      case CP_SCAN:
#endif
#ifdef PING
      case CP_PING_RESPONSE:
#endif
	 /* non-critical stuff, use UDP */
       send_udp:
#ifdef PING
	 packets_sent++;
#endif

	 V_UDPDIAG(("Sent %d on UDP port\n", packet->type));
	 if (debug)
	    printf("W- %8d: %3d b UDP\n", mstime(), size);
	 if (gwrite(udpSock, (char *) packet, size) != size) {
	    UDPDIAG(("gwrite on UDP failed.  Closing UDP connection\n"));
	    warning("UDP link severed");
	    /* serverDead=1; */
	    commModeReq = commMode = COMM_TCP;
	    commStatus = STAT_CONNECTED;
	    commSwitchTimeout = 0;
	    if (udpWin) {
	       udprefresh(UDP_STATUS);
	       udprefresh(UDP_CURRENT);
	    }
	    if (udpSock >= 0)
	       closeUdpConn();
	 }
	 break;

      default:
	 if (debug)
	    printf("W- %8d: %3d b TCP\n", mstime(), size);
	 /* critical stuff, use TCP */
	 if (gwrite(sock, (char *) packet, size) != size) {
	    printf("gwrite failed.  Server must be dead\n");
	    serverDead = 1;
	 }
      }
   }
}

static void
handlePlanet(packet)
    struct planet_spacket *packet;
{
   struct planet  *plan;
   /* FAT: prevent excessive redraw */
   int             redraw = 0;

#ifdef CORRUPTED_PACKETS
   if (packet->pnum < 0 || packet->pnum >= MAXPLANETS) {
      fprintf(stderr, "handlePlanet: bad index %d\n", packet->pnum);
      return;
   }
#endif

   plan = &planets[packet->pnum];

#if 0
   monpoprate(plan, packet);
#endif

   if (plan->pl_owner != packet->owner)
      redraw = 1;
   plan->pl_owner = packet->owner;
   if (plan->pl_owner < FED || plan->pl_owner > ORI)
      plan->pl_owner = NOBODY;
   if (plan->pl_info != packet->info)
      redraw = 1;
   plan->pl_info = packet->info;
   /* Redraw the planet because it was updated by server */

   if (plan->pl_flags != (int) ntohs(packet->flags))
      redraw = 1;
   plan->pl_flags = (int) ntohs(packet->flags);

   if (plan->pl_armies != ntohl(packet->armies)) {
#ifdef EM
      /*
       * don't redraw when armies change unless it crosses the '4' * army
       * limit. Keeps people from watching for planet 'flicker' * when
       * players are beaming
       */
      int             planetarmies = ntohl(packet->armies);
      if ((plan->pl_armies < 5 && planetarmies > 4) ||
	  (plan->pl_armies > 4 && planetarmies < 5))
#endif
	 redraw = 1;
   }
   plan->pl_armies = ntohl(packet->armies);
   if (plan->pl_info == 0) {
      plan->pl_owner = NOBODY;
   }
   if (redraw) {

      plan->pl_flags |= (PLREDRAW | PLCLEAR);
   }
}

static void
handlePhaser(packet)
    struct phaser_spacket *packet;
{
   struct phaser  *phas;
   register struct player *j;

#ifdef CORRUPTED_PACKETS
   if (packet->pnum < 0 || packet->pnum >= MAXPLAYER) {
      fprintf(stderr, "handlePhaser: bad index %d\n", packet->pnum);
      return;
   }
   if (packet->status == PHHIT &&
       (ntohl(packet->target) < 0 || ntohl(packet->target) >= MAXPLAYER)) {
      fprintf(stderr, "handlePhaser: bad target %d\n", (int) ntohl(packet->target));
      return;
   }
#endif

   phas = &phasers[packet->pnum];
   phas->ph_status = packet->status;

   if (phas->ph_status != PHFREE) {
      /* F_cloak_maxwarp can also handle this */
      j = &players[packet->pnum];
#ifdef DROP_FIX
      if (drop_fix) {
	 if (j->p_flags & PFCLOAK) {
	    j->p_flags &= ~PFCLOAK;
	 }
	 phas->ph_lastupdate = udcounter;
      }
#endif
      /* normalized maxfuse */
      phas->ph_maxfuse = (j->p_ship.s_phaserfuse * updates_per_second) / 10;
#if 0
      printf("max fuse set to %d\n", phas->ph_maxfuse);
#endif
   }
#if 0
   else
      printf("free phaser sent\n");
#endif

   phas->ph_dir = packet->dir;
   phas->ph_x = ntohl(packet->x);
   phas->ph_y = ntohl(packet->y);
   phas->ph_target = ntohl(packet->target);
   phas->ph_fuse = 0;

#ifdef ROTATERACE
   if (rotate) {
      rotate_coord(&phas->ph_x, &phas->ph_y, rotate_deg, GWIDTH / 2, GWIDTH / 2);
      rotate_dir(&phas->ph_dir, rotate_deg);
   }
#endif
}

void
handleMessage(packet)
    struct mesg_spacket *packet;
{
   if (packet->m_from >= MAXPLAYER)
      packet->m_from = 255;

#ifdef CORRUPTED_PACKETS
   packet->mesg[sizeof(packet->mesg) - 1] = '\0';
#endif

   /*
    * printf("flags: 0x%x\n", packet->m_flags); printf("from: %d\n",
    * packet->m_from); printf("recpt: %d\n", packet->m_recpt); printf("mesg:
    * %s\n", packet->mesg);
    */

   if (debug)
      printf("M- %s\n", packet->mesg);

   dmessage(packet->mesg, packet->m_flags, packet->m_from, packet->m_recpt);
}

#ifdef SHORT_PACKETS
static void
handleSMessage(packet)
    struct mesg_s_spacket *packet;
{
   char            buf[100], *bf = buf;

   if (packet->m_from >= MAXPLAYER)
      packet->m_from = 255;

   if (packet->m_from == 255) {
      strcpy(bf, "GOD->");
      bf += 5;
   } else {
      *bf++ = ' ';
      *bf++ = teamlet[players[packet->m_from].p_team];
      *bf++ = shipnos[players[packet->m_from].p_no];
      *bf++ = '-';
      *bf++ = '>';
   }

   switch (packet->m_flags & (MTEAM | MINDIV | MALL)) {
   case MALL:
      bf = strcpy_return(bf, "ALL");
      break;
   case MTEAM:
      bf = strcpy_return(bf, teamshort[me->p_team]);
      break;
   case MINDIV:
      /* I know that it's me -> xxx but i copied it straight ... */
      *bf++ = teamlet[players[packet->m_recpt].p_team];
      *bf++ = shipnos[packet->m_recpt];
      break;
   default:
      bf = strcpy_return(bf, "ALL");
      break;
   }
   while (bf - buf < 9)
      *bf++ = ' ';
   strcpy(bf, &packet->mesg);
   if (debug)
      printf("M- %s\n", buf);
   dmessage(buf, packet->m_flags, packet->m_from, packet->m_recpt);
}
#endif				/* SHORT_PACKETS */

static void
handleQueue(packet)
    struct queue_spacket *packet;
{
   queuePos = ntohs(packet->pos);
}

void
sendTeamReq(team, ship)
    int             team, ship;
{
   struct outfit_cpacket outfitReq;

   outfitReq.type = CP_OUTFIT;
   outfitReq.team = team;
   outfitReq.ship = ship;
   sendServerPacket((struct player_spacket *) & outfitReq);
}

static void
handlePickok(packet)
    struct pickok_spacket *packet;
{
   pickOk = packet->state;

#ifdef RECORD
#ifdef RECORD_DEBUG
   if(recordGame || playback)
     fprintf(RECORDFD, "handlePickok: pickOk=%d\n", pickOk);
#endif
   if(playback)
     teamReq = packet->pad2;
#endif

}

void
sendLoginReq(name, pass, login, query)
    char           *name, *pass;
    char           *login;
    char            query;
{
   struct login_cpacket packet;

   strcpy(packet.name, name);
   strcpy(packet.password, pass);
   if (strlen(login) > 15)
      login[15] = 0;
   strcpy(packet.login, login);
   packet.type = CP_LOGIN;
   packet.query = query;
   sendServerPacket((struct player_spacket *) & packet);
}

static void
handleLogin(packet)
    struct login_spacket *packet;
{
   loginAccept = packet->accept;

#ifdef wait
   if ((packet->pad2 == 69) && (packet->pad3 == 42)) {
      fprintf(stderr,
	      "Sorry, this client is incompatible with Paradise servers.\n");
      fprintf(stderr,
	  "You need to get a Paradise client (see rec.games.netrek FAQ)\n");

      sendByeReq();
      exit(0);
   }
#endif

#ifdef RECORD_DEBUG
   if(recordGame || playback)
     fprintf(RECORDFD, "handleLogin: accept=%d, me=%p\n", packet->accept, me);
#endif

   if (/* me  DBP && */ packet->accept) {
      /*
       * no longer needed .. we have it in xtrekrc MCOPY(packet->keymap,
       * mystats->st_keymap, 96);
       */
      mystats->st_flags = ntohl(packet->flags);
#ifdef nodef
      /* I think these are all obsolete */
      showShields = (me->p_stats.st_flags / ST_SHOWSHIELDS) & 1;
      keeppeace = (me->p_stats.st_flags / ST_KEEPPEACE) & 1;
      mapmode = (me->p_stats.st_flags / ST_MAPMODE) & 1;
#endif
      namemode = (me->p_stats.st_flags / ST_NAMEMODE) & 1;
      showlocal = (me->p_stats.st_flags / ST_SHOWLOCAL) & 3;
      showgalactic = (me->p_stats.st_flags / ST_SHOWGLOBAL) & 3;
   }
}

void
sendTractorReq(state, pnum)
    char            state;
    char            pnum;
{
   struct tractor_cpacket tractorReq;

   tractorReq.type = CP_TRACTOR;
   tractorReq.state = state;
   tractorReq.pnum = pnum;
   sendServerPacket((struct player_spacket *) & tractorReq);

   if (state) {
      /* stop force */
      fRepress = 0;
      fTractor = pnum | 0x40;
   } else
      fTractor = 0;
}

void
sendRepressReq(state, pnum)
    char            state;
    char            pnum;
{
   struct repress_cpacket repressReq;

   repressReq.type = CP_REPRESS;
   repressReq.state = state;
   repressReq.pnum = pnum;
   sendServerPacket((struct player_spacket *) & repressReq);

   if (state) {
      /* stop force */
      fTractor = 0;
      fRepress = pnum | 0x40;
   } else
      fRepress = 0;
}

void
sendDetMineReq(torp)
    short           torp;
{
   struct det_mytorp_cpacket detReq;

   detReq.type = CP_DET_MYTORP;
   detReq.tnum = htons(torp);
   sendServerPacket((struct player_spacket *) & detReq);
}

static void
handlePlasmaInfo(packet)
    struct plasma_info_spacket *packet;
{
   struct plasmatorp *thetorp;
   register struct player *j;

#ifdef CORRUPTED_PACKETS
   if (ntohs(packet->pnum) >= MAXPLAYER * MAXPLASMA) {
      fprintf(stderr, "handlePlasmaInfo: bad index %d\n", packet->pnum);
      return;
   }
#endif


   thetorp = &plasmatorps[ntohs(packet->pnum)];

   if (packet->status == PTEXPLODE && thetorp->pt_status == PTFREE) {
      /* FAT: redundant explosion; don't update p_nplasmatorp */
      return;
   }
#ifdef DROP_FIX
   if (drop_fix){
      thetorp->pt_lastupdate = udcounter;
      thetorp->pt_last_info_update = udcounter;
   }
#endif
   if (!thetorp->pt_status && packet->status) {
      j = &players[thetorp->pt_owner];
      j->p_nplasmatorp++;
   }
   if (thetorp->pt_status && !packet->status) {
      players[thetorp->pt_owner].p_nplasmatorp--;
   }
   thetorp->pt_war = packet->war;
   if (thetorp->pt_status != packet->status) {
      /* FAT: prevent explosion timer from being reset */
      thetorp->pt_status = packet->status;
      if (thetorp->pt_status == PTEXPLODE) {
	 thetorp->pt_fuse = NUMDETFRAMES;
      }
   }
}

static void
handlePlasma(packet)
    struct plasma_spacket *packet;
{
   struct plasmatorp *thetorp;
#ifdef CORRUPTED_PACKETS
   if (ntohs(packet->pnum) >= MAXPLAYER * MAXPLASMA) {
      fprintf(stderr, "handlePlasma: bad index %d\n", packet->pnum);
      return;
   }
#endif
   thetorp = &plasmatorps[ntohs(packet->pnum)];
#ifdef DROP_FIX
   if (drop_fix){

      thetorp->pt_lastupdate = udcounter;

      /* The server sometimes sends a plasma move right after it
	 has exploded.  Since the client sets plasma status to free when
	 the explosion has expired, we test the time difference from that
	 event to determine if this move is bogus and not a case of
	 a fired plasma being missed */
      
      if(!thetorp->pt_status && udcounter - thetorp->pt_last_info_update > 2){
	 struct player	*j;
	 /* whoops, missed info packet */
	 thetorp->pt_status = PTMOVE;	/* guess */
#if 0
printf("reactivated, status 1 (delay %d)\n", udcounter-thetorp->pt_last_info_update);
#endif
	 /* don't know war status, we'll rely on player team war status */
	 j = &players[thetorp->pt_owner];
	 j->p_nplasmatorp++;
	 thetorp->pt_war = (j->p_hostile | j->p_swar);
      }
   }
#endif

   thetorp->pt_x = ntohl(packet->x);
   thetorp->pt_y = ntohl(packet->y);

#ifdef ROTATERACE
   if (rotate) {
      rotate_coord(&thetorp->pt_x, &thetorp->pt_y, rotate_deg, GWIDTH / 2, GWIDTH / 2);
   }
#endif
}

static void
handleFlags(packet)
    struct flags_spacket *packet;
{
#ifdef CORRUPTED_PACKETS
   if (packet->pnum < 0 || packet->pnum >= MAXPLAYER) {
      fprintf(stderr, "handleFlags: bad index %d\n", packet->pnum);
      return;
   }
#endif
   if (players[packet->pnum].p_flags != ntohl(packet->flags)
#ifdef INCLUDE_VISTRACT
       || players[packet->pnum].p_tractor !=
       ((short) packet->tractor & (~0x40))
#endif
      ) {
      /* FAT: prevent redundant player update */
      redrawPlayer[(int) packet->pnum] = 1;
   } else
      return;

   players[(int) packet->pnum].p_flags = ntohl(packet->flags);
#ifdef INCLUDE_VISTRACT
   if (packet->tractor & 0x40)
      players[(int) packet->pnum].p_tractor = (short) packet->tractor & (~0x40);	/* ATM - visible
											 * tractors */
   else
#endif				/* INCLUDE_VISTRACT */
      players[(int) packet->pnum].p_tractor = -1;
}

static void
handleKills(packet)
    struct kills_spacket *packet;
{
#ifdef CORRUPTED_PACKETS
   if (packet->pnum < 0 || packet->pnum >= MAXPLAYER) {
      fprintf(stderr, "handleKills: bad index %d\n", packet->pnum);
      return;
   }
#endif
   if (players[packet->pnum].p_kills != ntohl(packet->kills) / 100.0) {
      players[packet->pnum].p_kills = ntohl(packet->kills) / 100.0;
      /* FAT: prevent redundant player update */
      updatePlayer[(int) packet->pnum] = 1;
#ifdef ARMY_SLIDER
      if (me == &players[(int) packet->pnum]) {
	 calibrate_stats();
	 redrawStats();
      }
#endif				/* ARMY_SLIDER */
   }
}

static void
handlePStatus(packet)
    struct pstatus_spacket *packet;
{
   register struct player *j;
#ifdef CORRUPTED_PACKETS
   if (packet->pnum < 0 || packet->pnum >= MAXPLAYER) {
      fprintf(stderr, "handlePStatus: bad index %d\n", packet->pnum);
      return;
   }
#endif
   j = &players[packet->pnum];
   if (packet->status == j->p_status)
      return;

#ifdef DROP_FIX
   /* guarantees we won't miss the kill message since this is TCP */
   if (drop_fix) {
      switch (packet->status) {
      case PALIVE:
	 switch (j->p_status) {
	    /* this happens when we enter the game */
	 case PFREE:
	    break;
	 default:
	    /* entering the game. */
	    j->p_kills = 0.;
	    break;
	 }
	 break;

      case POUTFIT:
	 /* just in case */
	 j->p_x = -10000;
	 j->p_y = -10000;

	 break;
      }
   }
#endif

   updatePlayer[(int) packet->pnum] = 1;

   if (packet->status == PEXPLODE) {
      j->p_explode = 0;
   }
   /*
    * Ignore DEAD status. Instead, we treat it as PEXPLODE. This gives us
    * time to animate all the frames necessary for the explosions at our own
    * pace.
    */
   if (packet->status == PDEAD) {
      packet->status = PEXPLODE;
   }
   j->p_status = packet->status;
   redrawPlayer[(int) packet->pnum] = 1;
}

static void
handleMotd(packet)
    struct motd_spacket *packet;
{
   packet->line[sizeof(packet->line) - 1] = '\0';
   newMotdLine(packet->line);
}

static void
handleMotdPic(packet)
    struct motd_pic_spacket *packet;
{
   int             x, y, color, index, line;

   index = ntohs(packet->index);
   x = ntohs(packet->x);
   y = ntohs(packet->y);
   line = ntohs(packet->line);
   color = packet->color;

   newMotdPic(index, x, y, line, color);
}

/*
 * fragmented bitmap re-assembly. NOTE: this is relying on reliable, ordered
 * transmision (TCP's forte)
 */
static void
handleBitmap(packet)
    struct bitmap_spacket *packet;
{
   int             w, h;
   int             size, psize;

   static int      fragment;
   static char    *curr_bits, *curr_bits_i;

#ifdef CORRUPTED_PACKETS
#ifdef RECORD
   if(!playback)
#endif
   if (chan != sock) {
      fprintf(stderr, "garbage packet discarded.\n");
      return;
   }
#endif

   w = ntohs(packet->width);
   h = ntohs(packet->height);
   size = (w + 7) / 8 * h;
   psize = ntohl(packet->size);

   if (fragment) {
      /* assembling fragments */
      MCOPY(packet->bits, curr_bits_i, psize);
      curr_bits_i += psize;
      if (curr_bits_i - curr_bits == size) {
	 /* done */
	 processBitmap(packet, size, curr_bits);
	 fragment = 0;
	 free(curr_bits);
      }
   } else if (psize < size) {
      fragment = 1;
      curr_bits = curr_bits_i = (char *) malloc(size);
      MCOPY(packet->bits, curr_bits_i, psize);
      curr_bits_i += psize;
   } else
      processBitmap(packet, psize, packet->bits);
}

static void
processBitmap(packet, size, bits)
    struct bitmap_spacket *packet;
    int             size;
    char           *bits;
{
   switch (packet->bitmap_type) {
   case BITMAP_MOTD:
      newMotdPicBitmap(size, ntohs(packet->width), ntohs(packet->height),
		       bits);
      break;
   default:
      fprintf(stderr, "Unknown bitmap type.\n");
   }
}

void
sendMessage(mes, group, indiv)
    char           *mes;
    int             group, indiv;
{
   struct mesg_cpacket mesPacket;
#ifdef SHORT_PACKETS
   if (recv_short) {
      int             size;
      size = strlen(mes);
      size += 5;		/* 1 for '\0', 4 packetheader */
      if ((size % 4) != 0)
	 size += (4 - (size % 4));
      mesPacket.pad1 = (char) size;
      sizes[CP_S_MESSAGE] = size;
      mesPacket.type = CP_S_MESSAGE;
   } else
#endif
      mesPacket.type = CP_MESSAGE;
   mesPacket.group = group;
   mesPacket.indiv = indiv;
   strncpy(mesPacket.mesg, mes, 80);
   sendServerPacket((struct player_spacket *) & mesPacket);
}

#ifdef SHORT_PACKETS
void
sendThreshold(v)
    unsigned short  v;
{
   struct threshold_cpacket p;

   p.type = CP_S_THRS;
   p.thresh = v;
   sendServerPacket((struct player_spacket *) & p);
}
#endif

static void
handleMask(packet)
    struct mask_spacket *packet;
{
   tournMask = packet->mask;
}

void
sendOptionsPacket()
{
   struct options_cpacket optPacket;

   optPacket.type = CP_OPTIONS;
   optPacket.flags =
      htonl(ST_MAPMODE * (mapmode != 0) +
	    ST_NAMEMODE * (namemode > 0) +
	    ST_SHOWSHIELDS * showShields +
	    ST_KEEPPEACE * keeppeace +
	    ST_SHOWLOCAL * showlocal +
	    ST_SHOWGLOBAL * showgalactic);

   MZERO(optPacket.keymap, 96);
   sendServerPacket((struct player_spacket *) & optPacket);
}

void
pickSocket(old)
    int             old;
{
   int             newsocket;
   struct socket_cpacket sockPack;

   newsocket = (getpid() & 32767);
   while (newsocket < 2048 || newsocket == old) {
      newsocket = (newsocket + 10687) & 32767;
   }
   sockPack.type = CP_SOCKET;
   sockPack.socket = htonl(newsocket);
   sockPack.version = (char) SOCKVERSION;
   sockPack.udp_version = (char) UDPVERSION;
   sendServerPacket((struct player_spacket *) & sockPack);
   /* Did we get new socket # sent? */
   if (serverDead)
      return;
   nextSocket = newsocket;
}

static void
handleBadVersion(packet)
    struct badversion_spacket *packet;
{
   switch (packet->why) {
   case 0:
      printf("Sorry, this is an invalid client version.\n");
      printf("You need a new version of the client code.\n");
      break;
   case 1:
   case 2:
   case 3:
   case 4:
   case 5:
   case 6:
      printf("Sorry, but you cannot play xtrek now.\n");
      printf("Try again later.\n");
      break;
   default:
      printf("Unknown message from handleBadVersion.\n");
      return;
   }
   exit(1);
}

int
gwrite(fd, buf, bytes)
    int             fd;
    char           *buf;
    register int    bytes;
{
   long            orig = bytes;
   register long   n;

#ifdef RECORD
   if(playback)
     return bytes;
#endif

   while (bytes) {
      n = write(fd, buf, bytes);
      if (n < 0) {
	 if (fd == udpSock) {
	    fprintf(stderr, "Tried to write %d, 0x%x, %d\n",
		    fd, (unsigned int) buf, bytes);
	    perror("write");
	    printUdpInfo();
	 }
	 return (-1);
      }
      bytes -= n;
      buf += n;
   }
   return (orig);
}


static void
handleHostile(packet)
    struct hostile_spacket *packet;
{
   register struct player *pl;

#ifdef CORRUPTED_PACKETS
   if (packet->pnum < 0 || packet->pnum >= MAXPLAYER) {
      fprintf(stderr, "handleHostile: bad index %d\n", packet->pnum);
      return;
   }
#endif

   pl = &players[packet->pnum];
   if (pl->p_swar != packet->war ||
       pl->p_hostile != packet->hostile) {
      /* FAT: prevent redundant player update & redraw */
      pl->p_swar = packet->war;
      pl->p_hostile = packet->hostile;
      updatePlayer[(int) packet->pnum] = 1;
      redrawPlayer[(int) packet->pnum] = 1;
   }
}

static void
handlePlyrLogin(packet)
    struct plyr_login_spacket *packet;
{
   register struct player *pl;


#ifdef RECORD_DEBUG
   if(recordGame || playback)
     fprintf(RECORDFD, "handlePlyrLogin: me=%p, packet->pnum=%d, name=%15s\n", 
	     me, packet->pnum, packet->name);
#endif

#ifdef CORRUPTED_PACKETS
#ifdef RECORD
   if(!playback)
#endif
   if (chan == udpSock) {
      fprintf(stderr, "garbage packet discarded.\n");
      return;
   }
#endif

#ifdef CORRUPTED_PACKETS
   if (packet->pnum < 0 || packet->pnum >= MAXPLAYER) {
      fprintf(stderr, "handlePlyrLogin: bad index %d\n", packet->pnum);
      return;
   }
   if (packet->rank < 0 || packet->rank >= NUMRANKS) {
      fprintf(stderr, "handlePlyrLogin: bad rank %d\n", packet->rank);
      return;
   }
   packet->name[sizeof(packet->name) - 1] = '\0';
   packet->monitor[sizeof(packet->monitor) - 1] = '\0';
   packet->login[sizeof(packet->login) - 1] = '\0';
#endif
   updatePlayer[(int) packet->pnum] = 1;

   pl = &players[(int) packet->pnum];

   strcpy(pl->p_name, packet->name);
   strcpyp_return(pl->p_monitor, packet->monitor, sizeof(pl->p_monitor));
   strcpy(pl->p_login, packet->login);
   pl->p_stats.st_rank = packet->rank;

#ifdef HOCKEY
   pl->p_ispuck = 0;
#endif

   /*
    * printf("read player login %s, %s, %s\n", pl->p_name, pl->p_monitor,
    * pl->p_login);
    */

   if (/* me dbp && */ packet->pnum == me->p_no) {
      /* This is me.  Set some stats */
      if (lastRank == -1) {
	 if (loggedIn) {
	    lastRank = packet->rank;
	 }
      } else {
	 if (lastRank != packet->rank) {
	    lastRank = packet->rank;
	    promoted = 1;
	 }
      }
   }
#ifdef HOCKEY
   else {
      /* Hockey check */
      if (strcmp(pl->p_name, puck_name) == 0 &&
	  strncmp(pl->p_monitor, puck_host, strlen(puck_host)) == 0) {
	 /* mark as puck */
	 pl->p_ispuck = 1;
#ifndef FONT_BITMAPS
	 savepuckbitmap();
#endif
      }
   }
#endif
}


static void
handleStats(packet)
    struct stats_spacket *packet;
{
   register struct player *pl;

#ifdef CORRUPTED_PACKETS
   if (packet->pnum < 0 || packet->pnum >= MAXPLAYER) {
      fprintf(stderr, "handleStats: bad index %d\n", packet->pnum);
      return;
   }
#endif

   pl = &players[packet->pnum];

   if (list_needs_stats)
      updatePlayer[(int) packet->pnum] = 1;

   pl->p_stats.st_tkills = ntohl(packet->tkills);
   pl->p_stats.st_tlosses = ntohl(packet->tlosses);
   pl->p_stats.st_kills = ntohl(packet->kills);
   pl->p_stats.st_losses = ntohl(packet->losses);
   pl->p_stats.st_tticks = ntohl(packet->tticks);
   pl->p_stats.st_tplanets = ntohl(packet->tplanets);
   pl->p_stats.st_tarmsbomb = ntohl(packet->tarmies);
   pl->p_stats.st_sbkills = ntohl(packet->sbkills);
   pl->p_stats.st_sblosses = ntohl(packet->sblosses);
   pl->p_stats.st_armsbomb = ntohl(packet->armies);
   pl->p_stats.st_planets = ntohl(packet->planets);
   pl->p_stats.st_maxkills = ntohl(packet->maxkills) / 100.0;
   pl->p_stats.st_sbmaxkills = ntohl(packet->sbmaxkills) / 100.0;

}

static void
handlePlyrInfo(packet)
    struct plyr_info_spacket *packet;
{
   register struct player *pl;
   static int      lastship = -1;

#ifdef CORRUPTED_PACKETS
   if (packet->pnum < 0 || packet->pnum >= MAXPLAYER) {
      fprintf(stderr, "handlePlyrInfo: bad index %d\n", packet->pnum);
      return;
   }
   if (packet->team < 0 || packet->team > ALLTEAM) {
      fprintf(stderr, "handlePlyrInfo: bad team %d\n", packet->team);
      return;
   }
#endif

#ifdef RECORD_DEBUG
   if(recordGame || playback)
     fprintf(RECORDFD, "handlePlyrInfo: pnum=%d, curship=%d, new=%d, lastship=%d\n",
	  packet->pnum, players[packet->pnum].p_ship.s_type, packet->shiptype,
	   lastship);
#endif

   updatePlayer[(int) packet->pnum] = 1;
   pl = &players[packet->pnum];
   getship(&pl->p_ship, packet->shiptype);

   if(me == pl && me->p_team != packet->team){
      redrawall = 1;
   }
   pl->p_team = packet->team;

   pl->p_mapchars[0] = teamlet[pl->p_team];
   pl->p_mapchars[1] = shipnos[pl->p_no];
   if (me == pl && lastship != me->p_ship.s_type) {
      redrawTstats();
      calibrate_stats();
      redrawStats();		/* TSH */

      /* set up ship-specific keymap */
      if (keymaps[myship->s_type])
	 mykeymap = keymaps[myship->s_type];
      else
	 mykeymap = keymaps[KEYMAP_DEFAULT];

      lastship = me->p_ship.s_type;

   }
   redrawPlayer[(int) packet->pnum] = 1;
}

void
sendUpdatePacket(speed)
    long            speed;
{
   struct updates_cpacket packet;

   packet.type = CP_UPDATES;
   timerDelay = speed;
   updates_per_second = 1000000 / timerDelay;
   cloak_phases = updates_per_second * 2 - 3;
   if (cloak_phases < 3)
       cloak_phases = 3;
   packet.usecs = htonl(speed);
   sendServerPacket((struct player_spacket *) & packet);
}

static void
handlePlanetLoc(packet)
    struct planet_loc_spacket *packet;
{
   struct planet  *pl;

#ifdef CORRUPTED_PACKETS
   if (packet->pnum < 0 || packet->pnum >= MAXPLANETS) {
      fprintf(stderr, "handlePlanetLoc: bad index\n");
      return;
   }
#endif

   pl = &planets[packet->pnum];
   pl_update[(int) packet->pnum].plu_x = pl->pl_x;
   pl_update[(int) packet->pnum].plu_y = pl->pl_y;

   if (pl_update[(int) packet->pnum].plu_update != -1) {
      pl_update[(int) packet->pnum].plu_update = 1;
       /*
       printf("update: %s, old (%d,%d) new (%d,%d)\n", pl->pl_name,
       pl->pl_x, pl->pl_y, ntohl(packet->x),ntohl(packet->y));
       */
       
   } else
      pl_update[(int) packet->pnum].plu_update = 0;

   pl->pl_x = ntohl(packet->x);
   pl->pl_y = ntohl(packet->y);
   strcpy(pl->pl_name, packet->name);
   pl->pl_namelen = strlen(packet->name);
   pl->pl_flags |= (PLREDRAW | PLCLEAR);
   reinitPlanets = 1;

#ifdef ROTATERACE
   if (rotate) {
      rotate_coord(&pl->pl_x, &pl->pl_y, rotate_deg, GWIDTH / 2, GWIDTH / 2);
   }
#endif
}

static void
handleReserved(packet)
    struct reserved_spacket *packet;
{
   struct reserved_cpacket response;

#ifdef CORRUPTED_PACKETS
#ifdef RECORD
   if(!playback)
#endif
   if (chan == udpSock) {
      fprintf(stderr, "garbage Reserved packet discarded.\n");
      return;
   }
#endif

#ifdef FOR_MORONS
   {				/* it _is_ an 'info' borg, after all.  ;-) */
      extern int      For_Morons;
      if (For_Morons)
	 return;
   }
#endif

#if !defined(BORG)
   encryptReservedPacket(packet, &response, serverName, me->p_no);
   sendServerPacket((struct player_spacket *) & response);
#endif				/* defined(BORG) */
}

#ifdef FEATURE
static void
handleFeature(packet)
    struct feature_spacket *packet;
{
   checkFeature(packet);
}

#endif				/* FEATURE */

#ifdef INCLUDE_SCAN
static void
handleScan(packet)
    struct scan_spacket *packet;
{
   struct player  *pp;
#ifdef CORRUPTED_PACKETS
   if (packet->pnum < 0 || packet->pnum >= MAXPLAYER) {
      fprintf(stderr, "handleScan: bad index\n");
      return;
   }
#endif

   if (packet->success) {
      pp = &players[packet->pnum];
      pp->p_fuel = ntohl(packet->p_fuel);
      pp->p_armies = ntohl(packet->p_armies);
      pp->p_shield = ntohl(packet->p_shield);
      pp->p_damage = ntohl(packet->p_damage);
      pp->p_etemp = ntohl(packet->p_etemp);
      pp->p_wtemp = ntohl(packet->p_wtemp);
      informScan(packet->pnum);
   }
}

void
informScan(p)
    int             p;
{
}

#endif				/* INCLUDE_SCAN */

/*
 * UDP stuff
 */
void
sendUdpReq(req)
    int             req;
{
   struct udp_req_cpacket packet;

   packet.type = CP_UDP_REQ;
   packet.request = req;

   if (req >= COMM_MODE) {
      packet.request = COMM_MODE;
      packet.connmode = req - COMM_MODE;
      sendServerPacket(&packet);
      return;
   }
   if (req == COMM_UPDATE) {
#ifdef SHORT_PACKETS
      if (recv_short) {		/* not necessary */
	 /* Let the client do the work, and not the network :-) */
	 register int    i;
	 for (i = 0; i < MAXPLAYER * MAXTORP; i++)
	    torps[i].t_status = TFREE;

	 for (i = 0; i < MAXPLAYER * MAXPLASMA; i++)
	    plasmatorps[i].pt_status = PTFREE;

	 for (i = 0; i < MAXPLAYER; i++) {
	    players[i].p_ntorp = 0;
	    players[i].p_nplasmatorp = 0;
	    phasers[i].ph_status = PHFREE;
	 }
      }
#endif
      sendServerPacket(&packet);
      warning("Sent request for full update");
      return;
   }
   if (req == commModeReq) {
      warning("Request is in progress, do not disturb");
      return;
   }
   if (req == COMM_UDP) {
      /* open UDP port */
      if (openUdpConn() >= 0) {
	 UDPDIAG(("Bound to local port %d on fd %d\n", udpLocalPort, udpSock));
      } else {
	 UDPDIAG(("Bind to local port %d failed\n", udpLocalPort));
	 commModeReq = COMM_TCP;
	 commStatus = STAT_CONNECTED;
	 commSwitchTimeout = 0;
	 if (udpWin)
	    udprefresh(UDP_STATUS);
	 warning("Unable to establish UDP connection");

	 return;
      }
   }
   /* send the request */
   packet.type = CP_UDP_REQ;
   packet.request = req;
   packet.port = htonl(udpLocalPort);
#ifdef GATEWAY
   if (!strcmp(serverName, gw_mach)) {
      packet.port = htons(gw_serv_port);	/* gw port that server should
						 * call */
      UDPDIAG(("+ Telling server to contact us on %d\n", gw_serv_port));
   }
#endif
#ifdef USE_PORTSWAP
   packet.connmode = CONNMODE_PORT;	/* have him send his port */
#else
   packet.connmode = CONNMODE_PACKET;	/* we get addr from packet */
#endif
   sendServerPacket((struct player_spacket *) & packet);

   /* update internal state stuff */
   commModeReq = req;
   if (req == COMM_TCP)
      commStatus = STAT_SWITCH_TCP;
   else
      commStatus = STAT_SWITCH_UDP;
   commSwitchTimeout = 25;	/* wait 25 updates (about five seconds) */

   UDPDIAG(("Sent request for %s mode\n", (req == COMM_TCP) ?
	    "TCP" : "UDP"));

#ifndef USE_PORTSWAP
   if ((req == COMM_UDP) && recvUdpConn() < 0) {
      UDPDIAG(("Sending TCP reset message\n"));
      packet.request = COMM_TCP;
      packet.port = 0;
      commModeReq = COMM_TCP;
      sendServerPacket((struct player_spacket *) & packet);
      /* we will likely get a SWITCH_UDP_OK later; better ignore it */
      commModeReq = COMM_TCP;
      commStatus = STAT_CONNECTED;
      commSwitchTimeout = 0;
   }
#endif

   if (udpWin)
      udprefresh(UDP_STATUS);
}


static void
handleUdpReply(packet)
    struct udp_reply_spacket *packet;
{
   struct udp_req_cpacket response;

   UDPDIAG(("Received UDP reply %d\n", packet->reply));
   commSwitchTimeout = 0;

   response.type = CP_UDP_REQ;

   switch (packet->reply) {
   case SWITCH_TCP_OK:
      if (commMode == COMM_TCP) {
	 UDPDIAG(("Got SWITCH_TCP_OK while in TCP mode; ignoring\n"));
      } else {
	 commMode = COMM_TCP;
	 commStatus = STAT_CONNECTED;
	 warning("Switched to TCP-only connection");
	 closeUdpConn();
	 UDPDIAG(("UDP port closed\n"));
	 if (udpWin) {
	    udprefresh(UDP_STATUS);
	    udprefresh(UDP_CURRENT);
	 }
      }
      break;
   case SWITCH_UDP_OK:
      if (commMode == COMM_UDP) {
	 UDPDIAG(("Got SWITCH_UDP_OK while in UDP mode; ignoring\n"));
      } else {
	 /* the server is forcing UDP down our throat? */
	 if (commModeReq != COMM_UDP) {
	    UDPDIAG(("Got unsolicited SWITCH_UDP_OK; ignoring\n"));
	 } else {
#ifdef USE_PORTSWAP
	    udpServerPort = ntohl(packet->port);
	    if (connUdpConn() < 0) {
	       UDPDIAG(("Unable to connect, resetting\n"));
	       warning("Connection attempt failed");
	       commModeReq = COMM_TCP;
	       commStatus = STAT_CONNECTED;
	       if (udpSock >= 0)
		  closeUdpConn();
	       if (udpWin) {
		  udprefresh(UDP_STATUS);
		  udprefresh(UDP_CURRENT);
	       }
	       response.request = COMM_TCP;
	       response.port = 0;
	       goto send;
	    }
#else
	    /* this came down UDP, so we MUST be connected */
	    /* (do the verify thing anyway just for kicks) */
#endif
	    UDPDIAG(("Connected to server's UDP port\n"));
	    commStatus = STAT_VERIFY_UDP;
	    if (udpWin)
	       udprefresh(UDP_STATUS);
	    response.request = COMM_VERIFY;	/* send verify request on UDP */
	    response.port = 0;
	    commSwitchTimeout = 25;	/* wait 25 updates */
	  send:
	    sendServerPacket((struct player_spacket *) & response);
	 }
      }
      break;
   case SWITCH_DENIED:
      if (ntohs(packet->port)) {
	 UDPDIAG(("Switch to UDP failed (different version)\n"));
	 warning("UDP protocol request failed (bad version)");
      } else {
	 UDPDIAG(("Switch to UDP denied\n"));
	 warning("UDP protocol request denied");
      }
      commModeReq = commMode;
      commStatus = STAT_CONNECTED;
      commSwitchTimeout = 0;
      if (udpWin)
	 udprefresh(UDP_STATUS);
      if (udpSock >= 0)
	 closeUdpConn();
      break;
   case SWITCH_VERIFY:
      UDPDIAG(("Received UDP verification\n"));
      break;
   default:
      fprintf(stderr, "netrek: Got funny reply (%d) in UDP_REPLY packet\n",
	      packet->reply);
      break;
   }
}


#define MAX_PORT_RETRY  10
int
openUdpConn()
{
   struct sockaddr_in addr;
   struct hostent *hp;
   int             attempts;

#ifdef RECORD
   if(playback)
     return 0;
#endif

   if (udpSock >= 0) {
      fprintf(stderr, "netrek: tried to open udpSock twice\n");
      return (0);		/* pretend we succeeded (this could be bad) */
   }
   if ((udpSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      perror("netrek: unable to create DGRAM socket");
      return (-1);
   }
#ifdef nodef
   set_udp_opts(udpSock);
#endif				/* nodef */

   if (udpSock >= max_fd)
      max_fd = udpSock + 1;

   addr.sin_addr.s_addr = INADDR_ANY;
   addr.sin_family = AF_INET;

   errno = 0;
   /*
    * udpLocalPort = (getpid() & 32767) + (random() % 256);
    */
   udpLocalPort = 33400;
   for (attempts = 0; attempts < MAX_PORT_RETRY; attempts++) {
      /*
       * while (udpLocalPort < 2048) { udpLocalPort = (udpLocalPort + 10687)
       * & 32767; }
       */
      udpLocalPort++;
#ifdef GATEWAY
      /* we need the gateway to know where to find us */
      if (!strcmp(serverName, gw_mach)) {
	 UDPDIAG(("+ gateway test: binding to %d\n", gw_local_port));
	 udpLocalPort = gw_local_port;
      }
#endif
      addr.sin_port = htons(udpLocalPort);
      if (bind(udpSock, (struct sockaddr *)&addr, sizeof(addr)) >= 0)
	 break;
   }
   if (attempts == MAX_PORT_RETRY) {
      perror("netrek: bind");
      UDPDIAG(("Unable to find a local port to bind to\n"));
      close(udpSock);
      udpSock = -1;
      return (-1);
   }
   UDPDIAG(("Local port is %d\n", udpLocalPort));

   /* determine the address of the server */
   if (!serveraddr) {
      if ((addr.sin_addr.s_addr = inet_addr(serverName)) == -1) {
	 if ((hp = gethostbyname(serverName)) == NULL) {
	    printf("Who is %s?\n", serverName);
	    exit(0);
	 } else {
	    addr.sin_addr.s_addr = *(long *) hp->h_addr;
	 }
      }
      serveraddr = addr.sin_addr.s_addr;
      UDPDIAG(("Found serveraddr == 0x%x\n", (unsigned int) serveraddr));
   }
   return (0);
}

#ifdef USE_PORTSWAP
int
connUdpConn()
{
   struct sockaddr_in addr;
   int             len;

#ifdef RECORD
   if(playback)
     return 0;
#endif

   addr.sin_addr.s_addr = serveraddr;
   addr.sin_family = AF_INET;
   addr.sin_port = htons(udpServerPort);

   UDPDIAG(("Connecting to host 0x%x on port %d\n", serveraddr, udpServerPort));
   if (connect(udpSock, &addr, sizeof(addr)) < 0) {
      perror("netrek: unable to connect UDP socket");
      printUdpInfo();		/* debug */
      return (-1);
   }
#ifdef nodef
   len = sizeof(addr);
   if (getsockname(udpSock, &addr, &len) < 0) {
      perror("netrek: unable to getsockname(UDP)");
      UDPDIAG(("Can't get our own socket; connection failed\n"));
      close(udpSock);
      udpSock = -1;
      return -1;
   }
   printf("udpLocalPort %d, getsockname port %d\n",
	  udpLocalPort, addr.sin_port);
#endif

   return (0);
}
#endif

#ifndef USE_PORTSWAP
int
recvUdpConn()
{
   fd_set          readfds;
   struct timeval  to;
   struct sockaddr_in from;
   int             fromlen, res;

#ifdef RECORD
   if(playback)
     return 0;
#endif

   MZERO(&from, sizeof(from));	/* don't get garbage if really broken */

#ifdef NETSTAT
   ns_init(3);
#endif

   /* we patiently wait until the server sends a packet to us */
   /* (note that we silently eat the first one) */
   UDPDIAG(("Issuing recvfrom() call\n"));
   printUdpInfo();
   fromlen = sizeof(from);
   /*
    * tryagain:
    */
   FD_ZERO(&readfds);
   FD_SET(udpSock, &readfds);
   to.tv_sec = 6;		/* wait 3 seconds, then abort */
   to.tv_usec = 0;
   if ((res = select(max_fd, &readfds, 0, 0, &to)) <= 0) {
      if (!res) {
	 UDPDIAG(("timed out waiting for response"));
	 warning("UDP connection request timed out");
	 return (-1);
      } else {
	 perror("select() before recvfrom()");
	 return (-1);
      }
   }
   if (recvfrom(udpSock, buf, BUFSIZE, 0, (struct sockaddr *)&from, &fromlen) < 0) {
      perror("recvfrom");
      UDPDIAG(("recvfrom failed, aborting UDP attempt"));
      return (-1);
   }
   if (from.sin_addr.s_addr != serveraddr) {
      /* safe? */
      serveraddr = from.sin_addr.s_addr;
      UDPDIAG(("Warning: from 0x%x, but server is 0x%x\n",
	   (unsigned int) from.sin_addr.s_addr, (unsigned int) serveraddr));
   }
   if (from.sin_family != AF_INET) {
      UDPDIAG(("Warning: not AF_INET (%d)\n", from.sin_family));
   }
   udpServerPort = ntohs(from.sin_port);
   UDPDIAG(("recvfrom() succeeded; will use server port %d\n", udpServerPort));
#ifdef GATEWAY
   if (!strcmp(serverName, gw_mach)) {
      UDPDIAG(("+ actually, I'm going to use %d\n", gw_port));
      udpServerPort = gw_port;
      from.sin_port = htons(udpServerPort);
   }
#endif

   if (connect(udpSock, (struct sockaddr *)&from, sizeof(from)) < 0) {
      perror("netrek: unable to connect UDP socket after recvfrom()");
      close(udpSock);
      udpSock = -1;
      return (-1);
   }
   return (0);
}
#endif

int
closeUdpConn()
{
  V_UDPDIAG(("Closing UDP socket\n"));

#ifdef RECORD
  if(playback)
    return 0;
#endif

   if (udpSock < 0) {
      fprintf(stderr, "netrek: tried to close a closed UDP socket\n");
      return (-1);
   }
   shutdown(udpSock, 2);
   close(udpSock);
   udpSock = -1;
   return 0;
}

void
printUdpInfo()
{
   struct sockaddr_in addr;
   int             len;

   len = sizeof(addr);
   if (getsockname(udpSock, (struct sockaddr *)&addr, &len) < 0) {
      /* perror("printUdpInfo: getsockname"); */
      return;
   }
   UDPDIAG(("LOCAL: addr=0x%x, family=%d, port=%d\n",
	    (unsigned int) addr.sin_addr.s_addr,
	    addr.sin_family, ntohs(addr.sin_port)));

   if (getpeername(udpSock, (struct sockaddr *)&addr, &len) < 0) {
      /* perror("printUdpInfo: getpeername"); */
      return;
   }
   UDPDIAG(("PEER : addr=0x%x, family=%d, port=%d\n",
	    (unsigned int) addr.sin_addr.s_addr,
	    addr.sin_family, ntohs(addr.sin_port)));
}

static void
handleSequence(packet)
    struct sequence_spacket *packet;
{
   static int      recent_count = 0, recent_dropped = 0;
   long            newseq;

   drop_flag = 0;
   if (chan != udpSock)
      return;			/* don't pay attention to TCP sequence #s */
   udpTotal++;
   recent_count++;

   /* update percent display every 256 updates (~50 seconds usually) */
   if (!(udpTotal & 0xff))
      if (udpWin)
	 udprefresh(UDP_DROPPED);

   newseq = (long) ntohs(packet->sequence);
   /* printf("read %d - ", newseq); */

   if (((unsigned short) sequence) > 65000 &&
       ((unsigned short) newseq) < 1000) {
      /* we rolled, set newseq = 65536+sequence and accept it */
      sequence = ((sequence + 65536) & 0xffff0000) | newseq;
   } else {
      /* adjust newseq and do compare */
      newseq |= (sequence & 0xffff0000);

      if (!udpSequenceChk) {	/* put this here so that turning seq check */
	 sequence = newseq;	/* on and off doesn't make us think we lost */
	 return;		/* a whole bunch of packets. */
      }
      if (newseq > sequence) {
	 /* accept */
	 if (newseq != sequence + 1) {
	    udpDropped += (newseq - sequence) - 1;
	    udpTotal += (newseq - sequence) - 1;       /* want TOTAL packets */
	    recent_dropped += (newseq - sequence) - 1;
	    recent_count += (newseq - sequence) - 1;
	    if (udpWin)
	       udprefresh(UDP_DROPPED);
	    UDPDIAG(("sequence=%d, newseq=%d, we lost some\n",
		     (int) sequence, (int) newseq));
	 }
	 sequence = newseq;
	 /* S_P2 */
	 if (shortversion == SHORTVERSION && recv_short) {
	    me->p_flags = (me->p_flags & 0xffff00ff) | (unsigned int) packet->flag16 << 8;
	 }
      } else {
	 /* reject */
	 if (packet->type == SP_SC_SEQUENCE) {
	    V_UDPDIAG(("(ignoring repeat %d)\n", (int) newseq));
	 } else {
	    UDPDIAG(("sequence=%d, newseq=%d, ignoring transmission\n",
		     (int) sequence, (int) newseq));
	 }
#ifdef PING
	 /*
	  * the remaining packets will be dropped and we shouldn't count the
	  * SP_SEQUENCE packet either
	  */
	 packets_received--;
#endif
	 drop_flag = 1;
      }
   }
   /* printf("newseq %d, sequence %d\n", newseq, sequence); */
   if (recent_count > UDP_RECENT_INTR) {
      /* once a minute (at 5 upd/sec), report on how many were dropped */
      /* during the last UDP_RECENT_INTR updates                       */
      udpRecentDropped = recent_dropped;
      recent_count = recent_dropped = 0;
      if (udpWin)
	 udprefresh(UDP_DROPPED);
   }
}

#ifdef SHORT_PACKETS
static void
handleShortReply(packet)
    struct shortreply_spacket *packet;
{
   switch (packet->repl) {
   case SPK_VOFF:
      /* S_P2 */
      if (shortversion == SHORTVERSION &&
	  recv_short == 0) {	/* retry for S_P 1 */
	 printf("Using Short Packet Version 1.\n");
	 shortversion = OLDSHORTVERSION;
	 sendShortReq(SPK_VON);
      } else {
	 recv_short = recv_short_opt = 0;
	 optionredrawoption(&recv_short_opt);
      }
      break;
   case SPK_VON:
      recv_short = recv_short_opt = 1;
      optionredrawoption(&recv_short_opt);
      spwinside = ntohs(packet->winside);
      spgwidth = ntohl(packet->gwidth);

#ifdef RECORD_DEBUG
      if(recordGame || playback)
	fprintf(RECORDFD, "Receiving Short Packet Version %d\n", shortversion);
      else
#endif
      printf("Receiving Short Packet Version %d\n", shortversion);
      break;

   case SPK_THRESHOLD:
      break;
   default:
      fprintf(stderr, "%s: unknown response packet value short-req: %d\n",
	      "netrek", packet->repl);
   }
}

static void
handleVTorpInfo(sbuf)
    unsigned char  *sbuf;
{
   unsigned char  *bitset, *which, *data, *infobitset, *infodata;
   struct torp    *thetorp;
   int             dx, dy;
   int             shiftvar;
   char            status, war;
   register int    i;
   register int    shift = 0;	/* How many torps are extracted (for shifting
				 * ) */
   register struct player *j;

   /* now we must find the data ... :-) */
   bitset = &sbuf[1];
   which = &sbuf[2];
   infobitset = &sbuf[3];
   /* Where is the data ? */
   data = &sbuf[4];
   infodata = &sbuf[(int) vtisize[(int) numofbits[(int) (unsigned char) sbuf[1]]]];

#ifdef CORRUPTED_PACKETS
   /* we probably should do something clever here - jmn */
   if (*which * 8 >= MAXPLAYER * MAXTORP) {
      fprintf(stderr, "handleVTorp: bad index %d\n", *which * 8);
      return;
   }
#endif

   thetorp = &torps[((unsigned char) *which * 8)];

   for (shift = 0, i = 0;	/* (unsigned char)*infobitset !=0 ||
	   (unsigned char )*bitset != 0 */ i < 8;
	thetorp++,
	*bitset >>= 1,
	*infobitset >>= 1,
	i++) {
#ifdef DROP_FIX
      if (drop_fix)
	 thetorp->t_lastupdate = udcounter;
#endif

      if (*bitset & 01) {
	 dx = (*data >> shift);
	 data++;
	 shiftvar = (unsigned char) *data;	/* to silence gcc */
	 shiftvar <<= (8 - shift);
	 dx |= (shiftvar & 511);
	 shift++;
	 dy = (*data >> shift);
	 data++;
	 shiftvar = (unsigned char) *data;	/* to silence gcc */
	 shiftvar <<= (8 - shift);
	 dy |= (shiftvar & 511);
	 shift++;
	 if (shift == 8) {
	    shift = 0;
	    data++;
	 }
	 /*
	  * Check for torp with no TorpInfo ( In case we missed a n
	  * updateAll)
	  */
	 if (!(*infobitset & 01)) {
	    if (thetorp->t_status == TFREE) {
	       thetorp->t_status = TMOVE;	/* guess */
	       j = &players[thetorp->t_owner];
	       j->p_ntorp++;
	    } else if (thetorp->t_owner == me->p_no &&
		       thetorp->t_status == TEXPLODE){
	       thetorp->t_status = TMOVE;
	    }
	 }
	 /* Check if torp is visible */
	 if (dx > SPWINSIDE || dy > SPWINSIDE) {
	    thetorp->t_x = -100000;	/* Not visible */
	    thetorp->t_y = -100000;
	 } else {		/* visible */
	    thetorp->t_x = my_x + ((dx - SPWINSIDE / 2) *
				   SCALE);
	    thetorp->t_y = my_y + ((dy - SPWINSIDE / 2) *
				   SCALE);
#ifdef ROTATERACE
	    if (rotate) {
	       rotate_coord(&thetorp->t_x, &thetorp->t_y, rotate_deg,
			    GWIDTH / 2, GWIDTH / 2);
	    }
#endif
	 }
      }
      /* if */
      else {			/* Got a TFREE ? */
	 if (!(*infobitset & 01)) {	/* No other TorpInfo for this Torp */
	    /* tsh */
	    if (thetorp->t_status && thetorp->t_status != TEXPLODE) {
	       players[thetorp->t_owner].p_ntorp--;
	       thetorp->t_status = TFREE;	/* That's no guess */
	    }
	 }
      }
      /* Now the TorpInfo */
      if (*infobitset & 01) {
	 war = (unsigned char) *infodata & 15 /* 0x0f */ ;
	 status = ((unsigned char) *infodata & 0xf0) >> 4;
	 infodata++;

	 /*
	  * if(status == TEXPLODE) printf("explode fuse set %d, status: %d,
	  * %d\n", thetorp->t_no, thetorp->t_status, __LINE__);
	  * 
	  * if(thetorp->t_status == TEXPLODE && status == TFREE) printf("we have
	  * a reset %d\n", __LINE__);
	  */

	 if (status == TEXPLODE && thetorp->t_status == TFREE) {
	    /* FAT: redundant explosion; don't update p_ntorp */
	    continue;
	 }
	 if (thetorp->t_status == TFREE && status) {
	    j = &players[thetorp->t_owner];
	    j->p_ntorp++;
	 }
	 if (thetorp->t_status && status == TFREE) {
	    players[thetorp->t_owner].p_ntorp--;
	 }
	 thetorp->t_war = war;
	 if (status != thetorp->t_status) {
	    /* FAT: prevent explosion reset */
	    thetorp->t_status = status;
	    if (thetorp->t_status == TEXPLODE) {
	       thetorp->t_fuse = NUMDETFRAMES;
	    }
	    /*
	     * if (thetorp->t_status == TFREE) printf("torp freed %d, %d\n",
	     * thetorp->t_no, __LINE__);
	     */
	 }
      }				/* if */
   }				/* for */
}

static void
handleVPlanet(sbuf)
    unsigned char  *sbuf;
{
   register int    i;
   register int    numofplanets;/* How many Planets are in the packet */
   struct planet  *plan;
   struct planet_s_spacket *packet = (struct planet_s_spacket *) & sbuf[2];
   /* FAT: prevent excessive redraw */
   int             redraw = 0;
   numofplanets = (unsigned char) sbuf[1];

   if (numofplanets > MAXPLANETS + 1) {
      fprintf(stderr, "handleVPlanet: number planets out of bounds: %d\n",
	      numofplanets);
      return;
   }
   for (i = 0; i < numofplanets; i++, packet++) {
      if (packet->pnum < 0 || packet->pnum >= MAXPLANETS) {
	 fprintf(stderr, "handleVPlanet: planet number out of bounds: %d\n",
		 packet->pnum);
	 continue;
      }
      redraw = 0;
      plan = &planets[packet->pnum];
      if (plan->pl_owner != packet->owner)
	 redraw = 1;
      plan->pl_owner = packet->owner;
      if (plan->pl_owner && (plan->pl_owner < FED || plan->pl_owner > ORI)) {
	 fprintf(stderr, "handleVPlanet: planet owner out of bounds: %d\n",
		 plan->pl_owner);
	 plan->pl_owner = NOBODY;
      }
      if (plan->pl_info != packet->info)
	 redraw = 1;
      plan->pl_info = packet->info;
      /* Redraw the planet because it was updated by server */

      if (plan->pl_flags != (int) ntohs(packet->flags))
	 redraw = 1;
      plan->pl_flags = (int) ntohs(packet->flags);

      if (plan->pl_armies != (unsigned char) packet->armies) {
#ifdef EM
	 /*
	  * don't redraw when armies change unless it crosses the '4' * army
	  * limit. Keeps people from watching for planet 'flicker' * when
	  * players are beaming
	  */
	 int             planetarmies = (unsigned char) packet->armies;
	 if ((plan->pl_armies < 5 && planetarmies > 4) ||
	     (plan->pl_armies > 4 && planetarmies < 5))
#endif
	    redraw = 1;
      }
      plan->pl_armies = (unsigned char) packet->armies;
      if (plan->pl_info == 0) {
	 plan->pl_owner = NOBODY;
      }
      if (redraw)
	 plan->pl_flags |= (PLREDRAW | PLCLEAR);

   }				/* FOR */
}

void
sendShortReq(state)
    char            state;
{
   struct shortreq_cpacket shortReq;

   shortReq.type = CP_S_REQ;
   shortReq.req = state;
   shortReq.version = shortversion;	/* need a var now because 2 S_P
					 * versions exist S_P2 */
   switch (state) {
   case SPK_VON:
      warning("Sending short packet request");
      break;
   case SPK_VOFF:
      warning("Sending old packet request");
      break;
   default:
      break;
   }
   if ((state == SPK_SALL || state == SPK_ALL) && recv_short) {
      /* Let the client do the work, and not the network :-) */

      register int    i;
      for (i = 0; i < MAXPLAYER * MAXTORP; i++)
	 torps[i].t_status = TFREE;

      for (i = 0; i < MAXPLAYER * MAXPLASMA; i++)
	 plasmatorps[i].pt_status = PTFREE;

      for (i = 0; i < MAXPLAYER; i++) {
	 players[i].p_ntorp = 0;
	 players[i].p_nplasmatorp = 0;
	 phasers[i].ph_status = PHFREE;
      }
      warning("Sent request for small update");
   }
   sendServerPacket((struct shortreq_cpacket *) & shortReq);
}


/* S_P2 */
static void
handleVKills(sbuf)
    unsigned char  *sbuf;
{
   register int    i, numofkills, pnum;
   register unsigned short pkills;
   register unsigned char *data = &sbuf[2];

   numofkills = (unsigned char) sbuf[1];

   for (i = 0; i < numofkills; i++) {
      pkills = (unsigned short) *data++;
      pkills |= (unsigned short) ((*data & 0x03) << 8);
      pnum = (unsigned char) *data++ >> 2;

#ifdef CORRUPTED_PACKETS
      if (pnum < 0 || pnum >= MAXPLAYER) {
	 fprintf(stderr, "handleKills: bad index %d\n", pnum);
	 return;
      }
#endif
      if (players[pnum].p_kills != ((float) pkills / 100.0)) {
	 players[pnum].p_kills = pkills / 100.0;
	 /* FAT: prevent redundant player update */
	 updatePlayer[(int) pnum] = 1;
#ifdef ARMY_SLIDER
	 if (me == &players[(int) pnum]) {
	    calibrate_stats();
	    redrawStats();
	 }
#endif				/* ARMY_SLIDER */
      }
   }				/* for */

}				/* handleVKills */

static void
handleVPhaser(sbuf)
    unsigned char  *sbuf;
{
   struct phaser  *phas;
   register struct player *j;
   struct phaser_s_spacket *packet = (struct phaser_s_spacket *) & sbuf[0];
   /* not nice but.. */
   register int    pnum, status = 0, target=0, x = 0, y = 0;
   register unsigned char dir = 0;

   status = (unsigned char) packet->status & 0x0f;
   pnum = (unsigned char) packet->pnum & 0x3f;

#ifdef CORRUPTED_PACKETS
   if (pnum < 0 || pnum >= MAXPLAYER) {
      fprintf(stderr, "handleVPhaser: bad index %d\n", pnum);
      return;
   }
#endif

   switch (status) {
   case PHFREE:
      break;
   case PHHIT:
      target = (unsigned char) packet->target & 0x3f;
      break;
   case PHMISS:
      dir = (unsigned char) packet->target;
      break;
   case PHHIT2:
      x = SCALE * (ntohs(packet->x));
      y = SCALE * (ntohs(packet->y));
      target = packet->target & 0x3f;
      break;
   default:
      x = SCALE * (ntohs(packet->x));
      y = SCALE * (ntohs(packet->y));
      target = packet->target & 0x3f;
      dir = (unsigned char) packet->dir;
      break;
   }

   phas = &phasers[pnum];
   phas->ph_status = status;

   if (phas->ph_status != PHFREE) {
      /* F_cloak_maxwarp can also handle this */
      j = &players[pnum];
#ifdef DROP_FIX
      if (drop_fix) {
	 if (j->p_flags & PFCLOAK) {
	    j->p_flags &= ~PFCLOAK;
	 }
	 phas->ph_lastupdate = udcounter;
      }
#endif
      /* normalized maxfuse */
      phas->ph_maxfuse = (j->p_ship.s_phaserfuse * updates_per_second) / 10;
   }
   phas->ph_dir = dir;
   phas->ph_x = x;
   phas->ph_y = y;
   phas->ph_target = target;
   phas->ph_fuse = 0;

#ifdef ROTATERACE
   if (rotate) {
      rotate_coord(&phas->ph_x, &phas->ph_y, rotate_deg, GWIDTH / 2, GWIDTH / 2);
      rotate_dir(&phas->ph_dir, rotate_deg);
   }
#endif
}

static void
handle_s_Stats(packet)
    struct stats_s_spacket *packet;
{
   register struct player *pl;

#ifdef CORRUPTED_PACKETS
   if (packet->pnum < 0 || packet->pnum >= MAXPLAYER) {
      fprintf(stderr, "handleStats: bad index %d\n", packet->pnum);
      return;
   }
#endif

   pl = &players[packet->pnum];

   if (list_needs_stats)
      updatePlayer[(int) packet->pnum] = 1;

   pl->p_stats.st_tkills = ntohs(packet->tkills);
   pl->p_stats.st_tlosses = ntohs(packet->tlosses);
   pl->p_stats.st_kills = ntohs(packet->kills);
   pl->p_stats.st_losses = ntohs(packet->losses);
   pl->p_stats.st_tticks = ntohl(packet->tticks);
   pl->p_stats.st_tplanets = ntohs(packet->tplanets);
   pl->p_stats.st_tarmsbomb = ntohl(packet->tarmies);
   pl->p_stats.st_sbkills = ntohs(packet->sbkills);
   pl->p_stats.st_sblosses = ntohs(packet->sblosses);
   pl->p_stats.st_armsbomb = ntohs(packet->armies);
   pl->p_stats.st_planets = ntohs(packet->planets);
   pl->p_stats.st_maxkills = ntohl(packet->maxkills) / 100.0;
   pl->p_stats.st_sbmaxkills = ntohl(packet->sbmaxkills) / 100.0;

}

void    
handleShipCap(packet)

   struct ship_cap_spacket *packet;
{
   unsigned short stype;

   stype = ntohs(packet->s_type);
   shipvals[stype].s_torpspeed = ntohs(packet->s_torpspeed);
   shipvals[stype].s_maxshield = ntohl(packet->s_maxshield);
   shipvals[stype].s_maxdamage = ntohl(packet->s_maxdamage);
   shipvals[stype].s_maxegntemp = ntohl(packet->s_maxegntemp);
   shipvals[stype].s_maxwpntemp = ntohl(packet->s_maxwpntemp);
   shipvals[stype].s_maxarmies = ntohs(packet->s_maxarmies);
   shipvals[stype].s_maxfuel = ntohl(packet->s_maxfuel);
   shipvals[stype].s_maxspeed = ntohl(packet->s_maxspeed);
   shipvals[stype].s_width = ntohs(packet->s_width);
   shipvals[stype].s_height = ntohs(packet->s_height);
   shipvals[stype].s_phaserdamage = ntohs(packet->s_phaserrange);
   getship(myship, myship->s_type);
}

char *
_status_s(t)

   int	t;
{
   static char	buf[10];
   switch(t){
      case PDEAD: return "PDEAD";
      case PEXPLODE: return "PEXPLODE";
      case PALIVE: return "PALIVE";
      case PFREE: return "PFREE";
      case POUTFIT: return "POUTFIT";
      default: sprintf(buf, "%d", t); return buf;
   }
}

static void
new_flags(data, which)
    unsigned int    data;
    int             which;
{
   register int    pnum, status;
   register unsigned int new, tmp;
   unsigned int    oldflags;
   struct player  *j;
   tmp = data;

   for (pnum = which * 16; pnum < (which + 1) * 16 && pnum < MAXPLAYER; 
       pnum++) {
      new = tmp & 0x03;
      tmp >>= 2;
      j = &players[pnum];

      if(!j->p_status) continue;

      oldflags = j->p_flags;
      switch (new) {
      case 0:			/* PDEAD/PEXPLODE */
	 status = PEXPLODE;
	 j->p_flags &= ~PFCLOAK;
	 break;
      case 1:			/* PALIVE & PFCLOAK */
	 status = PALIVE;
	 j->p_flags |= PFCLOAK;
	 break;
      case 2:			/* PALIVE & PFSHIELD */
	 status = PALIVE;
	 j->p_flags |= PFSHIELD;
	 j->p_flags &= ~PFCLOAK;
	 break;
      case 3:			/* PALIVE & NO shields */
	 status = PALIVE;
	 j->p_flags &= ~(PFSHIELD | PFCLOAK);
	 break;
      default:
	 status = 0;
	 break;
      }
      if (oldflags != j->p_flags)
	 redrawPlayer[pnum] = 1;

      if (j->p_status == status)
	 continue;
#if 0
printf("status before %s, after %s\n", 
   _status_s(j->p_status), _status_s(status));
#endif

      if (status == PEXPLODE) {
	 if (j->p_status == PALIVE) {
	    j->p_explode = 0;
	    j->p_status = status;
	 } else {		/* Do nothing */
	    redrawPlayer[pnum] = updatePlayer[pnum] = 1;
	    continue;
	 }
      } else {			/* really PALIVE ? */
	 if (pnum == me->p_no) {
	    /* Wait for POUTFIT */
	    if (j->p_status == POUTFIT || j->p_status == PFREE) {
#ifdef DROP_FIX
	       /* guarantees we won't miss the kill message since this is TCP */
	       if (drop_fix) {
		  switch (status) {
		  case PALIVE:
		     switch (j->p_status) {
			/* this happens when we enter the game */
		     case PFREE:
			break;
		     default:
			/* entering the game. */
			j->p_kills = 0.;
			break;
		     }
		     break;

		  case POUTFIT:
		     /* just in case */
		     j->p_x = -10000;
		     j->p_y = -10000;

		     break;
		  }
	       }
#endif
	       j->p_status = PALIVE;
	    }
	 } else {
#ifdef DROP_FIX
	    /* guarantees we won't miss the kill message since this is TCP */
	    if (drop_fix) {
	       switch (status) {
	       case PALIVE:
		  switch (j->p_status) {
		     /* this happens when we enter the game */
		  case PFREE:
		     break;
		  default:
		     /* entering the game. */
		     j->p_kills = 0.;
		     break;
		  }
		  break;

	       case POUTFIT:
		  /* just in case */
		  j->p_x = -10000;
		  j->p_y = -10000;

		  break;
	       }
	    }
#endif
	    j->p_status = status;
	 }
      }
      redrawPlayer[pnum] = updatePlayer[pnum] = 1;
   }				/* for */
}

#endif

static int sock_read(sock, buff, size)
     int sock, size;
     char *buff;
{

#ifdef RECORD
  if (playback)
    return readPlayback(recordFile, buff, size);
#endif

  return read(sock, buff, size);
}


#ifdef PACKET_LOG
static int      Max_CPS = 0;
static int      Max_CPSout = 0;
static time_t   Start_Time = 0;
static double   s2 = 0;
static int      sumpl = 0;
static int      numpl = 0;
static int      outdata_this_sec = 0;
static double   sout2 = 0;
static int      sumout = 0;
/* HW clumsy but who cares ... :-) */
static int      vari_sizes[NUM_PACKETS];
static int      cp_msg_size;	/* For CP_S_MESSAGE */

void
Log_Packet(type, act_size)
    char            type;
    int             act_size;	/* HW */
{
   static time_t   lasttime;
   static int      data_this_sec;
   time_t          this_sec;
   if (log_packets == 0)
      return;

   if (type <= 0 && type > NUM_PACKETS) {
      fprintf(stderr, "Attempted to log a bad packet? \n");
      return;
   }
   packet_log[(int) type]++;
   /* data_this_sec += handlers[(int)type].size; */
   data_this_sec += act_size;	/* HW */
   ALL_BYTES += act_size;	/* To get all bytes */
   if (handlers[(int) type].size == -1) {	/* vari packet */
      vari_sizes[(int) type] += act_size;
   }
   this_sec = time(NULL);
   if (this_sec != lasttime) {
      lasttime = this_sec;
      if (log_packets > 1) {
	 fprintf(stdout, "%d %d %d\n", (int) (this_sec - Start_Time), data_this_sec, outdata_this_sec);
      }
      if (Start_Time == 0) {
	 Start_Time = this_sec;
      }
      /*
       * ignore baudage on the first few seconds of reception -- * that's
       * when we get crushed by the motd being sent
       */
      if (lasttime > Start_Time + 10) {
	 if (data_this_sec > Max_CPS)
	    Max_CPS = data_this_sec;
	 if (outdata_this_sec > Max_CPSout)
	    Max_CPSout = outdata_this_sec;
	 sumpl += data_this_sec;
	 s2 += (data_this_sec * data_this_sec);
	 sout2 += outdata_this_sec * outdata_this_sec;
	 sumout += outdata_this_sec;
	 numpl++;
      }
      data_this_sec = 0;
      outdata_this_sec = 0;
   }
}

void
Log_OPacket(tpe, size)
    int             tpe;
    int             size;
{
   /* Log Packet will handle the per second resets of this */
   if (log_packets == 0)
      return;
   outpacket_log[tpe]++;
   outdata_this_sec += size;
#ifdef SHORT_PACKETS
   if (tpe == CP_S_MESSAGE)
      cp_msg_size += size;	/* HW */
#endif
}


/* print out out the cool information on packet logging */
void
Dump_Packet_Log_Info()
{
   int             i;
   time_t          Now;
   int             total_bytes = 0;
   int             outtotal_bytes = 0;
   int             calc_temp;
   Now = time(NULL);

   printf("Packet Logging Summary:\n");
   printf("Start time: %s ", ctime(&Start_Time));
   printf(" End time: %s Elapsed play time: %3.2f min\n",
	  ctime(&Now), (float) ((Now - Start_Time) / 60));
   printf("Maximum CPS in during normal play: %d bytes per sec\n", Max_CPS);
   printf("Standard deviation in: %d\n",
	  (int) sqrt((numpl * s2 - sumpl * sumpl) / (numpl * (numpl - 1))));
   printf("Maximum CPS out during normal play: %d bytes per sec\n", Max_CPSout);
   printf("Standard deviation out: %d\n",
     (int) sqrt((numpl * sout2 - sumout * sumout) / (numpl * (numpl - 1))));

#ifdef SHORT_PACKETS
   /* total_bytes = ALL_BYTES; *//* Hope this works  HW */
   for (i = 0; i <= NUM_PACKETS; i++) {	/* I think it must be <= */
      if (handlers[i].size != -1)
	 total_bytes += handlers[i].size * packet_log[i];
      else
	 total_bytes += vari_sizes[i];
   }				/* The result should be == ALL_BYTES HW */
#else
   for (i = 0; i <= NUM_PACKETS; i++) {
      total_bytes += handlers[i].size * packet_log[i];
   }
#endif
   for (i = 0; i <= NUM_SIZES; i++) {
#ifdef SHORT_PACKETS
      if (handlers[i].size != -1)
	 outtotal_bytes += outpacket_log[i] * sizes[i];
      else
	 outtotal_bytes += cp_msg_size;	/* HW */
#else
      outtotal_bytes += outpacket_log[i] * sizes[i];
#endif
   }

   printf("Total bytes received %d, average CPS: %4.1f\n",
	  total_bytes, (float) (total_bytes / (Now - Start_Time)));
   printf("Server Packets Summary:\n");
   printf("Num #Rcvd    Size   TotlBytes   %%Total\n");
   for (i = 0; i <= NUM_PACKETS; i++) {
#ifdef SHORT_PACKETS
      if (handlers[i].size != -1)
	 calc_temp = handlers[i].size * packet_log[i];
      else
	 calc_temp = vari_sizes[i];

      printf("%3d %5d    %4d   %9d   %3.2f\n",
	     i, packet_log[i], handlers[i].size, calc_temp,
	     (float) (calc_temp * 100 / total_bytes));
#else
      calc_temp = handlers[i].size * packet_log[i];
      printf("%3d %5d    %4d   %9d   %3.2f\n",
	     i, packet_log[i], handlers[i].size, calc_temp,
	     (float) (calc_temp * 100 / total_bytes));
#endif
   }
   printf("Total bytes sent %d, average CPS: %4.1f\n",
	  outtotal_bytes, (float) (outtotal_bytes / (Now - Start_Time)));
   printf("Client Packets Summary:\n");
   printf("Num #Sent    Size   TotlBytes   %%Total\n");
   for (i = 0; i <= NUM_SIZES; i++) {
#ifdef SHORT_PACKETS
      if (sizes[i] == -1)
	 calc_temp = cp_msg_size;
      else
	 calc_temp = sizes[i] * outpacket_log[i];
      printf("%3d %5d    %4d   %9d   %3.2f\n",
	     i, outpacket_log[i], sizes[i], calc_temp,
	     (float) (calc_temp * 100 / outtotal_bytes));
   }
#else
      calc_temp = sizes[i] * outpacket_log[i];
      printf("%3d %5d    %4d   %9d   %3.2f\n",
	     i, outpacket_log[i], sizes[i], calc_temp,
	     (float) (calc_temp * 100 / outtotal_bytes));
  }
#endif
}
#endif


#if 0
monpoprate(plan, packet)
    struct planet  *plan;
    struct planet_spacket *packet;
{
   static int      start, new_armies, tick;
   int             s;

   if (!start) {
      start = time(NULL);
   }
   if (((tick++) % 10) == 0) {
      s = time(NULL) - start;
      if (s) {
	 printf("rate after %d minutes: %d armies per hour.\n",
		s / 60,
		(3600 * new_armies) / s);
      }
   }
   if (!(packet->info & me->p_team))
      return;
   /* ignore bombing, beamup */
   if (packet->armies < plan->pl_armies)
      return;

   /* FAILURE, beam down on owned planet */
   new_armies += ntohl(packet->armies) - plan->pl_armies;


#endif

