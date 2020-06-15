#include "copyright2.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "netrek.h"

static char	*header =
"#\n"
"# This file contains configuration information for your client.\n"
"# To find out all the avilable configurations for the BRMH client, see the\n"
"# WWW (Netscape/Mosaic) URL:\n"
"#         %s\n"
"#\n";

static char     *geometry =
"\n"
"# top level window\n"
"netrek.geometry:        1005x830+0+0\n"
"\n"
"# 'P' command\n"
"planet.geometry:        53x42+670+0\n"
"planet.parent:          netrek\n"
"\n"
"# List of players\n"
"player.geometry:        82x27+0+550\n"
"player.parent:          netrek\n"
"player.mapped:          on\n"
"\n"
"# messages->ALL (toggled after '?' command)\n"
"review_all.geometry:    81x6+500+548\n"
"review_all.parent:      netrek\n"
"\n"
"# messages->TEAM (toggled after '?' command)\n"
"review_team.geometry:   81x6+500+620\n"
"review_team.parent:     netrek\n"
"\n"
"# messages->YOU (toggled after '?' command)\n"
"review_your.geometry:   81x6+500+692\n"
"review_your.parent:     netrek\n"
"\n"
"# kill messages (toggled after '?' command)\n"
"review_kill.geometry:   81x6+500+764\n"
"review_kill.parent:     netrek\n"
"\n"
"# phaser points \n"
"review_phaser.geometry: 81x4+500+550\n"
"review_phaser.parent:   netrek\n"
"review_phaser.mapped:   on\n"
"\n"
"# all messages (ALL,TEAM,YOU,kills)\n"
"review.geometry:        81x22+500+600\n"
"review.parent:          netrek\n"
"review.mapped:          on\n"
"\n"
"# graphical ship stats\n"
"stats.geometry:         160x95+334+604\n"
"stats.parent:           netrek\n"
"\n"
"# war window\n"
"war.geometry:           20x6+300+6\n"
"";

void
makeDefault()
{
   char		buf[BUFSIZ];
   char		*home = getenv("HOME");
   FILE		*fo;

   if(!home) return;	/* too risky */

   sprintf(buf, "%s/.netrekrc", home);
   fo = fopen(buf, "w");
   if(!fo) return;
   fprintf(fo, header, URL_XTREKRC);
   fprintf(fo, "%s", geometry);
   fclose(fo);
   printf("%s created.\n", buf);
}
