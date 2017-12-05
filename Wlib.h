#ifndef _Wlib_h_
#define _Wlib_h_
/*
 * Wlib.h
 * 
 * Include file for the Windowing interface.
 * 
 * Kevin P. Smith  6/11/89
 * 
 * The deal is this: Call W_Initialize(), and then you may call any of the
 * listed fuinctions. Also, externals you are allowed to pass or use include
 * W_BigFont, W_RegularFont, W_UnderlineFont, W_HighlightFont, W_White,
 * W_Black, W_Red, W_Green, W_Yellow, W_Cyan, W_Grey, W_Textwidth, and
 * W_Textheight.
 */
#include "copyright2.h"

typedef char   *W_Window;
typedef char   *W_Icon;
typedef char   *W_Font;
typedef char   *W_GC;
typedef int     W_Color;

extern W_Font   W_BigFont, W_RegularFont, W_UnderlineFont, W_HighlightFont,
		W_MesgFont;
extern W_Color  W_White, W_Black, W_Red, W_Green, W_Yellow, W_Cyan, W_Grey;
extern int      W_Textwidth, W_Textheight, W_MesgTextwidth, W_MesgTextheight;
extern int      W_FastClear;

#ifdef EXTRAFONTS
extern W_Font   W_MyPlanetFont, W_FriendlyPlanetFont, W_EnemyPlanetFont;
#endif

#define FONTS 5

#define W_EV_EXPOSE	1
#define W_EV_KEY	2
#define W_EV_BUTTON	3

#define W_LBUTTON	1
#define W_MBUTTON	2
#define W_RBUTTON	3

#define W_CTRLBUTTON	6
#define W_SHIFTBUTTON	3

#define KeyIsButton(x) (x >= 1 && x <= 12)

typedef struct event {
   int             type;
   W_Window        Window;
   int             key;
   int             x, y, width, height;
}               W_Event;

typedef struct fontInfo {
   void		*font;
   int          width, height, baseline;
} 		W_FontInfo;

#define W_BoldFont W_HighlightFont
#endif
