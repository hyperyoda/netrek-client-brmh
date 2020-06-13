
/* mostly borrwed from Nick Trown (COW) */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <pwd.h>
#include <string.h>
#include <ctype.h>
#ifdef hpux
#include <time.h>
#else				/* hpux */
#include <sys/time.h>
#endif
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "netrek.h"

#define KEYS		5

#define F_CHAOS		'C'
#define F_PARADISE	'P'
#define F_HOCKEY	'H'
#define F_BRONCO	'B'
#define F_DOGFIGHT	'F'
#define F_INL           'I'
#define F_STURGEON      'S'
#define F_OTHER         'O'

#define META_CACHE	".metacache"
#define NO_CACHE	0
#define READ_CACHE	1
#define READ_OLD_CACHE	2

#define MAX_SERVERS	48

#ifdef FORKNETREK
#define BUTTONS	3
#else
#define BUTTONS	2
#endif

struct servers {
   char            address[BUFSIZ];
   int             port;
   int             time;
   int             players;
   int             status;
   char            flag;
};

static struct servers *serverlist;
static int      num_servers;
static W_Window playerWin;
static int      quit_meta;

static char    *keystrings[KEYS] =
{"OPEN:", "Wait queue:", "Nobody playing", "* Timed out", "* Garbag"};

#define KEY_OPEN		0
#define KEY_WAITQ		1
#define KEY_NOBODY		2
#define KEY_NORESP		3	/* usually metaserver lying */
#define KEY_GARBAGE		4	/* usually metaserver lying */
#define KEY_UNKNOWN		5	/* special case */

#if __STDC__ || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

static int getnumber P_((char *string, int start));
static char *flagstring P_((char f));
static void readmeta P_((FILE *fsock, int cache_read));
static FILE *check_cache P_((int *last_cache));
static void parsemeta P_((void));
static void metawindow P_((void));
static void metarefresh P_((int i));
static void metaaction P_((char *metafile, W_Event *data));
static void metadone P_((void));
static void metainput P_((char *metafile));
static void handle_metafile P_((char *metafile));
static void show_playerlist P_((int sock));

#undef P_

#ifdef FORKNETREK
static void refresh();
static void reaper();
#endif

static int
getnumber(string, start)
    char           *string;
    int             start;
{
   char            temp[BUFSIZ];
   int             c;
   int             tc = 0;

   for (c = start; c <= strlen(string); c++) {
      if ((string[c] >= '0') && (string[c] <= '9')) {
	 temp[tc++] = string[c];
      } else if (tc > 0) {
	 temp[tc] = '\0';
	 return (atoi(temp));
      }
   }
   return 0;
}

static char *
flagstring(f)

   char	f;
{
   static char	buf[12];
   switch(f){
      case F_HOCKEY:
	 return "Hockey";
      case F_BRONCO:
	 return "Standard";
      case F_DOGFIGHT:
	 return "Dogfight";
      case F_CHAOS:
	 return "Chaos";
      case F_INL:
         return "INL";
      case F_STURGEON:
         return "Sturgeon";
      case F_OTHER:
         return "Other";

      default:
         sprintf(buf, "Unknown (%c)", buf[0]);
         return buf;
   }
}

/*
 * The connection to the metaserver is by Andy McFadden. This calls the
 * metaserver and parses the output into something useful
 */

int
open_port(host, port, verbose)
    char           *host;
    int             port;
    int             verbose;
{
   struct sockaddr_in 	addr;
   struct hostent 	*hp;
   int            	sock;

   /* Connect to the metaserver */
   /* get numeric form */
   if ((addr.sin_addr.s_addr = inet_addr(host)) == -1) {
      if ((hp = gethostbyname(host)) == NULL) {
	 if (verbose)
	    fprintf(stderr, "netrek: unknown host '%s'\n", host);
	 return (-1);
      } else {
	 addr.sin_addr.s_addr = *(long *) hp->h_addr;
      }
   }
   addr.sin_family = AF_INET;
   addr.sin_port = htons(port);
   if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      if (verbose)
	 perror("socket");
      return (-1);
   }

   if (connect(sock, (struct sockaddr *) & addr, sizeof(addr)) < 0) {
      if (verbose){
	 perror("connect");
      }
      fprintf(stderr, "%s: not listening or network failure.\n", host);
      close(sock);
      return (-1);
   }
   return (sock);
}

void
read_sock(sock)

   int	sock;
{
   char            line[BUFSIZ];
   FILE		   *fsock;

   /* let internal buffer code handle the line splitting */
   fsock = fdopen(sock, "r");

   while (fgets(line, BUFSIZ - 1, fsock)) {
      printf("%s", line);
   }
   fclose(fsock);
}


static void
readmeta(fsock, cache_read)
    FILE            *fsock;
    int		    cache_read;
{
   register int    i;
   char           *numstr;
   char            line[BUFSIZ];
   char            misc[BUFSIZ];
   FILE		   *fo = NULL;

   if(cache_read == NO_CACHE){
      char	*home = getenv("HOME");
      /* we're reading the metaserver, write out cache */
      if(home){
	 sprintf(misc, "%s/%s", home, META_CACHE);
	 fo = fopen(misc, "w");
      }
   }

   while (fgets(line, BUFSIZ - 1, fsock)) {

      if(fo)
	 fprintf(fo, "%s", line);

      if (sscanf(line, "-h %s -p %d %d %[^\n]",	/* parse line */
		 serverlist[num_servers].address,
		 &serverlist[num_servers].port,
		 &serverlist[num_servers].time,
		 misc) == 4) {

	 serverlist[num_servers].status = -1;

	 /*
	  * I don't have it checking the servers with nobody playing because
	  * the menu window would then be too large to fit on a 1024 x 768
	  * window. :(  - NBT
	  */

	 if(cache_read == READ_OLD_CACHE)
	    serverlist[num_servers].status = KEY_NORESP;
	 else for (i = 0; i < KEYS; i++) {
	    if ((numstr = strstr(misc, keystrings[i]))) {
	       serverlist[num_servers].status = i;
	       serverlist[num_servers].players = getnumber(numstr, 0);
	       break;
	    }
	 }

	 serverlist[num_servers].flag = misc[strlen(misc) - 1];

	 if (serverlist[num_servers].flag == F_PARADISE)
	    /* can't handle paradise servers */
	    serverlist[num_servers].status = -1;

	 switch (serverlist[num_servers].status) {
	 case KEY_OPEN:
	 case KEY_WAITQ:
	 case KEY_NOBODY:
	 case KEY_NORESP:
	 case KEY_GARBAGE:
	    num_servers++;	/* It's valid */
	    break;
	 default:
	    /* assuming that KEY_UNKNOWN does indeed mean somethings
	       wrong with the server */
	    break;
	 }
	 if(num_servers == MAX_SERVERS-1){
	    fprintf(stderr, "Too many servers (> %d)\n", MAX_SERVERS);
	    fprintf(stderr, "Client needs fixed.n");
	    break;
	 }
      }
   }

   if(fo)
      fclose(fo);

   fclose(fsock);
}

static FILE *
check_cache(last_cache)

   int	*last_cache;
{
   struct stat	mstat;
   char		metacache[BUFSIZ];
   char		*home = getenv("HOME");

   if(intDefault("metaCacheDisable", 0))
      return NULL;

   if(!home)
      return NULL;

   sprintf(metacache, "%s/%s", home, META_CACHE);

   if(stat(metacache, &mstat) < 0)
      return NULL;
   if(!mstat.st_size)
      return NULL;
   *last_cache = time(NULL) - mstat.st_mtime;
   return fopen(metacache, "r");
}

static void
parsemeta()
{
   int             	sock;
   int			last_cache = 0;
   FILE			*fi;

   fi = check_cache(&last_cache);

   /* read in last 5 minutes */
   if(fi && last_cache < 300){
      /* use the cached value */
      printf("netrek: using cached value from last 5 minutes ...\n");
      readmeta(fi, READ_CACHE);
   }
   else if ((sock = open_port(metaserver, metaport, 1)) > 0) {
      if(fi)
	 fclose(fi);

      /* let internal buffer code handle the line splitting */
      fi = fdopen(sock, "r");
      readmeta(fi, NO_CACHE);
   } else if(fi){
      /* metaserver screwed so use the cached value but blank out the player 
	 values, we don't know what they are */
      printf("netrek: can't connect to metaserver %s at port %d\n",
	      metaserver, metaport);
      printf("        using old cached value ...\n");
      readmeta(fi, READ_OLD_CACHE);
   }
   else{
      /* gotta fail */
      fprintf(stderr, "netrek: can't connect to metaserver %s at port %d\n",
	      metaserver, metaport);
      exit(0);
   }
}

/* Show the meta server menu window */

static void
metawindow()
{
   register int    	i,l;
   char			*s;

   if(!metaWin){
      metaWin = W_MakeMenu("MetaServer List", 0, 0, 72, num_servers + BUTTONS,
			   NULL, 2);
      W_DefineArrowCursor(metaWin);
   }
   else{
      W_ReinitMenu(metaWin, 72, num_servers + BUTTONS);
      W_ResizeMenu(metaWin, 72, num_servers + BUTTONS);
   }

   s = "Button 1 -- CONNECT       Button 3 -- PLAYER LIST";
   l = strlen(s);
   W_WriteText(metaWin, (72-l)/2, 0, W_Yellow, s, l, W_RegularFont);

   for (i = 0; i < num_servers; i++)
      metarefresh(i);

#ifdef FORKNETREK
   /* Add refresh option */
   s = "R E F R E S H";
   l = strlen(s);
   W_WriteText(metaWin, (72-l)/2, num_servers+BUTTONS-2, W_Green, s, l, 
      W_RegularFont);
#endif
   
   /* Add quit option */
   s = "Q U I T";
   l = strlen(s);
   W_WriteText(metaWin, (72-l)/2, num_servers+BUTTONS-1, W_Yellow, s, l, 
      W_RegularFont);

   /* Map window */
   W_MapWindow(metaWin);
}

/* Refresh item i */
static void
metarefresh(i)
    int             i;
{
   char            buf[BUFSIZ];
   int             color = textColor;

   switch (serverlist[i].status) {

   case KEY_OPEN:		/* open */
   case KEY_WAITQ:		/* wait q */
      color = W_Cyan;
      sprintf(buf, "%-40s %12s %-5d %s",
	      serverlist[i].address,
	      keystrings[serverlist[i].status],
	      serverlist[i].players,
	      flagstring(serverlist[i].flag));
      break;
   case KEY_NOBODY:
      sprintf(buf, "%-40s %12s       %s",
	      serverlist[i].address,
	      "Empty",
	      flagstring(serverlist[i].flag));
      break;

   case KEY_NORESP:
   case KEY_GARBAGE:
   case KEY_UNKNOWN:
   default:
      sprintf(buf, "%-40s %12s       %s",
	      serverlist[i].address,
	      "Unknown",
	      flagstring(serverlist[i].flag));
      break;
   }

   W_WriteText(metaWin, 0, i+1, color, buf, strlen(buf), W_RegularFont);
}

/* Check selection to see if was valid. If it was then we have a winner! */

static void
metaaction(metafile, data)
    char	*metafile;
    W_Event     *data;
{
   int             	sock;
#ifdef TREKHOPD
   extern int		use_trekhopd;
#endif
   int			use_observer_port = 0;

   if ((data->y >= 0) && (data->y < num_servers)) {
      xtrekPort = serverlist[data->y].port;
      serverName = strdup(serverlist[data->y].address);/* leak xxx */

      serverAlias = getalias(serverName);


      switch(data->key){
	 case W_RBUTTON:
	    W_DefineWaitCursor(metaWin);
	    W_Flush();
	    if((sock = open_port(serverName, xtrekPort-1, 1)) < 0) {
	       W_DefineArrowCursor(metaWin);
	       break;
	    }
#ifdef nodef
	    printf("\n%s =========================\n", 
	       strcap(serverName)/* distress.c */);
	    read_sock(sock);
#endif
	    show_playerlist(sock);
	    /* close(sock); */
	    W_DefineArrowCursor(metaWin);
	    break;
	 
	 case W_MBUTTON:
	    use_observer_port = 1;
	    /* fall through */

	 case W_LBUTTON:

	    W_DefineWaitCursor(metaWin);
	    W_Flush();
#ifdef TREKHOPD
	    if(use_trekhopd) goto metadone_label;	/* argh */
#endif
	    if ((sock = open_port(serverName, xtrekPort, 1)) < 0) {
	       serverlist[data->y].status = KEY_NORESP;
	       metarefresh(data->y);
	       close(sock);
	       W_DefineArrowCursor(metaWin);
	    } else {
	       close(sock);
#ifdef TREKHOPD
metadone_label:;
#endif
	       metadone();
#ifdef FORKNETREK
	       if(fork() == 0){
		  char	*args[32];
		  int	count = 0;

		  /* recommended in comp.windows.x faq */
		  W_ChildCloseDisplay();

		  /* get out of this process group so ^C on metawindow
		     doesn't kill everything spawned */
		  setpgid(0, 0);

		  args[count++] = program;

		  if(use_observer_port)
		     xtrekPort++;

		  addComLineArgs(args, &count);

		  execvp(program, args);
		  perror("execvp");
		  _exit(1);
	       }
	       W_DefineArrowCursor(metaWin);
	       /* allow 3 seperate wait queues for geometry specification */
	       waitnum ++ ; if(waitnum > 3) waitnum = 0;
#endif
	       
	    }
	 break;
      }

   }
#ifdef FORKNETREK
   else if (!metafile && data->y == num_servers && data->key == W_LBUTTON)
      refresh();
#endif
   
   else if (data->y == num_servers + (BUTTONS - 2) && 
	    data->key == W_LBUTTON)	/* quit */
      exit(0);
}

/* Unmap the metaWindow */

static void
metadone()
{
#ifndef FORKNETREK
   /* Unmap window */
   W_DestroyWindow(metaWin);
   metaWin = NULL;
   if(playerWin)
      W_DestroyWindow(playerWin);
   W_Flush();
   if (serverlist)
      free((void *) serverlist);
   quit_meta = 1;
#endif
}


/*
 * My own little input() function. I needed this so I don't have to use all
 * the bull in the main input(). Plus to use it I'd have to call mapAll()
 * first and the client would read in the default server and then call it up
 * before I can select a server.
 */

static void
metainput(metafile)
   
   char	*metafile;
{
   W_Event         data;

   while (!quit_meta) {
      W_NextEvent(&data);
      data.y --;	/* skip the title */
      switch ((int) data.type) {
      case W_EV_KEY:
#if 0
	 if (data.Window == metaWin)
	    metaaction(metafile, &data);
#endif
	 break;
      case W_EV_BUTTON:
	 if (data.Window == metaWin)
	    metaaction(metafile, &data);
	 break;
      case W_EV_EXPOSE:
	 break;
      default:
	 break;
      }
   }
}

static void 
handle_metafile(metafile)

   char *metafile;
{
   FILE	*fi;

   fi = fopen(metafile, "r");
   if(!fi){
      perror(metafile);
      return;
   }
   readmeta(fi, READ_OLD_CACHE);
   /* fi closed by readmeta */
}

void
do_metaserver(metafile)
   
   char	*metafile;
{
   char           *s;

#ifdef FORKNETREK
   SIGNAL(SIGCHLD, reaper);
#endif

   serverlist = (struct servers *) malloc(sizeof(struct servers) * MAX_SERVERS);
   MZERO((void *) serverlist, sizeof(struct servers) * MAX_SERVERS);
   
   if(metafile){
      /* tots' suggestion: use metafile as metaserver -- store INL
         servers and such */
      handle_metafile(metafile);
   }
   else {

      /* get metaserver defaults */
      if ((s = getdefault("metaserver")))
	 metaserver = s;
      metaport = intDefault("metaport", metaport);
#ifdef SHOW_DEFAULTS
      show_defaults("C-startup", "metaserver", metaserver,
		    "Metaserver host to use for -m startup option.");
      show_defaults("C-startup", "metaport", metaport,
		    "Metaserver port to use for -m startup option.");
#endif

      /* get input */
      parsemeta();
   }
   /* put up window */
   metawindow();
   /* wait for input */
   metainput(metafile);
}

#ifdef FORKNETREK

static void
refresh()
{
   W_DefineWaitCursor(metaWin);
   W_Flush();
   if(serverlist)
      free((char *)serverlist);
   num_servers = 0;

   serverlist = (struct servers *) malloc(sizeof(struct servers) * MAX_SERVERS);
   MZERO((void *) serverlist, sizeof(struct servers) * MAX_SERVERS);

   parsemeta();
   metawindow();
   W_DefineArrowCursor(metaWin);
}

static void
reaper(sig)

   int  sig;
{
   int          pid;

#ifdef SVR4
   while ((pid=waitpid(-1, 0, WNOHANG)) > 0)
#else
   while ((pid=wait3((union wait *) 0, WNOHANG, (struct rusage *) 0)) > 0)
#endif
        ;
}
#endif

static void
show_playerlist(sock)

   int	sock;
{
   char		*s;
   register	l;
   char		line[BUFSIZ];
   FILE		*fsock;
   W_Color	color;

   /* let internal buffer code handle the line splitting */
   fsock = fdopen(sock, "r");

   if(!playerWin){
      playerWin = W_MakeScrollingWindow("Player List", 
	 0, 0, 
	 80, 26, 0, 2);
      W_DefineUpDownCursor(playerWin);
   }
   W_MapWindow(playerWin);

   sprintf(line, "%s (%d) ", strcap(serverName), xtrekPort);
   l = strlen(line);
   while(l < 75)
      line[l++] = '=';
   line[l] = 0;

   W_WriteText(playerWin, 0, 0, W_Yellow, "", 1, W_MesgFont);
   W_WriteText(playerWin, (80-l)/2, 0, W_Yellow, line, l, W_MesgFont);
   W_WriteText(playerWin, 0, 0, W_Yellow, "", 1, W_MesgFont);
   W_FlushScrollingWindow(playerWin);
   W_Flush();

   while(fgets(line, BUFSIZ-1, fsock)){
      if((s=strrchr(line, '\n')))
	 *s = 0;

      /* if playerlist format changes significantly, following will fail.
	 assumption:
	 <team letter><number>:
      */
      color = textColor;
      s = line;
      /* simple minded parsing looking for "xx:" */
      while(isspace(*s)) s++;
      if(s[2] == ':'){
	 switch(s[0]){
	    case 'F': color = shipCol[remap[FED]]; break;
	    case 'R': color = shipCol[remap[ROM]]; break;
	    case 'O': color = shipCol[remap[ORI]]; break;
	    case 'K': color = shipCol[remap[KLI]]; break;
	    case 'I': color = shipCol[remap[IND]]; break;
	    default: break;
	 }
      }

      W_WriteText(playerWin, 0, 0, color, line, strlen(line), W_MesgFont);
   }
   W_FlushScrollingWindow(playerWin);

   fclose(fsock);
}
