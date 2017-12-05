#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef TSH
#include <X11/cursorfont.h>
#endif
#include <assert.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"

extern Display	*W_Display;
extern Window	W_Root;
static Window	sync_w;

void
spt_dupevent(new)

   XEvent	*new;
{
   XEvent	ev, *event = &ev;
   if(!sync_w) return;

   MCOPY(new, event, sizeof(XEvent));
   if(event->type == KeyPress){
      event->xkey.window = sync_w;
      XSendEvent(W_Display, sync_w, True, KeyPressMask, event);
   }
   else if(event->type == ButtonPress){
      event->xbutton.window = sync_w;
      XSendEvent(W_Display, sync_w, True, ButtonPressMask, event);
   }
}

void
spt_getwin(i)
   
   int	i;
{
   sync_w = i;
}
