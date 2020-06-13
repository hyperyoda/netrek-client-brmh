#ifdef RECORD

/*
 * Recording implementation by Dave Pinkney (dpinkney@cs.uml.edu)
 * Based on the Paradise (Netrek II) recording implemenation, 
 * author unknown.
 */

#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#ifdef hpux
#include <time.h>
#else				/* hpux */
#include <sys/time.h>
#endif				/* hpux */
#include <string.h>

#include "netrek.h"
#include "recorder.h"

#define DRAW_TIME 10
#define change_recorder_status()  update_dashboard = 1

FILE           *recordFile = NULL;     /* recorder */
char           *recordFileName = NULL; /* Added recorder <isae> */
char            tempfilename[] = "noFileName.trk"; /* default */
int             recordGame = 0;	       /* Don't record the game by default */
int             maxRecord = 0;         /* Max bytes to record */
int             recordIndiv=0;         /* Record individual messages?  */
int             confirmOverwrite=1;    /* Confirm overwrite of existing file */

long int bytes_recorded=0;             /* How many bytes have been recorded */
long int packets_recorded=0;           /* How many packets were recorded */
long int updates_recorded=0;           /* How fake update packets recorded */
int teamReq = -1;                      /* Used to keep track of our team */
int playback = 0;                      /* Playback mode? */
int playback_update = 0;               /* When to redraw */
int update_dashboard = 0;              /* Playback dashboard status change */
enum PlaybackMode pb_mode = CONT;      /* What playback mode */
int pb_paused = 0;                     /* Pause status */ 
int playback_terminate=0;              /* Did we quit or hit EOF */
int interval = 200 - DRAW_TIME;        /* Frame rate in milliseconds */
int fps = 5;                           /* Frames per second */
enum AlertMode pb_alert = PB_NONE;     /* Alert status during playback */
enum AlertMode pb_alert_scan = PB_NONE; /* What alert mode we're scanning for*/
int not_first_entry = 0;                /* First entry into the game */
int help_on = 0;                       /* In help mode? */

/* Attempt to record / playback in paradise compatibility mode. This entails
   using shortversion 10 (OLDSHORTVERSION) for record/playback and disabling 
   our use of the SHIP_CAP feature during record */
int paradise_compat = 0;                   

/* Function prototype */
#if __STDC__ || defined(__cplusplus)
extern int W_Sync(void);
#else
extern int W_Sync();
#endif


/* Return <0 to stop recorder, 0 to overwrite existing file, >0 if they got 
 *  new name
 * User can overwrite existing file, abort recording, or select a new 
 * filename
 */
int confirm_overwrite()
{
  static char new_name[200];
  char response[5];
  int done;

  done = 0;

  while(!done) {
    printf("\a\nRecord file \"%s\" exists!  Continue?\n", 
	    recordFileName);
    printf("Y: Overwrite, N: New name, Q: Stop recorder (y/n/q):\n");

    if(fgets(response, 5, stdin) == NULL)
      return -1;

    if(response[0] == 'y' || response[0] == 'Y')
      return 0;
    else if(response[0] == 'q' || response[0] == 'Q')
      return -1;
    else done=1;
  }
  printf("\nEnter the new filename:\n");

  if(fgets(new_name, 195, stdin) == NULL)
    return -1;

  if(new_name[strlen(new_name)-1] == '\n')
    new_name[strlen(new_name)-1] = '\0';

  recordFileName = new_name;
  return 1;
}


void check_record_filename()
{
  int accessible, confirm;

  if(recordFileName == NULL) {
    fprintf(RECORDFD, "recordFileName is NULL, will try using \"%s\".\n",
	    tempfilename);
    recordFileName = tempfilename;
  }

  confirmOverwrite = booleanDefault("confirmOverwrite", confirmOverwrite);
  if(!confirmOverwrite)
    return;

  /* First see if the file already exists, if it does require overwrite
     confirmation */
  while((accessible = access(recordFileName, F_OK)) == 0) {
    /* Files exists, get confirm */
    confirm = confirm_overwrite();
    if(confirm < 0) {
      recordGame = 0;
      printf("\nOk, recording is disabled.\n");
      return;
    }
    else if(confirm == 0) {
      printf("\nOk, will overwrite file \"%s\".\n", recordFileName);
      break;
    }
    else printf("\nOk, new record file is \"%s\".\n", recordFileName);
    /* confirm > 0, check the new filename to see if it exists */
  }
}


void startRecorder()
{

#ifdef RECORD_DEBUG
  fprintf(RECORDFD, "startRecorder(): maxRecord=%ld\n", maxRecord);
#endif

  if((recordFile = fopen(recordFileName, "wb")) == NULL) {
    fprintf(RECORDFD, 
	    "Couldn't open \"%s\" for writing.  Recording disabled.\n",
	    recordFileName);
    recordGame=0;
    return;
  }

  bytes_recorded = packets_recorded = updates_recorded = 0;
  fprintf(RECORDFD, "Recording game to file \"%s\".\n", recordFileName);
}


void stopRecorder()
{
  fclose(recordFile);
  recordGame=0;
  fprintf(RECORDFD, "Recorder stats:\n");
  fprintf(RECORDFD, 
	  "Wrote %ld packets (%ld bytes), with %ld fake updates to \"%s\".\n",
	  packets_recorded, bytes_recorded, updates_recorded, recordFileName);
}


void stopPlayback()
{
  if(playback_terminate)
    fprintf(RECORDFD, "Playback terminated.  Playback stats:\n");
  else fprintf(RECORDFD, "Playback file completed.  Playback stats:\n");

  fprintf(RECORDFD, 
	  "Read %ld packets (%ld bytes), with %ld fake updates from \"%s\".\n",
	  packets_recorded, bytes_recorded, 
	  updates_recorded, recordFileName);
}


void recordPacket(bufptr, size)
     char *bufptr;
     int size;
{
  static struct mesg_spacket *packet;
  static struct mesg_s_spacket *spacket;

#ifdef RECORD_DEBUG
  fprintf(RECORDFD, "%3d %3d\n", (unsigned int)(*bufptr), size);
#endif

  if(*bufptr == SP_PICKOK) {
    bufptr[2] = (char)teamReq;
  }
  else if(!recordIndiv) {
    /* If this is indiv message, skip it */
    if(*bufptr == SP_MESSAGE) {
      packet = (struct mesg_spacket *)bufptr;
      if(packet->m_flags & MINDIV) {
	return;
      }
    }
    else if(*bufptr == SP_S_MESSAGE) {
      spacket = (struct mesg_s_spacket *)bufptr;
      if(spacket->m_flags & MINDIV) {
	return;
      }
    }
  }

  if(maxRecord && (bytes_recorded + size) > maxRecord) {
    stopRecorder();
    warning("maxRecord reached, recoder stopped Captain!");
    return;
  }
  
  if((fwrite(bufptr, 1, size, recordFile)) != size) {
    fprintf(RECORDFD, "Error recording to \"%s\"!  Recording disabled.\n",
	    recordFileName);
    fclose(recordFile);
    recordFile = NULL;
    recordGame=0;
  }
  else {
    bytes_recorded += size;
    packets_recorded++;
  }
}


void recordUpdate()
{
  unsigned char update_buf[4];

  update_buf[0] = RECORD_UPDATE;

  update_buf[1] = (unsigned char) me->p_tractor;
  update_buf[2] = (unsigned char) ((me->p_flags & PFPLOCK) ? 
				   me->p_playerl : me->p_planet);
  update_buf[3] = '\0';   /* unused */

  updates_recorded++;
  
  recordPacket(update_buf, 4);
}


void startPlayback()
{
  if(!playback)
    return;

  if(recordFileName == NULL) {
    fprintf(RECORDFD, "recordFileName is NULL, will try using \"%s\".\n",
	    tempfilename);
    recordFileName = tempfilename;
  }

  if((recordFile = fopen(recordFileName, "rb")) == NULL) {
    fprintf(RECORDFD, "startPlayback:  Couldn't open \"%s\" for reading.\n",
	    recordFileName);
    exit(1);
  }
  fprintf(RECORDFD, "Replaying game from \"%s\".\n", recordFileName);

  /* use them as 'read' not recorded for playback */
  bytes_recorded = packets_recorded = updates_recorded = 0;
}


/* 
 * Add a new field to the statline, the current update number.
 * Don't use packet numbers because they advance much more quickly
 * and we only redraw on updates anyways.
 */
void playback_add_updates(char *buf)
{
#ifdef NO_SPRINTF
   {
      extern char    *itoa();
      register char *s = buf;

      if(updates_recorded / 10000) {
	s = itoa(s, updates_recorded / 10000, 4, 1);
	s = itoa(s, updates_recorded % 10000, 4, 2);
      }
      else {
	strcpy(buf, "    ");
	s = &buf[4];
	s = itoa(s, updates_recorded % 10000, 4, 1);
      }
      buf[8] = '\0';
   }
#else
   sprintf(buf, "%8ld", updates_recorded);
#endif
}


/* Add the current playback mode to buf: Add 7 characters always */
void playback_status(buf)
     char *buf;
{
  switch(pb_mode)
    {
    case CONT:
      if(pb_paused)
	strcpy(buf, "P  Cont");
      else strcpy(buf, "   Cont");
      break;
    case FRAME:
      if(pb_paused)
	strcpy(buf, "P  Fram");
      else strcpy(buf, "   Fram");
      break;
    case SCAN:
      if(pb_paused)
	strcpy(buf, "P");
      else strcpy(buf, " ");
      switch(pb_alert_scan) {
      case PB_DEATH:
	strcat(buf, "D ");
	break;
      case PB_RED:
	strcat(buf, "R ");
	break;
      case PB_YELLOW:
	strcat(buf, "Y ");
	break;
      case PB_GREEN:
	strcat(buf, "G ");
	break;
      case PB_NONE:
      default:
	strcat(buf, "  ");
	break;
      }
      strcat(buf, "Scan");
      break;
    default:
      fprintf(RECORDFD, "Error, unknown playback mode %d!\n", pb_mode);
      exit(1);
    }
}

/* 
 * Replace the clock with a counter of the number of updates.
 * Don't use packet numbers because they advance much more quickly
 * and we only redraw on updates anyways.
 */
void playback_clock()
{
   char            timebuf[9];

#ifdef NO_SPRINTF
   {
      extern char    *itoa();
      register char *s = timebuf;

      if(updates_recorded / 10000) {
	s = itoa(s, updates_recorded / 10000, 4, 1);
	s = itoa(s, updates_recorded % 10000, 4, 2);
      }
      else {
	strcpy(timebuf, "    ");
	s = &timebuf[4];
	s = itoa(s, updates_recorded % 10000, 4, 1);
      }
      timebuf[8] = '\0';
   }
#else
   sprintf(timebuf, "%8ld", updates_recorded);
#endif

   if(dashboardStyle == 0)
     W_WriteText(tstatw, 50 + (66 * W_Textwidth), 27, textColor, timebuf,
		 8, W_RegularFont);
   else W_WriteText(tstatw, 50 + (66 * W_Textwidth), 30, textColor, timebuf,
		    8, W_RegularFont);
}



/* Return bytes read or elements read? */
int readPlayback(f, buff, size)
     FILE *f;
     char *buff;
     int size;
{
  int num;

  if(!playback || size < 0) {
    fprintf(RECORDFD, "readPlayback: Error, playback=%d, packet size=%d.\n",
	    playback, size);
    return -1;
  }

  if(size > 4)
    size = 4;

  num = fread(buff, 1, size, f);

  if(num != size) {
    if(feof(f) || ferror(f)) {
      if(feof(f)) {
	stopPlayback();
	exit(0);
      }
      else {
	fprintf(RECORDFD, "readPlayback: File error while reading \"%s\".\n",
		recordFileName);
	exit(1);
      }
    }
    else {
      fprintf(RECORDFD, 
	      "readPlayback: read %d of %d bytes, but no feof or ferror!\n",
	      num, size);
    }
  }
  else bytes_recorded += num;

  return num;
}



/* Return true if we used the key, otherwise return 0 */
int playback_keyaction(data)
     W_Event *data;
{
  unsigned char key = data->key;
  int status = 1;
  char warn[160], *s;

  warn[0] = '\0';

  switch(key) 
    {
    case 'c':
    case 'C':
      if(help_on) {
	help_on = 0;
	warning("C:  Enter continuous playback mode (playback with selectable frame rate)");
      }
      else {
	pb_mode = CONT;
	if(pb_paused)
	  warning("Cont Mode:  1-9:Set fps  0:Max  +:Faster  -:Slower,  P:Unpause  ?:Help  Q:Quit");
	else warning("Cont Mode:  1-9:Set fps  0:Max  +:Faster  -:Slower,  P:Pause  ?:Help  Q:Quit");
      }
      break;
    case '0':
      if(help_on) {
	help_on = 0;
	warning("0:  In Continuous mode, playback as fast as possible");
      }
      else {
	if(pb_mode == CONT) {
	  interval = 0;        /* Full blast mode */
	  fps = 50;
	  if(updates_recorded % 2 == 0)
	    warning("I'm givin' ya all she's got, Captain!");
	  else warning("She's fallin' apart at the seams, Captain!");
	}
	else status=0;
      }
      break;
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      if(help_on) {
	help_on = 0;
	sprintf(warn, "%d:  In Continuous mode, playback at %d frames per second", (key - '0'), (key - '0'));
	warning(warn);
      }
      else {
	if(pb_mode == CONT) {
	  fps = key - '0';
	  interval = 1000 / fps -  DRAW_TIME;
	  strcpy(warn, "Continuous mode: ");
	  s = &warn[strlen(warn)];
	  s = itoa(s, fps, 2, 1);
	  *s++ = ' ';
	  strcpy(s, "frames per second");
	  warning(warn);
	}
	else status=0;
      }
      break;
    case '+':
      if(help_on) {
	help_on = 0;
	warning("+:  In Continuous mode, increment the playback frame rate");
      }
      else {
	if(pb_mode == CONT) {
	  fps++;
	  if(fps > 50) {
	    fps = 50;
	    interval = 0;
	  }
	  else {
	  interval = 1000 / fps - DRAW_TIME;
	  if(interval <= 0)
	    interval = 0;
	  }
	  
	  strcpy(warn, "Continuous mode: ");
	  s = &warn[strlen(warn)];
	  s = itoa(s, fps, 2, 1);
	  *s++ = ' ';
	  strcpy(s, "frames per second");
	  warning(warn);
	}
	else status=0;
      }
      break;
    case '-':
      if(help_on) {
	help_on = 0;
	warning("-:  In Continuous mode, decrement the playback frame rate");
      }
      else {
	if(pb_mode == CONT) {
	  fps--;
	  if(fps <= 0) {
	    fps = 0;
	    interval = 2000;     /* 1 frame per 2 seconds is slowest */
	  }
	  else {
	    interval = 1000 / fps - DRAW_TIME;
	    if(interval < 0)
	      interval = 0;
	  }
	  
	  strcpy(warn, "Continuous mode: ");
	  
	  if(fps == 0) {
	    strcat(warn, ".5 frames per second");
	  }
	  else {
	    s = &warn[strlen(warn)];
	    s = itoa(s, fps, 2, 1);
	    *s++ = ' ';
	    strcpy(s, "frames per second");
	  }
	  warning(warn);
	}
	else status=0;
      }
      break;
    case 'F':
    case 'f':
      if(help_on) {
	help_on = 0;
	warning("F:  Enter frame at a time mode (single step through recording)");
      }
      else {
	pb_mode = FRAME;
	if(pb_paused)
	  warning("Frame Mode:  Spacebar single steps,  P: Unpause  ?: Help  Q: Quit");
	else warning("Frame Mode:  Spacebar single steps,  P: Pause  ?: Help  Q: Quit");
      }
      break;
    case ' ':
      if(help_on) {
	help_on = 0;
	warning("Space:  In Frame mode, advance the recording one frame");
      }
      else {
	if(pb_mode == FRAME) {
	  /* Single step by setting playback_update to 0. This will cause us
	     to read up to an update.  Otherwise,for FRAME mode playback_update
	     is set to 1, causing us to skip reading and just redraw */
	  playback_update = 0;
	  if(pb_paused)
	    pb_paused = 0;
	}
	else status=0;
	/* else fprintf(RECORDFD, "space\n"); */
      }
      break;
    case 's':
    case 'S':
      if(help_on) {
	help_on = 0;
	warning("S:  Enter scan mode (scan for an alert status and pause)");
      }
      else {
	if(pb_mode != SCAN) {
	  pb_alert_scan = PB_NONE;
	  pb_paused = 1;
	}
	pb_mode = SCAN;
	if(pb_paused)
	  warning("Scan Mode:  R:Red  Y:Yellow  G:Green  D:Death,  P:Unpause  ?:Help  Q:Quit");
	else warning("Scan Mode:  R:Red  Y:Yellow  G:Green  D:Death,  P:Pause  ?:Help  Q:Quit");
	
	/* Set our current alert status */
	if(me->p_flags & PFRED) {
	  pb_alert = PB_RED;
	  /* fprintf(RECORDFD, "Starting alert:  RED\n"); */
	}
	else if(me->p_flags & PFYELLOW) {
	  pb_alert = PB_YELLOW;
	  /* fprintf(RECORDFD, "Starting alert:  YELLOW\n"); */
	}
	else if(me->p_flags & PFGREEN) {
	  pb_alert = PB_GREEN;
	  /* fprintf(RECORDFD, "Starting alert:  GREEN\n"); */
	}
	else if(me->p_status != PALIVE) {
	  pb_alert = PB_DEATH;
	  /* fprintf(RECORDFD, "Starting alert:  DEATH\n"); */
	}
	else {
	  pb_alert = PB_NONE;
	  /* fprintf(RECORDFD, "Starting alert:  NONE\n"); */
	}
      }
      break;
    case 'R':
    case 'r':
      if(help_on) {
	help_on = 0;
	warning("R:  In Scan mode, scan for next occurrence of Red Alert");
      }
      else {
	if(pb_mode == SCAN) {
	  pb_alert_scan = PB_RED;
	  pb_paused = 0;
	  
	  if(me->p_flags & PFRED) {
	    pb_alert = PB_RED;
	  }
	  else if(me->p_flags & PFYELLOW) {
	    pb_alert = PB_YELLOW;
	  }
	  else if(me->p_flags & PFGREEN) {
	    pb_alert = PB_GREEN;
	  }
	  else if(me->p_status != PALIVE) {
	    pb_alert = PB_DEATH;
	  }
	}
	else status=0;
      }
      break;
    case 'Y':
    case 'y':
      if(help_on) {
	help_on = 0;
	warning("Y:  In Scan mode, scan for next occurrence of Yellow Alert");
      }
      else {
	if(pb_mode == SCAN) {
	  pb_alert_scan = PB_YELLOW;
	  pb_paused = 0;
	  
	  if(me->p_flags & PFRED) {
	    pb_alert = PB_RED;
	  }
	  else if(me->p_flags & PFYELLOW) {
	    pb_alert = PB_YELLOW;
	  }
	  else if(me->p_flags & PFGREEN) {
	    pb_alert = PB_GREEN;
	  }
	  else if(me->p_status != PALIVE) {
	    pb_alert = PB_DEATH;
	  }
	}
	else status=0;
      }
      break;
    case 'G':
    case 'g':
      if(help_on) {
	help_on = 0;
	warning("G:  In Scan mode, scan for next occurrence of Green Alert");
      }
      else {
	if(pb_mode == SCAN) {
	  pb_alert_scan = PB_GREEN;
	  pb_paused = 0;
	  
	  if(me->p_flags & PFRED) {
	    pb_alert = PB_RED;
	  }
	  else if(me->p_flags & PFYELLOW) {
	    pb_alert = PB_YELLOW;
	  }
	  else if(me->p_flags & PFGREEN) {
	    pb_alert = PB_GREEN;
	  }
	  else if(me->p_status != PALIVE) {
	    pb_alert = PB_DEATH;
	  }
	}
	else status=0;
      }
      break;
    case 'D':
    case 'd':
      if(help_on) {
	help_on = 0;
	warning("D:  In Scan mode, scan for next Death");
      }
      else {
	if(pb_mode == SCAN) {
	  pb_alert_scan = PB_DEATH;
	  pb_paused = 0;
	  if(me->p_status != PALIVE)
	    pb_alert = PB_DEATH;
	}
	else status=0;
      }
      break;
    case 'p':
    case 'P':
      if(help_on) {
	help_on = 0;
	warning("P:  Toggle pause status");
      }
      else {
	pb_paused = !pb_paused;
      }
      break;
    case 'Q':
    case 'q':
      if(help_on) {
	help_on = 0;
	warning("Q:  Quit playback mode");
      }
      else {
	playback_terminate=1;
	stopPlayback();
	exit(0);
      }
      break;
    case '?':
      if(help_on) {
	warning("?:  Get help on a mode or command");
	help_on = 0;
      }
      else {
	help_on = 1;
	strcpy(warn, "Help on what key?  General: C,F,S,P,Q  Mode-Specific: ");
	switch(pb_mode) {
	case CONT:
	  strcat(warn, "1,2,3,4,5,6,7,8,9,0,+,-");
	  break;
	case FRAME:
	  strcat(warn, "space");
	  break;
	case SCAN:
	  strcat(warn, "G,Y,R,D");
	  break;
	default:
	  break;
	}
	warning(warn);
      }
      break;
    default:
      status = 0;       /* We didn't handle it, let keyaction try */
      break;
    }

  if(status) {
    /* Something changed, update status window */
    change_recorder_status();
  }
  else {
    if(help_on) {
      strcpy(warn, "That key is not a playback command");
      help_on = 0;
    }
    else {
      /* Wrong key pressed, give info and let keyaction try */
      strcpy(warn, "Modes:  C: Continuous  F: Frame  S: Scan, ");
      if(pb_paused)
	strcat(warn, "P: Unpause  ?: Help");
      else strcat(warn, "P: Pause  ?: Help");
      strcat(warn, "  Q: Quit");
    }
      warning(warn);
  }

  return status;
}



void playback_input()
{
   fd_set          readfds;
   register int    xsock = W_Socket();
   struct timeval timeout, time1, time2, *last, *now, *tmp;
   int lapse, cont_skip_draw=0;
   char warn[100];

   /****  For timing tests
   int count=0, beforeloop=0, loopcount=0;
   long int avg=0;
   long int drawavg=0;
   ****/

#ifdef RECORD_DEBUG
   fprintf(RECORDFD, "playback_input() called...\n");
#endif

   /* Get things started off */

   updatePlayer[me->p_no] = 1;  /* Make sure we're added to playerlist */

   if (W_IsMapped(playerw)) {  
     playerlist();
   }

   last = &time1;
   now = &time2;

   gettimeofday(last, NULL);
   gettimeofday(now, NULL);

   max_fd = xsock + 1;   /* for select */


   db_redraw(1);

   /* First time this function is called */
   if(!not_first_entry) {
     not_first_entry=1;
     pb_paused = 1;

     strcpy(warn, "Modes:  C: Continuous  F: Frame  S: Scan, ");
     if(pb_paused)
       strcat(warn, "P: Unpause  ?: Help");
     else strcat(warn, "P: Pause  ?: Help");
     strcat(warn, "  Q: Quit");
   }

   W_Sync();

#ifdef RECORD_DEBUG
   fprintf(RECORDFD, "Entering playback loop...\n");
#endif

   while (1) {


     FD_ZERO(&readfds);
     FD_SET(xsock, &readfds);

     /* Mode specific stuff */
     switch(pb_mode) {
     case CONT:

       /* 
	* Since we can't predict how long packet processing & redrawing will
	* take, use system time to determine how long its been since we last
	* entered the loop, and thus how long we should wait before updating
	* again. Hopefully this will keep things flowing smoothly. 
	*/
       
       gettimeofday(now, NULL);


       /* Elapsed time in milliseconds since last loop */
       lapse = (now->tv_sec - last->tv_sec) * 1000 +
	 (now->tv_usec - last->tv_usec) / 1000;
       
       /********* Timing tests
	 avg += lapse;
	 count++;
	 
	 if(count == 200) {
	 fprintf(RECORDFD, "Average loop time: %f\n",
	 (float)avg/(float)count);
	 avg = 0;
	 count = 0;
	 }
	 
	 fprintf(RECORDFD, "%3d ", lapse);
	 if(count++ == 18) {
	 count=0;
	 fprintf(RECORDFD, "\n");
	 }
	 ********/
       
       /* fprintf(RECORDFD, "mstime(): %d, lapse=%d\n", mstime(), lapse); */
       
       lapse -= interval;   /* we want an update every interval, schedule
			       for the difference */
       
       if(lapse >= interval) {
	 /* Select shouldn't wait, we're late */
       timeout.tv_sec = timeout.tv_usec = 0;
       }
       else {
	 /* How much time is left */
	 if(lapse < 0)   /* Never wait longer then the interval */
	   lapse = 0;
	 
	 /* Linux select needs this in the loop (select modifies timeout) */
	 timeout.tv_sec = (interval - lapse)/1000;
	 timeout.tv_usec = ((interval - lapse) % 1000) * 1000;
       }

       /*** Timing tests
	 fprintf(RECORDFD, "last=(%d,%d), now=(%d,%d), otherlapse=%d\n",
	 last->tv_sec, last->tv_usec, now->tv_sec, now->tv_usec, 
	 lapse);
	 fprintf(RECORDFD, "timeout=(%d,%d)\n", 
	 timeout.tv_sec, timeout.tv_usec);
        ***/

       if(pb_paused) {
	 /* While it's paused use a reasonable frame rate, reduce flicker */
	 timeout.tv_sec = 0;
	 timeout.tv_usec = 200000;
       }
       select(max_fd, &readfds, 0, 0, &timeout);
       break;   /* pb_mode == CONT */
     case FRAME:
       select(max_fd, &readfds, 0, 0, 0);

       playback_update = 1;  /* Default action is to not read anything */

       /* If space is hit this is set to 0, causing us to advance a frame */
       break;
     case SCAN:
       if(pb_paused) {
	 timeout.tv_sec = 0;
	 timeout.tv_usec = 200000;
       }
       else timeout.tv_sec = timeout.tv_usec = 0;

       select(max_fd, &readfds, 0, 0, &timeout);
       break;
     default:
       break;
     }  /* switch(pb_mode) */

     if (FD_ISSET(xsock, &readfds) || W_EventsQueued()) {
       /* playback_update = 1; 
	  If we want to redraw screen without reading packets first set
	  this */
       
       process_event();

       if(pb_mode == CONT) {
	 /* Don't read more packets now, it may be too soon */
	 cont_skip_draw=1;
       }
     }

     if(pb_mode == CONT) {
       if(cont_skip_draw) {
	 /* Keep the old times, we didn't read packets this loop */
	 cont_skip_draw = 0;
	 playback_update = 1;
	 /* fprintf(RECORDFD, "no read..\n"); */
       }
       else {
	 /* Swap timeval structs so "now" is "last" next time thru */
	 tmp = last;
	 last = now;
	 now = tmp;
       }
     }
       
     /* Pause is implemented by not reading packets */
     if(pb_paused)
       playback_update=1; /* This avoids packet reading and just draws */
     intrupt(&readfds);   /* Actual packet reading done here */

     if(not_first_entry == 1) {
       warning(warn);
       not_first_entry =2;
     }

     W_Sync();    /* Need this to get xevents out on time and for INL server */
   }
}


#endif
