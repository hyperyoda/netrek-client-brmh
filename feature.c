/*
 * Feature.c
 * 
 * March, 1994.    Joe Rumsey, Tedd Hadley
 * 
 * most of the functions needed to handle SP_FEATURE/CP_FEATURE packets.  fill
 * in the features list below for your client, and add a call to
 * reportFeatures just before the RSA response is sent. handleFeature should
 * just call checkFeature, which will search the list and set the appropriate
 * variable.  features unknown to the server are set to the desired value for
 * client features, and off for server/client features.
 * 
 * feature packets look like: 
 * struct feature_cpacket {              
 *   char 		  type; 
 *   char                 feature_type; 
 *   char                 arg1, arg2;
 *   int                  value; 
 *   char                 name[80]; 
 * };
 * 
 * type is CP_FEATURE, which is 60.  feature_spacket is identical.
 */


#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "copyright.h"

#if defined (_IBMR2) || defined(SYSV)
#include <time.h>
#endif				/* _IBMR2 */
#include "netrek.h"

#ifdef RECORD
#include "recorder.h"            /* only needed while debugging features */
#endif

/* not the actual packets: this holds a list of features to report for */
/* this client. */
struct feature {
   char           *name;
   int            *var;		/* holds allowed/enabled status */
   char            feature_type;/* 'S' or 'C' for server/client */
   int             value;	/* desired status */
   char           *arg1, *arg2;	/* where to copy args, if non-null */
   int		   send_w_rsa;	/* send with RSA */
};

#define FEATURE_ON	1
#define FEATURE_OFF	0

#define SEND_FEATURE	1
#define ALREADY_SENT	0

struct feature  features[] =
{
   {"FEATURE_PACKETS", &F_server_feature_packets, 'S', FEATURE_ON, 0, 0, 
      ALREADY_SENT},
   {"WHY_DEAD", &F_why_dead, 'S', FEATURE_ON, 0, 0, 
      SEND_FEATURE},
   {"CLOAK_MAXWARP", &F_cloak_maxwarp, 'S', FEATURE_ON, 0, 0, 
      SEND_FEATURE},
   {"SELF_8FLAGS", &F_self_8flags, 'S', FEATURE_ON, 0, 0, 
      SEND_FEATURE},
#ifdef nodef
   {"SELF_8FLAGS2", &F_self_8flags2, 'S', FEATURE_OFF, 0, 0, 
      SEND_FEATURE},
#endif
   {"RC_DISTRESS", &F_gen_distress, 'S', FEATURE_ON, 0, 0, 
      SEND_FEATURE},
#ifdef MULTILINE_MACROS
    {"MULTIMACROS", &multiline_enabled, 'S', 1, 0, 0, 
      SEND_FEATURE},
#endif
   {"MOTD_BITMAPS", &F_motd_bitmaps, 'S', FEATURE_ON, 0, 0, 
      ALREADY_SENT},
   {"PHASER_MULTI_SEND", &F_phaser_multi_send, 'S', FEATURE_ON, 0, 0, 
      SEND_FEATURE},
   {"SHIP_CAP", &F_ship_cap, 'S', FEATURE_ON, 0, 0, 
      SEND_FEATURE},
   {"FPS", &F_fps, 'S', FEATURE_ON, 0, 0,
      ALREADY_SENT},
   {"UPS", &F_ups, 'S', FEATURE_ON, 0, 0,
      ALREADY_SENT},
   {0, 0, 0, 0, 0, 0}
};

/* for macro win */
int
feature_lines()
{
   return (sizeof(features) / sizeof(features[0]) - 1);
}

void
checkFeature(packet)
    struct feature_spacket *packet;
{
   int             i;
#ifdef DEBUG
   if (packet->type != SP_FEATURE) {
      printf("Packet type %d sent to checkFeature!\n", packet->type);
      return;
   }
#endif
#ifdef nodef
   printf("%s: %s(%d)\n", &packet->name[0], ((ntohl(packet->value) == 1) ? 
		"ON" : (ntohl(packet->value) == 0) ? "OFF" : "UNKNOWN"),
	  ntohl(packet->value));
#endif

#ifdef RECORD_DEBUG
   fprintf(RECORDFD,
	   "%s: %s(%d)\n", &packet->name[0], ((ntohl(packet->value) == 1) ? 
		"ON" : (ntohl(packet->value) == 0) ? "OFF" : "UNKNOWN"),
	  ntohl(packet->value));
#endif

   for (i = 0; features[i].name != 0; i++) {
      if (feature_cmp(packet->name, features[i].name)) {
	 /*
	  * if server returns unknown, set to off for server mods, desired
	  * value for client mods. Otherwise,  set to value from server.
	  */
	 if(features[i].var)
	    *features[i].var = (ntohl(packet->value) == -1 ?
			 (features[i].feature_type == 'S' ? 0 : features[i].
			  value) :
			     ntohl(packet->value));
	 if (features[i].arg1)
	    *features[i].arg1 = packet->arg1;
	 if (features[i].arg2)
	    *features[i].arg2 = packet->arg2;
	 break;
      }
   }
   if (features[i].name == 0) {
      fprintf(stderr, "Feature %s from server unknown to client!\n", 
	      packet->name);
   }
   else if (!strcmp(features[i].name, "UPS"))
   {
       cloak_phases = F_ups * 2 - 3;
       if (cloak_phases < 3)
           cloak_phases = 3;
   }
}

/* call this from handleRSAKey, before sending the response. */
void
reportFeatures()
{
   struct feature *f;

#ifdef RECORD_DEBUG
   fprintf(RECORDFD, "F_server_feature_packets=%d\n",
	   F_server_feature_packets);
#endif

   if (!F_server_feature_packets)
      return;


   for (f = features; f->name != 0; f++) {
      if(f->send_w_rsa)
	 sendFeature(f->name,
		     f->feature_type,
		     f->value,
		     (f->arg1 ? *f->arg1 : 0),
		     (f->arg2 ? *f->arg2 : 0));
      /* otherwise it was already sent (FEATURE_PACKETS, MOTD_BITMAPS) */
#ifdef RECORD_DEBUG
      fprintf(RECORDFD,
	      "Send:(C->S) %s (%c): %d\n", f->name, f->feature_type, f->value);
#endif
#ifdef DEBUG
      printf("(C->S) %s (%c): %d\n", f->name, f->feature_type, f->value);
#endif
   }
}

void
sendFeature(name, feature_type, value, arg1, arg2)
    char           *name;
    char            feature_type;
    int             value;
    char            arg1, arg2;
{
   struct feature_cpacket packet;

   strncpy(packet.name, name, sizeof(packet.name));
   packet.type = CP_FEATURE;
   packet.name[sizeof(packet.name) - 1] = 0;
   packet.feature_type = feature_type;
   packet.value = htonl(value);
   packet.arg1 = arg1;
   packet.arg2 = arg2;
   sendServerPacket((struct player_spacket *) & packet);
}


int
feature_cmp(f, s)
    char           *f, *s;
{
   while (*f && *s) {
      if ((islower(*f) ? toupper(*f) : *f) != (islower(*s) ? toupper(*s) : *s))
	 return 0;
      f++;
      s++;
   }
   return !*f && !*s;
}

/* for macro win */
void
showFeatures()
{
   register 			i = 0;
   register struct feature 	*f;
   char				buf[80], *title;

   title = "SERVER FEATURES:";
   W_WriteText(macroWin, 1, i, W_Yellow, title, strlen(title), 
      W_RegularFont);
   i += 2;

   for(f=features; f->name; i++,f++){
      sprintf(buf, "%20s", f->name);
      W_WriteText(macroWin, 2, i, textColor, buf, strlen(buf), W_RegularFont);
      W_WriteText(macroWin, 24, i, *f->var?W_Green:textColor,
	 *f->var?" ON":"OFF", 3, W_RegularFont);
   }
}


#ifdef RECORD
int paradise_feature_fix()
{
  struct feature *f;

#ifdef RECORD_DEBUG
  fprintf(RECORDFD, "paradise_feature_fix() called\n");
#endif

  if(!F_server_feature_packets)
    return 0;

  for(f = features; f->name != 0; f++) {
    if(!(strcmp(f->name, "SHIP_CAP"))) {
#ifdef RECORD_DEBUG
      fprintf(RECORDFD, "Disabling SHIP_CAP feature for paradise compat.\n");
#endif
      /* wipe out the SHIP_CAP feature, paradise doesn't understand it */
      /*****
      f->name = NULL;
      f->var = NULL;
      f->feature_type = '\0';
      f->value = 0;
      f->arg1 = NULL;
      f->arg2 = NULL;
      f->send_w_rsa = 0;
      *****/
      /* Don't wipe it out, turn it off */
      f->value = 0;
      f->send_w_rsa = 0;   /* We sent it earlier with FEATURE_PACKETS and
			      MOTD_BITMAPS (from main) */
    }
  }
  return 0;
}

#endif
