/*
 * ranklist.c
 * 
 * Kevin P. Smith 12/5/88
 * 
 */
#include "copyright2.h"

#include <stdio.h>
#include "netrek.h"

void
ranklist()
{
   register int    i;
   char            buf[80];

   /*
    * W_ClearWindow(rankw);
    */
   (void) strcpy(buf, "  Rank       Hours  Defense  Ratings      DI");
   W_WriteText(rankw, 1, 1, textColor, buf, strlen(buf), W_BoldFont);
   for (i = 0; i < NUMRANKS; i++) {
      sprintf(buf, "%-11.11s %5.0f %8.2f %8.2f   %7.2f",
	      ranks[i].name,
	      ranks[i].hours,
	      ranks[i].defense,
	      ranks[i].ratings,
	      ranks[i].ratings * ranks[i].hours);
      if (mystats->st_rank == i) {
	 W_WriteText(rankw, 1, i + 2, W_Cyan, buf, strlen(buf), W_BoldFont);
      } else {
	 W_WriteText(rankw, 1, i + 2, textColor, buf, strlen(buf), W_RegularFont);
      }
   }
   strcpy(buf, "To achieve a rank, you need a high enough defense, and");
   W_WriteText(rankw, 1, i + 3, textColor, buf, strlen(buf), W_RegularFont);
   strcpy(buf, "either enough hours, and bombing + planet + offense ratings");
   W_WriteText(rankw, 1, i + 4, textColor, buf, strlen(buf), W_RegularFont);
   strcpy(buf, "above shown ratings, or too few hours, and a DI rating above");
   W_WriteText(rankw, 1, i + 5, textColor, buf, strlen(buf), W_RegularFont);
   strcpy(buf, "the shown DI rating.");
   W_WriteText(rankw, 1, i + 6, textColor, buf, strlen(buf), W_RegularFont);
}
