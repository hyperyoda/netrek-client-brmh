/*
 * ping.c
 * 
 */

#include "copyright2.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <math.h>
#include <errno.h>
#include "netrek.h"

#ifdef PING

/* These are used only in pingstats.c */

int             ping_iloss_sc = 0;	/* inc % loss 0--100, server to
					 * client */
int             ping_iloss_cs = 0;	/* inc % loss 0--100, client to
					 * server */
int             ping_tloss_sc = 0;	/* total % loss 0--100, server to
					 * client */
int             ping_tloss_cs = 0;	/* total % loss 0--100, client to
					 * server */
int             ping_lag = 0;	/* delay in ms of last ping */
int             ping_av = 0;	/* rt time */
int             ping_sd = 0;	/* std deviation */

static double   p_s2;
static int      p_sum, p_n;
static int      p_M, p_var;

void
handlePing(packet)		/* SP_PING */
    struct ping_spacket *packet;
{
   ping = 1;			/* we got a ping */

   /*
    * printf("ping received at %d (lag: %d)\n", msetime(), (int)packet->lag);
    */
   sendServerPingResponse((int) packet->number);
   ping_lag = ntohs(packet->lag);
   ping_iloss_sc = (int) packet->iloss_sc;
   ping_iloss_cs = (int) packet->iloss_cs;
   ping_tloss_sc = (int) packet->tloss_sc;
   ping_tloss_cs = (int) packet->tloss_cs;

   calc_lag();

   if (W_IsMapped(pStats))	/* pstat window */
      updatePStats();
}

void
startPing()
{
   static
   struct ping_cpacket packet;

   packet.type = CP_PING_RESPONSE;
   packet.pingme = 1;

   if(debug)
      printf("W- %8d: %3d b TCP\n", mstime(), (int)sizeof(struct ping_cpacket));

   if (gwrite(sock, (char *) &packet, sizeof(struct ping_cpacket)) !=
       sizeof          (struct ping_cpacket)) {
      return;
   }
}

void
stopPing()
{
   static
   struct ping_cpacket packet;

   packet.type = CP_PING_RESPONSE;
   packet.pingme = 0;

   if(debug)
      printf("W- %8d: %3d b TCP\n", mstime(), (int)sizeof(struct ping_cpacket));

   if (gwrite(sock, (char *) &packet, sizeof(struct ping_cpacket)) !=
       sizeof          (struct ping_cpacket)) {
       return;
   }
}

void
sendServerPingResponse(number)	/* CP_PING_RESPONSE */
    int             number;
{
   struct ping_cpacket packet;
   int             s;

   if (commMode == COMM_UDP){
      s = udpSock;
      packets_sent++;
   } else
      s = sock;

   packet.type = CP_PING_RESPONSE;
   packet.pingme = (char) ping;
   packet.number = (unsigned char) number;
   /* count this one */
   packet.cp_sent = htonl(packets_sent);
   packet.cp_recv = htonl(packets_received);

   /*
    * printf("ping response sent at %d\n", msetime());
    */

   if(debug)
      printf("W- %8d: %3d b %s\n", mstime(), (int)sizeof(struct ping_cpacket),
	 s == udpSock?"UDP":"TCP");

   if (gwrite(s, (char *) &packet, sizeof(struct ping_cpacket)) !=
       sizeof          (struct ping_cpacket)) {
       return;
   }
}

void
calc_lag()
{
   /* probably ghostbusted */
   if (ping_lag > 2000)
      return;

   p_n++;
   p_sum += ping_lag;
   p_s2 += (ping_lag * ping_lag);
   if (p_n == 1)
      return;

   p_M = p_sum / p_n;
   p_var = (p_s2 - p_M * p_sum) / (p_n - 1);

   ping_av = p_M;
   ping_sd = (int) sqrt((double) p_var);
   /*
   if (errno)
      perror("ping_sd sqrt");
   */
}

#endif				/* PING */
