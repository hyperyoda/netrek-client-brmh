/*
 * defaults.c
 * 
 * Kevin P. Smith  6/11/89
 * 
 * $Log: defaults.c,v $
 * Revision 1.3  2000/03/23 03:50:49  karthik
 * Remove strdup() for Linux
 *
 * Revision 1.2  2000/02/17 05:48:05  ahn
 * BRMH 2.3 from David Pinkney <dpinkney@cs.uml.edu>
 * Revision 1.6  1993/10/05  16:40:38  hadley checkin
 * 
 * Revision 1.6  1993/10/05  16:38:08  hadley checkin
 * 
 * 
 */

#include "copyright2.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/file.h>
#include "netrek.h"

extern struct dmacro_list dist_prefered[NUM_DIST];
extern struct dmacro_list *distmacro;
extern int      sizedist;

struct stringlist {
   char           *string;
   char           *value;
   struct stringlist *next;
};

struct stringlist *defaults = NULL;


/* Any modern Unix will have these  -karthik 20070424
char           *getenv();
#ifndef linux
char           *strdup();
#endif */

#ifdef NBTDIST

char           *
new_param(v)
    char           *v;
{
   char           *temp;

   temp = strdup(v);
#ifdef nodef
   if (strlen(temp) > 12) {
      printf("Distress stat: '%s' is too long. (max = 12)\n", v);
      temp[12] = '\0';
   }
#endif
   return temp;
}

void
figure_distress(file, v)
    char           *file, *v;
{
   int             index = 0;
   char           *check;

   if ((check = strstr(file, "d.sb.")) != NULL)
      index = 1;		/* its a SB param */
   else if ((check = strstr(file, "d.")) == NULL) {
      fprintf(stderr, "Unknown distress param %s\n", file);
      return;
   }
   file = check;
   /* set on/offs */
   if ((check = strstr(file, "whole")) != NULL) {
      distress[index].shld_on = 0;	/* reset the on/offs */
      distress[index].dam_on = 0;
      distress[index].wtmp_on = 0;
      distress[index].etmp_on = 0;
      distress[index].arms_on = 0;
      if (strstr(v, "shld") != NULL)
	 distress[index].shld_on = 1;
      if (strstr(v, "dam") != NULL)
	 distress[index].dam_on = 1;
      if (strstr(v, "wtmp") != NULL)
	 distress[index].wtmp_on = 1;
      if (strstr(v, "etmp") != NULL)
	 distress[index].etmp_on = 1;
      if (strstr(v, "arms") != NULL)
	 distress[index].arms_on = 1;
   }
   /* Do the shields */
   else if ((check = strstr(file, "shld")) != NULL) {
      if (strstr(check, "lvl"))
	 sscanf(v, "%d %d", &distress[index].min_shld, &distress[index].max_shld);
      else if (strstr(check, "low"))
	 distress[index].low_shld = new_param(v);
      else if (strstr(check, "mid"))
	 distress[index].mid_shld = new_param(v);
      else if (strstr(check, "high"))
	 distress[index].high_shld = new_param(v);
   }
   /* Do the Damage */
   else if ((check = strstr(file, "dam")) != NULL) {
      if (strstr(check, "lvl"))
	 sscanf(v, "%d %d", &distress[index].min_dam, &distress[index].max_dam);
      else if (strstr(check, "low"))
	 distress[index].low_dam = new_param(v);
      else if (strstr(check, "mid"))
	 distress[index].mid_dam = new_param(v);
      else if (strstr(check, "high"))
	 distress[index].high_dam = new_param(v);
   }
   /* Do the wtmp */
   else if ((check = strstr(file, "wtmp")) != NULL) {
      if (strstr(check, "lvl"))
	 sscanf(v, "%d %d", &distress[index].min_wtmp, &distress[index].max_wtmp);
      else if (strstr(check, "low"))
	 distress[index].low_wtmp = new_param(v);
      else if (strstr(check, "mid"))
	 distress[index].mid_wtmp = new_param(v);
      else if (strstr(check, "high"))
	 distress[index].high_wtmp = new_param(v);
   }
   /* Do the etmp */
   else if ((check = strstr(file, "etmp")) != NULL) {
      if (strstr(check, "lvl"))
	 sscanf(v, "%d %d", &distress[index].min_etmp, &distress[index].max_etmp);
      else if (strstr(check, "low"))
	 distress[index].low_etmp = new_param(v);
      else if (strstr(check, "mid"))
	 distress[index].mid_etmp = new_param(v);
      else if (strstr(check, "high"))
	 distress[index].high_etmp = new_param(v);
   }
   /* Do the armies */
   else if ((check = strstr(file, "arms")) != NULL) {
      if (strstr(check, "lvl"))
	 sscanf(v, "%d %d", &distress[index].min_arms, &distress[index].max_arms);
      else if (strstr(check, "low"))
	 distress[index].low_arms = new_param(v);
      else if (strstr(check, "mid"))
	 distress[index].mid_arms = new_param(v);
      else if (strstr(check, "high"))
	 distress[index].high_arms = new_param(v);
   }
   /* Do the fuel */
   else if ((check = strstr(file, "fuel")) != NULL) {
      if (strstr(check, "lvl"))
	 sscanf(v, "%d %d", &distress[index].min_fuel, &distress[index].max_fuel);
      else if (strstr(check, "low"))
	 distress[index].low_fuel = new_param(v);
      else if (strstr(check, "mid"))
	 distress[index].mid_fuel = new_param(v);
      else if (strstr(check, "high"))
	 distress[index].high_fuel = new_param(v);
   }
   /* What else is there except an error */
   else
      fprintf(stderr, "netrek: Unknown distress param %s\n", file);
}
#endif				/* NBTDIST */

/*
 * value returned:
 * 0  : no defaults file
 * 1  : defaults file
 * -1 : defaults file specified doesn't exist
 */

int
initDefaults(deffile)
    char           *deffile;	/* As opposed to defile? */
{
   FILE           *fp;
   char            file[BUFSIZ];
   char            buf[BUFSIZ];
   char           *home;
   char           *v, *v2;
   struct stringlist *new;
   register int	   line=0;
   int		   show_err = 0;

#ifdef MULTILINE_MACROS

   unsigned char   keysused[256];

   MZERO(keysused, sizeof(keysused));

#endif

   if(defaults_file)
      free((char *) defaults_file);

   /*
      Always initialise keymaps.  Note that initkeymaps relies
      on the definition here.
   */
   keymaps[KEYMAP_DEFAULT] = InitKeyMap(keymaps[KEYMAP_DEFAULT]);

   getshipdefaults();
   
   /* sizeof doesn't work if it isn't in the same source file, shoot me */
   MCOPY(dist_defaults, dist_prefered, sizedist);
   distmacro = dist_prefered;

   if (!deffile) {
      deffile = file;
      home = getenv("HOME");
      if (!findDefaults(home, /* initialized */ deffile)) {
	 if (home) {
	    sprintf(file, "%s/.xtrekrc", home);
	 } else {
	    strcpy(file, ".xtrekrc");
	 }
	 deffile = file;
      }
   }
   else
      show_err ++;

   fp = fopen(deffile, "r");
   if (!fp){
      if(show_err){
	 fprintf(stderr, "netrek: Can't open defaults file.\n");
	 perror(deffile);
	 return -1;
      }
      return 0;
   }
   defaults_file = strdup(deffile);	/* keep this around for
					   error messages */

   IFDEBUG(printf("Using defaults file %s\n", deffile);)
#ifdef NBT
   macrocnt = 0;		/* reset macros */
#endif
   while (fgets(buf, BUFSIZ - 2, fp)) {
      line++;
      if (*buf == '#')
	 continue;
      if (*buf != 0)
	 buf[strlen(buf) - 1] = '\0';
      v = buf;
      while (*v != ':' && *v != 0) {
	 v++;
      }
      if (*v == 0)
	 continue;
      *v = 0;
      v++;
      v2 = v;			/* mark start of line for defaults that may
				 * use initial spaces */
      while (*v == ' ' || *v == '\t') {
	 v++;
      }
#ifdef NBT
      if (strncmp(buf, "macro.", 6) == 0) {
	 if (macrocnt == MAX_MACRO) {
	    fprintf(stderr, "%s:%d: Maximum number of macros is %d\n", 
	       defaults_file, line, MAX_MACRO);
	 } else if (buf[6] == '?') {
	    fprintf(stderr, "%s:%d: Cannot use '?' for a macro\n",
	       defaults_file, line);
	 } else {
#ifdef FEATURE
	    macro[macrocnt].type = NBTM;
#endif
	    macro[macrocnt].key = buf[6];
	    macro[macrocnt].who = buf[8];
	    macro[macrocnt].string = strdup(v);
	    macrocnt++;
#ifdef MULTILINE_MACROS
	    if (keysused[(int) macro[macrocnt].key]) {
	       macro[(int) keysused[(int) macro[macrocnt].key] - 1].type =
		  NEWMULTIM;
	       macro[macrocnt].type = NEWMULTIM;
	    } else {
	       keysused[(int) macro[macrocnt].key] = macrocnt + 1;
	    }
#endif				/* MULTILINE_MACROS */
	 }
      } else
#endif

#ifdef FEATURE
      if (strncmp(buf, "mac.", 4) == 0) {
	 if (macrocnt == MAX_MACRO) {
	    fprintf(stderr, "%s:%d: Maximum number of macros is %d\n", 
	       defaults_file, line, MAX_MACRO);
	 } else if (buf[4] == '?')
	    fprintf(stderr, "%s:%d: Cannot use '?' for a macro\n",
	       defaults_file, line);
	 else {
	    int i = 0;

	    if (buf[4] == '^' && buf[5] != '.') {
	       i = 1;
	       macro[macrocnt].key = (buf[5] + 96);
	    }
	    else
	       macro[macrocnt].key = buf[4];

	    if (buf[5+i] == '.') {
	       if (buf[6+i] == '%') {
		  switch (buf[7+i]) {
		  case 'u':
		  case 'U':
		  case 'p':
		     macro[macrocnt].who = MACRO_PLAYER;
		     break;
		  case 't':
		  case 'z':
		  case 'Z':
		     macro[macrocnt].who = MACRO_TEAM;
		     break;
		  case 'g':
		     macro[macrocnt].who = MACRO_FRIEND;
		     break;
		  case 'h':
		     macro[macrocnt].who = MACRO_ENEMY;
		     break;
		  default:
		     macro[macrocnt].who = MACRO_ME;
		     break;
		  }
		  macro[macrocnt].type = NEWMMOUSE;
	       } else {
		  macro[macrocnt].who = buf[6+i];
		  macro[macrocnt].type = NEWMSPEC;
	       }
	    } else {
	       macro[macrocnt].who = '\0';
	       macro[macrocnt].type = NEWM;
#ifdef MULTILINE_MACROS

	       if (keysused[(int) macro[macrocnt].key]) {
		  fprintf(stderr, "%s:%d: Multiline macros of nonstandard types are not recommended. \n", defaults_file, line);
		  fprintf(stderr, "\tYou might experience strange behaviour of macros.\n");
		  fprintf(stderr, "\tType: unspecified macro, key: %c.\n", macro[macrocnt].key);
	       }
#endif				/* MULTILINE_MACROS */

	    }
#ifdef MULTILINE_MACROS

	    if (keysused[(int) macro[macrocnt].key]) {
	       macro[(int) keysused[(int) macro[macrocnt].key] - 1].type = NEWMULTIM;
	       macro[macrocnt].type = NEWMULTIM;
	    } else {
	       keysused[(int) macro[macrocnt].key] = macrocnt + 1;
	    }
#endif				/* MULTILINE_MACROS */

	    macro[macrocnt].string = strdup(v);
	    macrocnt++;
	 }
      } else if (strncasecmp(buf, "dist.", 5) == 0) {
	 int             offset = 5;
	 int             notdone = 1;
	 struct dmacro_list *dm;
	 struct dmacro_list *dm_def;


	 if (buf[6] == '.')
	    offset = 7;

	 for (dm = &dist_prefered[take], dm_def = &dist_defaults[take];
	      dm->name && notdone; dm++, dm_def++) {
	    if (strcmpi(buf + offset, dm->name) == 0) {
	       dm->macro = strdup(v2);

#ifdef DIST_KEY_NAME
	       if (offset == 7) {
		  /* I'm defining this as an implicit ckeymap'ing */
		  keymaps[KEYMAP_DEFAULT][buf[5]-32+96] = dm_def->c + 96;
	       }
#endif				/* DIST_KEY_NAME */

	       notdone = 0;
	       break;
	    }
	 }
	 if (notdone)
	    fprintf(stderr, "%s:%d: Unknown RCD \"%s\"\n", 
	       defaults_file, line, buf + offset);
      } else if (strncasecmp(buf, "singleMacro", 11) == 0) {
	 char           *str = v;
	 int             i;

	 singleMacro = malloc((strlen(v) > 64) ? strlen(v) : 64);

	 for (i = 0; *str; str++)
	    if (*str == '^')
	       singleMacro[i++] = (char) (*(++str) + 96);
	    else
	       singleMacro[i++] = *str;

	 singleMacro[i] = '\0';
      } else
#endif

#ifdef NBTDIST
      if (strncmp(buf, "d.", 2) == 0)
	 figure_distress(buf, v);
      else
#endif
      if (*v != 0) {
	 new = (struct stringlist *) malloc(sizeof(struct stringlist));
	 new->next = defaults;
	 new->string = strdup(buf);
	 new->value = strdup(v);
#ifdef nodef			/* bad idea -- colors use the '#' character */
	 /* get rid of trailing comments */
	 v = new->value;
	 while (*v && *v != '#')
	    v++;
	 if (*v == '#')
	    *v = 0;
#endif
	 defaults = new;
      }
   }
   fclose(fp);

#ifdef SHOW_DEFAULTS
   show_defaults("macros", "mac.k.T",  "Macro test",
      "Macros.  Format: mac.<key>.<destination>:  <macro>\n\
Macros are activated by typing the macro key followed by <key>.\n\
However, if the key occurs in 'singleMacro', the macro key isn't needed.\n\
Macro documentation is too extensive to go into here.  See the documentation\n\
file on the site where you got this client.");
   show_defaults("macros", "singleMacro",  "^t",
      "Single key macro keys. Format: singleMacro: <key><key>\n\
Key may be a normal key or a control key [preceded by '^'].  All keys\n\
that appear in this list that are also macro definitions [mac.] will not\n\
require the macro key to activate.");
   show_defaults("macros-RCD", "dist.z.pickup",  "%T%c->%O %p++ @ %L",
      "RCD. Format: dist[.key].[RCD description]: <macro>\n\
If key is given, it is interpreted as a control key.  RCD documentation is\n\
too extensive to go into here.  See the documenation file on the site where\n\
you got this client.");
#endif

   return 1;
}

/*
 * free up the defaults linked list, so that it can possibly be * re-read, or
 * for Possible cases where exit doesn't clear allocated * memory (for the
 * eventual Amiga port!)   -EM 12/3/92
 */
void
FreeDefaults()
{
   struct stringlist *freeme;
   struct stringlist *nextnode;
   nextnode = defaults;
   while (nextnode != NULL) {
      free(nextnode->value);
      free(nextnode->string);
      freeme = nextnode;
      nextnode = freeme->next;
      free(freeme);
   }
   defaults = NULL;
}

/* Any modern Unix will have strdup()  -karthik 20070424 */
#if 0
#ifndef linux
#ifndef _AIX
char           *
strdup(str)
#if defined(__STDC__)		/* doing this so strdup() will match
				 * prototype in string.h */
const
#endif
    char           *str;
{
   char           *s;

   s = (char *) malloc(strlen(str) + 1);
   strcpy(s, str);
   return (s);
}
#endif
#endif
#endif

/* kludge: look for serverName on value side of default string so we
   can find an alias on the left side */
char *
getalias(server)

   char	*server;
{
   struct stringlist 	*sl;
   char			*alias;
   char			serv_s[80];

   sl = defaults;
   while (sl != NULL) {
      if(strcmpi(sl->value, server) == 0){
	 if((alias = strchr(sl->string, '.'))){
	    strncpy(serv_s, sl->string, alias-sl->string);
	    serv_s[alias-sl->string] = 0;
	    if(strcmpi(serv_s, "server")!=0)
	       continue;
	    alias ++;
	    return alias;
	 }
      }
      sl = sl->next;
   }
   return NULL;
}

char           *
_getdefault(str)
    char           *str;
{
   struct stringlist *sl;

   sl = defaults;
   while (sl != NULL) {
      if (strcmpi(sl->string, str) == 0) {
	 return (sl->value);
      }
      sl = sl->next;
   }
   return (NULL);
}

char           *
getdefault(str)
    char           *str;
{
   char            buf[80];
   char           *s;

#ifdef GATEWAY
   if (serverNameRemote) {
      sprintf(buf, "%s.%s", str, serverNameRemote);
      s = _getdefault(buf);
      if (s)
	 return s;
   }
#else
   if (serverName) {
      sprintf(buf, "%s.%s", str, serverName);
      s = _getdefault(buf);
      if (s)
	 return s;
   }
#endif

   if (serverAlias) {
      sprintf(buf, "%s.%s", str, serverAlias);
      s = _getdefault(buf);
      if (s)
	 return s;
   }
   return _getdefault(str);
}

/*
 * strcmpi tweaked 9/17/92 E-Mehlhaff to not tweak the strings it's * called
 * with... * writing into a 'constant' strings space is bad!
 */
int
strcmpi(str1, str2)
    char           *str1, *str2;
{
   /* #ifdef strcasecmp */
   return (strcasecmp(str1, str2));
   /*
    * char            chr1, chr2; #else for(;;) { chr1 = isupper(*str1) ?
    * *str1 : toupper(*str1); chr2 = isupper(*str2) ? *str2 : toupper(*str2);
    * if (chr1 != chr2) return(chr2 - chr1); if (chr1==0 || chr2==0)
    * return(1); if (chr1==0 && chr2==0) return(0); str1++; str2++; } return
    * (0); #endif
    */
}

int
booleanDefault(def, preferred)
    char           *def;
    int             preferred;
{
   char           *str;

   str = getdefault(def);
   if (str == NULL)
      return (preferred);
   if (strcmpi(str, "on") == 0 || strcmpi(str, "true")==0 ) {
      return (1);
   } else {
      return (0);
   }
}

double
floatDefault(def, preferred)

   char			*def;
   double		preferred;
{
   char           *str;
   str = getdefault(def);
   if (!str)
      return preferred;
   return atof(str);
}

int
intDefault(def, preferred)
    char           *def;
    int             preferred;
{
   char           *str;
   str = getdefault(def);
   if (!str)
      return preferred;
   if(strcmpi(str, "off")== 0) return 0;
   if(strcmpi(str, "on")== 0) return 1;
   return atoi(str);
}

/*
 * no default file given on command line. See if serverName is defined.  If
 * it exists we look for HOME/.xtrekrc-<serverName> and .xtrekrc-<serverName>
 * Otherwise we try DEFAULT_SERVER.
 */

/*
 * since this is Find Defaults, I moved all the defaults file checking to *
 * it, and put in support for a system defaults file. * and it uses the
 * access() system call to determine if a defaults *  file exists. * note,
 * access() returns 0 if user can read file, -1 on error or if * they can't. *
 * -EM *
 * 
 * Is anyone else bothered by the fact that this writes to deffile * without
 * really knowing how much of deffile is allocated? *
 * 
 */

int
findDefaults(home, deffile)
    char           *home, *deffile;
{
   int             accessible = -1;

   /* first look for server specific .xtrekrc in $HOME */
   if (serverName) {
      if (home)
	 sprintf(deffile, "%s/.xtrekrc-%s", home, serverName);
      else
	 sprintf(deffile, ".xtrekrc-%s", serverName);
      IFDEBUG(printf("Looking for defaults file in %s\n", deffile);)
      accessible = access(deffile, R_OK);
      if (accessible == 0)
	 return 1;
      /* else return 0; */
   }
   /* then try default server server-speciric defaults file in $HOME */
   if (home)
      sprintf(deffile, "%s/.xtrekrc-%s", home, DEFAULT_SERVER);
   else
      sprintf(deffile, ".xtrekrc-%s", DEFAULT_SERVER);
   IFDEBUG(printf("Looking for defaults file in %s\n", deffile);)
   accessible = access(deffile, R_OK);
   if (accessible == 0)
      return 1;

   /* now try .xtrekrc in $HOME */
   if (home)
      sprintf(deffile, "%s/.xtrekrc", home);
   else
      sprintf(deffile, ".xtrekrc");
   IFDEBUG(printf("Looking for defaults file in %s\n", deffile);)
   accessible = access(deffile, R_OK);
   if (accessible == 0)
      return 1;

   /* and of course, try for a .netrekrc  in $HOME, for the new generation */
   /* now try .xtrekrc in $HOME */
   if (home)
      sprintf(deffile, "%s/.netrekrc", home);
   else
      sprintf(deffile, ".netrekrc");
   IFDEBUG(printf("Looking for defaults file in %s\n", deffile);)
   accessible = access(deffile, R_OK);
   if (accessible == 0)
      return 1;

#ifdef SYSTEM_DEFAULTFILE
   /* now try for a system default defaults file */
   sprintf(deffile, SYSTEM_DEFAULTFILE);
   IFDEBUG(printf("Looking for defaults file in %s\n", deffile);)
   accessible = access(deffile, R_OK);
   if (accessible == 0)
      return 1;
#endif
   return 0;
}

int
defaultShip(preferred)
    int             preferred;
{
   char           *type;

   type = getdefault("defaultShip");
#ifdef SHOW_DEFAULTS
   show_defaults("string", "defaultShip", classes[preferred],
	  "Default ship to use on button-click entry (SC,DD,CA,BB,AS,SB).");
#endif
   if (type == NULL)
      return preferred;
   if ((strcmpi(type, "scout") == 0) || (strcmpi(type, "SC") == 0))
      return SCOUT;
   else if ((strcmpi(type, "destroyer") == 0) || (strcmpi(type, "DD") == 0))
      return DESTROYER;
   else if ((strcmpi(type, "cruiser") == 0) || (strcmpi(type, "CA") == 0))
      return CRUISER;
   else if ((strcmpi(type, "battleship") == 0) || (strcmpi(type, "BB") == 0))
      return BATTLESHIP;
   else if ((strcmpi(type, "assault") == 0) || (strcmpi(type, "AS") == 0))
      return ASSAULT;
   else if ((strcmpi(type, "starbase") == 0) || (strcmpi(type, "SB") == 0))
      return STARBASE;
   else
      return preferred;
}

#ifdef SHOW_DEFAULTS

static FILE    *fo;

void
show_defaults(category, name, value, desc)
    char           *category;
    char           *name;
    char           *value;
    char           *desc;
{
   char            buf[BUFSIZ];
   char            key[80], shortn[80], *strchr();
   static char     olddesc[BUFSIZ], oldkey[80];
   int		   line = 'a';
   static int	   ccolor;
   register char  *s, *t;
   if (!fo) {
      sprintf(buf, "tmp-xtrekrc");
      fo = fopen(buf, "w");
      if (!fo) {
	 perror(buf);
	 exit(1);
      }
   }
   strcpy(shortn, name);

   if (strncmp(name, "color", 5) == 0 || 
       (strncmp(category, "windows", 7) == 0)) {
      if ((s = strchr(shortn, '.')))
	 *s = 0;
      if(ccolor)
	 line += 5;	/* xx */
      ccolor = 1;
   }

   s = desc;
   if (strcmp(category, "windows") != 0) {
      if (strcmp(desc, olddesc) != 0) {
	 sprintf(key, "(%s-%s-%c) ", category, shortn, line++);
	 fprintf(fo, "%s", key);
	 sprintf(key, "(%s-%s-%c) ", category, shortn, line++);
	 fprintf(fo, "\n%s# ", key);
	 while (*s) {
	    while (*s && *s != '\n') {
	       putc(*s, fo);
	       s++;
	    }
	    if (*s == '\n'){
	       sprintf(key, "(%s-%s-%c) ", category, shortn, line++);
	       fprintf(fo, "\n%s# ", key);
	    }
	    if (*s)
	       s++;
	 }
	 fprintf(fo, "\n");
      }
   } else if (strcmp(key, oldkey) != 0) {
      sprintf(key, "(%s-%s-%c) ", category, shortn, line++);
      fprintf(fo, "%s\n", key);
   }
   sprintf(buf, "%s:", name);
   sprintf(key, "(%s-%s-%c) ", category, shortn, line++);
   fprintf(fo, "%s%-32s %s\n", key, buf, value);
   strcpy(olddesc, desc);
   strcpy(oldkey, key);
}

void
finish_defaults()
{
   fclose(fo);
   fprintf(stderr, "Run mkdefault.\n");
}
#endif
