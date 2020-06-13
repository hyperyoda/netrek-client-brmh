/*
 * netstat.c
 * 
 */
#include "copyright2.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <math.h>
#include <errno.h>
#include "netrek.h"

/*
 * #define NETDEBUG
 */

static int      lastread;
static int      dead;
static int      start;
static int      counter;

static int      sum, n; /* total since last reset */
static double	s2;
static int      M, var;
static double   sd;

static int      suml, nl; /* total since last death */
static double	s2l;
static int      Ml, varl;
static double   sdl;

static int      nf;		/* network failures */

static char     nfthresh_s[8] = NETSTAT_DF_NFT_S;
static int      nfthresh = NETSTAT_DF_NFT;

void
ns_init(v)
    int             v;
{
   start = v;
   sum = n =  0;
   s2 = 0.;
   sd = 0.;

   suml = nl = 0;
   s2l = 0.;
   nf = 0;
   sdl = 0.;
}

void
ns_record_update(count)
    int             count;
{
   int             now;
   int             et;
   static int      lastupdateSpeed;
   static int      lasttime = -1;

   et = 1000 / updates_per_second;	/* expected time */

   if (!me)
      return;

   now = mstime();

   if (lasttime < (et + et / 4) && now - lastread < et / 2) {
#ifdef NETDEBUG
      printf("skipping %d\n", now - lastread);
#endif
      return;
   }
   /* wait a few updates to stabilize */
   if (start) {
      start--;
      lastread = now;
      return;
   }
   /* wait a few updates to stabilize */
   if (me->p_status != PALIVE) {
#ifdef NETDEBUG
      printf("waiting after death...\n");
#endif
      dead = 5;
      lastread = now;
      suml = nl = 0;
      s2l = 0;
      return;
   } else if (dead) {
      dead--;
      lastread = now;
      return;
   }
   /* reset if we change updates */
   if (updates_per_second != lastupdateSpeed) {
      ns_init(3);
      lastupdateSpeed = updates_per_second;
      lastread = now;
      return;
   }
   lastupdateSpeed = updates_per_second;

   lasttime = now - lastread;

   if (lasttime >= nfthresh) {
      nf++;			/* network failure */
      updateLMeter();
#ifdef NETDEBUG
      printf("network failure: %d\n", lasttime);;
#endif
   } else {
      counter++;
      ns_do_stat(lasttime, counter);
#ifdef NETDEBUG
      printf("%d\n", lasttime);
#endif
   }

   lastread = now;
}

void
ns_do_stat(v, c)
    int             v;
    int             c;
{
   int             uf;

   n++;
   nl++;
   sum += v;
   suml += v;
   s2 += (v * v);
   s2l += (v * v);

   if (n <= 1 || nl <= 1)
      return;

   uf = (updates_per_second * 10) / netstatfreq;
   if (uf == 0)
      uf = 1;

   if ((c % uf) == 0) {

      M = sum / n;
      var = (s2 - (int)(M * sum)) / (int)(n - 1);
      sd = (int) sqrt((double) var);

      Ml = suml / nl;
      varl = (s2l - (int)(Ml * suml)) / (int)(nl - 1);
      sdl = (int) sqrt((double) varl);

      updateLMeter();
   }
}

double
ns_get_tstat()
{
   return sd;
}

double
ns_get_lstat()
{
   return sdl;
}

int
ns_get_nfailures()
{
   return nf;
}

char           *
ns_get_nfthresh_s()
{
   return nfthresh_s;
}

void
ns_set_nfthresh_s(s)
    char           *s;
{
   strcpy(nfthresh_s, s);
}

int
ns_get_nfthresh()
{
   return nfthresh;
}

void
ns_set_nfthresh(v)
    int             v;
{
   nfthresh = v;
}
