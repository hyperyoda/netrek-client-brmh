/*
 * distsend.c
 *  Client specific portion of the RCD code
 */
#include "copyright.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include "netrek.h"

/* #$!@$#% length of address field of messages */
#define ADDRLEN 10

extern struct dmacro_list *distmacro;

/* this loads all sorts of useful data into a distress struct.
 */
struct distress *
loaddistress (i, data)
     enum dist_type i;
     W_Event *data;
{
  struct distress *dist;
  struct obtype *target;

  dist = (struct distress *) malloc (sizeof (struct distress));

  dist->sender = me->p_no;
  dist->dam = (100 * me->p_damage) / me->p_ship.s_maxdamage;
  dist->shld = (100 * me->p_shield) / me->p_ship.s_maxshield;
  dist->arms = me->p_armies;
  dist->fuelp = (100 * me->p_fuel) / me->p_ship.s_maxfuel;
  dist->wtmp = (100 * me->p_wtemp) / me->p_ship.s_maxwpntemp;
  dist->etmp = (100 * me->p_etemp) / me->p_ship.s_maxegntemp;
  /* so.. call me paranoid -jmn */
  dist->sts = (me->p_flags & 0xff) | 0x80;
  dist->wtmpflag = ((me->p_flags & PFWEP) > 0) ? 1 : 0;
  dist->etempflag = ((me->p_flags & PFENG) > 0) ? 1 : 0;
  dist->cloakflag = ((me->p_flags & PFCLOAK) > 0) ? 1 : 0;

  dist->distype = i;
  if (dist->distype > generic || dist->distype < take)
    dist->distype = generic;

  target = gettarget2 (me->p_x, me->p_y, TARG_PLANET);
  dist->close_pl = target->o_num;

  target = gettarget (data->Window, data->x, data->y, TARG_PLANET);
  dist->tclose_pl = target->o_num;

  target = gettarget2 (me->p_x, me->p_y, TARG_ENEMY);
  dist->close_en = target->o_num;

  target = gettarget (data->Window, data->x, data->y, TARG_ENEMY);
  dist->tclose_en = target->o_num;

  target = gettarget2 (me->p_x, me->p_y, TARG_FRIEND);
  dist->close_fr = target->o_num;

  target = gettarget (data->Window, data->x, data->y, TARG_FRIEND);
  dist->tclose_fr = target->o_num;

  target = gettarget2 (me->p_x, me->p_y, TARG_PLAYER|TARG_SELF);
  dist->close_j = target->o_num;

  target = gettarget (data->Window, data->x, data->y, TARG_PLAYER|TARG_SELF);
  dist->tclose_j = target->o_num;

  /* lets make sure these aren't something stupid */
  dist->cclist[0] = 0x80;
  dist->preappend[0] = '\0';
  dist->macroflag = 0;

  return (dist);
}

/* Coordinating function for _SENDING_ a RCD */
/* Send an rcd signal out to everyone. */

void
rcd (i, data)
     enum dist_type i;
     W_Event *data;
{
  char ebuf[200];
  struct distress *dist;
  char cry[MSG_LEN];
  char *info = NULL;
  int len;
  int recip;
  int group;

  group = MTEAM;
  recip = me->p_team;

  dist = loaddistress (i, data);

  if (F_gen_distress)
    {
      /* send a generic distress message */
      Dist2Mesg (dist, ebuf);
      pmessage (ebuf, recip, group | MDISTR);
    }
  else
    {
      len = makedistress (dist, cry, distmacro[dist->distype].macro);

      if (len > 0) {
          /* klude alert */
          info=cry;
          if (strncmp(getaddr2(MTEAM,recip),cry,8)==0)
            {
            /* this means we should _strip_ the leading bit because it's
               redundant */
            info=cry+9;
          }
        }

        pmessage (info, recip, group);
    }

  free (dist);
}

int
pmacro (mnum, who, data)
     int mnum;
     char who;
     W_Event *data;
{
  char addr;
  int group, len, recip;
  char cry[MSG_LEN];
  char *pm;
  struct distress *dist;


  if (!F_UseNewMacro)
    return 0;

  /* get recipient and group */
  if ((who == 't') || (who == 'T'))
    addr = teamlet[me->p_team];
  else
    addr = who;

  group = getgroup (addr, &recip);

  if (!group)
    {
      printf ("Bad group! %c %d %d\n",addr,recip,group);
      return (0);
    }


  pm = macro[mnum].string;

  dist = loaddistress (0, data);

  len = makedistress (dist, cry, pm);

  if (len > 0)
    {

#ifdef MULTILINE_MACROS

      if (multiline_enabled &&
	  (macro[mnum].type == NEWMULTIM))
	pmessage (cry, recip, group|MMACRO);

      else
#endif /* MULTILINE_MACROS */

	pmessage (cry, recip, group);
    }

  free (dist);
  return 1;
}


int
doMacro(key, data)

   int          key;
   W_Event      *data;
{
   int          found = 0;
   register     c;
   struct obtype *gettarget(), *target = NULL;
   char         who;
   int          targettype;

   if (key == '?') {
      showMacroWin();
      macro_off();
      return(0);
   } else {
      if (F_UseNewMacro) {
/* sorry guys, I only program in kludge - jn 6/3/93 */
         if (MacroNum > -1) {   /* macro identified, who to? */
            if (MacroNum >= MAX_MACRO)
               fprintf(stderr, "ERROR: Unknown macro number %d.\n", MacroNum);
            if (!pmacro(MacroNum, key, data))
               W_Beep();

            macro_off();
            return (0);
         }
      }
      for (c = 0; c < macrocnt && !found; c++) {
         if (macro[c].key == key) {
            found = 1;
            if (F_UseNewMacro) {
               switch (macro[c].type) {
               case NBTM:
                  pnbtmacro(c);
                  break;
               case NEWM:
                  MacroNum = c;
                  return (0);
                  break;
               case NEWMMOUSE:
                    /* first translate into who, then send */
                  switch (macro[c].who) {

                     struct player *j;
                     struct planet* l;

                     case MACRO_FRIEND:
                     case MACRO_ENEMY:
                     case MACRO_PLAYER:
                        targettype = TARG_PLAYER;
                        if (macro[c].who == MACRO_ENEMY)
                           targettype |= TARG_ENEMY;
                        else if (macro[c].who == MACRO_FRIEND)
                           targettype |= TARG_FRIEND;

                        target = gettarget(data->Window, data->x, data->y,
                                           TARG_PLAYER|TARG_CLOAK);
                        if (target->o_type == PLAYERTYPE) {
                           j = &players[target->o_num];
                           if(j->p_flags & PFCLOAK) maskrecip = 1;
                           who = j->p_mapchars[1];
                        }
                        else {
                           who = me->p_mapchars[1];
                           warning("Can only send a message to a player");
                        }
                        break;
                     case MACRO_TEAM:
                        target = gettarget(data->Window, data->x, data->y,
                                           TARG_PLAYER|TARG_PLANET);
                        if (target->o_type == PLANETTYPE) {
                           l = &planets[target->o_num];
                           who = teamlet[l->pl_owner];
                        }
                        else if (target->o_type == PLAYERTYPE) {
                           j = &players[target->o_num];
                           who = j->p_mapchars[0];
                        }
                        else {
                           who = me->p_mapchars[1];
                           warning("Player or planet only please");
                        }
                        break;
                     default:
                         who = me->p_mapchars[1];
                         break;
                  }
                  if (!pmacro(c,who,data))
                     W_Beep();
                  break;

               case NEWMULTIM:
                  found = 0;
                  if (!pmacro(c, macro[c].who, data))
                    {
                      W_Beep();
                    }
                  continue;
                  break;

               case NEWMSPEC:
                  if (!pmacro(c, macro[c].who, data))
                    {
                      W_Beep();
                    }
                  break;

               default:
                  fprintf(stderr, "ERROR: Unknown macro type %d.\n", macro[c].type);
                  break;
               }
            } else {
               pnbtmacro(c);
            }
            macro_off ();
            return(1);
         }
      }

   }

#ifdef nodef
/* scan for distress call here */
      for (i = take; distmacro[i].name; i++)
        {
          if (distmacro[i].c == data->key)
            {
              rcd (i, data);

              macro_off ();
              return (1);
              
            }
        }
#endif
   
   if (!found) 
     {
       W_Beep();
     }

   macro_off();
   return (0);
}

void
macro_off()
{
  warning (" ");                /* clear warning message */
  MacroMode = 0;
  MacroNum = -1;
}


/* Used in NEWMACRO, useful elsewhere also */
int 
getgroup(addr, recip)
   char                            addr;
   int                            *recip;
{
   *recip = 0;

   switch (addr) {
   case 'A':
      *recip = 0;
      return (MALL);
      break;
   case 'F':
      *recip = FED;
      return (MTEAM);
      break;
   case 'R':
      *recip = ROM;
      return (MTEAM);
      break;
   case 'K':
      *recip = KLI;
      return (MTEAM);
      break;
   case 'O':
      *recip = ORI;
      return (MTEAM);
      break;
   case 'G':
      *recip = 0;
      return (MGOD);
      break;
   case '0':
   case '1':
   case '2':
   case '3':
   case '4':
   case '5':
   case '6':
   case '7':
   case '8':
   case '9':
      if (players[addr - '0'].p_status == PFREE) {
         warning("That player left the game. message not sent.");
         return 0;
      }
      *recip = addr - '0';
      return (MINDIV);
      break;
   case 'a':
   case 'b':
   case 'c':
   case 'd':
   case 'e':
   case 'f':
   case 'g':
   case 'h':
   case 'i':
   case 'j':
   case 'k':
   case 'l':
   case 'm':
   case 'n':
   case 'o':
   case 'p':
   case 'q':
   case 'r':
   case 's':
   case 't':
   case 'u':
   case 'v':
   case 'w':
   case 'x':
   case 'y':
   case 'z':
      if (players[addr - 'a' + 10].p_status == PFREE) {
         warning("That player left the game. message not sent.");
         return 0;
      }
      *recip = addr - 'a' + 10;
      return (MINDIV);
      break;
   default:
      warning("Not legal recipient");
   }
   return 0;
}

void
pnbtmacro(c)
   int                             c;
{
   switch (macro[c].who) {
   case 'A':
      pmessage(macro[c].string, 0, MALL);
      break;
   case 'F':
      pmessage(macro[c].string, FED, MTEAM);
      break;
   case 'R':
      pmessage(macro[c].string, ROM, MTEAM);
      break;
   case 'K':
      pmessage(macro[c].string, KLI, MTEAM);
      break;
   case 'O':
      pmessage(macro[c].string, ORI, MTEAM);
      break;
   case 'T':
      pmessage(macro[c].string, me->p_team, MTEAM);
      break;
   }
}

int
rcd_lines()
{
   register			i;
   register struct dmacro_list  *d;
   for(i=0, d=&dist_prefered[1]; d->c; i++,d++)
      ;
   return i;
}

void
showRCDs(row)

   int	row;
{
   register                     i, j;
   register struct dmacro_list  *d;
   char                         buf[BUFSIZ], buf2[3], *title;

   row ++;
   title = "RCD (<control key> [mapped key] <descriptor> <macro>):";
   W_WriteText(macroWin, 1, row, W_Yellow, title, strlen(title), 
      W_RegularFont);
   row += 2;

   for(i=row, d=&dist_prefered[1]; d->c; i++,d++){
      /* try to find it in keymap */
      for(j=0; j< MAXKEY; j++){
	 if(mykeymap[j] != d->c+96)
	    continue;
	 if(j + 32 == d->c+96)
	    continue;
	 break;	/* only room for 1 currently */
      }
      if(j != MAXKEY){
	 /* control key? */
	 if(j > 96){
	    buf2[0] = '^';
	    buf2[1] = j-96+32;
	 }
	 else{
	    buf2[0] = ' ';
	    buf2[1] = j+32;
	 }
	 buf2[2] = 0;

	 sprintf(buf, "^%c %s %-14s %s",
	    d->c, buf2, d->name, d->macro);
      }
      else
	 sprintf(buf, "^%c    %-14s %s",
	    d->c, d->name, d->macro);

      W_WriteText(macroWin, 2, i, textColor,
         buf, strlen(buf), W_RegularFont);
   }
}
