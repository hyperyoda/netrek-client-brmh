#ifdef DYNAMIC_BITMAPS
/*
 * rotatebitmap.c *  Routines to rotate bitmaps *
 * 
 */
#include <errno.h>
#include <stdio.h>
#include <math.h>

#ifndef NO_X_HEADERS
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/Xmu/Drawing.h>
#include <X11/Xutil.h>
#else

#define NO_READ_BITMAPFILE
/* with that out of the way, all tht we need is XImage to be defined */

/*
 * WARNING:Also, make sure whatever calls RotateBitmap() uses the SAME *
 * XImage typedef, or else RotateBitmap() will be very confused  about * just
 * what its arguments
 */

/*
 * an abbreviated XImage struct,  useful only for this code. * some X stuff
 * might understand it, though
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
#endif



/*
 * define NO_READ_BITMAPFILE and this code file will #ifdef out all the *
 * routines and code related to the reading and writing of * the bitmap
 * file[s], and thus be useful as utilities for manipulating * bitmap files
 * in other programs *
 * 
 * #define NO_READ_BITMAPFILE
 */


#ifndef NO_READ_BITMAPFILE
/*
 * 'global' variables for the bitmap file reading code swiped from the *
 * 'bitmap' program code
 */
static int      raster_length;	/* how many chars in the raster[] array */
static unsigned char *stripped_name = "bitmap";
/* file name without directory path or extension */
#endif				/* NO_READ_BITMAPFILE */


/* used primarily when reading from the .xbm files */
#ifndef OUTTA_RANGE
#define OUTTA_RANGE -1
#endif
int             x_hot_spot = OUTTA_RANGE;
int             y_hot_spot = OUTTA_RANGE;

#define PI 		3.14152136

#ifndef NO_READ_BITMAPFILE
static XImage   image =
{
   0, 0,			/* (int) width, (int) height */
   0, XYBitmap, NULL,		/* (int) xoffset, (int) format, (char *) data */
   LSBFirst, 8,			/* (int) byte_order, (int) bitmap-unit */
   LSBFirst, 8, 1,		/* (int) bitmap_bit_order, bitmap_pad, depth */
   0				/* (int) bytes_per_line */
};

#endif

/* useful utility: number of 8 bit bytes to form a line in xbm format */
#define BYTEWIDTH(width)   (int) ((width) % 8  ? ((width)/8 + 1) : ((width)/8))
/*
 * basically, an extra byte is needed for the remaining bits if it's not *
 * divisible by 8.
 */

/* maximum size bitmaps that these routines can handle... */
#ifndef MAXHEIGHT
#define MAXHEIGHT 100
#endif

#ifndef MAXWIDTH
#define MAXWIDTH 100
#endif

#define TEMPSIZE 	MAXHEIGHT * BYTEWIDTH(MAXWIDTH)
unsigned char   rotatedarray[TEMPSIZE];	/* really, I ought to make this
					 * dynamic...  */
static XImage   rotatedimage =
{
   0, 0,			/* width, height */
   0, XYBitmap, rotatedarray,	/* xoffset, format, data */
   LSBFirst, 8,			/* byte_order, bitmap_unit */
   LSBFirst, 8, 1,		/* bitmap_bit_order, bitmap_pad, depth */
   0				/* bytes_per_line */
};



/* establish the functions used in this code */
XImage         *RotateBitmap( /* XImage bitmap,int rotation, int filtering */ );
int             pixel(		/* int x,int y, char * data, int width,int
			   height */ );
void            set_pixel(	/* int rot_x,int rot_y, char * data, int
			       width,int height */ );

#ifndef  NO_READ_BITMAPFILE
int             ReadBitmapFile( /* char * filename */ );
void            WriteXImageFile( /* char * filename, XImage imagedata */ );
void            usage();
#endif


/*
 * #define NETREK if this is going to be used as part of netrek code, * in
 * which there are arrays (double) Sin[256] and (double) Cos[256] * this way
 * this function  doesn't need to call its own Sin and Cos * functinos.
 */
#ifdef NETREK
extern double   Sin[];
extern double   Cos[];
#else
double          Sin(int angle);
double          Cos(int angle);
#endif



#ifndef NO_READ_BITMAPFILE

/*
 * the wrapper for the standalone rotation utility. * see the usage() routine
 * for usage instructions...
 */
int 
main(int argc  , char **argv) {
   char            string[80];
   int             angle;
   int             filtering = 5;
   XImage         *imagedata = NULL;

   if              (argc < 3)
{
usage();
exit(1);
}
                   argv++;
   argc--;			/* skip argv[0] */
   /* -f [##] option-- filtering level.  4-9 defualt 5 */
   if (**argv == '-' && (*argv)[1] == 'f') {
      argc--;
      argv++;
      if (argc <= 0)
	 usage();
      filtering = atoi(*argv);
      printf("Filtering level: %d(%s)\n", filtering, *argv);
      argc--;
      argv++;
      if (argc <= 0)
	 usage();
   }
   angle = atoi(*argv);
   /* put angle in range */
   while (angle < 0)
      angle += 256;
   angle = angle % 256;
   argc--;
   argv++;
   printf("Rotation angle: %d\n", angle);

#ifdef DEBUG
   fprintf(stderr, "rotation angle %d\n", angle);
#endif

   /* loop over all arguments */
   while (--argc >= 0) {
#ifdef DEBUG
      fprintf(stderr, "parsing argument: %s\n", *argv);
#endif
      ReadBitmapFile(*argv);
      imagedata = RotateBitmap(image, angle, filtering);

      /* fix name to format oldfilename.rotation */
      sprintf(string, "%s.%d", *argv, angle);

      WriteXImageFile(string, *imagedata);
      argv++;
   }
   return (0);
}
#endif				/* NO_READ_BITMAPFILE */


/*
 * RotateBitmap() : Rotates X image structure, * returns pointer to rotated
 * XImage, NULL on error * args: standard XImage structure, rotation(0-255), *
 * filtering  to use in non-right angle rotations (see below for explanation) *
 * Rotation is in units  such that 256 equals full circle *
 * 
 * for non-right angle rotations, 'filtering' is used. * rotate alg is
 * improved from version 1 in that we map each pixel from * rotated map onto
 * the non-rotated one. * furthermore, we check points to either side of the
 * center of the * rotated map's pixels. if enough of them (set by filtering)
 * are in a pixel, the * corresponding rotated pixel is mapped that color.
 */

/* maximum bitmap size handled.   */
XImage         *
RotateBitmap(bitmap, angle, filtering)
    XImage          bitmap;
    int             angle;
    int             filtering;
{
   int             i, j;
   int             x, y;
   int             width = bitmap.width;
   int             height = bitmap.height;
   double          cent_x = x_hot_spot;
   double          cent_y = y_hot_spot;
   int             maxbytes = 0;
   int             bytesperline = 0;
   double          increment = .30;
   double          x_test, y_test;
   static unsigned char bitfield[MAXWIDTH][MAXHEIGHT] =
   {0};				/* this really should be dynamically
				 * allocated */
   char           *rotatedata = rotatedimage.data;
   double          sinval, cosval;


#ifndef NETREK
   sinval = Sin(-angle);
   cosval = Cos(-angle);
#else
   /*
    * oddly, the netrek Sin[] and Cos[] are rotated oddly from normal * math
    * functions.  We must compensate for this`
    */

   if (angle >= 0 && angle < 256) {
      sinval = Cos[256 - angle];
      cosval = -Sin[256 - angle];
   } else {
      /* deal with illegal range angles by just assuming 0 rotation */
      sinval = 0;
      cosval = 1;
   }
#endif


   width = bitmap.width;
   height = bitmap.height;


   if (cent_x == OUTTA_RANGE || cent_y == OUTTA_RANGE) {
      cent_x = (((double) width) - 1) / 2;
      cent_y = (((double) height) - 1) / 2;
   }
   rotatedimage.width = width;
   rotatedimage.height = height;
   rotatedimage.bytes_per_line = bytesperline = BYTEWIDTH(width);


#ifndef DEBUG
   /* handle simple cases */
   if (angle == 0) {
      for (i = 0; i < bytesperline * height; i++)
	 rotatedimage.data[i] = bitmap.data[i];
      return (&rotatedimage);
   };
#endif

   if ((sinval * sinval) <= 0.0001 || (sinval * sinval) >= 0.9999) {
      /*
       * don't have to do fancy increment stuff for right angle * rotations
       */
      increment = 0;
      filtering = 1;
#ifdef DEBUG
      printf("Performing Right angle rotation!\n");
#endif
   }
#ifdef DEBUG
   printf("Sin(), Cos() of the angle are %f, %f\n", sinval, cosval);
#endif

   /* calculate number of bytes needed  */
   maxbytes = height * bytesperline;

   /* clear bitmap space */
   for (i = 0; i < maxbytes; i++)
      *(rotatedata + i) = 0;

   for (i = 0; i < height; i++) {	/* row number */
      for (j = 0; j < width; j++) {	/* column number */
	 /*
	  * note for case where increment==0, following loops should get *
	  * executed only _once_
	  */
	 bitfield[i][j] = 0;
	 for (x_test = i - increment; x_test <= i + increment; x_test += increment) {
	    for (y_test = j - increment; y_test <= j + increment; y_test += increment) {
	       x = (int) (cosval * (x_test - cent_x) +
			  sinval * (y_test - cent_y) + cent_x + .5);
	       y = (int) (-sinval * (x_test - cent_x) +
			  cosval * (y_test - cent_y) + cent_x + .5);
	       bitfield[i][j] += pixel(x, y, bitmap.data, width, height);
	       if (x_test == i && y_test == j)
		  bitfield[i][j] += pixel(x, y, bitmap.data, width, height);
	       /* double weight of pixels exactly on */
	       if (increment == 0)
		  y_test++;	/* prevent infinite loop! */
	    }
	    if (increment == 0)
	       x_test++;	/* prevent infinite loop! */
	 }
      }
   }


   for (i = 0; i < height; i++) {	/* row number */
      for (j = 0; j < width; j++) {	/* column number */
	 if (bitfield[i][j] >= filtering) {
	    set_pixel(i, j, rotatedata, width, height);
	 }
      }
   }
   return (&rotatedimage);
}


/*
 * returns 1 if pixel corresponding to x,y in bitmap array data is set *
 * returns 0 in other cases
 */
int 
pixel(x, y, data, width, height)
    int             x, y;
    char           *data;
    int             width, height;
{
   int             bytepos;
   int             bitmask;
#ifdef MASSIVEDEBUG
   int             j;
#endif

   if (x < 0 || x >= width)
      return (0);
   if (y < 0 || y >= height)
      return (0);

   bytepos = x * BYTEWIDTH(width) + (y / 8);
   bitmask = 1 << (y % 8);

#ifdef MASSIVEDEBUG
   for (j = 0; j < BYTEWIDTH(width) * height; j++) {
      fprintf(stderr, "/%d", *(data + j));
   }
   fprintf(stderr, "\n");

   fprintf(stderr, "Checking pixel at %d, %d, (byte %d, mask %d)",
	   x, y, bytepos, bitmask);
#endif
   if (*(data + bytepos) & bitmask) {
#ifdef BIGDEBUG
      fprintf(stderr, "-\n");
#endif
      return (1);
   } else {
#ifdef BIGDEBUG
      fprintf(stderr, "#\n");
#endif
      return (0);
   }
}

/*
 * sets a pixel in the width/height data bitmap array, at * rot_x, rot_y
 */
void 
set_pixel(rot_x, rot_y, data, width, height)
    int             rot_x, rot_y;
    char           *data;
    int             width, height;
{
   int             bytepos;
   int             bitmask;
   if (rot_x < 0 || rot_x >= width)
      return;
   if (rot_y < 0 || rot_y >= height)
      return;

   bytepos = rot_x * BYTEWIDTH(width) + rot_y / 8;
   bitmask = 1 << (rot_y % 8);

   *(data + bytepos) |= bitmask;

   return;
}


#ifndef NO_READ_BITMAPFILE

/*
 * Shamelessy grabbed from the source code to 'bitmap' from the * mit X
 * standard Distribution...
 */
int 
ReadBitmapFile(filename)
    char           *filename;
{
   unsigned int    width, height;
   int             x_hot, y_hot;
   unsigned char  *data;
   int             status;

   status = XmuReadBitmapDataFromFile(filename, &width, &height, &data,
				      &x_hot, &y_hot);
   if (status != BitmapSuccess)
      return status;

   image.width = width;
   image.height = height;
   image.data = (char *) data;
   image.bytes_per_line = (image.width + 7) / 8;
   raster_length = image.bytes_per_line * image.height;
   x_hot_spot = x_hot;
   y_hot_spot = y_hot;

   return BitmapSuccess;
}


void 
WriteXImageFile(char *filename, XImage image) {
   FILE           *file;
   register int    i;
   int             num_bytes = 0;

                   file = fopen(filename, "w");
   if              (file == NULL)
{
fprintf(stderr, "Ack! Couldn't save %s!\n", filename);
}
#ifdef DEBUG
                   fprintf(stderr, "Writing to file %s (%d)\n", filename, file);
#endif
   fprintf(file, "#define %s_width %d\n", stripped_name, rotatedimage.width);
   fprintf(file, "#define %s_height %d\n", stripped_name, rotatedimage.height);
   if (x_hot_spot != OUTTA_RANGE)
      fprintf(file, "#define %s_x_hot %d\n", stripped_name, x_hot_spot);
   if (y_hot_spot != OUTTA_RANGE)
      fprintf(file, "#define %s_y_hot %d\n", stripped_name, y_hot_spot);
   fprintf(file, "static char %s_bits[] = {\n   0x%02x",
	   stripped_name, (unsigned char) rotatedimage.data[0]);

   num_bytes = BYTEWIDTH(rotatedimage.width) * rotatedimage.height;
#ifdef DEBUG
   fprintf(stderr, "number of bytes: %d\n", num_bytes);
#endif
   for (i = 1; i < num_bytes; i++) {
      fprintf(file, ",");
      fprintf(file, (i % 12) ? " " : "\n   ");
      fprintf(file, "0x%02x", (unsigned char) rotatedimage.data[i]);
   }
   fprintf(file, "};\n");
}


double 
Sin(int angle) {
   return (sin((float) angle * PI / 128));
}

    double          Cos(int angle) {
   return (cos((float) angle * PI / 128));
    }

    void            usage()
{
   fprintf(stderr, "Usage:  rotatebitmap [-f filterlevel] angle bitmaplist\n");
   fprintf(stderr, "This program takes the bitmap files given to it, and\n");
   fprintf(stderr, "rotates them, outputting them as [name].angle \n");
   fprintf(stderr, "NOTE: angle is in units such that 0-255 describes a full circle \n");
   exit(1);
}
#endif	/* NO_READ_BITMAPFILE */
#endif
