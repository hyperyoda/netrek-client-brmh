#include "copyright.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#if defined (_IBMR2) || defined(SYSV)
#include <time.h>
#endif				/* _IBMR2 */
#include <sys/time.h>
#include "netrek.h"

#define TESTCAP(c,s)    (c ? strcap(s) : s)

static int                      version_sent = 0;

/* OLD */
void
CheckFeatures(m)
   char                           *m;
{
   char                            buf[BUFSIZ];
   int			           features=0;

   strcpy(buf, "BRMH: Features enabled: ");

   if (strstr(m, "NO_NEWMACRO")) {
      F_UseNewMacro = 0;
      strcat(buf, "NO_NEWMACRO, ");
      features++;
   }
   if (strstr(m, "NO_SMARTMACRO")) {
      F_UseNewMacro = 0;
      strcat(buf, "NO_SMARTMACRO, ");
      features++;
   }
   if (strstr(m, "WHY_DEAD")) {
      F_why_dead = 1;
      strcat(buf, "WHY_DEAD, ");
      features++;
   }
   if (strstr(m, "RC_DISTRESS")) {
      F_gen_distress = 1;
      strcat(buf, "RC_DISTRESS, ");
      features++;
   }

   if(!features) return;
   buf[strlen(buf)-2] = 0;

   W_WriteText(reviewWin, 0, 0, W_White, buf, strlen(buf), W_MesgFont);
   W_WriteText(messwa, 0, 0, W_White, buf, strlen(buf), W_MesgFont);
}

void
sendVersion()
{
   char                            client_ver[15];
   if (!version_sent) {
      version_sent = 1;
      sprintf(client_ver, "@%s.%d", VERSION, PATCHLEVEL);

      sendMessage(client_ver, MINDIV|MCONFIG, me->p_no);
   }
}
