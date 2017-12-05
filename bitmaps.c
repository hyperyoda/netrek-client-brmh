#ifdef DYNAMIC_BITMAPS
/*
 * * bitmaps.c * stuff to be used for the dynamic bitmaps code * Eric
 * mehlhaff 12/5/92
 */

#include <stdio.h>
#include "netrek.h"

#include "bitmapstuff.h"
/*
 * make sure the  XIcon struct used in bitmaps.c and * RotateBitmap.c are the
 * SAME! * Otherwise, RotateBitmap() will be very confused what its *
 * arguments are!
 */


#if defined( DEBUG) || defined (BITMAP_DEBUG)
int             OwnBitmapNum = 0;	/* so it can be played with from
					 * options  menu! */
#endif

/* pointers to all the bitmap data for ships and such */
unsigned char  *BitmapDataPtrs[NUM_BITMAP_TYPES][VIEWS];
/* sizes of all the ships and such, height assumed equal to width */
short           BitmapSizes[NUM_BITMAP_TYPES];

/*
 * stdbitmap num is old team * num_types, + special types * default is 0
 */
int 
StdBitmapNum(team, shiptype)
    int             team;
    int             shiptype;
{
   int             typenum = 0;

   if (shiptype < 0 || shiptype > NUM_TYPES) {
      fprintf(stderr, "WARNING:illegal Shiptype %d in StdBitmapNum!\n", shiptype);
   }
   switch (team) {
   case IND:
      typenum = SPEC_TYPES + NUM_TYPES * 0 + shiptype;
      break;
   case FED:
      typenum = SPEC_TYPES + NUM_TYPES * 1 + shiptype;
      break;
   case ROM:
      typenum = SPEC_TYPES + NUM_TYPES * 2 + shiptype;
      break;
   case KLI:
      typenum = SPEC_TYPES + NUM_TYPES * 3 + shiptype;
      break;
   case ORI:
      typenum = SPEC_TYPES + NUM_TYPES * 4 + shiptype;
      break;
   default:
      printf("Warning:StdBitmapNum type fallthrough!\n");
      typenum = shiptype;
   }

   if (typenum < 0 || typenum >= NUM_BITMAP_TYPES) {
      IFDEBUG(printf("Warning: StdBitmapNum range error fallthrough!\n");)
      typenum = 0;		/* handle bad cases */
   }
   return (typenum);
}

int 
PlayerBitmap(j)
    struct player  *j;
{
#if defined( DEBUG) || defined (BITMAP_DEBUG)
   if (j == me && OwnBitmapNum &&
       (BitmapSizes[OwnBitmapNum] != NO_BITMAP)) {

      return (OwnBitmapNum);	/* lets player mess with their bitmap! */
   }
#endif

   return StdBitmapNum(j->p_team, j->p_ship.s_type);
}

/*
 * return pointer to bitmap array for a ship view * if the bit in view_avail
 * for this view is set for this view, * use it. * it not, rotate from one
 * that is available (preferrably in the 0-3 range * or 0-7 range (for right
 * angle rotations ) *
 * 
 * args: * char *shiparray: pointer to array of bitmap data for the ship * int
 * view:  view number to generate * int    rotate_mode: how to deal with the
 * data * int   identifying 'number' of the bitmap. *
 * 
 * int width, height
 */
#ifndef BYTESPERVIEW
#define BYTESPERVIEW 60
#endif

unsigned char  *
RotateShipViews(shiparray, view, rotate_mode, bitmapnum, size)
    unsigned char   shiparray[VIEWS][BYTESPERVIEW];
    int             view;
    int             rotate_mode;
    int             bitmapnum;
    int             size;
{
   int             rotate_angle;
   XImage          OriginalBitmap;
   XImage         *RotatedBitmap;
   int             source_view;
   unsigned char  *dataptr;
   int             bytesperline = 0;
   int             height = size, width = size;
   int             bytesperview = 60;
   int             views_avail = 1;

   IFDEBUG(printf("RotateShipView(): num:%d view:%d mode: %d\n", bitmapnum, view, rotate_mode);)
   /* argument checking! */
   if (view < 0 || view > 16)
      view = 0;

   /* get view to translate from and get rotate angle */
   if (rotate_mode & FOUR_SOURCE) {
      source_view = view % 4;
      views_avail = 4;
   } else if (rotate_mode & EIGHT_SOURCE) {
      source_view = view % 8;
      views_avail = 8;
   } else if (rotate_mode & SINGLE_SOURCE) {
      source_view = 0;
      views_avail = 1;
   } else {
      source_view = view;
      views_avail = 16;
   }
   bytesperline = width / 8 + ((width % 8) ? 1 : 0);
   bytesperview = bytesperline * height;

   /*
    * dereferencing hell?? &( (char (*)[views_avail][bytesperview])
    * shiparray[source_view][0]); a real pity dereferencing is done
    * compiletime...
    */

   if (rotate_mode & ROTATE_BIT) {
      rotate_angle = (view - source_view) * (256 / 16);
   } else {
      rotate_angle = 0;
   }

   IFDEBUG(printf("RotateShipViews from:%d to:%d angle %d\n", source_view, view, rotate_angle);)
   /* perform a rotation? */
   if (rotate_angle != 0) {
      OriginalBitmap.width = width;
      OriginalBitmap.height = height;
      OriginalBitmap.xoffset = 0;
      OriginalBitmap.format = XYBitmap;
      OriginalBitmap.data = &(shiparray[source_view][0]);
      OriginalBitmap.bitmap_unit = 0;
      OriginalBitmap.bitmap_bit_order = 0;
      OriginalBitmap.bitmap_pad = 0;
      OriginalBitmap.depth = 0;
      OriginalBitmap.bytes_per_line = bytesperline;
      IFDEBUG(printf("Rotating... \n");)
      RotatedBitmap = RotateBitmap(OriginalBitmap, rotate_angle, 5);
      dataptr = RotatedBitmap->data;
   } else {
      dataptr = &(shiparray[source_view][0]);
      IFDEBUG(printf("NOT Rotating... %d\n", dataptr);)
   }

   /*
    * keep a static table of bitmap data * copy the bitmap data into malloc'd
    * space if it was generated by * rotation or copying * otherwise just set
    * a pointer in the data table to show it has been * created.
    */

   /* don't know where to put this.  Jsut deal with it... */
   if (bitmapnum < 0 || bitmapnum >= NUM_BITMAP_TYPES) {
      IFDEBUG(fprintf(stderr, "WARNING: RotateView() - illegal bitmapnum %d \n", bitmapnum);)
      return dataptr;
   }
   /* keep records of the size of this bitmap? */
   BitmapSizes[bitmapnum] = size;

   /* free old data pointer, if it was set */
   if (BitmapDataPtrs[bitmapnum][view] != NULL) {
      free(BitmapDataPtrs[bitmapnum][view]);
      IFDEBUG(printf("Freeing bitmap data pointer %d.%d\n", bitmapnum, view);)
   }
   if (source_view != view || rotate_mode & COPY_SOURCE) {
      /*
       * rotated or copied -- we need to copy source into space * allocated
       * by malloc
       */
      BitmapDataPtrs[bitmapnum][view] = (unsigned char *)
	 malloc((sizeof(unsigned char)) * bytesperline * height);
      if (BitmapDataPtrs[bitmapnum][view] == NULL) {
	 IFDEBUG(printf("Out of memory grabbing space for bitmap data %d.%d\n", bitmapnum, view);)
	 return dataptr;
      } else {
	 MCOPY(dataptr, BitmapDataPtrs[bitmapnum][view], bytesperline * height);
      }
      dataptr = BitmapDataPtrs[bitmapnum][view];
      IFDEBUG(printf("Rotate: returning %l after rotate/malloc\n", dataptr);)
   } else {
      /*
       * the original data for this is the source, * odds are,
       * rotation/copying never occurred -- * source view and target view are
       * the same! * put an entry in BitmapDataPtrs to reflect this
       */
      IFDEBUG(printf("Rotate: returning\n");)
      BitmapDataPtrs[bitmapnum][view] = NULL;
   }
   IFDEBUG(printf("Rotate: returning:%d\n", dataptr);)
   return dataptr;
}
#endif
