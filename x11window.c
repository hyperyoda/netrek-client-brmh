/*
 * x11window.c
 * 
 * Kevin P. Smith  6/11/89 Much modified by Jerry Frain and Joe Young
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <sys/types.h>
#include <sys/time.h>
#ifdef SETTCPNODELAY
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif
#include <assert.h>
#include "netrek.h"

#define MAX_TEXT_WIDTH		90

#ifdef FONT_BITMAPS
# define MAXFONTS	FONTS+F_NUMFONTS+1
#else
# define MAXFONTS	FONTS+1
#endif

/* XFIX speedup */
#define MAXCACHE	128

/* changes too good to risk leaving out, by Richard Caley (rjc@cstr.ed.ac.uk) */
#define RJC
#define FOURPLANEFIX

#define NORMAL_FONT     "6x10"
#define MESG_FONT	"6x10"
#define BOLD_FONT       "-*-clean-bold-r-normal--10-100-75-75-c-60-*"
#define ITALIC_FONT     "-*-clean-bold-r-normal--10-100-75-75-c-60-*"
#define BIG_FONT        "-*-lucidatypewriter-*-*-*-*-40-*-*-*-*-*-*-*"
#define IND_FONT        "-*-clean-bold-r-normal--10-100-75-75-c-60-*"

#ifdef TTS
#define TTS_FONT	"-misc-fixed-bold-r-normal--15-*"
#endif

#define PtInBox(px, py, xl, yt, xr, yb) \
                                \
   (                            \
      ((int)px >= (int)xl) &&           \
      ((int)px <= (int)xr) &&           \
      ((int)py >= (int)yt) &&           \
      ((int)py <= (int)yb))


static char    *_nfonts[] =
{
   NORMAL_FONT,
   "-*-clean-medium-r-normal--10-100-75-75-c-60-*",
   "6x10",
   NULL,
};
static char    *_bfonts[] =
{
   BOLD_FONT,
   "-*-clean-bold-r-normal--10-100-75-75-c-60-*",
   "6x10",
   NULL,
};
static char    *_ifonts[] =
{
   ITALIC_FONT,
   "-*-clean-bold-r-normal--10-100-75-75-c-60-*",
   "6x10",
   NULL,
};
static char    *_bgfonts[] =
{
   BIG_FONT,
   "6x10",
   "6x10",
   NULL,
};

#define BITGC 4

#define WHITE   	0
#define BLACK   	1
#define RED     	2
#define GREEN   	3
#define YELLOW  	4
#define CYAN   		5
#define GREY		6
#define MAX_COLOR	7

static int      zero = 0;
static int      one = 1;
static int      two = 2;
static int      three = 3;
static int	four = 4;	/* not used */
static int      five = 5;

int 		W_FastClear = 0;
Display        *W_Display;
Window          W_Root;
Colormap        W_Colormap;
int             W_Screen;
#ifdef FOURPLANEFIX
Visual         *W_Visual;
#endif
W_Font          W_BigFont = (W_Font) & zero, W_RegularFont = (W_Font) & one;
W_Font          W_HighlightFont = (W_Font) & two, W_UnderlineFont = (W_Font) & three;
W_Font		W_MesgFont = (W_Font) &five;
W_Color         W_White = WHITE, W_Black = BLACK, W_Red = RED, W_Green = GREEN;
W_Color         W_Yellow = YELLOW, W_Cyan = CYAN, W_Grey = GREY;
int             W_Textwidth, W_Textheight,
		W_MesgTextwidth, W_MesgTextheight;

int             W_in_message = 0;	/* jfy -- for Jerry's warp message */

/* TTS: moved this out so we can use the 8th color */
static unsigned long   planes[3];

#define SCROLL_THUMB_WIDTH	5
static int	scrollbar = 1;
static int	scroll_thumb_width = SCROLL_THUMB_WIDTH;
static GC	scroll_thumb_gc;
static Pixmap	scroll_thumb_pixmap;
static int	scroll_lines = 100;	/* save 100 lines */

/* Setup for ICCCM delete window. */
Atom		wm_protocols,
		wm_delete_window;


#ifdef RJC
extern W_Window baseWin;
static XClassHint class_hint =
{
   "netrek", "Netrek",
};

static XWMHints wm_hint =
{
   InputHint | StateHint,
   True,
   NormalState,
   None,
   None,
   0, 0,
   None,
   None,
};

static XSizeHints wm_size_hint;
#endif				/* RJC */

static W_Event  W_myevent;
static int      W_isEvent = 0;

struct colors {
   char           	*name;
   GC              	contexts[MAXFONTS];
   Pixmap          	pixmap;
   int             	pixelValue;
};

struct icon {
   Window          window;
   Pixmap          bitmap;
   int             width, height;
   Pixmap          pixmap;
};

#define WIN_GRAPH	1
#define WIN_TEXT	2
#define WIN_MENU	3
#define WIN_SCROLL	4

struct window {
   Window          window;
   int             type;
   char           *data;
   int             mapped;
   int             width, height;
   char           *name;
};

struct scrollingWindow {
   int		   	lines;
   int			updated;
   int			topline;
   struct stringList	*head;
   struct stringList	*tail;
   struct stringList	*index;
};

struct stringList {
   char           	string[MAX_TEXT_WIDTH];
   W_Color         	color;
   W_Font		font;
   struct stringList 	*next, *prev;
};

struct menuItem {
   int		   column;
   char           *string;
   W_Color         color;
};

static
struct colors   colortable[] =
{
   {"white"},
   {"black"},
   {"red"},
   {"green"},
   {"yellow"},
   {"cyan"},
   {"light grey"},
   {"dummy"}			/* extensions */
};

struct windowlist {
   struct window  *window;
   struct windowlist *next;
};

#define HASHSIZE 29
#define hash(x) (((int) (x)) % HASHSIZE)

static struct windowlist *hashtable[HASHSIZE];

static struct fontInfo fonts[MAXFONTS];

static struct window   myroot;

/* Last entry reserved for extensions */
#define NCOLORS (sizeof(colortable)/sizeof(colortable[0]))-1
#define W_Void2Window(win) ((win) ? (struct window *) (win) : &myroot)
#define W_Window2Void(window) ((W_Window) (window))
#define W_Void2Icon(bit) ((struct icon *) (bit))
#define W_Icon2Void(bit) ((W_Icon) (bit))
#define W_GCListToVoid(gcl)	((W_GC *) (gcl))
#define fontNum(font) (*((int *) font))
#define TILESIDE 16

#define WIN_EDGE 5		/* border on l/r edges of text windows */
#define MENU_PAD 5		/* border on t/b edges of text windows */
#define MENU_BAR 2		/* width of menu bar */

static char     gray[] =
{
   0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,
   0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,
   0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,
   0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55
};

static char     striped[] =
{
   0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
   0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f,
   0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
   0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0
};

static char     solid[] =
{
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

#if __STDC__ || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

/* x11window.c */
static long WMXYHintMode_default P_((void));
static void scrollUp P_((struct window *win, int y));
static void scrollDown P_((struct window *win, int y));
static void scrollPosition P_((struct window *win, int y));
static void scrollTo P_((struct window *win, struct scrollingWindow *sw, int topline));
static void scrollScrolling P_((W_Event *wevent));
static void configureScrolling P_((struct window *win, int x, int y, int width, int height));
static int checkGeometry P_((char *name, int *x, int *y, int *width, int *height));
static void checkFont P_((XFontStruct *fontinfo, char *fontname));
static void setFontValues P_((W_Font font, XFontStruct *f_info));
static void GetFonts P_((void));
static XFontStruct *find_font P_((char *oldf, char **fonts, char **newfontname));
static void GetColors P_((void));
static void FlushClearAreaCache P_((Window win, int in));
static void FlushLineCache P_((Window win, int color));
static struct window *newWindow P_((Window window, int type));
static struct window *findWindow P_((Window window));
static void addToHash P_((struct window *win));
static void AddToScrolling P_((struct window *win, W_Color color, W_Font font, char *str, int len));
static void drawThumb P_((struct window *win, struct scrollingWindow *sw));
static void redrawScrolling P_((struct window *win));
static void redrawMenu P_((struct window *win));
static void redrawMenuItem P_((struct window *win, int n));
static void changeMenuItem P_((struct window *win, int col, int n, char *str, int len, W_Color color));
static void deleteWindow P_((struct window *window));
static void checkParent P_((char *name, W_Window *parent));
static void UpdateBackground P_((W_Window window));
static void UpdateBorder P_((W_Window window));
static void UpdateGeometry P_((W_Window window));

#undef P_

#define XDEBUG	0

#if XDEBUG
/* X debugging */
int 
_myerror(d, e)
    Display        *d;
    XErrorEvent    *e;
{
   abort();
}
#endif


static long 
WMXYHintMode_default()		/* BRM2.0pl18 */
{
   static int fetched = 0;
   static long WMXYHM_default;
   char *hm_default_string;

   if(!fetched)
   {  hm_default_string = getdefault("WMXYHintMode");
      if(!hm_default_string || strcmp(hm_default_string,"USPosition") == 0)
         WMXYHM_default = USPosition;
      else WMXYHM_default = PPosition;
      fetched = 1;
   }
   return WMXYHM_default;
}

static void
scrollUp(win, y)

   struct window	*win;
   int			y;
{
   struct scrollingWindow	*sw = (struct scrollingWindow *) win->data;
   int				savedlines = sw->lines - win->height;

   if(savedlines < 0) savedlines = 0;

   if(sw->topline + savedlines > 0){
      if(!sw->index) {
	 fprintf(stderr, "scroll error, NULL index (scrollUp).\n");
	 return;
      }
      sw->index = sw->index->next;
      sw->topline --;
      redrawScrolling(win);
   }
}

static void
scrollDown(win, y)

   struct window	*win;
   int			y;
{
   struct scrollingWindow	*sw = (struct scrollingWindow *) win->data;

   if(sw->topline < 0){
      if(!sw->index) {
	 fprintf(stderr, "scroll error, NULL index (scrollDown).\n");
	 return;
      }
      sw->index = sw->index->prev;
      sw->topline ++;
      redrawScrolling(win);
   }
}


static void
scrollPosition(win, y)

   struct window	*win;
   int			y;
{
   struct scrollingWindow	*sw = (struct scrollingWindow *) win->data;
   int				savedlines, maxrow, winheight,
				newtop;

   savedlines = sw->lines - win->height;
   if(savedlines < 0) savedlines = 0;
   maxrow = sw->lines < win->height ? sw->lines : win->height;
   winheight = win->height * W_MesgTextheight + MENU_PAD * 2;

   newtop = (y * (savedlines + maxrow + 1))/ winheight - savedlines;
   if(newtop < -savedlines)
      newtop = -savedlines;
   else if(newtop > 0)
      newtop = 0;
   scrollTo(win, sw, newtop);
}

static void
scrollTo(win, sw, topline)

   struct window		*win;
   struct scrollingWindow	*sw;
   int				topline;
{
   while(topline < sw->topline){
      if(!sw->index) {
	 fprintf(stderr, "scroll error, NULL index (1).\n");
	 break;
      }
      sw->index = sw->index->next;
      sw->topline --;
   }
   while(topline > sw->topline){
      if(!sw->index) {
	 fprintf(stderr, "scroll error, NULL index (2).\n");
	 break;
      }
      sw->index = sw->index->prev;
      sw->topline ++;
   }
   redrawScrolling(win);
}
   
static void
scrollScrolling(wevent)
   
   W_Event	*wevent;
{
   switch(wevent->key){
      case W_RBUTTON:
	 scrollUp(W_Void2Window(wevent->Window), wevent->y);
	 break;
      case W_LBUTTON:
	 scrollDown(W_Void2Window(wevent->Window), wevent->y);
	 break;
      case W_MBUTTON:
	 scrollPosition(W_Void2Window(wevent->Window), wevent->y);
	 break;
      default:
	 break;
   }
}

/* BRM2.00pl18 */
static void 
configureScrolling(win, x, y, width, height)

   struct window *win;
   int		 x, y;	/* TODO */
   int		 width, height;

{   
   int 			new_text_width, new_text_height;
   int 			new_real_width,new_real_height;
   XWindowAttributes	wa;
   int			sw = scrollbar ? scroll_thumb_width : 0;

#if 0
   /* XXX: can't shrink window */

   if(width <= win->width*W_MesgTextwidth+ WIN_EDGE*2 &&
      height <= win->height*W_MesgTextheight + MENU_PAD*2)
      return;
#endif

   XGetWindowAttributes(W_Display, win->window, &wa);

   new_text_width = (wa.width - WIN_EDGE*2 - sw)/W_MesgTextwidth;
   new_text_height = (wa.height - MENU_PAD*2)/W_MesgTextheight;

   if(new_text_width <= 0) new_text_width = 1;
   if(new_text_height <= 0) new_text_height = 1;
   if(new_text_width >= MAX_TEXT_WIDTH) new_text_width = MAX_TEXT_WIDTH-1;

   new_real_width = new_text_width*W_MesgTextwidth + WIN_EDGE*2 + sw;
   new_real_height = new_text_height*W_MesgTextheight + MENU_PAD*2;

   if(new_real_height != wa.height || new_real_width != wa.width){
      XResizeWindow(W_Display, win->window,new_real_width,new_real_height);
   }

   win->width = new_text_width;
   win->height = new_text_height;

   /* an expose event will follow a resize request, triggering 
      redrawScrolling */
}

/* BRM2.00pl18 */
/*****************************************************************************/
/*   Looks up any default geometry specified in the defaults file and        */
/*   returns the values found there.  Geometry should be of the form         */
/*        [=][<width>x<height>][{+-}<xoffset>{+-}<yoffset>]                  */
/*                                                                           */
/*   The result returned indicates which values were set.                    */
/*    XValue, YValue, WidthValue, HeightValue				     */
/*                                                                           */
/*****************************************************************************/

static int 
checkGeometry(name, x, y, width, height)
   char *name;
   int *x, *y, *width, *height;
{  
   char	buf[160], *geom_default;
   register	i;
   int		mask;

   sprintf(buf,"%s.geometry",name);
   geom_default = getdefault(buf);
#ifdef SHOW_DEFAULTS
   {
      char	df[80], desc[80];
      sprintf(df, "%dx%d%s%d%s%d", *width, *height,(*x<0?"":"+"),*x,
					(*y<0?"":"+"),*y);
      show_defaults("windows", buf, df, 
	 "Default window geometry (use 'xwininfo -tree' on the netrek window \n\
to get names.)");
   }
#endif
   if(!geom_default) return 0;                                /* nothing set */

   strcpy(buf, geom_default);

   /* Eat trailing spaces */
   for (i = strlen(buf) - 1; i && isspace(buf [i]); i--)
     buf [i] = '\0';

   mask = XParseGeometry(buf, x, y, (unsigned int *)width, 
					     (unsigned int *)height);
   if(!mask) {
      fprintf(stderr, "netrek: Geometry parse error on \"%s.geometry: %s\"\n",
	 name, buf);
   }
   return mask;
}

#ifdef SETTCPNODELAY
void
set_tcp_nodelay(v)

   int	v;
{
   int		   mi = v;
   if(setsockopt(ConnectionNumber(W_Display), IPPROTO_TCP,
	      TCP_NODELAY, (char *)&mi, sizeof(int)) < 0){
      /* harmless error
      perror("X socket tcp_nodelay failed.");
      */
   }
}
#endif

void 
W_Initialize(str)
    char           *str;
{
   int             i;
   static int	   init = 0;


#ifdef DEBUG
   printf("Initializing...\n");
#endif
   if(!init){
      for (i = 0; i < HASHSIZE; i++) {
	 hashtable[i] = NULL;
      }
      if ((W_Display = XOpenDisplay(str)) == NULL) {
	 extern char *getenv();
	 if(str)
	    fprintf(stderr, "netrek: Can't open display %s.\n", str);
	 else
	    fprintf(stderr, "netrek: Can't open display %s.\n", 
	       (getenv("DISPLAY")!=NULL)?getenv("DISPLAY"):"(none)");
	 exit(1);
      }
#ifdef SETTCPNODELAY
      set_tcp_nodelay(1);
#endif
   }

    /* Setup for ICCCM delete window. */
    wm_protocols = XInternAtom(W_Display, "WM_PROTOCOLS", False);
    wm_delete_window = XInternAtom(W_Display, "WM_DELETE_WINDOW", False);
      
#ifdef SHOW_DEFAULTS
   show_defaults("display", "scrollbar", scrollbar?"on":"off",
            "Scrollbars displayed on scrollable text windows.");
   show_defaults("display", "scrollSaveLines", "100",
            "Maximum lines saved in each text window.");
#endif

   /* display scroll thumb */
   scrollbar = booleanDefault("ScrollBar", scrollbar);
   scroll_lines = intDefault("ScrollSaveLines", scroll_lines);

#ifdef SHOW_DEFAULTS
   {
      char	buf[10];
      sprintf(buf, "%d", scroll_thumb_width);
   show_defaults("display", "scrollbarWidth", buf,
            "Scrollbars width. (Note that scroll control is allowed anywhere in the window.)");
   }
#endif
   scroll_thumb_width = intDefault("ScrollBarWidth", scroll_thumb_width);

#if XDEBUG
   XSynchronize(W_Display, True);
   XSetErrorHandler(_myerror); 
#endif

   W_Root = DefaultRootWindow(W_Display);
#ifdef FOURPLANEFIX
   W_Visual = DefaultVisual(W_Display, DefaultScreen(W_Display));
#endif
   W_Screen = DefaultScreen(W_Display);
   W_Colormap = DefaultColormap(W_Display, W_Screen);
   myroot.window = W_Root;
   myroot.type = WIN_GRAPH;
   GetFonts();
   GetColors();
   getColorDefs();

#ifdef TTS
   init_tts();
#endif

   init = 1;
}

void
W_CloseDisplay()
{
   if(W_Display) XCloseDisplay(W_Display);
}

/* after fork */
void
W_ChildCloseDisplay()
{
   close(ConnectionNumber(W_Display));
}

/*
 * Make sure the font will work, ie: that it fits in the 6x10 character
 * cell that we expect. (BRM)
 */
static void
checkFont (fontinfo, fontname)
     XFontStruct *fontinfo;
     char *fontname;
{
  if (fontinfo->max_bounds.width != 6 ||
      fontinfo->min_bounds.width != 6 ||
      fontinfo->descent + fontinfo->ascent != 10 ||
      fontinfo->min_bounds.lbearing < 0 ||
      fontinfo->max_bounds.rbearing > 6 ||
      fontinfo->max_bounds.ascent > 8 ||
      fontinfo->max_bounds.descent > 2) {
      fprintf (stderr, 
	 "netrek: Font '%s' \n", fontname);
      fprintf(stderr, 
	 "\tdoes not conform to 6x10 character cell rules.\n");
   }
}

static void
setFontValues(font, f_info)

   W_Font	font;
   XFontStruct	*f_info;
{
   struct fontInfo	*f;

   f = &fonts[fontNum(font)];

   f->baseline = f_info->max_bounds.ascent;
   f->width = f_info->max_bounds.width;
   f->height = f_info->max_bounds.descent + f_info->max_bounds.ascent;
   f->font = (void *) f_info;
}

static void
GetFonts()
{
   Font            regular, italic, bold, big, mesg;
   int             i;
   XGCValues       values;
   XFontStruct    *fontinfo;
   char           *fontname, *newfontname;

#ifdef FONT_BITMAPS
   F_init(fonts, FONTS);
#endif

   fontname = getdefault("font");
   if (fontname == NULL)
      fontname = NORMAL_FONT;
#ifdef SHOW_DEFAULTS
   show_defaults("fonts", "font", fontname,
      "Default font.");
#endif
   fontinfo = XLoadQueryFont(W_Display, fontname);
   if (fontinfo == NULL) {
      fontinfo = find_font(fontname, _nfonts, &newfontname);
      fontname = newfontname;
   }
   if (fontinfo == NULL) {
      fprintf(stderr, "netrek: Can't find any fonts!\n");
      exit(1);
   }
   checkFont(fontinfo, fontname);
   regular = fontinfo->fid;
   W_Textwidth = fontinfo->max_bounds.width;
   W_Textheight = fontinfo->max_bounds.descent + fontinfo->max_bounds.ascent;

   setFontValues(W_RegularFont, fontinfo);

   fontname = getdefault("boldfont");
   if (fontname == NULL)
      fontname = BOLD_FONT;
#ifdef SHOW_DEFAULTS
   show_defaults("fonts", "boldfont", fontname,
      "Bold font -- used for highlighting.");
#endif
   fontinfo = XLoadQueryFont(W_Display, fontname);
   if (fontinfo == NULL) {
      fontinfo = find_font(fontname, _bfonts, &newfontname);
      fontname = newfontname;
   }
   if (fontinfo == NULL) {
      bold = regular;

      setFontValues(W_HighlightFont, 
	 (XFontStruct *)fonts[fontNum(W_RegularFont)].font);
   } else {
      checkFont(fontinfo, fontname);
      bold = fontinfo->fid;

      setFontValues(W_HighlightFont, fontinfo);

      if (fontinfo->max_bounds.width > W_Textwidth)
	 W_Textwidth = fontinfo->max_bounds.width;
      if (fontinfo->max_bounds.descent + fontinfo->max_bounds.ascent > W_Textheight)
	 W_Textheight = fontinfo->max_bounds.descent + fontinfo->max_bounds.ascent;
   }

   fontname = getdefault("italicfont");
   if (fontname == NULL)
      fontname = ITALIC_FONT;
#ifdef SHOW_DEFAULTS
   show_defaults("fonts", "italicfont", fontname,
      "Italic font.");
#endif
   fontinfo = XLoadQueryFont(W_Display, fontname);
   if (fontinfo == NULL) {
      fontinfo = find_font(fontname, _ifonts, &newfontname);
      fontname = newfontname;
   }
   if (fontinfo == NULL) {
      italic = regular;

      setFontValues(W_UnderlineFont, 
	 (XFontStruct *)fonts[fontNum(W_RegularFont)].font);
   } else {
      checkFont(fontinfo, fontname);
      italic = fontinfo->fid;

      setFontValues(W_UnderlineFont, fontinfo);
      if (fontinfo->max_bounds.width > W_Textwidth)
	 W_Textwidth = fontinfo->max_bounds.width;
      if (fontinfo->max_bounds.descent + fontinfo->max_bounds.ascent > W_Textheight)
	 W_Textheight = fontinfo->max_bounds.descent + fontinfo->max_bounds.ascent;
   }

   fontname = getdefault("bigfont");
   if (fontname == NULL)
      fontname = BIG_FONT;
#ifdef SHOW_DEFAULTS
   show_defaults("fonts", "bigfont", fontname,
      "Large font -- used to display number of players per team.");
#endif
   fontinfo = XLoadQueryFont(W_Display, fontname);
   if (fontinfo == NULL) {
      fontinfo = find_font(fontname, _bgfonts, &newfontname);
      fontname = newfontname;
   }
   if (fontinfo == NULL) {
      big = regular;

      setFontValues(W_BigFont, 
	 (XFontStruct *)fonts[fontNum(W_RegularFont)].font);
   } else {
      big = fontinfo->fid;

      setFontValues(W_BigFont, fontinfo);
   }

   fontname = getdefault("mesgfont");
   if (fontname == NULL)
      fontname = MESG_FONT;
#ifdef SHOW_DEFAULTS
   show_defaults("fonts", "mesgfont", fontname,
      "Message and menu font.");
#endif
   fontinfo = XLoadQueryFont(W_Display, fontname);
   if (fontinfo == NULL) {
      fontinfo = find_font(fontname, _nfonts, &newfontname);
      fontname = newfontname;
   }
   if (fontinfo == NULL) {
      mesg = regular;

      setFontValues(W_MesgFont, 
	 (XFontStruct *)fonts[fontNum(W_RegularFont)].font);
   } else {
#if 0
      checkFont(fontinfo, fontname);
#endif
      mesg = fontinfo->fid;

      setFontValues(W_MesgFont, fontinfo);
      W_MesgTextwidth = fontinfo->max_bounds.width;
      W_MesgTextheight = fontinfo->max_bounds.descent + 
			    fontinfo->max_bounds.ascent;
   }

   for (i = 0; i < NCOLORS; i++) {
      values.font = big;
      if(colortable[i].contexts[0])
	 XFreeGC(W_Display, colortable[i].contexts[0]);
	 
      colortable[i].contexts[0] = XCreateGC(W_Display, W_Root, GCFont, &values);
      XSetGraphicsExposures(W_Display, colortable[i].contexts[0], False);
      values.font = regular;
      if(colortable[i].contexts[1])
	 XFreeGC(W_Display, colortable[i].contexts[1]);
      colortable[i].contexts[1] = XCreateGC(W_Display, W_Root, GCFont, &values);
      XSetGraphicsExposures(W_Display, colortable[i].contexts[1], False);
      XSetLineAttributes(W_Display, colortable[i].contexts[1],
			 2, LineSolid, CapNotLast, JoinMiter);
      values.font = bold;
      if(colortable[i].contexts[2])
	 XFreeGC(W_Display, colortable[i].contexts[2]);
      colortable[i].contexts[2] = XCreateGC(W_Display, W_Root, GCFont, &values);
      XSetGraphicsExposures(W_Display, colortable[i].contexts[2], False);
      values.font = italic;
      if(colortable[i].contexts[3])
	 XFreeGC(W_Display, colortable[i].contexts[3]);
      colortable[i].contexts[3] = XCreateGC(W_Display, W_Root, GCFont, &values);
      XSetGraphicsExposures(W_Display, colortable[i].contexts[3], False);
      if(i == W_Black)
	 values.function = GXcopy;
      else
	 values.function = GXor;

      if(colortable[i].contexts[BITGC])
	 XFreeGC(W_Display, colortable[i].contexts[BITGC]);
      colortable[i].contexts[BITGC] = XCreateGC(W_Display, W_Root, GCFunction,
						&values);
      XSetGraphicsExposures(W_Display, colortable[i].contexts[BITGC], False);
      {
	 static char     dl[] =
	 {1, 8};
	 /* was 3 */
	 XSetLineAttributes(W_Display, colortable[i].contexts[BITGC],
			    0, LineOnOffDash, CapButt, JoinMiter);
	 XSetDashes(W_Display, colortable[i].contexts[BITGC], 0, dl, 2);
      }
      values.font = mesg;
      if(colortable[i].contexts[5])
	 XFreeGC(W_Display, colortable[i].contexts[5]);
      colortable[i].contexts[5] = XCreateGC(W_Display, W_Root, GCFont, &values);
      XSetGraphicsExposures(W_Display, colortable[i].contexts[5], False);

#ifdef FONT_BITMAPS
      F_initGC(W_GCListToVoid(colortable[i].contexts), 5, fonts);
#endif
   }
}

static XFontStruct    *
find_font(oldf, fonts, newfontname)
    char           *oldf, **fonts, **newfontname;
{
   XFontStruct    *fi;
   char          **f;
   fprintf(stderr, "netrek: Can't find font %s.  Trying others...\n",
	   oldf);
   for (f = fonts; *f; f++) {
      if (strcmp(*f, oldf) != 0) {
	 if ((fi = XLoadQueryFont(W_Display, *f))){
	    *newfontname = *f;
	    return fi;
	 }
      }
   }
   return NULL;
}

#ifdef FOURPLANEFIX
static unsigned short extrared[8] =
{0x00, 0x20, 0x40, 0x60, 0x80, 0xa0, 0xb0, 0xc0};
static unsigned short extragreen[8] =
{0x40, 0x60, 0x80, 0xa0, 0xb0, 0xc0, 0x00, 0x20};
static unsigned short extrablue[8] =
{0x80, 0xa0, 0xb0, 0xc0, 0x00, 0x20, 0x40, 0x60};
#endif

int bw = 0;
int forceMono = 0;

static void
GetColors()
{
   XGCValues       values;
   int             i, j;
   XColor          foo;
   int             white, black;
   unsigned long   pixel;
   static long	   prev_pixel;
   char            defaultstring[100];
   char           *defaults;
#ifdef FOURPLANEFIX
   unsigned long   extracolors[8];
   XColor          colordef;
#endif

#ifdef SHOW_DEFAULTS
   show_defaults("display", "forceMono", forceMono?"on":"off",
      "Force black&white even if color supported.");
#endif
   forceMono = booleanDefault("forcemono", forceMono);	/* 11/14/91 TC */

   bw = 0;
   
   if (DisplayCells(W_Display, W_Screen) <= 2 || forceMono) {
      bw = 1;
      
      white = WhitePixel(W_Display, W_Screen);
      black = BlackPixel(W_Display, W_Screen);
      for (i = 0; i < NCOLORS; i++) {

	 if (i != W_Black) {
	    colortable[i].pixelValue = white;
	 } else {
	    colortable[i].pixelValue = black;
	 }
	 
	 /*
	  * We assume white is 0 or 1, and black is 0 or 1. We adjust
	  * graphics function based upon who is who.
	  */
	 if (white == 0 && i != W_Black) {	/* Black is 1 */
	    XSetFunction(W_Display, colortable[i].contexts[BITGC],
			 GXand);
	 }
      }
    } else if (DefaultVisual(W_Display, W_Screen)->class == TrueColor) {
/* Stuff added by sheldon@iastate.edu 5/28/93
 * This is supposed to detect a TrueColor display, and then do a lookup of
 * the colors in default colormap, instead of creating new colormap
 */
      for (i=0; i<NCOLORS; i++) {
	 sprintf(defaultstring, "color.%s", colortable[i].name);

	 defaults=getdefault(defaultstring);
	 if (defaults==NULL) defaults=colortable[i].name;
#ifdef SHOW_DEFAULTS
         show_defaults("display", defaultstring, defaults,
            "Color remapping.\n\
Format: color.<color name>: <color-spec>\n\
Color-spec can be a name [red] or #rgb [#f00, #ff0000]");
#endif
	 if(!XParseColor(W_Display, W_Colormap, defaults, &foo)){
	    fprintf(stderr, "netrek: Unknown color \"%s\"\n", defaults);
	 }
	 /* allow re-entry */
	 if(colortable[i].pixmap) continue;

	 XAllocColor(W_Display, W_Colormap, &foo);
	 colortable[i].pixelValue = foo.pixel;
      }
   }  else {
#ifdef FOURPLANEFIX
      if (!prev_pixel && 
	  !XAllocColorCells(W_Display, W_Colormap, False, planes, 3,
			    &prev_pixel, 1)) {
	 /* couldn't allocate 3 planes, make a new colormap */
	 W_Colormap = XCreateColormap(W_Display, W_Root, W_Visual, AllocNone);
	 if (!XAllocColorCells(W_Display, W_Colormap, False, planes, 3,
			       &pixel, 1)) {
	    fprintf(stderr, "netrek: Cannot create new colormap\n");
	    exit(1);
	 }
	 /*
	  * and fill it with at least 8 more colors so when mouse is inside
	  * netrek windows, use might be able to see his other windows
	  */
	 if (XAllocColorCells(W_Display, W_Colormap, False, NULL, 0,
			      extracolors, 8)) {
	    colordef.flags = DoRed | DoGreen | DoBlue;
	    for (i = 0; i < 8; i++) {
	       colordef.pixel = extracolors[i];
	       colordef.red = extrared[i] << 8;
	       colordef.green = extragreen[i] << 8;
	       colordef.blue = extrablue[i] << 8;
	       XStoreColor(W_Display, W_Colormap, &colordef);
	    }
	 }
      }
#else
      XAllocColorCells(W_Display, W_Colormap, False, planes, 3, &prev_pixel, 1);
#endif

      pixel = prev_pixel;
      for (i = 0; i < NCOLORS; i++) {
	 sprintf(defaultstring, "color.%s", colortable[i].name);

	 defaults = getdefault(defaultstring);
	 if (defaults == NULL)
	    defaults = colortable[i].name;
#ifdef SHOW_DEFAULTS
	 show_defaults("display", defaultstring, defaults,
	    "Color remapping.\n\
Format: color.<color name>: <color-spec>\n\
Color-spec can be a name [red] or #rgb [#f00, #ff0000]");
#endif
	 if(!XParseColor(W_Display, W_Colormap, defaults, &foo)){
	    fprintf(stderr, "netrek: Unknown color \"%s\"\n", defaults);
	 }
	 /*
	  * Black must be the color with all the planes off. That is the only
	  * restriction I concerned myself with in the following case
	  * statement.
	  */
	 switch (i) {
	 case WHITE:
	    foo.pixel = pixel | planes[0] | planes[1] | planes[2];
	    break;
	 case BLACK:
	    foo.pixel = pixel;
	    break;
	 case RED:
	    foo.pixel = pixel | planes[0];
	    break;
	 case CYAN:
	    foo.pixel = pixel | planes[1];
	    break;
	 case YELLOW:
	    foo.pixel = pixel | planes[2];
	    break;
	 case GREY:
	    foo.pixel = pixel | planes[0] | planes[1];
	    break;
	 case GREEN:
	    foo.pixel = pixel | planes[1] | planes[2];
	    break;
	 }
	 XStoreColor(W_Display, W_Colormap, &foo);
	 colortable[i].pixelValue = foo.pixel;
      }
   }
   for (i = 0; i < NCOLORS; i++) {
      for (j = 0; j < MAXFONTS; j++) {
	 XSetForeground(W_Display, colortable[i].contexts[j],
			colortable[i].pixelValue);
	 XSetBackground(W_Display, colortable[i].contexts[j],
            colortable[ i== W_Black ? W_White : W_Black].pixelValue);
      }
   }

   /*
      Assign pixmap values independent of display type so that
      "stippleBorder" works on all displays  (James Soutter 21/11/95)
   */
   
   for (i = 0; i < NCOLORS; i++) {
      if(colortable[i].pixmap){
	 XFreePixmap(W_Display, colortable[i].pixmap);
	 colortable[i].pixmap = 0;
      }
   }
	
   colortable[W_Red].pixmap = XCreatePixmapFromBitmapData(W_Display,
                 W_Root, striped, TILESIDE, TILESIDE,
                 colortable[W_Red].pixelValue,
                 colortable[W_Black].pixelValue,
                 DefaultDepth(W_Display, W_Screen));
              
   colortable[W_Yellow].pixmap = XCreatePixmapFromBitmapData(W_Display,
                 W_Root, gray, TILESIDE, TILESIDE,
                 colortable[W_Yellow].pixelValue,
                 colortable[W_Black].pixelValue,
                 DefaultDepth(W_Display, W_Screen));
		 
   colortable[W_Green].pixmap = XCreatePixmapFromBitmapData(W_Display,
                 W_Root, solid, TILESIDE, TILESIDE,
                 colortable[W_Green].pixelValue,
                 colortable[W_Black].pixelValue,
                 DefaultDepth(W_Display, W_Screen));

   /* extra stuff */

   /* grey bitmap */

   if(scroll_thumb_gc){
      XFreeGC(W_Display, scroll_thumb_gc);
      scroll_thumb_gc = 0;
   }
   if(scroll_thumb_pixmap){
      XFreePixmap(W_Display, scroll_thumb_pixmap);
      scroll_thumb_pixmap = 0;
   }
      
   if(scrollbar){

      scroll_thumb_pixmap = XCreatePixmapFromBitmapData(W_Display,
                 W_Root, gray, TILESIDE, TILESIDE,
                 colortable[W_White].pixelValue,
                 colortable[W_Black].pixelValue,
                 DefaultDepth(W_Display, W_Screen));

      values.fill_style = FillTiled;
      values.tile = scroll_thumb_pixmap;
						
      scroll_thumb_gc = XCreateGC(W_Display, W_Root, 
	GCFillStyle|GCTile, &values);
   }
}

void
W_TopWindowTitle(win)

   W_Window	win;
{
   struct window	*w = W_Void2Window(win);
   char           	*s;
   char            	buf[BUFSIZ];

#ifdef GATEWAY
   if(win == waitWin)
      strcpy(buf, serverNameRemote);
   else
      sprintf(buf, "Netrek  @  %s", serverNameRemote);
   s = buf;
#else
   if(win == waitWin)
      strcpy(buf, serverName);
   else
      sprintf(buf, "Netrek  @  %s", serverName);
   s = buf;
#endif
   /* but title on command line will override */

   if (title)
      s = title;

   XStoreName(W_Display, w->window, s);
}

W_Window 
W_MakeWindow(name, x, y, width, height, parent, border, color)
    char           *name;
    int             x, y, width, height;
    W_Window        parent;
    int             border;
    W_Color         color;
{
   struct window  *newwin;
   Window          wparent;
   XSetWindowAttributes attrs;
   int		   gr;

#ifdef DEBUG
   printf("New window...\n");
#endif

   gr = checkGeometry(name, &x, &y, &width, &height);
   checkParent(name, &parent);
   wparent = W_Void2Window(parent)->window;
   attrs.border_pixel = colortable[color].pixelValue;
   attrs.event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ExposureMask;

   if(strcmp(name, "message") == 0)		/* TSH */
      attrs.event_mask |= LeaveWindowMask;

#ifdef RJC
   if (strcmp(name, "netrek_icon") == 0)	/* hack, icon should not
						 * select for inpu t */
      attrs.event_mask = ExposureMask;
#endif				/* RJC */

   attrs.background_pixel = colortable[W_Black].pixelValue;
#ifdef RJC
   /* NOTE: CWDontPropagate seems to crash in OpenWindows */
   attrs.do_not_propagate_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ExposureMask;
#endif				/* RJC */
   newwin = newWindow(XCreateWindow(W_Display, wparent, x, y, width, height, border,
			    CopyFromParent, InputOutput, CopyFromParent,
			    CWBackPixel | CWEventMask | CWBorderPixel 
			    /* | CWDontPropagate*/,
			    &attrs),
			WIN_GRAPH);

   XStoreName(W_Display, newwin->window, name);
#ifdef RJC
   wm_size_hint.min_width =
      wm_size_hint.max_width = wm_size_hint.base_width = width;
   wm_size_hint.min_height =
      wm_size_hint.max_height = wm_size_hint.base_height = height;
   wm_size_hint.flags = USSize | PMinSize | PMaxSize | PBaseSize;
   if((gr & XValue) || (gr & YValue)){
      wm_size_hint.flags |= USPosition;
      wm_size_hint.x = x;
      wm_size_hint.y = y;
   }
   XSetWMNormalHints(W_Display, newwin->window, &wm_size_hint);

   class_hint.res_name = name;
   XSetClassHint(W_Display, newwin->window, &class_hint);
   XSetWMHints(W_Display, newwin->window, &wm_hint);
   if (wparent == W_Root && baseWin){
      /* XXX */
      if(strncmp(name, "wait", 4) != 0 &&
         strcmp(name, "Motd") != 0)
	 XSetTransientForHint(W_Display, newwin->window,
			      W_Void2Window(baseWin)->window);
   }
#endif				/* RJC */
   newwin->name = strdup(name);
   newwin->width = width;
   newwin->height = height;
   if (wparent != W_Root)
      if (W_CheckMapped(name))
	 W_MapWindow(W_Window2Void(newwin));

#ifdef DEBUG
   printf("New graphics window %d, child of %d\n", newwin, parent);
#endif

#ifdef FOURPLANEFIX
   XSetWindowColormap(W_Display, newwin->window, W_Colormap);
#endif

   return (W_Window2Void(newwin));
}

void 
W_ChangeBorder(window, color)
    W_Window        window;
    int             color;
{
#ifdef DEBUG
   printf("Changing border of %d\n", window);
#endif

   /* fix inexplicable color bug */
   if ((stippleBorder || bw) && colortable[color].pixmap){
      XSetWindowBorderPixmap(W_Display, W_Void2Window(window)->window,
			     colortable[color].pixmap);
   }
   else
      XSetWindowBorder(W_Display, W_Void2Window(window)->window,
		       colortable[color].pixelValue);


}

void 
W_MapWindow(window)
    W_Window        window;
{
   struct window  *win;

   win = W_Void2Window(window);
#ifdef DEBUG
   printf("Mapping %s\n", win->name);
#endif
   if (win->mapped)
      return;
   win->mapped = 1;
   XMapRaised(W_Display, win->window);
}

void 
W_UnmapWindow(window)
    W_Window        window;
{
   struct window  *win;

   win = W_Void2Window(window);
#ifdef DEBUG
   printf("UnMapping %s\n", win->name);
#endif
   if (win->mapped == 0)
      return;
   win->mapped = 0;
   XUnmapWindow(W_Display, win->window);
}

int 
W_IsMapped(window)
    W_Window        window;
{
   struct window  *win;

   win = W_Void2Window(window);
   if (win == NULL)
      return (0);
   return (win->mapped);
}

void 
W_FillArea(window, x, y, width, height, color)
    W_Window        window;
    int             x, y, width, height;
    W_Color         color;
{
   struct window  *win;

#ifdef DEBUG
   printf("Clearing (%d %d) x (%d %d) with %d on %d\n", x, y, width, height,
	  color, window);
#endif
   win = W_Void2Window(window);
   switch (win->type) {
   case WIN_GRAPH:
      XFillRectangle(W_Display, win->window, colortable[color].contexts[0],
		     x, y, width, height);
      break;
   case WIN_SCROLL:
      XFillRectangle(W_Display, win->window, colortable[color].contexts[0],
		     WIN_EDGE + x * W_MesgTextwidth, 
		     MENU_PAD + y * W_MesgTextheight,
		     width * W_MesgTextwidth, height * W_MesgTextheight);
      break;
   default:
      XFillRectangle(W_Display, win->window, colortable[color].contexts[0],
		     WIN_EDGE + x * W_Textwidth, MENU_PAD + y * W_Textheight,
		     width * W_Textwidth, height * W_Textheight);
   }
}

/* XFIX */

/* one cache for w, one for mapw */
static XRectangle _rcache[2][MAXCACHE];
static int      _rcache_index[2];

static void 
FlushClearAreaCache(win, in)
    Window          win;
    int		    in;
{
   XFillRectangles(W_Display, win, colortable[backColor].contexts[0],
		   _rcache[in], _rcache_index[in]);
   _rcache_index[in] = 0;
}

/* caches provided for local and mapw only */
void 
W_CacheClearArea(window, x, y, width, height)
    W_Window        window;
    int             x, y, width, height;
{
   Window          win = W_Void2Window(window)->window;
   register XRectangle *r;
   register int	   i = (window == w ? 0 : /* mapw */1);

   if (_rcache_index[i] == MAXCACHE)
      FlushClearAreaCache(win, i);

   r = &_rcache[i][_rcache_index[i]++];
   r->x = (short) x;
   r->y = (short) y;
   r->width = (unsigned short) width;
   r->height = (unsigned short) height;
}

void 
W_FlushClearAreaCache(window)
    W_Window        window;
{
   Window          win = W_Void2Window(window)->window;
   register int	   i = (window == w ? 0 : /* mapw */1);

   if (_rcache_index[i])
      FlushClearAreaCache(win, i);
}

/* XFIX: clears now instead of filling. */
void 
W_ClearArea(window, x, y, width, height)
    W_Window        window;
    int             x, y, width, height;
{
   struct window  *win;

#ifdef DEBUG
   printf("Clearing (%d %d) x (%d %d) on %d\n", x, y, width, height,
	  window);
#endif
   win = W_Void2Window(window);
   switch (win->type) {
   case WIN_GRAPH:
      /* XFIX: changed */
      XClearArea(W_Display, win->window, x, y, width, height, False);
      break;
   case WIN_SCROLL:
      XClearArea(W_Display, win->window, WIN_EDGE + x * W_MesgTextwidth,
		 MENU_PAD + y * W_MesgTextheight, 
		 width * W_MesgTextwidth, height * W_MesgTextheight, False);
      break;
   default:
      /* XFIX: changed */
      XClearArea(W_Display, win->window, WIN_EDGE + x * W_Textwidth,
		 MENU_PAD + y * W_Textheight, width * W_Textwidth, height * W_Textheight, False);
      break;
   }
}

void 
W_ClearWindow(window)
    W_Window        window;
{
#ifdef DEBUG
   printf("Clearing %d\n", window);
#endif
   if (window == NULL)
      return;
   XClearWindow(W_Display, W_Void2Window(window)->window);
}

int 
W_EventsPending()
{
   if (W_isEvent)
      return (1);
   while (XPending(W_Display)) {
      if (W_SpNextEvent(&W_myevent)) {
	 W_isEvent = 1;
	 return (1);
      }
   }
   return (0);
}

int
W_EventsQueued()
{
   return XEventsQueued(W_Display, QueuedAlready);
}

int
W_ReadEvents()
{
   XEvent		event;
static
   struct timeval  timeout = { 0, 0 };
   fd_set          readfds;

   FD_ZERO(&readfds);
   FD_SET(ConnectionNumber(W_Display), &readfds);
   if(select(max_fd, &readfds, 0, 0, &timeout) == 1){
      XPeekEvent(W_Display, &event);
      return 1;
   }
   return 0;
}

void 
W_NextEvent(wevent)
    W_Event        *wevent;
{
   if (W_isEvent) {
      *wevent = W_myevent;
      W_isEvent = 0;
      return;
   }
   while (W_SpNextEvent(wevent) == 0);
}

int 
W_SpNextEvent(wevent)
    W_Event        *wevent;
{
   XEvent		event;
   XKeyEvent		*key;
   XButtonEvent 	*button;
   XExposeEvent   	*expose;
   char			ch;
   struct window	*win;
#ifdef CONTROL_KEY
   register int 	control_key = 0;
#endif

#ifdef DEBUG
   printf("Getting an event...\n");
#endif
   key = (XKeyEvent *) & event;
   button = (XButtonEvent *) & event;
   expose = (XExposeEvent *) & event;

   for (;;) {
      XNextEvent(W_Display, &event);

      win = findWindow(key->window);
      if (win == NULL){
	 /*
	 printf("no window found (%d, local:%d)\n", key->window, 
						    ((struct window *)
						    w)->window);
	 */
	 return (0);
      }

#ifdef SPTEST
      spt_dupevent(&event);
#else
      if (key->send_event == True && event.type != ClientMessage) 	
                      /* event sent by another client */
	 return 0;
#endif

      if ((event.type == KeyPress || event.type == ButtonPress) &&
	  win->type == WIN_MENU) {
	 if (key->y % (W_Textheight + MENU_PAD * 2 + MENU_BAR) >=
	     W_Textheight + MENU_PAD * 2)
	    return (0);
	 key->y = key->y / (W_Textheight + MENU_PAD * 2 + MENU_BAR);
      }
      switch ((int) event.type) {
      case ClientMessage:
	 if(event.xclient.message_type == wm_protocols &&
            event.xclient.data.l[0] == wm_delete_window){
	    /* if non main window, close it, otherwise quit */
	    if((baseWin && win == W_Void2Window(baseWin)) || 
	       (waitWin && win == W_Void2Window(waitWin)) ||
	       (metaWin && win == W_Void2Window(metaWin))){

	       XCloseDisplay(W_Display);
	       exit(0);	
	    }
	    else{
	       W_UnmapWindow(W_Window2Void(win));
	    }
	 }
	 break;

      case LeaveNotify:	/* for message window -- jfy */
	 if (win == (struct window *) messagew) {
	    W_in_message = 0;
	 }
	 return (0);
	 break;
      case KeyPress:
#ifdef CONTROL_KEY
	 if (key->state & ControlMask) {
	    control_key = 1;
	    key->state = key->state & ~ControlMask;
	 }
	 else
	    control_key = 0;
#endif
	 if (XLookupString(key, &ch, 1, NULL, NULL) > 0){
	    wevent->type = W_EV_KEY;
	    wevent->Window = W_Window2Void(win);
	    wevent->x = key->x;
	    wevent->y = key->y;
#ifdef CONTROL_KEY
	    if (control_key)
	       wevent->key = ch + 96;
	    else
	       wevent->key = ch;
#else
	    wevent->key = ch;
#endif
        if (!messageon && !control_key && ((win == w) || (win == mapw)) && (me->p_status == PALIVE))
            if (ch >= 32 && ch < MAXKEY)
                if (mykeymap && (mykeymap[ch - 32] == 't'))
                    torprepeat = 1;
	    return (1);
	 }
	 return (0);
      case KeyRelease:
          if (XLookupString(key, &ch, 1, NULL, NULL) > 0)
              if (ch >= 32 && ch < MAXKEY)
                  if (mykeymap && (mykeymap[ch - 32] == 't'))
                      torprepeat = 0;
          return(0);
	 break;
      case ButtonPress:

	 wevent->type = W_EV_BUTTON;
	 wevent->Window = W_Window2Void(win);
	 wevent->x = button->x;
	 wevent->y = button->y;

	 switch (button->button & 0xf) {
	 case Button3:
	    wevent->key = W_RBUTTON;
	    break;
	 case Button1:
	    wevent->key = W_LBUTTON;
	    break;
	 case Button2:
	    wevent->key = W_MBUTTON;
	    break;
	    // make the additional mouse buttons work by faking
	    // shift and control presses
	 case Button4:
	    wevent->key = W_LBUTTON + W_SHIFTBUTTON;
	    break;
	 case Button5:
	    wevent->key = W_MBUTTON + W_SHIFTBUTTON;
	    break;
	 case 6:
	    wevent->key = W_RBUTTON + W_SHIFTBUTTON;
	    break;
	 case 7:
	    wevent->key = W_LBUTTON + W_CTRLBUTTON;
	    break;
	 case 8:
	    wevent->key = W_MBUTTON + W_CTRLBUTTON;
	    break;
	 case 9:
	    wevent->key = W_RBUTTON + W_CTRLBUTTON;
	    break;
	 case 10:
	    wevent->key = W_LBUTTON + W_CTRLBUTTON + W_SHIFTBUTTON;
	    break;
	 case 11:
	    wevent->key = W_MBUTTON + W_CTRLBUTTON + W_SHIFTBUTTON;
	    break;
	 case 12:
	    wevent->key = W_RBUTTON + W_CTRLBUTTON + W_SHIFTBUTTON;
	    break;
	 }
	 
	 if(extended_mouse){
	    if(button->state & ControlMask)
	       wevent->key += W_CTRLBUTTON;
	       
	    if(button->state & ShiftMask)
	       wevent->key += W_SHIFTBUTTON;
	 }

	 if(win->type == WIN_SCROLL){
	    scrollScrolling(wevent);
	    return 0;
	 }
	 
	 return 1;

	 
      case Expose:
	 if (expose->count != 0 /* xx */ && W_Window2Void(win) != mapw)
	    /* was return 0 */
	    continue;

	 switch(win->type){
	    case WIN_SCROLL:
	       configureScrolling(win, expose->x, expose->y, 
				    expose->width, expose->height);
	       redrawScrolling(win);
	       return (0);
	    
	    case WIN_MENU:
	       redrawMenu(win);
	       return (0);

	    default:
	       wevent->x = expose->x;
	       wevent->y = expose->y;
	       wevent->width = expose->width;		/* new -- tsh */
	       wevent->height = expose->height;	/* new -- tsh */
	       wevent->type = W_EV_EXPOSE;
	       wevent->Window = W_Window2Void(win);
	       break;
	 }
	 return (1);

      default:
	 return (0);
	 break;
      }
   }
}

void 
W_MakeLine(window, x0, y0, x1, y1, color)
    W_Window        window;
    int             x0, y0, x1, y1;
    W_Color         color;
{
   Window          win;

#ifdef DEBUG
   printf("Line on %d\n", window);
#endif
   win = W_Void2Window(window)->window;
   XDrawLine(W_Display, win, colortable[color].contexts[0], x0, y0, x1, y1);
}

void
W_MakePhaserLine(window, x0,y0, x1,y1, color)

   W_Window	window;
   int		x0,y0,x1,y1;
   W_Color	color;
{
   Window          win;
#ifdef DEBUG
   printf("Line on %d\n", window);
#endif
   win = W_Void2Window(window)->window;
   XDrawLine(W_Display, win, colortable[color].contexts[1], x0, y0, x1, y1);
}

/* XFIX */

static XSegment _lcache[NCOLORS][MAXCACHE];
static int      _lcache_index[NCOLORS];

static void 
FlushLineCache(win, color)
    Window          win;
    int             color;
{
   XDrawSegments(W_Display, win, colortable[color].contexts[0],
		 _lcache[color], _lcache_index[color]);
   _lcache_index[color] = 0;
}

/* for local window only */
void 
W_CacheLine(window, x0, y0, x1, y1, color)
    W_Window        window;
    int             x0, y0, x1, y1, color;
{
   Window          win = W_Void2Window(window)->window;
   register XSegment *s;

   if (_lcache_index[color] == MAXCACHE)
      FlushLineCache(win, color);

   s = &_lcache[color][_lcache_index[color]++];
   s->x1 = (short) x0;
   s->y1 = (short) y0;
   s->x2 = (short) x1;
   s->y2 = (short) y1;
}

void 
W_FlushLineCaches(window)
    W_Window        window;
{
   Window          win = W_Void2Window(window)->window;
   register        i;
   for (i = 0; i < NCOLORS; i++) {
      if (_lcache_index[i])
	 FlushLineCache(win, i);
   }
}

void 
W_MakeTractLine(window, x0, y0, x1, y1, color)
    W_Window        window;
    int             x0, y0, x1, y1;
    W_Color         color;
{
   Window          win;

#ifdef DEBUG
   printf("Line on %d\n", window);
#endif
   win = W_Void2Window(window)->window;
   XDrawLine(W_Display, win, colortable[color].contexts[BITGC], x0, y0, 
      x1, y1);
}

void 
W_WriteTriangle(window, x, y, s, t, color)
    W_Window        window;
    int             x, y, s;
    int             t;
    W_Color         color;
{
   struct window  *win = W_Void2Window(window);
   XPoint          points[4];

   if (t == 0) {
      points[0].x = x;
      points[0].y = y;
      points[1].x = x + s;
      points[1].y = y - s;
      points[2].x = x - s;
      points[2].y = y - s;
   } else {
      points[0].x = x;
      points[0].y = y;
      points[1].x = x + s;
      points[1].y = y + s;
      points[2].x = x - s;
      points[2].y = y + s;
   }

   /* for XDrawLines */
   points[3].x = points[0].x;
   points[3].y = points[0].y;


   if (fillTriangle)
      XFillPolygon(W_Display, win->window, colortable[color].contexts[0],
		   points, 3, Convex, CoordModeOrigin);
   else
      XDrawLines(W_Display, win->window, colortable[color].contexts[0],
		 points, 4, CoordModeOrigin);
}

void 
W_WriteText(window, x, y, color, str, len, font)
    W_Window        window;
    int             x, y, len;
    W_Color         color;
    W_Font          font;
    char           *str;
{
   struct window  *win;
   int             addr;

#ifdef DEBUG
   printf("Text for %d @ (%d, %d) in %d: [%s]\n", window, x, y, color, str);
#endif
   win = W_Void2Window(window);
   switch (win->type) {
   case WIN_GRAPH:
      addr = fonts[fontNum(font)].baseline;
      XDrawImageString(W_Display, win->window,
		       colortable[color].contexts[fontNum(font)],
		       x, y + addr, str, len);
      break;
   case WIN_SCROLL:
      AddToScrolling(win, color, font, str, len);
      
      break;
   case WIN_MENU:
      changeMenuItem(win, x, y, str, len, color);
      break;
   default:
      addr = fonts[fontNum(font)].baseline;
      XDrawImageString(W_Display, win->window,
		       colortable[color].contexts[fontNum(font)],
	     x * W_Textwidth + WIN_EDGE, MENU_PAD + y * W_Textheight + addr,
		       str, len);
      break;
   }
}

void 
W_MaskText(window, x, y, color, str, len, font)
    W_Window        window;
    int             x, y, len;
    W_Color         color;
    W_Font          font;
    char           *str;
{
   struct window  *win;
   int             addr;

   addr = fonts[fontNum(font)].baseline;
#ifdef DEBUG
   printf("TextMask for %d @ (%d, %d) in %d: [%s]\n", window, x, y, color, str);
#endif
   win = W_Void2Window(window);
   XDrawString(W_Display, win->window,
	  colortable[color].contexts[fontNum(font)], x, y + addr, str, len);
}

W_Icon 
W_StoreBitmap(width, height, data, window)
    int             width, height;
    W_Window        window;
    char           *data;
{
   struct icon    *newicon;
   struct window  *win;

#ifdef DEBUG
   printf("Storing bitmap for %d (%d x %d)\n", window, width, height);
   fflush(stdout);
#endif
   win = W_Void2Window(window);
   newicon = (struct icon *) malloc(sizeof(struct icon));
   newicon->width = width;
   newicon->height = height;
   newicon->bitmap = XCreateBitmapFromData(W_Display, win->window,
					   data, width, height);
#ifdef nodef
   /* XFIX: changed to Pixmap */
   white = WhitePixel(W_Display, W_Screen);
   black = BlackPixel(W_Display, W_Screen);
   newicon->bitmap = XCreatePixmapFromBitmapData(W_Display, W_Root, data,
						 width, height, white, black,
						 DefaultDepth(W_Display,
							      W_Screen));
#endif	/* nodef */

   newicon->window = win->window;
   newicon->pixmap = 0;
   return (W_Icon2Void(newicon));
}

void 
W_WriteBitmap(x, y, bit, color)
    int             x, y;
    W_Icon          bit;
    W_Color         color;
{
   struct icon    *icon;

   icon = W_Void2Icon(bit);
#ifdef DEBUG
   printf("Writing bitmap to %d\n", icon->window);
#endif
   XCopyPlane(W_Display, icon->bitmap, icon->window,
	 colortable[color].contexts[BITGC], 0, 0, icon->width, icon->height,
	      x, y, 1);

}


void 
W_TileWindow(window, bit)
    W_Window        window;
    W_Icon          bit;
{
   Window          win;
   struct icon    *icon;

#ifdef DEBUG
   printf("Tiling window %d\n", window);
#endif
   icon = W_Void2Icon(bit);
   win = W_Void2Window(window)->window;

   if (icon->pixmap == 0) {
      icon->pixmap = XCreatePixmap(W_Display, W_Root,
	      icon->width, icon->height, DefaultDepth(W_Display, W_Screen));
      XCopyPlane(W_Display, icon->bitmap, icon->pixmap,
	   colortable[W_White].contexts[0], 0, 0, icon->width, icon->height,
		 0, 0, 1);
   }
   XSetWindowBackgroundPixmap(W_Display, win, icon->pixmap);
   XClearWindow(W_Display, win);

   /*
    * if (icon->pixmap==0) { icon->pixmap=XMakePixmap(icon->bitmap,
    * colortable[W_White].pixelValue, colortable[W_Black].pixelValue); }
    * XChangeBackground(win, icon->pixmap); XClear(win);
    */
}

void 
W_UnTileWindow(window)
    W_Window        window;
{
   Window          win;

#ifdef DEBUG
   printf("Untiling window %d\n", window);
#endif
   win = W_Void2Window(window)->window;

   XSetWindowBackground(W_Display, win, colortable[W_Black].pixelValue);
   XClearWindow(W_Display, win);
}

W_Window 
W_MakeTextWindow(name, x, y, width, height, parent, border)
    char           *name;
    int             x, y, width, height;
    W_Window        parent;
    int             border;
{
   struct window  *newwin;
   Window          wparent;
   XSetWindowAttributes attrs;
   int		   gr;

#ifdef DEBUG
   printf("New window...\n");
#endif
   gr = checkGeometry(name, &x, &y, &width, &height);
   checkParent(name, &parent);
   attrs.border_pixel = colortable[W_White].pixelValue;
   attrs.event_mask = ExposureMask;
   attrs.background_pixel = colortable[W_Black].pixelValue;
#ifdef RJC
   /* NOTE: CWDontPropagate seems to crash in OpenWindows */
   attrs.do_not_propagate_mask = ExposureMask;
#endif				/* RJC */
   wparent = W_Void2Window(parent)->window;
   newwin = newWindow( XCreateWindow(W_Display, wparent, x, y,
   width * W_Textwidth + WIN_EDGE * 2, MENU_PAD * 2 + height * W_Textheight,
			border, CopyFromParent, InputOutput, CopyFromParent,
				      CWBackPixel | CWEventMask |
				      CWBorderPixel /*| CWDontPropagate*/,
				      &attrs), WIN_TEXT);
#ifdef RJC
   class_hint.res_name = name;
   XSetClassHint(W_Display, newwin->window, &class_hint);
   XSetWMHints(W_Display, newwin->window, &wm_hint);
   if (wparent == W_Root && baseWin != NULL) {
      XSetTransientForHint(W_Display, newwin->window,
			   W_Void2Window(baseWin)->window);
   }
#endif				/* RJC */
   XStoreName(W_Display, newwin->window, name);

   if((gr & XValue) || (gr & YValue)){
      wm_size_hint.flags = USPosition;
      wm_size_hint.x = x;
      wm_size_hint.y = y;
      XSetWMNormalHints(W_Display, newwin->window, &wm_size_hint);
   }

   newwin->name = strdup(name);
   newwin->width = width;
   newwin->height = height;
   if (wparent != W_Root)
      if (W_CheckMapped(name))
	 W_MapWindow(W_Window2Void(newwin));
#ifdef DEBUG
   printf("New text window %d, child of %d\n", newwin, parent);
#endif
#ifdef FOURPLANEFIX
   XSetWindowColormap(W_Display, newwin->window, W_Colormap);
#endif
   return (W_Window2Void(newwin));
}

static struct window  *
newWindow(window, type)
    Window          window;
    int             type;
{
   struct window  *newwin;

   /* new -- tsh */
   (void) XSetWMProtocols (W_Display, window, &wm_delete_window, 1);

   newwin = (struct window *) malloc(sizeof(struct window));
   newwin->window = window;
   newwin->type = type;
   newwin->mapped = 0;
   addToHash(newwin);
   return (newwin);
}

static struct window  *
findWindow(window)
    Window          window;
{
   struct windowlist *entry;
   entry = hashtable[hash(window)];
   while (entry != NULL) {
      if (entry->window->window == window)
	 return (entry->window);
      entry = entry->next;
   }
   return (NULL);
}

static void
addToHash(win)
    struct window  *win;
{
   struct windowlist **new;

#ifdef DEBUG
   printf("Adding to %d\n", hash(win->window));
#endif
   new = &hashtable[hash(win->window)];
   while (*new != NULL) {
      new = &((*new)->next);
   }
   *new = (struct windowlist *) malloc(sizeof(struct windowlist));
   (*new)->next = NULL;
   (*new)->window = win;
}

W_Window 
W_MakeScrollingWindow(name, x, y, width, height, parent, border)
    char           *name;
    int             x, y, width, height;
    W_Window        parent;
    int             border;
{
   struct window  *newwin;
   Window          wparent;
   XSetWindowAttributes attrs;
   XSizeHints     *sz_hints;
   int		   gcheck_result;
   struct scrollingWindow	*sw;
   int		   scw = (scrollbar ? scroll_thumb_width : 0);

#ifdef DEBUG
   printf("New window...\n");
#endif
   gcheck_result = checkGeometry(name, &x, &y, &width, &height);
   checkParent(name, &parent);
   wparent = W_Void2Window(parent)->window;
   attrs.border_pixel = colortable[W_White].pixelValue;
   attrs.event_mask = ExposureMask | ButtonPressMask;
   attrs.background_pixel = colortable[W_Black].pixelValue;

#ifdef RJC
   /* NOTE: CWDontPropagate seems to crash in OpenWindows */
   attrs.do_not_propagate_mask = ExposureMask;
#endif				/* RJC */
   newwin = newWindow(
			XCreateWindow(W_Display, wparent, x, y,
   width * W_MesgTextwidth + WIN_EDGE * 2 + scw,
			MENU_PAD * 2 + height * W_MesgTextheight,
			border, CopyFromParent, InputOutput, CopyFromParent,
				      CWBackPixel | CWEventMask |
				      CWBorderPixel /*| CWDontPropagate*/,
				      &attrs), WIN_SCROLL);
   class_hint.res_name=name;
   sz_hints = XAllocSizeHints();
   sz_hints->width_inc = W_MesgTextwidth;
   sz_hints->height_inc = W_MesgTextheight;
   sz_hints->min_width = WIN_EDGE*2 + W_MesgTextwidth + scw;
   sz_hints->min_height = MENU_PAD*2 + W_MesgTextheight;
   sz_hints->base_width = WIN_EDGE*2 + scw;
   sz_hints->base_height = MENU_PAD*2;
   sz_hints->flags = PResizeInc | PMinSize | PBaseSize;
   if(gcheck_result & XValue || gcheck_result & YValue)
      sz_hints->flags |= WMXYHintMode_default();
   XStoreName(W_Display,newwin->window,name);
   XSetWMNormalHints(W_Display,newwin->window,sz_hints);
   XFree((void *) sz_hints);
   XSetClassHint(W_Display, newwin->window, &class_hint);
   XSetWMHints(W_Display, newwin->window, &wm_hint);
   if (wparent==W_Root && baseWin != NULL)
      XSetTransientForHint(W_Display, newwin->window,W_Void2Window(baseWin)->window);

   newwin->name = strdup(name);
   sw = (struct scrollingWindow *) malloc(sizeof(struct scrollingWindow));
   sw->lines = 0;
   sw->updated = 0;
   sw->head = sw->tail = sw->index = NULL;
   sw->topline = 0;
   newwin->data = (char *) sw;
   newwin->width = width;
   newwin->height = height;
   if (wparent != W_Root)
      if (W_CheckMapped(name))
	 W_MapWindow(W_Window2Void(newwin));
#ifdef DEBUG
   printf("New scroll window %d, child of %d\n", newwin, parent);
#endif
#ifdef FOURPLANEFIX
   XSetWindowColormap(W_Display, newwin->window, W_Colormap);
#endif
   return (W_Window2Void(newwin));
}

/*
 * Add a string to the string list of the scrolling window.
 */
static void
AddToScrolling(win, color, font, str, len)
    struct window  *win;
    W_Color         color;
    W_Font	    font;
    char           *str;
    int             len;
{
   struct scrollingWindow	*sw;
   struct stringList 		*new;

   /* simple, fast */

   sw = (struct scrollingWindow *) win->data;
   if(sw->lines > scroll_lines /* some large number */){
      /* resuse tail */
      new = sw->tail;
      sw->tail = new->prev;
      new->prev->next = NULL;
      new->prev = NULL;
      new->next = sw->head;
      sw->head->prev = new;
      sw->head = new;
   }
   else {
      new = (struct stringList *) malloc(sizeof(struct stringList));
      new->next = sw->head;
      new->prev = NULL;
      if(sw->head)
	 sw->head->prev = new;
      sw->head = new;
      if(!sw->tail)
	 sw->tail = new;
      sw->lines ++;
      /*
      printf("adding one line \"%s\".\n", str);
      */
   }
   sw->index = sw->head;	/* input forces to end of list */
   sw->topline = 0;

   sw->updated ++;	/* mark for W_FlushScrollingWindow */

   strncpy(new->string, str, MAX_TEXT_WIDTH-1);
   new->color = color;
   new->font = font;

   if(len >= MAX_TEXT_WIDTH){
      new->string[MAX_TEXT_WIDTH-1] = 0;
   }
   else{
      /* we pad out the string with spaces so we don't have to clear
         the window */
      memset(&new->string[len], ' ', MAX_TEXT_WIDTH-len-1);
      new->string[MAX_TEXT_WIDTH-1] = 0;
   }
}

void
W_FlushScrollingWindow(window)

   W_Window	window;
{
   struct window		*win = W_Void2Window(window);
   struct scrollingWindow	*sw;
   if(!win->mapped)
      return;
   if(win->type != WIN_SCROLL){
      fprintf(stderr, "bad window type in W_FlushScrollingWindow.\n");
      return;
   }
   sw = (struct scrollingWindow *) win->data;
   if(!sw->updated)
      return;
#ifndef NO_COPYAREA
   else {
      register struct stringList	*item;
      register			y;
   
      if(win->height > sw->updated){
	 XCopyArea(W_Display, win->window, win->window, 
	    colortable[W_White].contexts[0], 
	    WIN_EDGE, MENU_PAD + sw->updated * W_MesgTextheight,
	    win->width * W_MesgTextwidth, 
	    (win->height - sw->updated)*W_MesgTextheight,
	       WIN_EDGE, MENU_PAD);
      }


      y = (win->height -1) * W_MesgTextheight + fonts[5].baseline;

      for(item = sw->index; item && y > 0 && sw->updated; item=item->next, 
						      y -= W_MesgTextheight,
						      sw->updated -- ){
	 XDrawImageString(W_Display, win->window,
		colortable[item->color].contexts[fontNum(item->font)],
			     WIN_EDGE, MENU_PAD + y, item->string, 
			     win->width);
      }
      sw->updated = 0;
      if(scrollbar)
	 drawThumb(win, sw);
   }
#else
   redrawScrolling(win);
#endif
}

static void
drawThumb(win, sw)

   struct window		*win;
   struct scrollingWindow	*sw;
{
   register	x, y, h;
   int		savedlines, maxrow,
		thumbTop, thumbHeight, totalHeight,
		winheight;

/*
   savedlines : Number of offscreen text lines,
      sw->lines - win->height

   maxrow + 1 : Number of onscreen  text lines,
      
      min(sw->lines + 1, win->height+1)

   sw->topline    : -Number of lines above the last max_row+1 lines

        thumbTop    = screen->topline + screen->savedlines;
        thumbHeight = screen->max_row + 1;
        totalHeight = thumbHeight + screen->savedlines;

        XawScrollbarSetThumb(scrollWidget,
         ((float)thumbTop) / totalHeight,
         ((float)thumbHeight) / totalHeight);
*/

   savedlines = sw->lines - win->height;
   if(savedlines < 0) savedlines = 0;
   maxrow = sw->lines < win->height ? sw->lines : win->height;
   winheight = win->height * W_MesgTextheight + MENU_PAD * 2;

   thumbTop = sw->topline + savedlines;
   thumbHeight = maxrow + 1;
   totalHeight = thumbHeight + savedlines;

   x = win->width * W_MesgTextwidth + WIN_EDGE * 2;
   y = winheight * thumbTop / totalHeight;
   h = winheight * thumbHeight / totalHeight;
	 
   XClearArea(W_Display, win->window, x, 0, scroll_thumb_width, winheight,
      False);
   XFillRectangle(W_Display, win->window, scroll_thumb_gc,
      x, y, 
      scroll_thumb_width, h);
   XDrawLine(W_Display, win->window, colortable[W_Red].contexts[0], 
	     x, 0, x, winheight);
}

static void
redrawScrolling(win)
    struct window  *win;
{
   int             		y;
   struct scrollingWindow	*sw;
   register struct stringList	*item;

   if(!win->mapped)
      return;

   /* simple, fast */

   sw = (struct scrollingWindow *)win->data;
   if(!sw->lines) return;
   sw->updated = 0;

   y = (win->height -1) * W_MesgTextheight + fonts[5].baseline;
   for(item = sw->index; item && y > 0; item = item->next, y -= W_MesgTextheight){
      XDrawImageString(W_Display, win->window,
			  colortable[item->color].contexts[fontNum(item->font)],
			  WIN_EDGE, MENU_PAD + y, item->string, 
			  win->width);
   }
   if(scrollbar)
      drawThumb(win, sw);
}

W_Window 
W_MakeMenu(name, x, y, width, height, parent, border)
    char           *name;
    int             x, y, width, height;
    W_Window        parent;
    int             border;
{
   struct window  *newwin;
   struct menuItem *items;
   Window          wparent;
   int             i;
   XSetWindowAttributes attrs;
   int		   gr;

#ifdef DEBUG
   printf("New window...\n");
#endif
   gr = checkGeometry(name, &x, &y, &width, &height);
   checkParent(name, &parent);
   wparent = W_Void2Window(parent)->window;
   attrs.border_pixel = colortable[W_White].pixelValue;
   attrs.event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ExposureMask;
   attrs.background_pixel = colortable[W_Black].pixelValue;
#ifdef RJC
   /* NOTE: CWDontPropagate seems to crash in OpenWindows */
   attrs.do_not_propagate_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ExposureMask;
#endif				/* RJC */
   newwin = newWindow(
			XCreateWindow(W_Display, wparent, x, y,
				      width * W_Textwidth + WIN_EDGE * 2,
   height * (W_Textheight + MENU_PAD * 2) + (height - 1) * MENU_BAR, border,
				CopyFromParent, InputOutput, CopyFromParent,
				      CWBackPixel | CWEventMask |
				      CWBorderPixel /*| CWDontPropagate*/,
				      &attrs),
			WIN_MENU);
#ifdef RJC
   class_hint.res_name = name;
   XSetClassHint(W_Display, newwin->window, &class_hint);
   XSetWMHints(W_Display, newwin->window, &wm_hint);
   if (wparent == W_Root && baseWin != NULL) {
      XSetTransientForHint(W_Display, newwin->window,
			   W_Void2Window(baseWin)->window);
   }
#endif				/* RJC */
   XStoreName(W_Display, newwin->window, name);
   if((gr & XValue) || (gr & YValue)){
      wm_size_hint.flags = USPosition;
      wm_size_hint.x = x;
      wm_size_hint.y = y;
      XSetWMNormalHints(W_Display, newwin->window, &wm_size_hint);
   }
   newwin->name = strdup(name);
   items = (struct menuItem *) malloc(height * sizeof(struct menuItem));
   for (i = 0; i < height; i++) {
      /* new: allocate once and reuse -tsh */
      items[i].column = 0;
      items[i].string = (char *)malloc(MAX_TEXT_WIDTH);	
      items[i].color = W_White;
   }
   newwin->data = (char *) items;
   newwin->width = width;
   newwin->height = height;
   if (wparent != W_Root)
      if (W_CheckMapped(name))
	 W_MapWindow(W_Window2Void(newwin));
#ifdef DEBUG
   printf("New menu window %d, child of %d\n", newwin, parent);
#endif
#ifdef FOURPLANEFIX
   XSetWindowColormap(W_Display, newwin->window, W_Colormap);
#endif
   return (W_Window2Void(newwin));
}

static void
redrawMenu(win)
    struct window  *win;
{
   int             count;

   for (count = 1; count < win->height; count++) {
      XFillRectangle(W_Display, win->window,
		     colortable[W_White].contexts[0],
	  0, count * (W_Textheight + MENU_PAD * 2) + (count - 1) * MENU_BAR,
		     win->width * W_Textwidth + WIN_EDGE * 2, MENU_BAR);
   }

   for (count = 0; count < win->height; count++) {
      redrawMenuItem(win, count);
   }
}

static void
redrawMenuItem(win, n)
    struct window  *win;
    int             n;
{
   struct menuItem *items;

   if(n >= win->height) return; /* jkelley's bug */

   items = (struct menuItem *) win->data;
   XFillRectangle(W_Display, win->window,
		  colortable[W_Black].contexts[0],
	  WIN_EDGE, n * (W_Textheight + MENU_PAD * 2 + MENU_BAR) + MENU_PAD,
		  win->width * W_Textwidth, W_Textheight+2);

   if (items[n].string) {
      XDrawString(W_Display, win->window,
		       colortable[items[n].color].contexts[1],
		       WIN_EDGE + W_Textwidth * items[n].column,
		       n * (W_Textheight + MENU_PAD * 2 + MENU_BAR) + MENU_PAD + fonts[1].baseline,
		       items[n].string, strlen(items[n].string));
   }
}

static void
changeMenuItem(win, col, n, str, len, color)
    struct window  *win;
    int		    col;
    int             n;
    char           *str;
    int             len;
    W_Color         color;
{
   struct menuItem *items;

   items = (struct menuItem *) win->data;

   if(n >= win->height) return; /* jkelley's bug */

#ifdef nodef
   if (items[n].string) {
      free(items[n].string);
   }
   news = malloc(len + 1);
   strncpy(news, str, len);
   news[len] = 0;
   items[n].string = news;
   items[n].color = color;
#endif
   
   strncpy(items[n].string, str, MAX_TEXT_WIDTH-1);
   items[n].string[MAX_TEXT_WIDTH-1] = 0;
   items[n].color = color;
   items[n].column = col;
   redrawMenuItem(win, n);
}

#ifdef TCURSORS

void 
W_DefineCursorFromBitmap(window, mapbits, width, height, maskbits, maskwidth, maskheight)
    W_Window        window;
    unsigned char  *mapbits;
    int             width, height;
    unsigned char  *maskbits;
    int             maskwidth, maskheight;
{
   Cursor          new;
   Pixmap          Cursormaskbit;
   Pixmap          Cursorbit;
   struct window  *win = W_Void2Window(window);
   static XColor   f, b;

   f.pixel = colortable[W_White].pixelValue;
   b.pixel = colortable[W_Black].pixelValue;

   XQueryColor(W_Display, W_Colormap, &f);
   XQueryColor(W_Display, W_Colormap, &b);

   Cursorbit = XCreateBitmapFromData(W_Display, win->window, 
				     mapbits, width, height);
   Cursormaskbit = XCreateBitmapFromData(W_Display, 
				      win->window, maskbits, 
				      maskwidth, maskheight);
   new = XCreatePixmapCursor(W_Display, Cursorbit, Cursormaskbit,
			     &b, &f, 5, 5);
   XRecolorCursor(W_Display, new, &f, &b);
   XDefineCursor(W_Display, win->window, new);
}

void 
W_DefineTCrossCursor(window)
    W_Window        window;
{
static
   Cursor          new;
   struct window  *win = W_Void2Window(window);
   static
   XColor          f, b;

   if(new){
      XDefineCursor(W_Display, win->window, new);
      return;
   }

   f.pixel = colortable[W_White].pixelValue;
   b.pixel = colortable[W_Black].pixelValue;

   XQueryColor(W_Display, W_Colormap, &f);
   XQueryColor(W_Display, W_Colormap, &b);

   new = XCreateFontCursor(W_Display, XC_tcross);
   XRecolorCursor(W_Display, new, &f, &b);
   XDefineCursor(W_Display, win->window, new);
}

void 
W_DefineTrekCursor(window)
    W_Window        window;
{
static
   Cursor          new;
   struct window  *win = W_Void2Window(window);
   XColor          f, b;

   if(new){
      XDefineCursor(W_Display, win->window, new);
      return;
   }

   f.pixel = colortable[W_Yellow].pixelValue;
   b.pixel = colortable[W_Black].pixelValue;

   XQueryColor(W_Display, W_Colormap, &f);
   XQueryColor(W_Display, W_Colormap, &b);

   new = XCreateFontCursor(W_Display, XC_trek);
   XRecolorCursor(W_Display, new, &f, &b);
   XDefineCursor(W_Display, win->window, new);
}

void 
W_DefineWarningCursor(window)
    W_Window        window;
{
static
   Cursor          new;
   struct window  *win = W_Void2Window(window);
   XColor          f, b;

   if(new){
      XDefineCursor(W_Display, win->window, new);
      return;
   }

   f.pixel = colortable[W_Red].pixelValue;
   b.pixel = colortable[W_Black].pixelValue;

   XQueryColor(W_Display, W_Colormap, &f);
   XQueryColor(W_Display, W_Colormap, &b);

   new =  XCreateFontCursor(W_Display, XC_pirate);
   XRecolorCursor(W_Display, new, &f, &b);
   XDefineCursor(W_Display, win->window, new);
}

void 
W_DefineArrowCursor(window)
    W_Window        window;
{
static
   Cursor          new;
   struct window  *win = W_Void2Window(window);
   XColor          f, b;

   if(new){
      XDefineCursor(W_Display, win->window, new);
      return;
   }

   f.pixel = colortable[W_Black].pixelValue;
   b.pixel = colortable[W_White].pixelValue;

   XQueryColor(W_Display, W_Colormap, &f);
   XQueryColor(W_Display, W_Colormap, &b);

   new = XCreateFontCursor(W_Display, XC_left_ptr);
   XRecolorCursor(W_Display, new, &f, &b);
   XDefineCursor(W_Display, win->window, new);
}

void 
W_DefineTextCursor(window)
    W_Window        window;
{
static
   Cursor          new;
   struct window  *win = W_Void2Window(window);
   XColor          f, b;

   if(new){
      XDefineCursor(W_Display, win->window, new);
      return;
   }

   f.pixel = colortable[W_Yellow].pixelValue;
   b.pixel = colortable[W_Black].pixelValue;

   XQueryColor(W_Display, W_Colormap, &f);
   XQueryColor(W_Display, W_Colormap, &b);

   new = XCreateFontCursor(W_Display, XC_xterm);
   XRecolorCursor(W_Display, new, &f, &b);
   XDefineCursor(W_Display, win->window, new);
}

void 
W_DefineWaitCursor(window)
    W_Window        window;
{
static
   Cursor          new;
   struct window  *win = W_Void2Window(window);
   static
   XColor          f, b;

   if(new){
      XDefineCursor(W_Display, win->window, new);
      return;
   }

   f.pixel = colortable[W_White].pixelValue;
   b.pixel = colortable[W_Black].pixelValue;

   XQueryColor(W_Display, W_Colormap, &f);
   XQueryColor(W_Display, W_Colormap, &b);

   new = XCreateFontCursor(W_Display, XC_watch);
   XRecolorCursor(W_Display, new, &f, &b);
   XDefineCursor(W_Display, win->window, new);
}

void 
W_DefineUpDownCursor(window)
    W_Window        window;
{
static
   Cursor          new;
   struct window  *win = W_Void2Window(window);
   static
   XColor          f, b;

   if(new){
      XDefineCursor(W_Display, win->window, new);
      return;
   }

   f.pixel = colortable[W_White].pixelValue;
   b.pixel = colortable[W_Black].pixelValue;

   XQueryColor(W_Display, W_Colormap, &f);
   XQueryColor(W_Display, W_Colormap, &b);

   new = XCreateFontCursor(W_Display, XC_sb_v_double_arrow);
   XRecolorCursor(W_Display, new, &f, &b);
   XDefineCursor(W_Display, win->window, new);
}

#endif

/* NOTE: this routine creates a cursor, therefore it should never be called
   more then once. */
void
W_DefineCursorFromGlyph(window, font, chr, mask_chr, color)

   W_Window	window;
   W_Font	font;
   char		*chr, *mask_chr;
   W_Color	color;
{
   Cursor		new;
   static XColor	f, b;
   struct window	*win = W_Void2Window(window);
   XFontStruct		*fs = (XFontStruct *)fonts[fontNum(font)].font;

   f.pixel = colortable[color].pixelValue;
   b.pixel = colortable[W_Black].pixelValue;
   XQueryColor(W_Display, W_Colormap, &f);
   XQueryColor(W_Display, W_Colormap, &b);

   new = XCreateGlyphCursor(W_Display, fs->fid, fs->fid,
				       (unsigned int)(unsigned char) *chr,
				       (unsigned int)(unsigned char) *mask_chr,
				       &f, &b);
   XDefineCursor(W_Display, win->window, new);
}


/*
 * ACK!  This only works if you define the same cursor in the * windows, and
 * is really hosed if you do different cursors * in the various windows. I
 * think....  -ERic
 */
void 
W_DefineCursor(window, width, height, bits, mask, xhot, yhot)
    W_Window        window;
    int             width, height, xhot, yhot;
    char           *bits, *mask;
{
   static char    *oldbits = NULL;
   static Cursor   curs;
   Pixmap          cursbits;
   Pixmap          cursmask;
   struct window  *win;
   XColor          whiteCol, blackCol;

#ifdef DEBUG
   printf("Defining cursor for %d\n", window);
#endif
   win = W_Void2Window(window);
   whiteCol.pixel = colortable[W_White].pixelValue;
   XQueryColor(W_Display, W_Colormap, &whiteCol);
   blackCol.pixel = colortable[W_Black].pixelValue;
   XQueryColor(W_Display, W_Colormap, &blackCol);

   if (!oldbits || oldbits != bits) {
      cursbits = XCreateBitmapFromData(W_Display, win->window,
				       bits, width, height);
      cursmask = XCreateBitmapFromData(W_Display, win->window,
				       mask, width, height);
      oldbits = bits;
      curs = XCreatePixmapCursor(W_Display, cursbits, cursmask,
				 &whiteCol, &blackCol, xhot, yhot);
      XFreePixmap(W_Display, cursbits);
      XFreePixmap(W_Display, cursmask);
   }
   XDefineCursor(W_Display, win->window, curs);
}

void 
W_Beep()
{
   XBell(W_Display, 0);
}

int 
W_WindowWidth(window)
    W_Window        window;
{
   return (W_Void2Window(window)->width);
}

int 
W_WindowHeight(window)
    W_Window        window;
{
   return (W_Void2Window(window)->height);
}

int 
W_Socket()
{
   return (ConnectionNumber(W_Display));
}

void 
W_DestroyWindow(window)
    W_Window        window;
{
   struct window  *win;

#ifdef DEBUG
   printf("Destroying %d\n", window);
#endif
   win = W_Void2Window(window);
   deleteWindow(win);
   XDestroyWindow(W_Display, win->window);
   free((char *) win);
}

#ifdef nodef
void 
W_SetTransientForHint(w, pw)
    W_Window        w, pw;
{
   XSetTransientForHint(W_Display, W_Void2Window(w)->window,
			pw?(W_Void2Window(pw)->window):0);
}
#endif

static void
deleteWindow(window)
    struct window  *window;
{
   struct windowlist **rm;
   struct windowlist *temp;

   rm = &hashtable[hash(window->window)];
   while (*rm != NULL && (*rm)->window != window) {
      rm = &((*rm)->next);
   }
   if (*rm == NULL) {
      printf("Attempt to delete non-existent window!\n");
      return;
   }
   temp = *rm;
   *rm = temp->next;
   free((char *) temp);
}

void 
W_SetIconWindow(main, icon)
    W_Window        main;
    W_Window        icon;
{
   static
   XWMHints        hints;

   XSetIconName(W_Display, W_Void2Window(icon)->window, W_Void2Window(main)->name);

   /* XXX: setting the icon window without setting input and state
      hints has the effect or resetting the latter (W_MakeWindow) */
   hints.flags = IconWindowHint | InputHint | StateHint;
   hints.input = True;
   hints.initial_state = NormalState;
   hints.icon_window = W_Void2Window(icon)->window;
   XSetWMHints(W_Display, W_Void2Window(main)->window, &hints);
}

static void
checkParent(name, parent)
    char           *name;
    W_Window       *parent;
{
   char           *adefault;
   char            buf[100];
   int             i;
   struct windowlist *windows;

   sprintf(buf, "%s.parent", name);
   adefault = getdefault(buf);
#ifdef SHOW_DEFAULTS
   show_defaults("windows", buf, adefault?adefault:
	  (*parent?W_Void2Window(*parent)->name:"root"),
      "Window hierarchy parent.");
#endif
   if (adefault == NULL)
      return;
   /* parent must be name of other window or "root" */
   if (strcmpi(adefault, "root") == 0) {
      *parent = W_Window2Void(&myroot);
      return;
   }
   for (i = 0; i < HASHSIZE; i++) {
      windows = hashtable[i];
      while (windows != NULL) {
	 if (strcmpi(adefault, windows->window->name) == 0) {
	    *parent = W_Window2Void(windows->window);
	    return;
	 }
	 windows = windows->next;
      }
   }
}

int
W_CheckMapped(name)
    char           *name;
{
   char            buf[100];

   sprintf(buf, "%s.mapped", name);
#ifdef SHOW_DEFAULTS
   show_defaults("windows", buf, "off",
      "Map window by default.");
#endif
   return (booleanDefault(buf, 0));
}

void 
W_WarpPointer(window)
    W_Window        window;
{
   static int      warped_from_x = 0, warped_from_y = 0;

   if (window == NULL) {
      if (W_in_message) {
	 XWarpPointer(W_Display, None, W_Root, 0, 0, 0, 0, warped_from_x, warped_from_y);
	 W_in_message = 0;
      }
   } else {
      W_FindMouse(&warped_from_x, &warped_from_y);
      XWarpPointer(W_Display, None, W_Void2Window(window)->window, 0, 0, 0, 0, 0, 0);
      W_in_message = 1;
   }
}

void
W_FindMouse(x, y)
    int            *x, *y;
{
   Window          theRoot, theChild;
   int             wX, wY, rootX, rootY, status;
   unsigned int    wButtons;

   status = XQueryPointer(W_Display, W_Root, &theRoot, &theChild, &rootX, &rootY, &wX, &wY, &wButtons);
   if (status == True) {
      *x = wX;
      *y = wY;
   } else {
      *x = 0;
      *y = 0;
   }
}

int 
W_FindMouseInWin(x, y, w)
    int            *x, *y;
    W_Window        w;
{
   Window          theRoot, theChild;
   int             wX, wY, rootX, rootY, status;
   unsigned int    wButtons;
   struct window  *win = W_Void2Window(w);
   Window          thisWin = win->window;

   status = XQueryPointer(W_Display, thisWin, &theRoot, &theChild,
			  &rootX, &rootY, &wX, &wY, &wButtons);
   if (status == True) {
      /*
       * if it's in the window we specified then the values returned should
       * be within the with and height of the window
       */
      if (wX <= win->width && wY <= win->height) {
	 *x = wX;
	 *y = wY;
	 return 1;
      }
   }
   *x = 0;
   *y = 0;

   return 0;
}

void
W_Flush()
{
   XFlush(W_Display);
}

void
W_ReposWindow(window, newx, newy)

   W_Window	window;
   int		newx,newy;
{
   Window	win = W_Void2Window(window)->window;

   XMoveWindow(W_Display, win, newx, newy);
}

void
W_ResizeWindow(window, neww, newh)              /* TSH 2/93 */

   W_Window     window;
   int          neww, newh;
{
   struct window	*win = W_Void2Window(window);

   XResizeWindow(W_Display, win->window, (unsigned int) neww, 
					 (unsigned int) newh);
}

void
W_ReinitMenu(window, neww, newh)

   W_Window	window;
   int		neww, newh;
{
   struct window	*win = W_Void2Window(window);
   struct menuItem	*items;
   register		i;

   items = (struct menuItem *) win->data;
   for(i=0; i< win->height; i++){
      free((char *) items[i].string);
   }
   free ((char *)items);
   items = (struct menuItem *) malloc(newh * sizeof(struct menuItem));
   for(i=0; i< newh; i++){
      items[i].column = 0;
      items[i].string = (char *) malloc(MAX_TEXT_WIDTH);
      items[i].color = W_White;
   }
   win->data = (char *) items;
}

/* this procedure should only be used if the menu is initially defined
   by W_MakeMenu as large as it will get.  If menu may grow, call 
   W_ReinitMenu first */

void
W_ResizeMenu(window, neww, newh)                /* TSH 2/93 */

   W_Window     window;
   int          neww, newh;
{
   struct window	*w = W_Void2Window(window);

   w->width = neww;
   w->height = newh;

   W_ResizeWindow(window, neww*W_Textwidth+WIN_EDGE*2,
            newh*(W_Textheight+MENU_PAD*2)+(newh-1)*MENU_BAR);
}

void
W_ResizeTextWindow(window, neww, newh)                /* TSH 2/93 */

   W_Window     window;
   int          neww, newh;
{
   W_ResizeWindow(window, neww*W_Textwidth+WIN_EDGE*2, 
     newh*W_Textheight + WIN_EDGE*2 );
}

int
W_Mono()
{
   return DisplayCells(W_Display, W_Screen) <= 2;
}

#ifdef TTS

static GC		_tts_gc;
static XFontStruct	*_tts_fontinfo;
static int		_tts_th, _tts_tw;

int
W_TTSTextHeight()
{
   return _tts_th;
}

int
W_TTSTextWidth(s, l)

   char	*s;
   int	l;
{
  return XTextWidth(_tts_fontinfo, s, l);
}

void
init_tts()
{
   char		*fontname;
   XGCValues	values;
   char		*color;
   XColor	xc;

   if(forceMono || DisplayCells(W_Display, W_Screen) <= 2 ||
        (DefaultVisual(W_Display, W_Screen)->class == TrueColor)){
      /* this is not going to work at all for b/w */
      tts = 0;
      return;
   }

   fontname = getdefault("tts_font");
   if(!fontname)
      fontname = TTS_FONT;
#ifdef SHOW_DEFAULTS
   show_defaults("tts", "tts_font", fontname, "TTS font.");
#endif
   
   _tts_fontinfo = XLoadQueryFont(W_Display, fontname);
   if(!_tts_fontinfo){
      fprintf(stderr, "netrek: Can't find font \"%s\".\n", fontname);
      _tts_fontinfo = XLoadQueryFont(W_Display, "fixed");
   }
   if(!_tts_fontinfo){
      fprintf(stderr, "netrek: Can't find any fonts.\n");
      exit(1);
   }
   _tts_th = _tts_fontinfo->max_bounds.descent + 
	     _tts_fontinfo->max_bounds.ascent;
   _tts_tw = _tts_fontinfo->max_bounds.width;

   values.font = _tts_fontinfo->fid;

   color = getdefault("tts_color");
   if(!color)
      color = "#777";
#ifdef SHOW_DEFAULTS
   show_defaults("tts", "tts_color", color, "TTS msg color.");
#endif
   if(!XParseColor(W_Display, W_Colormap, color, &xc)){
      fprintf(stderr, "netrek: Unknown color \"%s\"\n", color);
      (void)XParseColor(W_Display, W_Colormap, "#777", &xc);
   }
   /* using the 8th color allocated in GetColors() */
   xc.pixel = colortable[W_Black].pixelValue | planes[0] | planes[2];
   XStoreColor(W_Display, W_Colormap, &xc);
   values.foreground = xc.pixel;
   values.function = GXor;

   if(_tts_gc)
      XFreeGC(W_Display, _tts_gc);

   _tts_gc = XCreateGC(W_Display, W_Root, GCFont|GCForeground|GCFunction,
				&values);

   XSetGraphicsExposures(W_Display, _tts_gc, False);
}

void
W_EraseTTSText(window, max_width, y, width)

   W_Window	window;
   int		max_width;
   int		y;
   int		width;
{
   register int		x = (max_width - width)/2;

   if(x < 0) x = 4;
   y -= W_TTSTextHeight();

   W_ClearArea(window, x, y, width, W_TTSTextHeight());
}

void
W_WriteTTSText(window, max_width, y, width, str, len)

   W_Window	window;
   int		max_width;	/* max_width of window */
   int		y;		/* y coordinate */
   int		width;		/* actual width */
   char		*str;		/* string */
   int		len;		/* length of string */
{
   struct window	*win = W_Void2Window(window);
   register int		x = (max_width - width)/2;

   if(x < 0) x = 4;

   y -= _tts_fontinfo->max_bounds.descent;

   /*
   y -= W_TTSTextHeight();
   y += _tts_fontinfo->max_bounds.ascent;
   */

   XDrawString(W_Display, win->window, _tts_gc, x, y, str, len);
}

#endif /* TTS */

/* functions for changing appearence on the fly */

void
W_UpdateWindow(window)

   W_Window	window;
{
   if(!window) return;
   UpdateBackground(window);
   UpdateBorder(window);
   UpdateGeometry(window);
}

static void
UpdateBackground(window)

   W_Window	window;
{
   struct window	*win;

   if(!window) return;

   win = W_Void2Window(window);

   XSetWindowBackground(W_Display, win->window, 
      colortable[W_Black].pixelValue);
}

static void
UpdateBorder(window)

   W_Window	window;
{
   struct window	*win;

   if(!window) return;

   win = W_Void2Window(window);

   XSetWindowBorder(W_Display, win->window, 
      colortable[W_White].pixelValue);
}

static void
UpdateGeometry(window)
   
   W_Window	window;
{
   struct window	*win;
   int			x, y, width, height;
   int			mask;
   XSizeHints		hints_return;
   long			supplied_return;	/* ignored */

   if(!window) 
      return;
   
   win = W_Void2Window(window);

   W_UnmapWindow(window);

   mask = checkGeometry(win->name, &x, &y, &width, &height);
   if(!mask)
      /* no default specified */
      return;
   
   switch(win->type){
      case WIN_TEXT:
	 if(mask & WidthValue)
	    width = width * W_Textwidth + WIN_EDGE * 2;
	 if(mask & HeightValue)
	    height = MENU_PAD * 2 + height * W_Textheight;
	 break;
      case WIN_SCROLL:
	 if(mask & WidthValue)
	    width = width * W_MesgTextwidth + WIN_EDGE * 2;
	 if(mask & HeightValue)
	    height = MENU_PAD * 2 + height * W_MesgTextheight;
	 break;
      case WIN_MENU:
	 if(mask & WidthValue)
	    width = width * W_Textwidth + WIN_EDGE * 2;
	 if(mask & HeightValue)
	    height = height * (W_Textheight + MENU_PAD * 2) + 
		  (height - 1) * MENU_BAR;
      default:
	 break;
   }

   if((mask & XValue) && (mask & YValue))
      XMoveWindow(W_Display, win->window, x, y);

   if((mask & WidthValue) && (mask & HeightValue))
      XResizeWindow(W_Display, win->window, (unsigned int) width,
					 (unsigned int) height);

   XGetWMNormalHints(W_Display, win->window, &hints_return, 
					     &supplied_return);
   
   if((mask & XValue) && (mask & YValue)){
      hints_return.flags |= USPosition;
      hints_return.x = x;
      hints_return.y = y;
      XSetWMNormalHints(W_Display, win->window, &hints_return);
   }
}

/* paradise functinso */

void
W_WriteWinBitmap(win, x, y, bit, color)
    W_Window win;
    int     x, y;
    W_Icon  bit;
    W_Color color;
{
    struct icon *icon;
    Window  original;

    if(color >= MAX_COLOR)
      color = 0;

    icon = W_Void2Icon(bit);
    original = icon->window;
    icon->window = W_Void2Window(win)->window;
    W_WriteBitmap(x, y, bit, color);
    icon->window = original;
    return;
}

void
W_FreeBitmap(bit)
    W_Icon  bit;
{
    struct icon *icon;
    icon = W_Void2Icon(bit);
    XFreePixmap(W_Display, icon->bitmap);
    free(icon);
}

char *
W_FetchBuffer(l)

   int	*l;
{
   char	*m = XFetchBytes(W_Display, l);
   return m;
}

void
W_FreeBuffer(m)

   char	*m;
{
   XFree((void *)m);
}

void
W_ChangeBackground(win, color)

   W_Window	win;
   W_Color	color;
{
   struct window	*w = W_Void2Window(win);

   XSetWindowBackground(W_Display, w->window, colortable[color].pixelValue);
   XClearWindow(W_Display, w->window);
}

#ifdef RECORD

/* Some stupid X bug is delaying dashboard updates on playback without
   this function. It might be linux-specific, not sure.  Maybe because in 
   playback so many things are drawn before the W_Flush is called
 */

int W_Sync()
{
  XSync(W_Display, False);
  return 0;
}
#endif
