#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>

#include <X11/Intrinsic.h>
#include <X11/Xos.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <X11/Xaw/Cardinals.h>
#include <X11/Xaw/Simple.h>

#include "netrek.h"

#define STARTX			50
#define TIMEUNIT		100
#define WINUNIT			25
#define YSCALE			((double)WINUNIT/TIMEUNIT)

char 	*_file;
int	_line = 0;
int	_maxsize, _maxtime;

#define foreachUpdate(e)	for(e=_page_start; e; e=e->next)
#define foreachType(l, e)	for(e=l?l->head:NULL; e; e=e->next)

typedef struct _TypeElt {
   int			type, size;
   char			*message;
   struct _TypeList	*list;
   struct _TypeElt	*next, *prev;
} TypeElt;

typedef struct _TypeList {
   struct _TypeElt	*head, *tail;
   int			num_elts;
} TypeList;

typedef struct _UpdateElt {
   int			tcp;
   long			time;
   int			size;
   TypeList		*type_list;
   struct _UpdateList	*list;
   struct _UpdateElt	*next, *prev;
} UpdateElt;

typedef struct _UpdateList {
   struct _UpdateElt	*head, *tail;
   int			num_elts;
} UpdateList;

UpdateList	*_update_list = NULL;
int		_num_pages;
UpdateElt	*_page_start;

typedef struct {

   char                 *program;
   XtAppContext         appContext;
   Display              *display;
   Screen               *screen;
   Window               window, icon_win;

   Dimension            width,height;

   Pixel                foreground, background;
   XFontStruct		*font;
   int			baseline;
   GC                   gc;

   Pixel		type_foreground, type_background;
   XFontStruct		*type_font;
   int			type_baseline;
   GC                   type_gc;

   Pixel		type_tcp_foreground, type_tcp_background;
   XFontStruct		*type_tcp_font;
   int			type_tcp_baseline;
   GC                   type_tcp_gc;

   Pixel		mess_foreground, mess_background;
   XFontStruct		*mess_font;
   int			mess_baseline;
   GC                   mess_gc;

   Widget               toplevel, draw;

   Dimension		max_size;

   Boolean		pname;

} Globals; 

#if __STDC__ || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

/* xdebug.c */
TypeList *add_type_elt_to_list P_((TypeElt *e, TypeList *l));
UpdateList *add_update_elt_to_list P_((UpdateElt *e, UpdateList *l));
TypeList *add_type_to_list P_((int type, int size, char *m, TypeList *l));
UpdateList *add_update_to_list P_((int tcp, long time, int size, TypeList *tl, UpdateList *l));
void main P_((int argc, char **argv));
void read_file P_((void));
void do_configure P_((Widget w, XEvent *ev, String *params, Cardinal *num_params));
void do_redraw P_((Widget w, XEvent *ev, String *params, Cardinal *num_params));
void do_quit P_((Widget w, XEvent *ev, String *params, Cardinal *num_params));
void do_page_up P_((Widget w, XEvent *ev, String *params, Cardinal *num_params));
void do_page_down P_((Widget w, XEvent *ev, String *params, Cardinal *num_params));
void do_incr P_((Widget w, XEvent *ev, String *params, Cardinal *num_params));
void do_toggle_pname P_((Widget w, XEvent *ev, String *params, Cardinal *num_params));
void redraw P_((void));
char *ttostring P_((int t));
void summarize P_((void));

#undef P_

Globals _globals;

#define offset(field)           XtOffset(Globals *, field)

static XtResource application_resources[] = {
   { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
      offset(foreground), XtRString, "grey" },
   { XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel),
      offset(background), XtRString, "white" },
   { XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
      offset(font), XtRString, "fixed" },

   { "typeForeground", "TypeForeground", XtRPixel, sizeof(Pixel),
      offset(type_foreground), XtRString, "red3" },
   { "typeBackground", "TypeBackground", XtRPixel, sizeof(Pixel),
      offset(type_background), XtRString, "black" },
   { "typeFont", "TypeFont", XtRFontStruct, sizeof(XFontStruct *),
      offset(type_font), XtRString, "6x10" },

   { "typeTCPForeground", "TypeTCPForeground", XtRPixel, sizeof(Pixel),
      offset(type_tcp_foreground), XtRString, "darkgreen" },
   { "typeTCPBackground", "TypeTCPBackground", XtRPixel, sizeof(Pixel),
      offset(type_tcp_background), XtRString, "black" },
   { "typeTCPFont", "TypeTCPFont", XtRFontStruct, sizeof(XFontStruct *),
      offset(type_tcp_font), XtRString, "6x10" },

   { "messForeground", "MessForeground", XtRPixel, sizeof(Pixel),
      offset(mess_foreground), XtRString, "grey" },
   { "messBackground", "MessBackground", XtRPixel, sizeof(Pixel),
      offset(mess_background), XtRString, "white" },
   { "messFont", "MessFont", XtRFontStruct, sizeof(XFontStruct *),
      offset(mess_font), XtRString, "6x10" },

   { XtNwidth, XtCWidth, XtRDimension, sizeof(Dimension), 
     offset(width), XtRString, "800" },
   { XtNheight, XtCHeight, XtRDimension, sizeof(Dimension), 
     offset(height), XtRString, "800" },

   { "maxSize", "MaxSize", XtRDimension, sizeof(Dimension), 
     offset(max_size), XtRString, "0" },
};
#undef offset

static char *fallback_resources[] = {
   "XDebug*draw.background:        white",
   NULL,
};

static XrmOptionDescRec options[] = {
   {"-maxsize", ".maxSize", XrmoptionSepArg,	NULL },
};

static char     *translations = 
   ":<Key>q:            quit()\n\
    <ConfigureNotify>:  configure()\n\
    <Expose>:           redraw()\n\
    <Btn1Down>:		page_down()\n\
    :<Key>space:	page_down()\n\
    <Btn2Down>:		incr()\n\
    <Btn3Down>:		page_up()\n\
    :<Key>t:		toggle_pname()\n\
    :<Key>b:		page_up()";

static XtActionsRec     actions[] = {
   { "quit",            do_quit },
   { "configure",       do_configure },
   { "redraw",          do_redraw },
   { "page_up",         do_page_up },
   { "page_down",       do_page_down },
   { "incr",       	do_incr },
   { "toggle_pname",    do_toggle_pname },
};

TypeList *
add_type_elt_to_list(e, l)

   TypeElt	*e;
   TypeList	*l;
{
   if(!l){
      l = (TypeList *) malloc(sizeof(TypeList));
      l->head = e;
      l->head->next = l->head->prev = NULL;
      l->num_elts = 1;
      l->tail = e;
      e->list = l;
   }
   else if(!l->head){
      l->head = e;
      l->tail = e;
      e->next = e->prev = NULL;
      l->num_elts = 1;
      e->list = l;
   }
   else{
      e->prev = l->tail;
      l->tail->next = e;
      l->tail = e;
      l->num_elts ++;
      e->list = l;
   }
   return l;
}

UpdateList *
add_update_elt_to_list(e, l)

   UpdateElt	*e;
   UpdateList	*l;
{
   if(!l){
      l = (UpdateList *) malloc(sizeof(UpdateList));
      l->head = e;
      l->head->next = l->head->prev = NULL;
      l->num_elts = 1;
      l->tail = e;
      e->list = l;
   }
   else if(!l->head){
      l->head = e;
      l->tail = e;
      e->next = e->prev = NULL;
      l->num_elts = 1;
      e->list = l;
   }
   else{
      e->prev = l->tail;
      l->tail->next = e;
      l->tail = e;
      l->num_elts ++;
      e->list = l;
   }
   return l;
}

TypeList *
add_type_to_list(type, size, m, l)

   int		type;
   int		size;
   char		*m;
   TypeList	*l;
{
   TypeElt	*e = (TypeElt *) malloc(sizeof(TypeElt));
   e->type = type;
   e->size = size;
   if(m)
      e->message = strdup(m);
   else
      e->message = NULL;
   e->list = NULL;
   e->next = e->prev = NULL;
   return add_type_elt_to_list(e, l);
}

UpdateList *
add_update_to_list(tcp, time, size, tl, l)

   int		tcp;
   long		time;
   int		size;
   TypeList	*tl;
   UpdateList	*l;
{
   UpdateElt	*e = (UpdateElt *) malloc(sizeof(UpdateElt));
   e->tcp = tcp;
   e->time = time;
   e->size = size;
   e->type_list = tl;
   e->list = NULL;
   e->next = e->prev = NULL;
   return add_update_elt_to_list(e, l);
}

void
main(argc, argv)
   
   int	argc;
   char	**argv;
{
   XGCValues            xgcv;
   Widget               draw;

   _globals.program = argv[0];
   _globals.toplevel = XtAppInitialize(&_globals.appContext, "XDebug", 
      options, XtNumber(options), &argc, argv, fallback_resources,
      NULL, (Cardinal) 0);
   XtGetApplicationResources(_globals.toplevel, (caddr_t)&_globals,
      application_resources, XtNumber(application_resources), NULL, 0);
   XtAppAddActions(_globals.appContext, actions, XtNumber(actions));

   if(argc == 1){
      fprintf(stderr, "usage: %s [-maxsize <bytes>] <BRMH -D output>\n", 
	 argv[0]);
      exit(0);
   }
   _file = argv[1];
   read_file();

   draw = XtVaCreateManagedWidget("draw", simpleWidgetClass,
      _globals.toplevel,
      XtNwidth, _globals.width,
      XtNheight, _globals.height,
      NULL);

   
   _num_pages = (YSCALE * _maxtime)/ _globals.height;
   _page_start = _update_list->head;

   _globals.pname = True;
   _globals.draw = draw;

   XtVaSetValues(_globals.draw, XtNtranslations,
      XtParseTranslationTable(translations), NULL);

   XtRealizeWidget(_globals.toplevel);
   
   _globals.window = XtWindow(_globals.draw);
   _globals.display= XtDisplay(_globals.draw);

   xgcv.foreground = _globals.foreground;
   xgcv.background = _globals.background;
   xgcv.font	   = _globals.font->fid;
   _globals.baseline = _globals.font->max_bounds.ascent;
   _globals.gc = XtGetGC(_globals.draw, GCForeground|GCBackground|GCFont, 
				&xgcv);

   xgcv.foreground = _globals.type_foreground;
   xgcv.background = _globals.type_background;
   xgcv.font	   = _globals.type_font->fid;
   _globals.type_baseline = _globals.type_font->max_bounds.ascent;
   _globals.type_gc = XtGetGC(_globals.draw, GCForeground|GCBackground|GCFont, 
				&xgcv);

   xgcv.foreground = _globals.type_tcp_foreground;
   xgcv.background = _globals.type_tcp_background;
   xgcv.font	   = _globals.type_tcp_font->fid;
   _globals.type_tcp_baseline = _globals.type_tcp_font->max_bounds.ascent;
   _globals.type_tcp_gc = XtGetGC(_globals.draw, 
				GCForeground|GCBackground|GCFont, &xgcv);

   xgcv.foreground = _globals.mess_foreground;
   xgcv.background = _globals.mess_background;
   xgcv.font	   = _globals.mess_font->fid;
   _globals.mess_baseline = _globals.mess_font->max_bounds.ascent;
   _globals.mess_gc = XtGetGC(_globals.draw, GCForeground|GCBackground|GCFont, 
				&xgcv);

   XtAppMainLoop(_globals.appContext);
   exit(0);
}

void
read_file()
{
   FILE		*fi;
   char		buf[BUFSIZ];
   int		u_time, u_size;
   char		u_proto[4];
   TypeList	*type_list = NULL;
   int		t_type, t_size;
   register	rcount=0;

   _maxsize = 0;
   _maxtime = 0;

   fi = fopen(_file, "r");
   if(!fi){
      perror(_file);
      exit(1);
   }

   while(fgets(buf, BUFSIZ, fi)){
      _line ++;
      switch(buf[0]){
	 case 'R':
	    if(buf[1] != '-') continue;
	    rcount++;
	    if(type_list){
	       _update_list = add_update_to_list(strcmp(u_proto, "TCP") == 0,
		  u_time, u_size, type_list, _update_list);
	    }
	    if(sscanf(buf, "R- %d: %d b %s", &u_time, &u_size, u_proto) != 3){
	       fprintf(stderr, "%s:%d: syntax error\n", _file, _line);
	       continue;
	    }
	    if(u_time > _maxtime)
	       _maxtime = u_time;
	    if(u_size > _maxsize)
	       _maxsize = u_size;

	    type_list = NULL;
	    break;
	 case 'T':
	    if(buf[1] != '-') continue;
	    if(sscanf(buf, "T- %d %d", &t_type, &t_size) != 2){
	       fprintf(stderr, "%s:%d: syntax error\n", _file, _line);
	       return;
	    }
	    type_list = add_type_to_list(t_type, t_size, NULL, type_list);
	    break;
	 
	 case 'M':
	    {
	       char	*s;
	       if(buf[1] != '-') continue;
	       s = strrchr(buf, '\n');
	       if(s) *s = 0;
	       type_list = add_type_to_list(-1, -1, &buf[3], type_list);
	       break;
	    }

	 case 'W':
	    if(buf[1] != '-') continue;
	    /* TODO */
	    break;
	 default:
	    continue;
      }
   }
   if(_globals.max_size != 0)
      _maxsize = _globals.max_size;

   printf("read %d updates.\n", rcount);
   fclose(fi);
}

void 
do_configure(w, ev, params, num_params)

   Widget	w;
   XEvent	*ev;
   String	*params;
   Cardinal	*num_params;
{
   XtVaGetValues(_globals.draw,
      XtNwidth, &_globals.width,
      XtNheight, &_globals.height, NULL);
   XClearWindow(_globals.display, _globals.window);
}

void 
do_redraw(w, ev, params, num_params)

   Widget	w;
   XEvent	*ev;
   String	*params;
   Cardinal	*num_params;
{
   XExposeEvent         *expose = (XExposeEvent *)ev;
   if(expose->count != 0) return;
   redraw();
}

void
do_quit(w, ev, params, num_params)
   
   Widget	w;
   XEvent	*ev;
   String	*params;
   Cardinal	*num_params;

{
   summarize();
   exit(0);
}

void
do_page_up(w, ev, params, num_params)
   
   Widget	w;
   XEvent	*ev;
   String	*params;
   Cardinal	*num_params;

{
   register int	y;
   register UpdateElt	*u_e;

   y = 0;
   for(u_e=_page_start; u_e; u_e=u_e->prev){
      y = YSCALE * (u_e->time - _page_start->time);
      if(y <= -_globals.height && u_e != _page_start)
	 break;
   }
   if(u_e){
      if(u_e->next && u_e->next != _page_start)
	 _page_start = u_e->next;
      else
	 _page_start = u_e;
   }
   else
      _page_start = _update_list->head;
   XClearWindow(_globals.display, _globals.window);
   redraw();
}

void
do_page_down(w, ev, params, num_params)
   
   Widget	w;
   XEvent	*ev;
   String	*params;
   Cardinal	*num_params;
{
   register int	y;
   register UpdateElt	*u_e;

   y = 0;
   for(u_e=_page_start; u_e; u_e=u_e->next){
      y = YSCALE * (u_e->time - _page_start->time);
      if(y >= _globals.height && u_e != _page_start)
	 break;
   }
   if(u_e){
      if(u_e->prev && u_e->prev != _page_start)
	 _page_start = u_e->prev;
      else
	 _page_start = u_e;
   }
   else
      _page_start = _update_list->tail;
   XClearWindow(_globals.display, _globals.window);
   redraw();
}

void
do_incr(w, ev, params, num_params)
   
   Widget	w;
   XEvent	*ev;
   String	*params;
   Cardinal	*num_params;
{
   if(ev->xbutton.y > _globals.height/2)
      _page_start = _page_start->next?_page_start->next:_page_start;
   else
      _page_start = _page_start->prev?_page_start->prev:_page_start;
   XClearWindow(_globals.display, _globals.window);
   redraw();
}

void
do_toggle_pname(w, ev, params, num_params)
   
   Widget	w;
   XEvent	*ev;
   String	*params;
   Cardinal	*num_params;
{
   _globals.pname = !_globals.pname;
   XClearWindow(_globals.display, _globals.window);
   redraw();
}

void
redraw()
{
   register		tpos, num_mes;
   register UpdateElt	*u_e;
   register TypeElt	*t_e;
   double		xscale = (double)_maxsize/(_globals.width - STARTX-10);
   register		x,y, w, h, spacing, value;
   char			buf[32];
   GC			gc;
   int			baseline;
   XFontStruct		*font;

   spacing = YSCALE * TIMEUNIT;
   value = ((_page_start->time+TIMEUNIT-1)/TIMEUNIT) * TIMEUNIT;
   y = (value - _page_start->time) * YSCALE;


   while(y < _globals.height){
      sprintf(buf, "%d", value);
      XDrawLine(_globals.display, _globals.window, _globals.gc,
	 STARTX, y, _globals.width, y);
      XDrawString(_globals.display, _globals.window, _globals.gc,
	 0, y+_globals.baseline/2, buf, strlen(buf));
      y += spacing;
      value += TIMEUNIT;
   }

   foreachUpdate(u_e){
      
      y = YSCALE * u_e->time - YSCALE * _page_start->time;
      if(y >= _globals.height-_globals.type_baseline) break;
      h = _globals.type_baseline+2;
      w = u_e->size/xscale;

      x = STARTX;
      num_mes = 0;
      if(u_e->tcp){
	 gc = _globals.type_tcp_gc;
	 baseline = _globals.type_tcp_baseline;
	 font = _globals.type_tcp_font;
      }
      else {
	 gc = _globals.type_gc;
	 baseline = _globals.type_baseline;
	 font = _globals.type_font;
      }
      foreachType(u_e->type_list, t_e){
	 if(t_e->message){
	    XDrawString(_globals.display, _globals.window, _globals.mess_gc,
	       STARTX, 
	       y + _globals.mess_baseline + baseline +
		   _globals.baseline + num_mes * _globals.mess_baseline
		   + 2,
	       t_e->message, strlen(t_e->message));
	    num_mes ++;
	 }
	 else {
	    w = t_e->size/xscale;
	    XDrawRectangle(_globals.display, _globals.window, gc,
	       x, y, w, h);
	    if(_globals.pname)
	       sprintf(buf, "%s", ttostring(t_e->type));
	    else
	       sprintf(buf, "%d", t_e->type);
	    tpos = x+(w-XTextWidth(font, buf, strlen(buf)))/2;

	    XDrawString(_globals.display, _globals.window, gc,
	       tpos, y+baseline+1, buf, strlen(buf));

	    x += w;
	    if(x > _globals.width) break;
	 }
      }
      sprintf(buf, "%d", u_e->size);
      tpos = STARTX+(u_e->size/xscale - XTextWidth(_globals.font, buf,
	 strlen(buf)))/2;
      if(tpos > _globals.width - 50)
	 tpos = _globals.width - 50;
      XDrawString(_globals.display, _globals.window, gc,
	 tpos, y+2*baseline+2, buf, strlen(buf));
   }
}

char *
ttostring(t)

   int	t;
{
   switch(t){
      case SP_MESSAGE: return "MSG";
      case SP_PLAYER_INFO: return "PLYINFO";
      case SP_KILLS: return "KILLS";
      case SP_PLAYER: return "PL";
      case SP_TORP_INFO: return "TINFO";
      case SP_TORP: return "TORP";
      case SP_PHASER: return "PHASER";
      case SP_PLASMA_INFO: return "PLINFO";
      case SP_PLASMA: return "PLASMA";
      case SP_WARNING: return "WARN";
      case SP_MOTD: return "MOTD";
      case SP_YOU: return "YOU";
      case SP_QUEUE: return "QUEUE";
      case SP_STATUS: return "STATUS";
      case SP_PLANET: return "PLANET";
      case SP_PICKOK: return "PICK";
      case SP_LOGIN: return "LOGIN";
      case SP_FLAGS: return "FLAGS";
      case SP_MASK: return "MASK";
      case SP_PSTATUS: return "PSTAT";
      case SP_BADVERSION: return "BADV";
      case SP_HOSTILE: return "HOST";
      case SP_STATS: return "STATS";
      case SP_PL_LOGIN: return "PL_LOGIN";
      case SP_RESERVED: return "RESERVED";
      case SP_PLANET_LOC: return "PL_LOC";
      case SP_SCAN: return "SCAN";
      case SP_UDP_REPLY: return "UDPR";
      case SP_SEQUENCE: return "SEQ";
      case SP_SC_SEQUENCE: return "SC_SEQ";
      case SP_RSA_KEY: return "RSA_KEY";
      case SP_MOTD_PIC: return "MOTDP";
      case SP_S_REPLY: return "S_REPLY";
      case SP_S_MESSAGE: return "S_MSG";
      case SP_S_WARNING: return "S_WARN";
      case SP_S_YOU: return "S_YOU";
      case SP_S_YOU_SS: return "S_YOU_SS";
      case SP_S_PLAYER: return "S_PLY";
      case SP_PING: return "PING";
      case SP_S_TORP: return "S_TORP";
      case SP_S_TORP_INFO: return "S_TI";
      case SP_S_8_TORP: return "S_8T";
      case SP_S_PLANET: return "S_PL";
      case SP_FEATURE: return "FEA";
      case SP_BITMAP: return "BITM";

      case SP_S_SEQUENCE: return "P2_SEQ";
      case SP_S_PHASER: return "P2_PHAS";
      case SP_S_KILLS: return "P2_KILL";
      case SP_S_STATS: return "P2_STAT";

      default: 
	 fprintf(stderr, "unknown packet type %d\n", t);
	 return "UNKNOWN";
   }
}

void
summarize()
{
   register UpdateElt   *u_e;
   register TypeElt     *t_e;
   register		i, tbytes = 0, tpackets=0;
   static struct summary {
      int	packets;
      int	bytes;
   } types[512];

   foreachUpdate(u_e){
      foreachType(u_e->type_list, t_e){
	 if(t_e->type < 0) continue;	/* message */
	 if(t_e->type >= 512) {
	    fprintf(stderr, "whoops, too many types (%d)\n",
	       t_e->type);
	    exit(1);
	 }
	 types[t_e->type].packets ++;
	 types[t_e->type].bytes += t_e->size;
      }
   }

   for(i=0; i< 512; i++){
      if(types[i].packets){
	 printf("%17s: %10d packets for %10d bytes\n", 
	    ttostring(i), types[i].packets, types[i].bytes);
	 tbytes += types[i].bytes;
	 tpackets += types[i].packets;
      }
   }
   printf("%17s: %10d\n", "Total bytes", tbytes);
   printf("%17s: %10d\n", "Total packets", tpackets);
}
