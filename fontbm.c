
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/types.h>
#include <sys/time.h>
#include <assert.h>
#include "netrek.h"

#define fontmessage()	fprintf(stderr, "You must do 'xset +fp <netrek-font-directory>' or have it\n\
in your .xinitrc in order for netrek to run.\n");


#define F_LPLANET	"netrek_lplanetfont"
#define F_LPLANET_NUM	FONTS+1
static F_lplanet_num = F_LPLANET_NUM;

#define F_MPLANET	"netrek_mplanetfont"
#define F_MPLANET_NUM	FONTS+2
static F_mplanet_num = F_MPLANET_NUM;

#define F_MISC		"netrek_miscfont"
#define F_MISC_NUM	FONTS+3
static F_misc_num = F_MISC_NUM;

#define F_SHIP1		"netrek_shipfont1"
#define F_SHIP1_NUM	FONTS+4
static F_ship1_num = F_SHIP1_NUM;

#define F_SHIP2		"netrek_shipfont2"
#define F_SHIP2_NUM	FONTS+5
static F_ship2_num = F_SHIP2_NUM;

#define F_SHIP3		"netrek_shipfont3"
#define F_SHIP3_NUM	FONTS+6
static F_ship3_num = F_SHIP3_NUM;

#define F_SMALL		"netrek_smallfont"
#define F_SMALL_NUM	FONTS+7
static F_small_num = F_SMALL_NUM;

extern Display	*W_Display;
extern Window	W_Root;

#define W_VoidToGCList(v)	((GC *) v)

void
F_init(fonts, lastused)
   W_FontInfo	*fonts;
   int		lastused;

{
   XFontStruct	*f;
   lastused++;
   if(!(f = XLoadQueryFont(W_Display, F_LPLANET))){
      fprintf(stderr, "netrek: Can't find bitmap font \"%s\"\n", F_LPLANET);
      fontmessage();
      exit(1);
   }
   fonts[lastused].baseline = f->max_bounds.ascent;
   fonts[lastused].font = (void *)f;
   lastused++;
   if(!(f = XLoadQueryFont(W_Display, F_MPLANET))){
      fprintf(stderr, "netrek: Can't find bitmap font \"%s\"\n", F_MPLANET);
      fontmessage();
      exit(1);
   }
   fonts[lastused].baseline = f->max_bounds.ascent;
   fonts[lastused].font = (void *)f;
   lastused++;
   if(!(f = XLoadQueryFont(W_Display, F_MISC))){
      fprintf(stderr, "netrek: Can't find bitmap font \"%s\"\n", F_MISC);
      fontmessage();
      exit(1);
   }
   fonts[lastused].baseline = f->max_bounds.ascent;
   fonts[lastused].font = (void *)f;
   lastused++;
   if(!(f = XLoadQueryFont(W_Display, F_SHIP1))){
      fprintf(stderr, "netrek: Can't find bitmap font \"%s\"\n", F_SHIP1);
      fontmessage();
      exit(1);
   }
   fonts[lastused].baseline = f->max_bounds.ascent;
   fonts[lastused].font = (void *)f;
   lastused++;
   if(!(f = XLoadQueryFont(W_Display, F_SHIP2))){
      fprintf(stderr, "netrek: Can't find bitmap font \"%s\"\n", F_SHIP2);
      fontmessage();
      exit(1);
   }
   fonts[lastused].baseline = f->max_bounds.ascent;
   fonts[lastused].font = (void *)f;
   lastused++;
   if(!(f = XLoadQueryFont(W_Display, F_SHIP3))){
      fprintf(stderr, "netrek: Can't find bitmap font \"%s\"\n", F_SHIP3);
      fontmessage();
      exit(1);
   }
   fonts[lastused].baseline = f->max_bounds.ascent;
   fonts[lastused].font = (void *)f;
   lastused++;
   if(!(f = XLoadQueryFont(W_Display, F_SMALL))){
      fprintf(stderr, "netrek: Can't find bitmap font \"%s\"\n", F_SMALL);
      fontmessage();
      exit(1);
   }
   fonts[lastused].baseline = f->max_bounds.ascent;
   fonts[lastused].font = (void *)f;
   lastused++;
}

void
F_initGC(gclist_v, lastused, fonts)
   
   W_GC		*gclist_v;
   int		lastused;
   W_FontInfo	*fonts;
{
   GC		*gclist = W_VoidToGCList(gclist_v);
   XGCValues	values;
   register	i;

   for(i=0; i < F_NUMFONTS; i++){
      lastused++;
      values.font = ((XFontStruct *)fonts[lastused].font)->fid;
      if(gclist[lastused])
	 XFreeGC(W_Display, gclist[lastused]);
      gclist[lastused] = XCreateGC(W_Display, W_Root, GCFont, &values);
      XSetGraphicsExposures(W_Display, gclist[lastused], False);
   }
}

/*
   
   planet_width/planet_height			30/30
   cloak_width/cloak_height			20/20
   shield_width/shield_height			20/20

   sbexp_width/sbexp_height			80/80
   ex_width/ex_height				64/64
   plasmacloud_width/plasmacloud_height		13/13

   cloud_width/cloud_height			9/9
   eplasmacloud_width/eplasmacloud_height	7/7
   mplasmacloud_width/mplasmacloud_height	5/5

   mplanet_width/mplanet_height			16/16
*/

/* local planet character */
char *
F_planetChar(l)

   struct planet  *l;
{
   static char  r;
   if(!(l->pl_info & me->p_team)) return "\015";

   switch(showlocal){
      case 0:           /* owner */
         r = remap[l->pl_owner] + 8;
         return &r;

      case 1:           /* resource */
      case 3:
         r=0;
         if(l->pl_armies > 4)
            r += 4;
         if(l->pl_flags & PLREPAIR)
            r += 2;
         if(l->pl_flags & PLFUEL)
            r += 1;
	 if(showlocal == 3)
	    r += '\017';
	 return &r;

      case 2:           /* nothing */
#if 0
         if(l->pl_armies < 5)
            return "\020";
         else
#endif
            return "\010";
         

      default:
	 return "";
   }
}

W_Font 
F_planetFont()
{
   return (W_Font) &F_lplanet_num;
}

char *
F_cloakChar()
{
   return "\205";
}

W_Font
F_cloakFont()
{
   return (W_Font) &F_ship3_num;
}

char *
F_shieldChar(i)

   int	i;
{
   static char	r;
   r = 128 + i;
   return &r;
}

W_Font
F_shieldFont()
{
   return (W_Font) &F_ship3_num;
}

#ifdef HOCKEY
char *
F_puckChar(dir)

   int	dir;

{
   static char	r;
   r = '\206' + rosette(dir);
   return &r;
}
#endif

char *
F_shipChar(team, ship, dir)
   int			team;
   int			ship;
   unsigned char	dir;
{
   static char	r;

   r = ship * 16 + rosette(dir);

   switch(team){
      case FED:
      case KLI:
	 r += 128;
   }
   return &r;
}

#ifdef HOCKEY
W_Font
F_puckFont()
{
   return (W_Font) & F_ship3_num;
}
#endif

W_Font
F_playerFont(team)

   int	team;
{
   switch(team){
      case IND:
      case FED:
	 return (W_Font) &F_ship1_num;
      case ROM:
      case KLI:
	 return (W_Font) &F_ship2_num;
      case ORI:
	 return (W_Font) &F_ship3_num;
      default:
	 return (W_Font) &F_ship1_num;
   }
}

char *
F_expSbChar(frame)
   int	frame;
{
   static char	r;
   r = frame;
   return &r;
}

W_Font 
F_explodeFont()
{
   return (W_Font) &F_misc_num;
}

char *
F_expChar(frame)
   
   int	frame;
{
   static char	r;
   r = 7 + frame;
   return &r;
}

W_Font
F_smallFont()
{
   return (W_Font) &F_small_num;
}

char *
F_expTorpChar(frame)

   int	frame;
{
   static char	r;
   r = frame;
   return &r;
}

char *
F_expPlasmaTorpChar(frame)

   int	frame;
{
   static char	r;
   r = 12 + frame;
   return &r;
}

char *
F_plasmaTorpEnemyChar()
{
   return "\05";
}

char *
F_plasmaTorpChar()
{
   return "\06";
}

char *
F_planetMapChar(l)

   struct planet	*l;
{
   static char  r;
   if(!(l->pl_info & me->p_team)) return "\015";

   switch(showgalactic){
      case 0:           /* owner */
         r = remap[l->pl_owner] + 8;
         return &r;
         
      case 1:           /* resource */
         r=0;
         if(l->pl_armies > 4)
            r += 4;
#ifdef MOOBITMAPS
	 else if(myPlanetBitmap)
	    r += 13;
#endif
         if(l->pl_flags & PLREPAIR)
            r += 2;
         if(l->pl_flags & PLFUEL)
            r += 1;
#ifdef MOOBITMAPS
	 if(myPlanetBitmap){
	    /* NASTY kludge (font bitmaps screwed up)*/
	    if(r == 13) r = '\025';
	 }
#endif
         return &r;
         break;

      case 2:           /* nothing */
	 if(l->pl_armies < 5)
	    return "\025";
	 else
	    return "\010";
      default:
         return "";
   }
}
   
W_Font 
F_planetMapFont()
{
   return (W_Font) &F_mplanet_num;
}

char *
F_phaserChar()
{
   return "\07";
}

W_Font
F_phaserFont()
{
   return (W_Font) &F_small_num;
}
