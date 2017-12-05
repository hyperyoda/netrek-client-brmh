#ifndef _bitmapstuff_h_
/*
 * bitmapstuff.h * support includes for the server and/or file definable *
 * ship bitmaps
 */

/* prevent problems from multiple including.. */
#ifndef BITMAP_TYPES
#define BITMAP_TYPES

#define SPEC_TYPES 8

#define NUM_BITMAP_TYPES  50
/* really should be:( NUM_TEAMS * NUM_TYPES + SPEC_TYPES) */
/* 5 teams (4+indep) * 8 plus 10 special types */

/* and you'd define the special types here... */
#define MYSTERYSHIP 	0	/* generic ship */
#define DEFENDER	1	/* planetary defender, hosers */
#define BORGCUBE	2	/* borg cube... */
#define ROGUESTAR       3	/* flys around zapping things... */
#define SPACEMONSTER	4	/* generic space monster */
#define ORGANIANS	5	/* enforcers of galactic peace */
#define BLACKHOLE	6	/* a space 'object' */
#define DEATHSTAR	7	/* Superuser's ship, the ATT death star! */

#ifdef /* #ifndef _XLIB_H_ */ nodef
/*
 * an abbreviated XImage struct,  useful for when we don't need all of Xlib.h
 */
typedef struct _XImage {
   int             width, height;
   int             xoffset;
   int             format;
   unsigned char  *data;
   int             byte_order;
   int             bitmap_unit;
   int             bitmap_bit_order;
   int             bitmap_pad;
   int             depth;
   int             bytes_per_line;
}               XImage;
#define XYBitmap 0
#define LSBFirst 0
#endif				/* _XLIB_H_ */

#ifndef _XLIB_H_
#include <X11/Xlib.h>
#endif

/* Bitmap rotation defines */
/* this use bitmasks, so don't add defines! */
/*
 * * i.e. 0x01 means use rotation, otherwise copy *      0x02 means a single
 * source bitmap *      0x04 means 4 source bitmaps *      0x08 means 8
 * source bitmaps *
 * 
 * If no bits are set, all view bitmaps are assumed to be present!
 */
#define ROTATE_BIT	0x1
#define SINGLE_SOURCE	0x2
#define FOUR_SOURCE	0x4
#define	EIGHT_SOURCE	0x8
#define COPY_SOURCE   0x10	/* copy the data into malloc's space, even *
				 * if it's not rotated */
#define STD_SHIP_FLGS   0x5	/* ships with only 4 views */
#define STD_BASE_FLGS   0x2	/* bases only have the same view 4 times over */

#define NO_BITMAP	0
/*
 * pointer to show that an entry in BitmapSizes is not defined * by a static,
 * instead of malloc'd
 */

/* prototype bitmap rotation alg... */
unsigned char  *RotateShipViews();
XImage         *RotateBitmap();

#ifndef VIEWS
#define VIEWS 16
#endif


#endif
#endif
