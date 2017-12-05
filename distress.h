#ifndef _distress_h_
#define _distress_h_
/*
 * distress.h
 */
#include "copyright.h"

struct dmacro_list
{
    char c;
    char *name;
    char *macro;
};

#ifndef _in_distress_c_

extern int	F_gen_distress;
extern char *singleMacro;
extern struct dmacro_list *distmacro;
extern struct dmacro_list dist_defaults[];
extern struct dmacro_list dist_prefered[];
extern int sizedist;

#endif /* _in_distress_c_ */


#define MDISTR  0xC0
#define NEWMULTIM       4


enum dist_type
{
    /* help me do series */
    take = 1, ogg, bomb, space_control,
    save_planet,
    base_ogg,
    help3, help4,

    /* doing series */
    escorting, ogging, bombing, controlling,
    asw,
    asbomb,
    doing3, doing4,

    /* other info series */
    free_beer,                  /* ie. player x is totally hosed now */
    no_gas,                     /* ie. player x has no gas */
    crippled,                   /* ie. player x is way hurt but may have gas */
    pickup,                     /* player x picked up armies */
    pop,                        /* there was a pop somewhere */
    carrying,                   /* I am carrying */
    other1, other2,

    /* just a generic distress call */
    generic
};

struct distress
{
    unsigned char sender;
    unsigned char dam, shld, arms, wtmp, etmp, fuelp, sts;
    unsigned char wtmpflag, etempflag, cloakflag, distype, macroflag;
    unsigned char close_pl, close_en, tclose_pl, tclose_en, pre_app,
      i;
    unsigned char close_j, close_fr, tclose_j, tclose_fr;
    unsigned char cclist[6];    /* allow us some day to cc a message up to 5 people */
    /* sending this to the server allows the server to do the cc action */
    /* otherwise it would have to be the client ... less BW this way */
    char preappend[80];         /* text which we pre or append */
};


/* The General distress has format:

   byte1: 00yzzzzz
   where zzzzz is dist_type, and y is 1 if this is a more complicated macro
   and not just a simple distress (a simple distress will ONLY send ship
   info like shields, armies, status, location, etc.). I guess y=1 can be for !
   future expansion.

   byte2: 1fff ffff - f = percentage fuel remaining (0-100)
   byte3: 1ddd dddd - % damage
   byte4: 1sss ssss - % shields remaining
   byte5: 1eee eeee - % etemp
   byte6: 1www wwww - % wtemp
   byte7: 100a aaaa - armies carried
   byte8: (lsb of me->p_status) & 0x80
   byte9: 1ppp pppp - planet closest to me
   byte10: 1eee eeee - enemy closest to me
   byte11: 1ppp pppp - planet closest to target
   byte12: 1eee eeee - enemy closest to target
   byte13: 1ttt tttt - tclose_j
   byte14: 1jjj jjjj - close_j
   byte15: 1fff ffff - tclose_fr
   byte16: 1ccc cccc - close_fr
   byte17+: cc list (each player to cc this message to is 11pp ppp)
   cc list is terminated by 0x80 (pre-pend) or 0100 0000 (append) )
   byte18++: the text to pre or append .. depending on termination above.
   text is null terminated and the last thing in this distress
 */

#endif /* _distress_h_ */
