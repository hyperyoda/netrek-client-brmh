/*
 * detonate.c
 */
#include "copyright.h"

#include <stdio.h>
#include <sys/types.h>
#include "netrek.h"

/* Detonate torp */

/*
 * * Detonating torps have become a difficult part of the game.  Players *
 * quickly learned that detonating their own torps when the cloud was *
 * around another player, caused that player to die very quickly.  I *
 * removed that feature because it lead to people not having to shoot * well
 * to collect kills.  Now when players detonate their own torps, * the torps
 * just vanish and become available for future firing.
 */

void
detmine()
{
   register int    i;

   for (i = 0; i < MAXTORP; i++) {
      if (torps[i + (me->p_no * MAXTORP)].t_status == TMOVE ||
	  torps[i + (me->p_no * MAXTORP)].t_status == TSTRAIGHT) {
	 sendDetMineReq(i + (me->p_no * MAXTORP));
#ifdef SHORT_PACKETS
	 if (recv_short)
	    break;		/* Let the server det for me */
#endif
      }
   }
}
