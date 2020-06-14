/*
 * main.c
 */
#include "copyright.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
#include <pwd.h>
#ifdef hpux
#include <time.h>
#else	/* hpux */
#include <sys/time.h>
#endif				/* hpux */
#ifndef hpux
#include <sys/wait.h>
#endif				/* hpux */
#include <sys/file.h>
#include "netrek.h"
#include "version.h"

#ifdef RECORD
#include "recorder.h"
#endif

jmp_buf         env;

void	gb();


#ifdef GATEWAY
/*-----------------------------------------------------------*/

/* we want these for the client subnet validation */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

extern char *getenv ();
char *home, homedot[256];

#define DEFAULT_GATEWAY         "rebel"

unsigned long mkaddr ();

int             	serv_port;	/* used for blessedness checking */

static char *gateway = DEFAULT_GATEWAY;

#ifdef TREKHOPD
static int      trekhopd_port = 6592;
int             use_trekhopd = 0;
int             port_req = 6592;
char           *host_req = "rebel";
#endif

typedef struct {
   char            id[16];
   char            inet_addr[24];
   int             remote_port;
   int             gw_port;
   char            full_name[64];
   char            comment[40];
}               SERVER_LIST;
#define MAX_SERVER      128
static SERVER_LIST servers[MAX_SERVER];	/* that ought to be enough */
#define SERVER_DIR      "/usr/local/games/"
#define SERVER_FILE     ".trekgwrc"
static int      server_count = 0;
#define WSPC    " \t"


unsigned long
strToNetaddr(str)
    char           *str;
{
   SERVER_LIST    *slp;
   char           *t;
   unsigned long   answer;
   int             i;

   if (!server_count) {
      fprintf(stderr, "netrek: No server list, cannot resolve id\n");
      exit(1);
   }
   /* find the one we want */
   for (i = 0, slp = servers; i < server_count; i++, slp++) {
      if (!strcmp(str, slp->id) || !strcmp(str, slp->full_name)) {
	 printf("%s is %s(%d) (%s)\n", slp->id, slp->full_name,
		slp->remote_port, slp->comment);
	 xtrekPort = slp->gw_port;
	 str = slp->inet_addr;
	 break;
      }
   }
   if (i == server_count) {
      fprintf(stderr, "netrek: Specified server not found.\n");
      exit(1);
   }
   /* now "str" is either the original string or slp->inet_addr */
   /* (this will be wrong if -H isn't last on command line) */
   answer = 0;
   t = str;
   for (i = 0; i < 4; i++) {
      answer = (answer << 8) | atoi(t);
      while (*t && *t != '.')
	 t++;
      if (*t)
	 t++;
   }
#ifdef TREKHOPD
   /* do things slightly different */
   if (slp->id == NULL) {
      fprintf(stderr, "ERROR: host ID '%s' unknown\n", str);
      exit(1);
   }
   xtrekPort = trekhopd_port;	/* ought to have an arg to specify this */
   port_req = slp->remote_port;
   host_req = slp->full_name;
   printf("Connecting to %s (%d) via trekhopd (%s %d)\n", host_req,
	  port_req, serverName, xtrekPort);
#else
   printf("Connecting to %s through %s port %d\n", str, serverName, xtrekPort);
#endif
   return (answer);
}


/* for trekhopd, only gw_local_port is important */
/* (should be possible to eliminate that too, but I want minimal changes) */
typedef struct {
   int             uid;
   int             serv_port;
   int             port;
   int             local_port;
}               UDPMAP;
#define MAX_UDPMAP      32
static UDPMAP   udpmap[MAX_UDPMAP];
static int      map_count;

void
getUdpPort()
{
   int             i;
   uid_t           uid;

   uid = getuid();

   gw_mach = gateway;
   for (i = 0; i < map_count; i++) {
      if (uid == udpmap[i].uid) {
	 gw_serv_port = udpmap[i].serv_port;
	 gw_port = udpmap[i].port;
	 gw_local_port = udpmap[i].local_port;
	 return;
      }
   }

   /* not recognized; use default UDP port */
   gw_serv_port = 5000;
   gw_port = 5001;
   gw_local_port = 5000;
}

/*
 * More network "correct" routine
 */

unsigned long
mkaddr (m)

     char *m;
{
  struct in_addr ad;
  struct hostent *hp;

  hp = gethostbyname (m);
  if (!hp)
    {
      ad.s_addr = inet_addr (m);
      if (ad.s_addr == -1)
        {
          fprintf (stderr, "netrek: unknown host \'%s\'\n", m);
          exit (1);
        }
    }
  else
    MCOPY(hp->h_addr, (char *) &ad, hp->h_length);

  return ad.s_addr;
}


/*
 * This is not very robust.
 */
void
read_servers()
{
  FILE *fp;
  SERVER_LIST *slp;
  UDPMAP *ump;
  char buf[128];
  int state;
  char *cp;
  char	file[BUFSIZ];
  int	line;
  int	error = 0;

  server_count = map_count = 0;

  fp = NULL;
  if (getenv ("HOME") != NULL)
    {
      strcpy (homedot, getenv ("HOME"));
      strcat (homedot, "/");
      strcat (homedot, SERVER_FILE);
      fp = fopen (homedot, "r");
      strcpy(file, homedot);
    }
  if (fp == NULL)
    {
      /* failed, try common one */
      strcpy (buf, SERVER_DIR);
      strcat (buf, SERVER_FILE);
      fp = fopen (buf, "r");
      strcpy(file, buf);
    }
  if (fp == NULL)
    {
      /* failed, try current directory */
      fp = fopen (SERVER_FILE, "r");
      strcpy(file, SERVER_FILE);
    }
  if (fp == NULL)
    {
      /* failed, give up */
      perror ("warning: Unable to open server list");
      fprintf (stderr, "Tried to open '%s', '%s', and './%s'\n",
               homedot, buf, SERVER_FILE);
      return;
    }

   state = 0;
   line = 1;
   while (!error) {
      fgets(buf, 128, fp);
      if (ferror(fp) || feof(fp)) {
	 if (ferror(fp))
	    perror("fgets");
	 break;
      }
      line++;
      /* skip blank lines and lines which start with '#' */
      if (*buf == '\0' || *buf == '\n' || *buf == '#')
	 continue;
      buf[strlen(buf) - 1] = '\0';	/* strip the trailing '\n' */
      switch (state) {
      case 0:			/* "trekhopd" or "gw" */
#ifdef TREKHOPD
	 use_trekhopd = 0;
	 if (!strcmp(buf, "trekhopd"))
	    use_trekhopd = 1;
#endif
	 state++;
	 break;
      case 1:			/* gateway host */
	 gateway = (char *) malloc(strlen(buf) + 1);
	 strcpy(gateway, buf);
	 state++;
	 break;
      case 2:			/* trekhopd port */
	 trekhopd_port = atoi(buf);
	 state++;
	 break;
      case 3:			/* UDP map */
	 if (!strcmp(buf, "END")) {
	    state++;
	    break;
	 }
          if (map_count >= MAX_UDPMAP)
            {
              fprintf (stderr, "UDP map too large; max is %d entries\n",
                       MAX_UDPMAP);
              break;
            }

	 ump = &udpmap[map_count];
	 cp = strtok(buf, WSPC);/* skip ascii uid */
	 cp = strtok(NULL, WSPC);
	 if(!cp) {
	    error++;
	    fprintf(stderr, "%s:%d: syntax error on user id.\n",
	       file, line);
	    break;
	 }
	 ump->uid = atoi(cp);
	 cp = strtok(NULL, WSPC);
	 if(!cp) {
	    error++;
	    fprintf(stderr, "%s:%d: syntax error on server port.\n",
	       file, line);
	    break;
	 }
	 ump->serv_port = atoi(cp);
	 cp = strtok(NULL, WSPC);
	 if(!cp) {
	    error++;
	    fprintf(stderr, "%s:%d: syntax error on port.\n",
	       file, line);
	    break;
	 }
	 ump->port = atoi(cp);
	 cp = strtok(NULL, WSPC);
	 if(!cp) {
	    error++;
	    fprintf(stderr, "%s:%d: syntax error on local port.\n",
	       file, line);
	    break;
	 }
	 ump->local_port = atoi(cp);
#ifdef DEBUG
	 printf("%2d: %-8d %-8d %-8d %-8d\n", map_count,
		ump->uid, ump->serv_port, ump->port, ump->local_port);
#endif
	 map_count++;
	 break;

      case 4:			/* host description */
	 if (!strcmp(buf, "END")) {
	    state++;
	    break;
	 }
          if (server_count >= MAX_SERVER)
            {
              fprintf (stderr, "server list too large; max is %d entries\n",
                       MAX_SERVER);
              break;
            }

	 slp = &servers[server_count];
	 cp = strtok(buf, WSPC);
	 if(!cp) {
	    error++;
	    fprintf(stderr, "%s:%d: syntax error on id.\n",
	       file, line);
	    break;
	 }
	 strcpy(slp->id, cp);
	 cp = strtok(NULL, WSPC);
	 if(!cp) {
	    error++;
	    fprintf(stderr, "%s:%d: syntax error on address.\n",
	       file, line);
	    break;
	 }
	 strcpy(slp->inet_addr, cp);
	 cp = strtok(NULL, WSPC);
	 if(!cp) {
	    error++;
	    fprintf(stderr, "%s:%d: syntax error on remote port.\n",
	       file, line);
	    break;
	 }
	 slp->remote_port = atoi(cp);
	 cp = strtok(NULL, WSPC);
	 if(!cp) {
	    error++;
	    fprintf(stderr, "%s:%d: syntax error on gateway port.\n",
	       file, line);
	    break;
	 }
	 slp->gw_port = atoi(cp);
	 cp = strtok(NULL, WSPC);
	 if(!cp) {
	    error++;
	    fprintf(stderr, "%s:%d: syntax error on server name.\n",
	       file, line);
	    break;
	 }
	 strcpy(slp->full_name, cp);
	 cp = strtok(NULL, "\"\t");
	 if(!cp) {
	    error++;
	    fprintf(stderr, "%s:%d: syntax error on server comments.\n",
	       file, line);
	    break;
	 }
	 strcpy(slp->comment, cp);
#ifdef DEBUG
	 printf("%2d: %-9s %-15s %-5d %-5d %-25s \"%s\"\n", server_count,
		slp->id, slp->inet_addr, slp->remote_port, slp->gw_port,
		slp->full_name, slp->comment);
#endif
	 server_count++;
	 break;
      case 5:			/* all done! */
	 break;
      default:
	 fprintf(stderr, "Whoops!\n");
	 exit(2);
      }
   }

   fclose(fp);

   if(error)
      exit(1);
}

#endif				/* GATEWAY */

#ifdef EM
void            handle_segfault();
void            handle_exception();
#endif
void		get_core();	/* NEW */

#ifdef PACKET_LOG
extern int      log_packets;
#endif

#ifdef FOR_MORONS
extern int      For_Morons;
#endif

/* move these to data.c one of these days */
char           	*defaultsFile = NULL;
char		*pseudo_name = NULL,
		*pseudo_passwd = NULL;

void
main(argc, argv)
    int             argc;
    char          **argv;
{
   int             team = -1, s_type;
   int             usage = 0;
   int             err = 0;
   char           *name, *ptr, *cp;
   struct passwd  *pwent;
   int             passive = 0, disable_autologin = 0;
   extern char    *getenv();
#ifdef SPTEST
   int		   sptest=0;
#endif
   int		   noargs = 1;	/* no server or metaserver supplied */
#ifdef GATEWAY
   int 		   hset = 0;
#endif
   int		   whosplaying = 0;
#ifdef META
   int		   usemeta = 0;
   char		   *metafile = NULL;
#endif
   program = argv[0];

#ifdef GATEWAY
  /* restrict this client to certain machines */
  {
    struct sockaddr_in saddr;
    struct hostent *hp;
    char myname[64];
    long myaddr, myaddr_restricted, myaddr_mask;

    if (gethostname (myname, 64) < 0)
      {
        perror ("gethostname");
        exit (1);
      }
    if ((myaddr = inet_addr (myname)) == -1)
      {
        if ((hp = gethostbyname (myname)) == NULL)
          {
            fprintf (stderr, "unable to get addr for local host\n");
            exit (1);
          }
        myaddr = *(long *) hp->h_addr;
      }
    if ((myaddr_restricted = inet_addr(MYADDR)) == -1){
      fprintf(stderr, "Malformed dot address \"%s\"\n", MYADDR);
      exit(1);
    }
    if ((myaddr_mask = inet_addr(MYADDR_MASK)) == -1){
      fprintf(stderr, "Malformed dot address mask \"%s\"\n", MYADDR_MASK);
      exit(1);
    }

    /*printf("myname = '%s', myaddr = 0x%.8lx\n", myname, myaddr); */
    if ((myaddr & myaddr_mask) != myaddr_restricted)
      {
        fprintf (stderr, "Sorry, you may not run this client on this host\n");
        exit (1);
      }
  }
#endif

   defaultsFile = getenv("XTREKRC");

   name = *argv++;
   argc--;
   if ((ptr = strrchr(name, '/')) != NULL)
      name = ptr + 1;
#ifdef GATEWAY
   read_servers();
   netaddr = 0;
   serverName = gateway;
   serverNameRemote = NULL;
#endif
   while (*argv) {
      if (**argv == '-')
	 ++* argv;
      else
	 break;

      argc--;
      ptr = *argv++;
      while (*ptr) {
	 switch (*ptr) {
	 case 'v':
	    printVersion(name);
	    break;
	 case 'u':
	    usage++;
	    break;
	 case 's':
	    if (*argv) {
	       xtrekPort = atoi(*argv);
	       passive = 1;
	       argv++;
	       argc--;
	    }
	    break;
	 case 'p':
	    if (*argv) {
	       xtrekPort = atoi(*argv);
	       argv++;
	       argc--;
	    }
	    break;

#ifndef GATEWAY
	 case 'c':
	    whosplaying = 1;
	    break;
#endif
#ifdef META
	 case 'm':
	     usemeta = 1;
	     noargs = 0;
	     break;

	 case 'M':	/* note incompatibility with Moron mode */
	     if (*argv && *argv[0] != '-') {
		metafile = *argv;
		argv++;
		argc--;
	     }
	     break;
#endif

	 case 'C':
	    if (*argv && *argv[0] != '-') {
	       pseudo_name = *argv;
	       argv++;
	       argc--;
	    }
	    else
	       disable_autologin = 1;
	    break;
	 case 'A':
	    if (*argv){
	       pseudo_passwd = *argv;
	       argv++;
	       argc--;
	    }
	    break;
	 case 'd':
	    display = *argv;
	    argc--;
	    argv++;
	    break;
#ifdef FOR_MORONS
	 case 'M':
	    if (For_Morons) {
	       For_Morons = 0;
	    } else {
	       For_Morons = 1;
	    }
	    break;
#endif
	 case 'h':
#ifdef GATEWAY
	    gw_mach = *argv;
#endif
	    serverName = *argv;
	    argc--;
	    argv++;
	    noargs = 0;
	    break;
#ifdef GATEWAY
	 case 'H':
	    hset++;
	    serverNameRemote = *argv;
	    netaddr = strToNetaddr(*argv);
	    argc--;
	    argv++;
	    noargs = 0;
	    break;
#endif

	 case 't':		/* USED?? */
	    title = *argv;
	    argc--;
	    argv++;
	    break;
	 case 'r':
	    defaultsFile = *argv;
	    argv++;
	    argc--;
	    break;

	 case 'D':
	       debug++;
	       if(debug == 2) {
		 printf("udpDebug turned on\n");
		 udpDebug = 1;
	       }
	       break;
#ifdef PACKET_LOG
	 case 'P':
	    log_packets++;
	    break;
#endif
#ifdef RECORD
	 case 'R':
	    recordFileName = *argv;
	    argv++;
	    argc--;
	    recordGame = 1;
	    break;
	 case 'F':
	   recordFileName = *argv;
	   argv++;
	   argc--;
	   playback = 1;
	   break;
	 case 'Z':
	   paradise_compat = 1;
	   shortversion = OLDSHORTVERSION;
	   break;
#endif
#ifdef LOGMESG
	 case 'l':
	    logMess = 1;
	    logFileName = *argv;
	    argv++;
	    argc--;
	    break;
#endif
#ifdef SPTEST
	 case 'S':
	    spt_getwin(atoi(*argv));
	    argv++;
	    argc--;
	    break;
#endif
#ifdef FORKNETREK
	 case 'w':
	    if (*argv) {
	       waitnum = atoi(*argv);
	       argv++;
	       argc--;
	    }
	    break;
#endif

	 default:
	    fprintf(stderr, "%s: unknown option '%c'\n", name, *ptr);
	    err++;
	    break;
	 }
	 if(argc < 0){
	    printUsage(name);
	    exit(1);
	 }
	 ptr++;
      }
   }

#ifdef RECORD
   if(paradise_compat)
     fprintf(RECORDFD, "Paradise compabitiblity mode enabled (shortversion=%d).\n", shortversion);

   if(recordGame && playback) {
     fprintf(RECORDFD, 
	     "Record Game (-f) and Playback (-F) are mutually exclusive!\n");
     exit(1);
   }
#endif

#ifdef EXPIRATIONDATE
   {
      time_t          nowtime;
      nowtime = time(NULL);
      if (nowtime > EXPIRATIONDATE) {
	 printf("This program has reached its planned-obsolescence date!\n")
	    printf("Please try to get a newer version!\n");
	 exit(666);
      } else {
	 nowtime = EXPIRATIONDATE;
	 printf("This program will become non-functioonal after %s\n"
		ctime(&nowtime));
      }
   }
#endif

#ifdef LOGMESG
   /* open messages logfile */
   if (logMess){
      logFile = fopen(logFileName, "w+");
      if (logFile == NULL) {
	 perror(logFileName);
	 exit(1);
      }
   }
#endif

#ifdef EM
#ifndef NO_TRAP
   SIGNAL(SIGSEGV, handle_segfault);
   SIGNAL(SIGFPE, handle_exception);
#endif				/* DEBUG */
#endif				/* EM */
   SIGNAL(SIGPIPE, SIG_IGN);

#ifdef nodef
   SIGNAL(SIGINT, gb);	/* ghostbust debug test */
#endif

   if(initDefaults(defaultsFile) == 0){
      makeDefault();
      /* do it again */
      (void) initDefaults(defaultsFile);
   }

   if (usage || err || argc > 0) {
      printUsage(name);
      exit(1);
   }

#ifdef RECORD
   /* playback / record start used to be here */
#endif


#ifdef SHOW_DEFAULTS
   show_defaults("B-startup", "metaDefault", "off",
      "If no server argument, use metaserver.");
#endif

#ifdef RECORD
   if(playback) {
     noargs = 0;
     usemeta = 0;
     serverName = "playback";
     W_Initialize(display);	/* moved from newwin.c */
   } else
#endif
 {
   if(noargs){
     if(booleanDefault("metaDefault", 0))
       usemeta = 1;
     else if(!getdefault("server")){ /* NEW BEHAVIOR, no arguments, show special metaserver message */
       printMetaInfo(name);
       exit(1);
     }
   }

#ifdef GATEWAY
   if (!netaddr && !usemeta){
#ifdef TREKHOPD
      use_trekhopd = 0;       
#endif
      serverNameRemote = serverName;
      fprintf(stderr,
	 "netrek: no remote address set (-H).  Restricted server will not work.\n");
   }

#endif

#ifdef nodef
   /* compatability */
   if (argc > 0)
      display = argv[0];
#endif

   /* SUPPRESS 8 */
   srandom(getpid() * time((long *) 0));

#ifdef EM
#ifdef SHOW_DEFAULTS
   show_defaults("B-startup", "# <feature>[.<alias/server-name>]", "value",
      "All defaults variables support appending a server alias or server name\n\
after the variable name.  The value specified will be used only if you are \n\
connected to that server.  Values are searched by server-name, then by alias\n\
then by base variable name.");
#endif

   /*
    * server nicknames: * if you do netrek -h foo * and "server.foo:
    * etrek.netrek.com" exists in your netrekrc * the server that is called
    * is netrek.netrek.com * also, port.foo: x   will make the port called x
    */
#ifdef SHOW_DEFAULTS
      show_defaults("B-startup", "server.alias", "x.y.z",
	 "Aliases.  Replace x.y.z with full server name, and specify <alias>\n\
From then on you can specify that server by doing 'netrek -h <alias>'");
#endif

#ifdef SHOW_DEFAULTS
      show_defaults("B-startup", "server", "default.server.edu",
	 "Name of the default server to use if the -h flag isn't given.");
#endif

   if(!serverName){
      serverName = getdefault("server");
      if(!serverName)
	 serverName = DEFAULT_SERVER;
   }
#endif

   /* look for alias */
   {
      char		buf[100], *real_name, *server;
#ifdef GATEWAY
      if(hset)
	 server = serverNameRemote;
      else
	 server = serverName;
#else
      server = serverName;
#endif
      sprintf(buf, "server.%s", server);
      real_name = _getdefault(buf);
      if(real_name && strcmp(server, real_name) != 0){
	 serverAlias = server;
	 server = real_name;
#ifdef GATEWAY
	 serverNameRemote = server;
	 if(!hset)
	    serverName = server;
#else
	 serverName = server;
#endif
      }
      if(!serverAlias){
	 serverAlias = getalias(server);
      }
   }

   /* if defaults is re-read, these pointers would be freed, store them
      instead */
   if(serverName)
      serverName = strdup(serverName);
#ifdef GATEWAY
   if(serverNameRemote)
      serverNameRemote = strdup(serverNameRemote);
#endif
   if(serverAlias)
      serverAlias = strdup(serverAlias);

   if (xtrekPort < 0) {
#ifdef SHOW_DEFAULTS
      show_defaults("B-startup", "port", "2592",
	 "Number of the default port to use if the -p flag isn't given.");
#endif
      xtrekPort = intDefault("port", -1);
      if(xtrekPort < 0)
	 xtrekPort = DEFAULT_PORT;
   }

#ifndef GATEWAY
   if(whosplaying){
      check_whosplaying(serverName, xtrekPort);
      exit(0);
   }
#endif

   W_Initialize(display);	/* moved from newwin.c */

#ifdef META
   if(usemeta)
      /* either we return from this with a new serverName or we exit */
      do_metaserver(metafile);
#endif


#ifdef GATEWAY
   /* pick a nice set of UDP ports */
   getUdpPort();
#endif
 }  /* if not playback */


#ifdef FOR_MORONS
#ifdef SHOW_DEFAULTS
	 show_defaults("misc", "forMorons", For_Morons?"on":"off",
	    "EM");
#endif
   For_Morons = booleanDefault("ForMorons", For_Morons);
#endif

   newwin(display, name);	/* open memory...? Leftover from xtrek code! */

   /* Get login name */
   if ((cp = getdefault("username")) != 0)
   	(void) strncpy(login, cp, sizeof(login));
   else if ((pwent = getpwuid(getuid())) != NULL)
      (void) strncpy(login, pwent->pw_name, sizeof(login));
   else
      (void) strncpy(login, "****", sizeof(login));
   login[sizeof(login) - 1] = '\0';

   openmem();

#ifdef RECORD
   if(playback) {
#ifdef RECORD_DEBUG
     fprintf(RECORDFD, "Calling startPlayback()...recordGame=%d\n",
	     recordGame);
#endif
     startPlayback();
   }
   else
#endif
  {
#ifdef RECORD
    if(!playback) {
      /* Find out if we're going to record. If so, find out if the filename
	 if ok now... it might require input if they give us an existing 
	 filename and if we have to do this before we connect, otherwise
	 they might ghostbust (if they take too long)
	 */
      if(recordGame || (recordGame = booleanDefault("recordGame", recordGame)))
	{
	  if(!recordFileName)
	    recordFileName = getdefault("recordFile");
	  check_record_filename();
	}
    }
#endif
    if (!passive) {
       callServer(xtrekPort, serverName);
    } else {
       connectToServer(xtrekPort);
    }
  }
#ifdef HOCKEY
#ifdef SHOW_DEFAULTS
   show_defaults("hockey", "puckName", PUCK_NAME,
      "Puck name [used by the client along with puckHost to find the \n\
hockey puck].");
#endif
   if ((cp = getdefault("puckName")) != 0)
      (void) strncpy(puck_name, cp, sizeof(puck_name));
   puck_name[sizeof(puck_name) - 1] = '\0';
#ifdef SHOW_DEFAULTS
   show_defaults("hockey", "puckHost", PUCK_HOST,
      "Puck host [used by the client along with puckName to find the \n\
hockey puck].");
#endif
   if ((cp = getdefault("puckHost")) != 0)
      (void) strncpy(puck_host, cp, sizeof(puck_host));
   puck_host[sizeof(puck_host) - 1] = '\0';
#endif

#ifdef FEATURE
   sendFeature("FEATURE_PACKETS", 'S', 1, 0, 0);
   sendFeature("MOTD_BITMAPS", 'C', 1, 0, 0);
#ifdef RECORD   
   if(paradise_compat) {
     sendFeature("SHIP_CAP", 'S', 0, 0, 0);
#ifdef RECORD_DEBUG
     fprintf(RECORDFD, "Asking server for SHIP_CAP off\n");
#endif
   }
#endif
#endif

#ifdef SHOW_DEFAULTS
   show_defaults("display", "noObscureEntry", non_obscure ? "on" : "off",
   "blink wait queue window instead of opening netrek window when in game.");
#endif
   non_obscure = booleanDefault("noObscureEntry", non_obscure);

   findslot();

#ifdef RECORD_DEBUG
   if(playback)
     fprintf(RECORDFD, "findslot() finished\n");
#endif

   lastm = mctl->mc_current;	/* unused */

   mapAll();

#ifdef SHOW_DEFAULTS
   show_defaults("B-startup", "name", "guest",
      "The default player name shown at the login window.");
#endif
   if(pseudo_name){
#ifdef AUTOLOGIN
      autologin = 1;
#endif
      (void) strncpy(pseudo, pseudo_name, sizeof(pseudo));
      pseudo[sizeof(pseudo) - 1] = '\0';
   }
   else {
      if ((cp = getdefault("name")) != 0)
	 (void) strncpy(pseudo, cp, sizeof(pseudo));
      else
	 (void) strncpy(pseudo, login, sizeof(pseudo));
      pseudo[sizeof(pseudo) - 1] = '\0';
   }

#ifdef AUTOLOGIN
#ifdef SHOW_DEFAULTS
   show_defaults("B-startup", "autologin", "off",
      "Automatically log-in.  This field makes sense only if name\n\
and password are also set, but with server aliasing this can be selected\n\
on a server basis, i.e. autologin.bigbang: on");
   show_defaults("B-startup", "password", "",
      "Password for auto-login, i.e. password.bigbang: secret.");
#endif
   if(!disable_autologin)
      autologin = booleanDefault("autologin", autologin);

   if(pseudo_passwd){
      (void) strncpy(defpasswd, pseudo_passwd, sizeof(defpasswd));
      defpasswd[sizeof(defpasswd)-1] = 0;
   }
   else {
      if((cp = getdefault("password")) != 0)
	 (void) strncpy(defpasswd, cp, sizeof(defpasswd));
      else
	 *defpasswd = 0;
      defpasswd[sizeof(defpasswd)-1] = 0;
   }
#endif

#ifdef RECORD
   if(playback) {
     /* Read up to the loginAccept packets, so we are in proper sync */
     readFromServer(sock);
#ifdef RECORD_DEBUG
     fprintf(RECORDFD, "loginAccept scan completed\n");
#endif

     /* copied from getname */
     MZERO(mystats, sizeof(struct stats));
     mystats->st_tticks = 1;
     mystats->st_flags = ST_MAPMODE + ST_NAMEMODE + ST_SHOWSHIELDS +
       ST_KEEPPEACE + ST_SHOWLOCAL * 2 + ST_SHOWGLOBAL * 2;
   }
   else
#endif
   getname(pseudo);

   loggedIn = 1;

#ifdef RECORD_DEBUG
   fprintf(RECORDFD, "Should be logged in now.\n");
#endif


   /* moo does it all in readDefaults in getdefault.c */
   readDefaults();

#ifdef RECORD
   /* make sure recordGame in .xtrekrc doesn't override playback */
   if(playback)
     recordGame = 0;
   else if(recordFile) {
     /* Recorder has been activated, make sure defaults don't override
	command line */
     recordGame = 1;
   }
#endif

#ifdef RECORD_DEBUG
   fprintf(RECORDFD, "readDefaults() done\n");
#endif

   /*
    * Set p_hostile to hostile, so if keeppeace is on, the guy starts off
    * hating everyone (like a good fighter should)
    */
   me->p_hostile = (FED | ROM | KLI | ORI);

#ifdef unused
   me->p_planets = 0;
   me->p_genoplanets = 0;
   me->p_armsbomb = 0;
   me->p_genoarmsbomb = 0;
#endif
   /* Set up a reasonable default */
   me->p_whydead = KQUIT;
   me->p_team = ALLTEAM;
   s_type = defaultShip(CRUISER);

#ifdef PING
#ifdef SHOW_DEFAULTS
   show_defaults("C-startup", "dontPing", "off",
      "Don't attempt to start ping packets from the server.");
#endif
   no_ping = booleanDefault("DontPing", 0);
   /*
    * this will always be done once.  If the server doesn't support ping
    * packets it will ignore this request
    */
   if(!no_ping)
      startPing();
#endif

   setjmp(env);			/* Reentry point of game */

   if (messageon)
      message_off();            /* ATM */

#ifdef RECORD
   if(playback) {
     /* Read up to the pickOk packet, so we are in proper sync */
     if(!not_first_entry) {
       readFromServer(sock);
#ifdef RECORD_DEBUG
       fprintf(RECORDFD, "pickOk scan completed\n");
#endif
     }
   }
   else
#endif
 {
   /* give the player the motd and find out which team he wants */
   entrywindow(&team, &s_type);
   if (team == -1) {
      W_DestroyWindow(w);
      sendByeReq();
      sleep(1);
      printf("OK, bye!\n");
#ifdef PACKET_LOG
      if (log_packets)
	 Dump_Packet_Log_Info();
#endif
#ifdef SHOW_DEFAULTS
      finish_defaults();
#endif

#ifdef RECORD
      if(recordGame)
	stopRecorder();
#endif

      exit(0);
   }
 }

   sendVersion();

#ifdef RECORD_DEBUG
   fprintf(RECORDFD, "Calling getship, myship->s_type=%d\n",
	   myship->s_type);
#endif
   
   getship(myship, myship->s_type);

   /* set up ship-specific keymap */
   if(keymaps[myship->s_type])
      mykeymap = keymaps[myship->s_type];
   else
      mykeymap = keymaps[KEYMAP_DEFAULT];
      
   redrawall = 1;
   enter();
   calibrate_stats();

   W_ClearWindow(w);
   W_ClearWindow(mapw);

   /*
    * for (i = 0; i < NSIG; i++) { SIGNAL(i, SIG_IGN); }
    */

   me->p_status = PALIVE;	/* Put player in game */
#ifdef DROP_FIX
   if(drop_fix) me->p_kills = 0.;
#endif
   
   updatePlayer[me->p_no] = 1;

   if (showStats)		/* Default showstats are on. */
      W_MapWindow(statwin);

#ifdef NETSTAT
   if (W_IsMapped(lMeter))
      redrawLMeter();
#endif

#ifdef PING
   if (W_IsMapped(pStats))
      redrawPStats();
#endif


   if (commMode != COMM_UDP && tryUdp) {
      sendUdpReq(COMM_UDP);
   }
   if (udpWin) {
      /* update any fields affected by xtrekrc */
      udprefresh(UDP_SEQUENCE);
      udprefresh(UDP_DEBUG);
      udprefresh(UDP_SEND);
      udprefresh(UDP_RECV);
   }

#ifdef SHORT_PACKETS		/* should we be checking for udp on here? */
   if (tryShort) {
      sendShortReq(SPK_VON);
      tryShort = 0;		/* do it once only */
   }
#endif

   /* Get input until the player quits or dies */
#ifdef RECORD
   if(playback)
     playback_input();
   else
#endif
   input();
}

/* } */

void
printVersion(prog)

   char	*prog;
{
   fprintf(stderr, "BRMH version %s %s %s\n", VERSION
#ifdef FONT_BITMAPS
   , "(external font bitmaps)"
#else
   , ""
#endif
#ifdef GATEWAY
   , "(gateway code)"
#else
   , ""
#endif
   );
   
   fprintf(stderr, "%16s:\t%s\n", "Client Name", CLIENTNAME);
   fprintf(stderr, "%16s:\t%s\n", "Client Version", VERSION);
   fprintf(stderr, "%16s:\t%s\n", "Client OS", CLIENTOS);
   fprintf(stderr, "%16s:\t%s\n", "Client Maintainer", MAINTENANCE);
   fprintf(stderr, "%16s:\t%s\n", "Comments", CLIENTCOMMENTS);
}

void
printMetaInfo(prog)
   
   char		*prog;
{
   char		*s;

   fprintf(stderr, 
"Use \"%s -m\" to find out where netrek is being played.\n\n",
      prog);
   fprintf(stderr,
"The following definitions are used and can be modified in ~/.netrekrc:\n");

   fprintf(stderr,
"\nmetaserver: %s\nmetaport: %d\n",
   (s = getdefault("metaserver"))?s:metaserver,
   intDefault("metaport", metaport));

   fprintf(stderr, 
"\n(Add \"metaDefault: on\" to use the metaserver by default\n");
   fprintf(stderr, 
"instead of printing this message.)\n");

   fprintf(stderr, 
"\nUse \"%s -u\" for general program usage and options.\n",
      prog);
}

void
printUsage(prog)
   char           *prog;
{
   fprintf(stderr, "\nBRMH %s usage: %s [options]\n\n", VERSION, prog);
   fprintf(stderr, "Where common options include:\n");
#ifdef META
   fprintf(stderr, "   [-m]                Check all servers (via metaserver)\n");
#endif
#ifdef GATEWAY
   fprintf(stderr, "   [-H servername]     Specify a server\n");
#else
   fprintf(stderr, "   [-h servername]     Specify a server\n");
#endif
   fprintf(stderr, "   [-p port number]    Specify a port to connect to\n");

   fprintf(stderr, "\nOther options:\n");
#ifndef GATEWAY
   fprintf(stderr, "   [-c]                Check who's playing on selected server\n");
#endif
   fprintf(stderr, "   [-C [character]]    Login with character (or disable autologin)\n");
   fprintf(stderr, "   [-A password]       Character password\n");
   fprintf(stderr, "   [-r defaultsfile]   Specify defaults file\n");
   fprintf(stderr, "   [-d display]        Specify X display\n");
   fprintf(stderr, "   [-v]                Version information only.\n");
   fprintf(stderr, "   [-u]                This message.\n");
   fprintf(stderr, "   [-l filename]       Record messages into 'filename'\n");

#ifdef RECORD
   fprintf(stderr, "   [-R filename]       Record game to file\n");
   fprintf(stderr, "   [-F filename]       Playback recorded game\n");
   fprintf(stderr, "   [-Z]                Paradise compatibility mode (for record/playback)\n");
#endif

   fprintf(stderr, "\nRare options:\n");
   fprintf(stderr, "   [-s socketnum]      Specify listen socket port for manual start\n");
#ifdef FOR_MORONS
   fprintf(stderr, "   [-M]                Toggle netrek-for-morons mode\n");
#endif
#ifdef PACKET_LOG
   fprintf(stderr, "   [-P]                Log server packets, repeat for increased information\n");
#endif
   fprintf(stderr, "   [-D]                Debugging level (use twice for UDP info)\n");
   fprintf(stderr, "\nAt runtime, select 'show help window' for a list of commands.\n");
#ifdef XTREKRC_HELP
   fprintf(stderr, "Select 'show xtrekrc window' for a list of .netrekrc options.\n");
#endif
   fprintf(stderr, "For a detailed description of the configuration file .netrekrc\n");
   fprintf(stderr, "see the WWW (Netscape/Mosaic) URL:\n\t%s\n", URL_XTREKRC);
   fprintf(stderr, "For client updates and new releases, see the WWW (Netscape/Mosaic) URL\n");
   fprintf(stderr, "\t%s\n", URL_FTP);

   fprintf(stderr, "\nNETREK RESOURCES\n");
   fprintf(stderr, "\t%s\n",URL_NETREK);
   fprintf(stderr, "\tnews:rec.games.netrek\n\n");
   fprintf(stderr, "BUG REPORTS\n");
   fprintf(stderr, "%s\n", MAINTENANCE);
}

#ifdef EM
/*
 * *    code to mention that there was an error, but not to crash *
 * catastrophicly. This may need to be hacked to have a limit *    of errors
 * per update or something, to keep from having runaway *    segfualt loops
 * dumping endlessly to a tty.
 */
int             errcount = 0;

void 
handle_segfault()
{
   printf("segmentation error detected; attempting to continue\n");
   if (errcount++ > 10)
      SIGNAL(SIGSEGV, SIG_DFL);
   /* core dump normally on next error */
}

void 
handle_exception()
{
   printf("floating exception error detected; attempting to continue\n");
   if (errcount++ > 10)
      SIGNAL(SIGSEGV, SIG_DFL);
   /* core dump normally on next error */
}
#endif

void
get_core()
{
   fprintf(stderr, "SIGPIPE\n");
   abort();
}

void
gb()
{
   if(nextSocket != -1){
      if(udpSock)
	 closeUdpConn();
      if(sock)
      connectToServer(nextSocket);
   }
}

void
check_whosplaying(name, port)
   
   char	*name;
   int	port;
{
   int		sock;

   sock = open_port(name, port-1, 1);
   if(sock < 0)
      exit(1);
   read_sock(sock);
}

#ifdef FORKNETREK
void
addComLineArgs(args, count)

   char	**args;
   int	*count;
{
   char	buf[32];

   if(pseudo_name){
      args[(*count)++] = "-C";
      args[(*count)++] = pseudo_name;
   }
   if(pseudo_passwd){
      args[(*count)++] = "-A";
      args[(*count)++] = pseudo_passwd;
   }
   if(display){
      args[(*count)++] = "-d";
      args[(*count)++] = display;
   }
   if(defaultsFile){
      args[(*count)++] = "-r";
      args[(*count)++] = defaultsFile;
   }

#ifdef LOGMESG
   if(logMess){
      args[(*count)++] = "-l";
      args[(*count)++] = logFileName;
   }
#endif

#ifdef TREKHOPD
   if(use_trekhopd)
      args[(*count)++] = "-H";
   else
#endif
      args[(*count)++] = "-h";

   args[(*count)++] = serverName;

#ifdef TREKHOPD
   if(!use_trekhopd)
#endif
   {
      args[(*count)++] = "-p";
      sprintf(buf, "%d", xtrekPort);
      args[(*count)++] = strdup(buf);
   }

   args[(*count)++] = "-w";
   sprintf(buf, "%d", waitnum);
   args[(*count)++] = strdup(buf);
   args[(*count)++] = NULL;
}
#endif
