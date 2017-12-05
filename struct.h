#ifndef _struct_h_
#define _struct_h_
/*
 * struct.h for the client of an xtrek socket protocol.
 * 
 * Most of the unneeded stuff in the structures has been thrown away.
 * 
 * $Log: struct.h,v $
 * Revision 1.2  2000/02/17 05:48:05  ahn
 * BRMH 2.3 from David Pinkney <dpinkney@cs.uml.edu>
 *
 * Revision 1.6  1993/10/05  16:40:38  hadley
 * checkin
 *
 */
#include "copyright.h"

struct status {
   unsigned char   tourn;	/* Tournament mode? */
   /* These stats only updated during tournament mode */
   unsigned int    armsbomb, planets, kills, losses, time;
   /* Use long for this, so it never wraps */
   unsigned long   timeprod;
};

#define PFREE 0
#define POUTFIT 1
#define PALIVE 2
#define PEXPLODE 3
#define PDEAD 4

#define PFSHIELD	0x00000001
#define PFREPAIR	0x00000002
#define PFBOMB		0x00000004
#define PFORBIT		0x00000008
#define PFCLOAK		0x00000010
#define PFWEP		0x00000020
#define PFENG		0x00000040
#define PFROBOT		0x00000080
#define PFBEAMUP	0x00000100
#define PFBEAMDOWN	0x00000200
#define PFSELFDEST	0x00000400
#define PFGREEN		0x00000800
#define PFYELLOW	0x00001000
#define PFRED		0x00002000
#define PFPLOCK		0x00004000	/* Locked on a player */
#define PFPLLOCK	0x00008000	/* Locked on a planet */
#define PFCOPILOT	0x00010000	/* Allow copilots */
#define PFWAR		0x00020000	/* computer reprogramming for war */
#define PFPRACTR	0x00040000	/* practice type robot (no kills) */
#define PFDOCK          0x00080000	/* true if docked to a starbase */
#define PFREFIT         0x00100000	/* true if about to refit */
#define PFREFITTING	0x00200000	/* true if currently refitting */
#define PFTRACT  	0x00400000	/* tractor beam activated */
#define PFPRESS  	0x00800000	/* pressor beam activated */
#define PFDOCKOK	0x01000000	/* docking permission */

#define PFOBSERV        0x8000000       /* INL observer */

#define KQUIT		0x01	/* Player quit */
#define KTORP		0x02	/* killed by torp */
#define KPHASER		0x03	/* killed by phaser */
#define KPLANET		0x04	/* killed by planet */
#define KSHIP		0x05	/* killed by other ship */
#define KDAEMON		0x06	/* killed by dying daemon */
#define KWINNER		0x07	/* killed by a winner */
#define KGHOST		0x08	/* killed because a ghost */
#define KGENOCIDE	0x09	/* killed by genocide */
#define KPROVIDENCE	0x0a	/* killed by a hacker */
#define KPLASMA         0x0b	/* killed by a plasma torpedo */
#define TOURNEND	0x0c	/* tournament game ended */
#define KOVER		0x0d	/* game over  */
#define TOURNSTART	0x0e	/* tournament game starting */
#define KBINARY	        0x0f	/* bad binary */

#ifdef GALAXY
#define NUM_TYPES 8
#else
#define NUM_TYPES 7
#endif				/* GALAXY */
#define SCOUT 0
#define DESTROYER 1
#define CRUISER 2
#define BATTLESHIP 3
#define ASSAULT 4
#define STARBASE 5
#ifdef GALAXY
#define SGALAXY	6
#define ATT	7
#else
#define ATT	6
#endif				/* SGALAXY */

struct ship {
   short           s_phaserdamage;
   int             s_maxspeed;
   int             s_maxfuel;
   int             s_maxshield;
   int             s_maxdamage;
   int             s_maxegntemp;
   int             s_maxwpntemp;
   short           s_maxarmies;
   short           s_width;
   short           s_height;
   short           s_type;
   int             s_torpspeed;
#ifdef DROP_FIX
   int		   s_phaserfuse;
#endif
};

struct stats {
   double          st_maxkills;	/* max kills ever */
   int             st_kills;	/* how many kills */
   int             st_losses;	/* times killed */
   int             st_armsbomb;	/* armies bombed */
   int             st_planets;	/* planets conquered */
#ifdef unused
   int             st_ticks;	/* Ticks I've been in game */
#endif
   int             st_tkills;	/* Kills in tournament play */
   int             st_tlosses;	/* Losses in tournament play */
   int             st_tarmsbomb;/* Tournament armies bombed */
   int             st_tplanets;	/* Tournament planets conquered */
   int             st_tticks;	/* Tournament ticks */
   /* SB stats are entirely separate */
   int             st_sbkills;	/* Kills as starbase */
   int             st_sblosses;	/* Losses as starbase */
#ifdef unused
   int             st_sbticks;	/* Time as starbase */
#endif
   double          st_sbmaxkills;	/* Max kills as starbase */
   long            st_lastlogin;/* Last time this player was played */
   int             st_flags;	/* Misc option flags */
#ifdef nodef
   char            st_keymap[MAXKEY];	/* keymap for this player */
#endif
   int             st_rank;	/* Ranking of the player */
};

#define ST_MAPMODE      1
#define ST_NAMEMODE     2
#define ST_SHOWSHIELDS  4
#define ST_KEEPPEACE    8
#define ST_SHOWLOCAL    16	/* two bits for these two */
#define ST_SHOWGLOBAL   64

struct player {
   int             p_no;
#ifdef unused
   int             p_updates;	/* Number of updates ship has survived */
#endif
   int             p_status;	/* Player status */
   unsigned int    p_flags;	/* Player flags */
   char            p_name[16];
   char            p_login[16];
   char            p_monitor[16];	/* Monitor being played on */
   char            p_mapchars[2];	/* Cache for map window image */
   struct ship     p_ship;	/* Personal ship statistics */
   int             p_x;
   int             p_y;
   unsigned char   p_dir;	/* Real direction */
#ifdef unused
   unsigned char   p_desdir;	/* desired direction */
   int             p_subdir;	/* fraction direction change */
#endif
   int             p_speed;	/* Real speed */
#ifdef unused
   short           p_desspeed;	/* Desired speed */
   int             p_subspeed;	/* Fractional speed */
#endif
   short           p_team;	/* Team I'm on */
   int             p_damage;	/* Current damage */
#ifdef unused
   int             p_subdamage;	/* Fractional damage repair */
#endif
   int             p_shield;	/* Current shield power */
#ifdef unused
   int             p_subshield;	/* Fractional shield recharge */
#endif
   short           p_cloakphase;/* Drawing stage of cloaking
				 * engage/disengage. */
   short           p_ntorp;	/* Number of torps flying */
   short           p_nplasmatorp;	/* Number of plasma torps active */
   char            p_hostile;	/* Who my torps will hurt */
   char            p_swar;	/* Who am I at sticky war with */
   float           p_kills;	/* Enemies killed */
   short           p_planet;	/* Planet orbiting or locked onto */
   short           p_playerl;	/* Player locked onto */
#ifdef ARMY_SLIDER
   int             p_armies;	/* XXX: for stats */
#else
   short           p_armies;
#endif				/* ARMY_SLIDER */
   int             p_fuel;
   short           p_explode;	/* Keeps track of final explosion */
   int             p_etemp;
#ifdef unused
   short           p_etime;
#endif
   int             p_wtemp;
#ifdef unused
   short           p_wtime;
#endif
   short           p_whydead;	/* Tells you why you died */
   short           p_whodead;	/* Tells you who killed you */
   struct stats    p_stats;	/* player statistics */
#ifdef unused
   short           p_genoplanets;	/* planets taken since last genocide */
   short           p_genoarmsbomb;	/* armies bombed since last genocide */
   short           p_planets;	/* planets taken this game */
   short           p_armsbomb;	/* armies bombed this game */
   int             p_ghostbuster;
   int             p_docked;	/* If starbase, # docked to, else pno base
				 * host */
   int             p_port[4];	/* If starbase, pno of ship docked to that
				 * port, else p_port[0] = port # docked to on
				 * host.   */
#endif
   short           p_tractor;	/* What player is in tractor lock */
#ifdef unused
   int             p_pos;	/* My position in the player file */
#endif

#ifdef DROP_FIX
   int		   p_lastupdate;	/* last time position updated in
					   udcount */
#endif
   
#ifdef HOCKEY
   char            p_ispuck;
#endif
};

struct statentry {
   char            name[16], password[16];
   struct stats    stats;
};

/* Torpedo states */

#define TFREE 0
#define TMOVE 1
#define TEXPLODE 2
#define TDET 3
#define TOFF 4
#define TSTRAIGHT 5		/* Non-wobbling torp */

struct torp {
   int             t_no;
   int             t_status;	/* State information */
   int             t_owner;
   int             t_x;
   int             t_y;
   unsigned char   t_dir;	/* direction */
#ifdef BD
   short           t_turns;	/* rate of change of direction if tracking */
#endif
#ifdef unused
   int             t_damage;	/* damage for direct hit */
   int             t_speed;	/* Moving speed */
#endif
   int             t_fuse;	/* Life left in current state */
   char            t_war;	/* enemies */
#ifdef unused
   char            t_team;	/* launching team */
   char            t_whodet;	/* who detonated... */
#endif
#ifdef DROP_FIX
   int		   t_lastupdate;
#endif
};

/* Plasma Torpedo states */

#define PTFREE 0
#define PTMOVE 1
#define PTEXPLODE 2
#define PTDET 3

struct plasmatorp {
   int             pt_no;
   int             pt_status;	/* State information */
   int             pt_owner;
   int             pt_x;
   int             pt_y;
#ifdef unused
   unsigned char   pt_dir;	/* direction */
   short           pt_turns;	/* ticks turned per cycle */
   int             pt_damage;	/* damage for direct hit */
   int             pt_speed;	/* Moving speed */
#endif
   int             pt_fuse;	/* Life left in current state */
   char            pt_war;	/* enemies */
   char            pt_team;	/* launching team */
#ifdef DROP_FIX
   int		   pt_lastupdate;
   int		   pt_last_info_update;
#endif
};

#define PHFREE 0x00
#define PHHIT  0x01		/* When it hits a person */
#define PHMISS 0x02
#define PHHIT2 0x04		/* When it hits a photon */

struct phaser {
   int             ph_status;	/* What it's up to */
   unsigned char   ph_dir;	/* direction */
   int             ph_target;	/* Who's being hit (for drawing) */
   int             ph_x, ph_y;	/* For when it hits a torp */
   int             ph_fuse;	/* Life left for drawing */
   int		   ph_maxfuse;	/* max fuse, normalized for updates per 
				   second */
#ifdef unused
   int             ph_damage;	/* Damage inflicted on victim */
#endif
#ifdef DROP_FIX
   int		   ph_lastupdate;
#endif
};


#ifdef RSA
struct rsa_key {
   unsigned char   client_type[KEY_SIZE];
   unsigned char   architecture[KEY_SIZE];
   unsigned char global[KEY_SIZE];
   unsigned char   public[KEY_SIZE];
};
#endif

/*
 * An important note concerning planets:  The game assumes that the planets
 * are in a 'known' order.  Ten planets per team, the first being the home
 * planet.
 */

/* the lower bits represent the original owning team */
#define PLREPAIR 0x010
#define PLFUEL 0x020
#define PLAGRI 0x040
#define PLREDRAW 0x080		/* Player close for redraw */
#define PLHOME 0x100		/* home planet for a given team */
#define PLCOUP 0x200		/* Coup has occured */
#define PLCHEAP 0x400		/* Planet was taken from undefended team */
/* NEW */
#define PLCLEAR 0x800		/* should the planet area be erased before
				   redraw */

struct planet {
   int             pl_no;
   int             pl_flags;	/* State information */
   int             pl_owner;
   int             pl_x;
   int             pl_y;
   char            pl_name[16];
   int             pl_namelen;	/* Cuts back on strlen's */
   int             pl_armies;
   int             pl_info;	/* Teams which have info on planets */
#ifdef unused
   int             pl_deadtime;	/* Time before planet will support life */
   int             pl_couptime;	/* Time before coup may take place */
#endif
};

#define MVALID 0x01
#define MGOD   0x10
#define MMOO   0x12

/* order flags by importance (0x100 - 0x400) */
/* restructuring of message flags to squeeze them all into 1 byte - jmn */
/* hopefully quasi-back-compatible:
   MVALID, MINDIV, MTEAM, MALL, MGOD use up 5 bits. this leaves us 3 bits.
   since the server only checks for those flags when deciding message
   related things and since each of the above cases only has 1 flag on at
   a time we can overlap the meanings of the flags */

#define MINDIV 0x02
/* these go with MINDIV flag */
#ifdef STDBG
#define MDBG   0x20
#endif
#define MCONFIG    0x40
#define MDIST	0x60

#ifdef MULTILINE_MACROS
#define MMACRO 0x80
#endif

#define MTEAM  0x04
/* these go with MTEAM flag */
#define MTAKE  0x20
#define MDEST  0x40
#define MBOMB  0x60
#define MCOUP1 0x80
#define MCOUP2 0xA0

#define MALL   0x08
/* these go with MALL flag */
#define MGENO  0x20     /* MGENO is not used in INL server but belongs here */
#define MCONQ  0x20     /* not enought bits to distinguish MCONQ/MGENO :-(*/
#define MKILLA 0x40
#define MKILLP 0x60
#define MKILL  0x80
#define MLEAVE 0xA0
#define MJOIN  0xC0
#define MGHOST 0xE0
/* MMASK not used in INL server */

#define MWHOMSK  0x1f   /* mask with this to find who msg to */
#define MWHATMSK 0xe0   /* mask with this to find what message about */

struct message {
#ifdef unused
   int             m_no;
#endif
   int             m_flags;
#ifdef unused
   int             m_time;
#endif
   int             m_recpt;
   char            m_data[80];
};

/* message control structure */

struct mctl {
   int             mc_current;
};

/* This is a structure used for objects returned by mouse pointing */

#define PLANETTYPE 0x1
#define PLAYERTYPE 0x2

struct obtype {
   int             o_type;
   int             o_num;
};

struct rank {
   float           hours, ratings, defense;
   char           *name;
};

struct memory {
   struct player   players[MAXPLAYER];
   struct torp     torps[MAXPLAYER * MAXTORP];
   struct plasmatorp plasmatorps[MAXPLAYER * MAXPLASMA];
   struct status   status[1];
   struct planet   planets[MAXPLANETS];
   struct phaser   phasers[MAXPLAYER];
   struct mctl     mctl[1];
   struct message  messages[MAXMESSAGE];
};

struct plupdate {

   int             plu_update;
   int             plu_x, plu_y;
};

struct macro_list {
   int		   type;
   unsigned
   char            key;
   char            who;
   char           *string;
};

struct distress_list {		/* need one for ships and one for SBs */
   int             min_shld, max_shld;
   int             min_dam, max_dam;
   int             min_wtmp, max_wtmp;
   int             min_etmp, max_etmp;
   int             min_arms, max_arms;
   int             min_fuel, max_fuel;
   char           *low_shld;
   char           *mid_shld;
   char           *high_shld;
   char           *low_dam;
   char           *mid_dam;
   char           *high_dam;
   char           *low_wtmp;
   char           *mid_wtmp;
   char           *high_wtmp;
   char           *low_etmp;
   char           *mid_etmp;
   char           *high_etmp;
   char           *low_arms;
   char           *mid_arms;
   char           *high_arms;
   char           *low_fuel;
   char           *mid_fuel;
   char           *high_fuel;
   int             shld_on;
   int             dam_on;
   int             wtmp_on;
   int             etmp_on;
   int             arms_on;
   int             fuel_on;
};
#endif

/* MOTD structures */
struct piclist {
    int	    index;
    int     line;
    int	    color;
    W_Icon  thepic;
    int     x, y;
    int     width, height;
    struct piclist *next;
};
