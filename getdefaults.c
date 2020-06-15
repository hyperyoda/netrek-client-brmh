/*
 * getdefaults.c : Read defaults from .xtrekrc file here <isae@iastate.edu>
 */

#include "copyright2.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include "netrek.h"

extern int      oldBless;	/* from main.c */
#ifdef RECORD
#include "recorder.h"
#else
#define RECORDFD stdout
#endif

void
readDefaults()
{
   char           *s;
   extern int      bw;
   int             dashboard=0;

   if(defaults_file)
     fprintf(RECORDFD, "Reading default values from '%s'... ", defaults_file);

   /* Read keymap */
   initkeymaps();

   /* Boolean defaults */
#ifdef SHOW_DEFAULTS
   {
      char            buf[10];
      sprintf(buf, "%d", namemode);
      show_defaults("display", "showPlanetNames", buf,
		    "Show planet names (0-off, 1-short, 2-normal)");
   }
#endif
   namemode = intDefault("showPlanetNames", namemode);
#ifdef SHOW_DEFAULTS
   show_defaults("display", "showStats", showStats ? "on" : "off",
		 "Show stats window");
#endif
   showStats = booleanDefault("showStats", showStats);

#ifdef SHOW_DEFAULTS
   show_defaults("display", "showShields", showShields ? "on" : "off",
		 "Show ship shields");
#endif
   showShields = booleanDefault("showShields", showShields);
#ifdef SHOW_DEFAULTS
   show_defaults("misc", "keepPeace", keeppeace ? "on" : "off",
		 "Keep peace with races after death.");
#endif
   keeppeace = booleanDefault("keepPeace", keeppeace);
#ifdef SHOW_DEFAULTS
   show_defaults("window", "phaserWindow", phaserWindow ? "on" : "off",
		 "Show phaser-hit messages in seperate window.  See also review_phaser\n\
for default mapping and geometry (message widths 13, 32, 80 supported).");
#endif
   phaserWindow = booleanDefault("phaserWindow", phaserWindow);
#ifdef SHOW_DEFAULTS
   show_defaults("misc", "reportKills", reportKills ? "on" : "off",
		 "Show kill messages.");
#endif

   reportKills = booleanDefault("reportKills", reportKills);
#ifdef SHOW_DEFAULTS
   show_defaults("misc", "reportKillsInReview",
		 reportKillsInReview ? "on" : "off",
		 "Show kill messages in the review (all messages) window.");
#endif
   reportKillsInReview = booleanDefault("reportKillsInReview",
					reportKillsInReview);

#ifdef SHOW_DEFAULTS
   show_defaults("misc", "reportAllInReview",
		 reportAllInReview ? "on" : "off",
	    "Show all-board messages in the review (all messages) window.");
#endif
   reportAllInReview = booleanDefault("reportAllInReview",
				      reportAllInReview);

#ifdef SHOW_DEFAULTS
   show_defaults("misc", "reportTeamInReview",
		 reportTeamInReview ? "on" : "off",
	   "Show team-board messages in the review (all messages) window.");
#endif
   reportTeamInReview = booleanDefault("reportTeamInReview",
				       reportTeamInReview);

#ifdef SHOW_DEFAULTS
   show_defaults("misc", "reportIndInReview",
		 reportIndInReview ? "on" : "off",
	   "Show individual messages in the review (all messages) window.");
#endif
   reportIndInReview = booleanDefault("reportIndInReview",
				      reportIndInReview);

#ifdef SHOW_DEFAULTS
   show_defaults("misc", "reportPhaserInReview",
		 reportPhaserInReview ? "on" : "off",
	   "Show phaser messages in the review (all messages) window.");
#endif
   reportPhaserInReview = booleanDefault("reportPhaserInReview",
				      reportPhaserInReview);

#ifdef MOOBITMAPS
#ifdef SHOW_DEFAULTS
   show_defaults("display", "newPlanetBitmaps", myPlanetBitmap ? "on" : "off",
		 "Use MOO planet bitmaps.");
#endif
   myPlanetBitmap = booleanDefault("newPlanetBitmaps", myPlanetBitmap);
#endif				/* MOOBITMAPS */
#ifdef SHOW_DEFAULTS
   show_defaults("display-tractor", "showTractorPressor", showTractorPressor ? "on" : "off",
		 "Show your own tractor/pressor.");
#endif
   showTractorPressor = booleanDefault("showTractorPressor", showTractorPressor);
#ifdef SHOW_DEFAULTS
   show_defaults("display", "fillTriangle", fillTriangle ? "on" : "off",
		 "If locks are shown, whether or not to fill the triangle.");
#endif
   fillTriangle = booleanDefault("fillTriangle", fillTriangle);
#ifdef SHOW_DEFAULTS
   show_defaults("display-tractor", "continueTractor", continueTractor ? "on" : "off",
		 "If off, only shows tractors for a short time.");
#endif
   continueTractor = booleanDefault("continueTractor", continueTractor);
#ifdef SHOW_DEFAULTS
   show_defaults("display", "extraAlertBorder", extraBorder ? "on" : "off",
		 "Also use inside border to show alert status.");
#endif
   extraBorder = booleanDefault("extraAlertBorder", extraBorder);
#ifdef NETSTAT
#ifdef SHOW_DEFAULTS
   show_defaults("misc", "netstats", netstat ? "on" : "off",
		 "Keep lag statistics.");
#endif
   netstat = booleanDefault("netstats", netstat);
#endif
#ifdef SHOW_DEFAULTS
   show_defaults("misc", "warp", warp ? "on" : "off",
	       "Warp the mouse to the message window during message send.");
#endif
   warp = booleanDefault("warp", warp);
#ifdef SHOW_DEFAULTS
   show_defaults("UDP", "tryUdp", tryUdp ? "on" : "off",
		 "Try to use udp upon entering the game.");
#endif
   tryUdp = booleanDefault("tryUdp", tryUdp);
#ifdef SHOW_DEFAULTS
   show_defaults("UDP", "udpSequenceChk", udpSequenceChk ? "on" : "off",
		 "Throw out udp packets that arrive out of order.");
#endif
   udpSequenceChk = booleanDefault("udpSequenceCheck", udpSequenceChk);

#ifdef SHOW_DEFAULTS
   show_defaults("display", "galacticFrequent", "on",
		 "Update galactic map frequently.");
#endif
   if (booleanDefault("galacticFrequent", 1))
      mapmode = 2;

#ifdef LOGMESG
#ifdef SHOW_DEFAULTS
   show_defaults("misc", "logMessage", logMess ? "on" : "off",
		 "Log messages.");
#endif
   logMess = booleanDefault("logMessage", logMess);
#endif
#ifdef SHOW_DEFAULTS
   show_defaults("display", "varyShields", VShieldBitmaps ? "on" : "off",
		 "Change shields to reflect damage.");
#endif
   VShieldBitmaps = booleanDefault("varyShields", VShieldBitmaps);

#ifdef SHOW_DEFAULTS
   show_defaults("display", "useMsgw", use_msgw ? "on" : "off",
		 "Display the last message in the warning message window.");
#endif
   use_msgw = booleanDefault("useMsgw", use_msgw);
#ifdef SHOW_DEFAULTS
   show_defaults("display", "phaserMsgI", phas_msgi ? "on" : "off",
		 "Display phaser-hit points in the individual window. (See also phaserWindow.)");
#endif
   phas_msgi = booleanDefault("phaserMsgI", phas_msgi);

#ifdef RECORD
   /* My attempt to control the recorder during the game */
#ifdef SHOW_DEFAULTS
   {
     char ints[11];
     show_defaults("Recorder", "recordGame", recordGame ? "on" : "off",
		   "Record game.");
     show_defaults("Recorder", "recordFile", "", 
		   "Name of file to record game in.");
     sprintf(ints, "%d", maxRecord);
     show_defaults("Recorder", "maxRecord", ints, 
		   "Max bytes of game to record.");
     show_defaults("Recorder", "recordIndiv", recordIndiv ? "on" : "off",
		   "Record individual messages.");
     show_defaults("Recorder", "confirmOverwrite", 
		   confirmOverwrite ? "on" : "off",
		   "Confirm overwrite of existing file by recorder.");
   }
#endif
   recordGame = booleanDefault("recordGame", recordGame);
   recordIndiv = booleanDefault("recordIndiv", recordIndiv);
   confirmOverwrite = booleanDefault("confirmOverwrite", confirmOverwrite);

   maxRecord =  intDefault("maxRecord", maxRecord);

   if (recordFileName == NULL) {
      recordFileName = getdefault("recordFile");
   }

#endif

   /* Numeric defaults */
#ifdef SHOW_DEFAULTS
   {
      char            ints[10];
      sprintf(ints, "%d", showlocal);
      show_defaults("display", "showLocal", ints,
		    "Show Local: 0 -- owner, 1 -- resoures, 2 -- nothing, 3 -- rabbit-ears");
   }
#endif
   showlocal = intDefault("showLocal", showlocal);
   if (showlocal > 3)
      showlocal = 3;
#ifdef SHOW_DEFAULTS
   {
      char            ints[10];
      sprintf(ints, "%d", showgalactic);
      show_defaults("display", "showGalactic", ints,
	"Galactic: 0 -- show owner, 1 -- show resoures, 2 -- show nothing");
   }
#endif
   showgalactic = intDefault("showGalactic", showgalactic);
   if (showgalactic > 2)
      showgalactic = 2;

#ifdef SHOW_DEFAULTS
   {
      char            ints[10];
      sprintf(ints, "%d", showLock);
      show_defaults("display", "showLock", ints,
		    "0 -- dont show lock, 1 -- show on galactic, \n\
2 -- show on local, 3 -- show on both.");
   }
#endif
   showLock = intDefault("showLock", showLock);
   if (showlocal > 3)
      showlocal = 3;
#ifdef SHOW_DEFAULTS
   {
      char            ints[10];
      sprintf(ints, "%d", udpDebug);
      show_defaults("UDP", "udpDebug", ints,
		    "Level of UDP code debugging.");
   }
#endif
   udpDebug = intDefault("udpDebug", udpDebug);
#ifdef SHOW_DEFAULTS
   {
      char            ints[10];
      sprintf(ints, "%d", udpClientSend);
      show_defaults("UDP", "udpClientSend", ints,
		    "0 -- TCP, 1 -- simple UDP, 2 -- enforced UDP (state only)\n\
3 -- enforce UDP (state & weapon)");
   }
#endif
   udpClientSend = intDefault("udpClientSend", udpClientSend);
#ifdef SHOW_DEFAULTS
   {
      char            ints[10];
      sprintf(ints, "%d", udpClientRecv);
      show_defaults("UDP", "udpClientReceive", ints,
		    "0 -- TCP, 1 -- simple UDP, 2 -- fat UDP");
   }
#endif
   udpClientRecv = intDefault("udpClientReceive", udpClientRecv);

#ifdef NETSTAT
#ifdef SHOW_DEFAULTS
   {
      char            ints[10];
      sprintf(ints, "%d", netstatfreq);
      show_defaults("misc", "netStatFreq", ints,
		    "Lag stats update: 1 -- least often, 10 -- most often.");
   }
#endif
   netstatfreq = intDefault("netstatfreq", netstatfreq);
   if (netstatfreq <= 0)
      netstatfreq = 1;
#endif

#ifdef SHORT_PACKETS
#ifdef SHOW_DEFAULTS
   show_defaults("C-startup", "tryShort", tryShort ? "on" : "off",
		 "Try short-packets [HW] upon entering.");
#endif
   tryShort = booleanDefault("tryShort", tryShort);
#endif

#ifdef EM
#ifdef SHOW_DEFAULTS
   show_defaults("display", "sortPlayers", sortPlayers ? "on" : "off",
		 "Sort player listings by team.");
   show_defaults("display", "sortPlayersObs", sortPlayersObs ? "on" : "off",
		 "Show players before observers.");
   show_defaults("display", "teamOrder", (!teamOrder ? "None (default)" :
					  (teamOrder == 1 ? "First" : "Last")),
		 "Order in which to list your team.");
#endif
   sortPlayers = booleanDefault("sortPlayers", sortPlayers);
   sortPlayersObs = booleanDefault("sortPlayersObs", sortPlayersObs);
   teamOrder =  intDefault("teamOrder", teamOrder);
#endif

#ifdef SHOW_DEFAULTS
   show_defaults("display", "newPlayerList", alt_playerlist ? "on" : "off",
		 "Reduced stats format for the player list. (Same as playerlist: nTRNKlM)\n\
Don't use with 'playerlist:'");
#endif
   alt_playerlist = booleanDefault("newPlayerList", alt_playerlist);

#ifdef SHOW_DEFAULTS
   show_defaults("display", "playerlist", "nTRNKWLrODd",
"Configurable player list format. Letters correspond to fields, which are\n\
 n -- ship number\n\
 T -- ship type\n\
 R -- rank\n\
 N -- name\n\
 K -- kills\n\
 l -- login name\n\
 O -- offense\n\
 W -- wins\n\
 D -- defense\n\
 L -- losses\n\
 S -- stats (total ratings)\n\
 r -- ratio\n\
 d -- DI (normalized ratings * hours)\n\
 B -- bombing\n\
 b -- armies bombed\n\
 P -- planets\n\
 p -- planets taken\n\
 M -- client machine\n\
 H -- hours played\n\
 k -- max kills\n\
 V -- kills/hour \n\
 v -- deaths/hour");
#endif
   
   plist = getdefault("playerlist");
   if(plist && alt_playerlist) alt_playerlist = 0;

#ifdef SHOW_DEFAULTS
   show_defaults("misc", "newMesgFlags", new_messages ? "on" : "off",
		 "Server-dependent. Should be on for all new servers.");
#endif
   new_messages = booleanDefault("newMesgFlags", new_messages);

#ifdef SHOW_DEFAULTS
   {
      char            ints[10];
      sprintf(ints, "%d", tclock);
      show_defaults("display", "clock", ints,
		    "Stat clock: 0 -- no clock, 1 -- h:m, 2 -- h:m:s.");
   }
#endif
   tclock = intDefault("clock", tclock);

#ifdef SHOW_DEFAULTS
   show_defaults("display", "shortKillMesg", abbr_kmesg ? "on" : "off",
		 "Shrink and line up all kill messages.");
#endif
   abbr_kmesg = booleanDefault("shortKillMesg", abbr_kmesg);

#ifdef SHOW_DEFAULTS
   {
      char            ints[10];
      sprintf(ints, "%d", updates_per_second);
      show_defaults("startup", "updatesPerSecond", ints,
		 "Number of updates per second (most servers limit to 5).");
   }
#endif
   updates_per_second = intDefault("updatesPerSecond", updates_per_second);

#ifdef FEATURE
   {
      char           *macrokey_s = getdefault("macroKey");
#ifdef SHOW_DEFAULTS
      show_defaults("input", "macroKey", "x",
		"Name of key to use for macro escape [TAB,ESC, or <key>].");
#endif
      if (macrokey_s) {
	 if (strcmp(macrokey_s, "TAB") == 0)
	    macrokey = 9;
	 else if (strcmp(macrokey_s, "ESC") == 0)
	    macrokey = 27;
	 else
	    macrokey = (int) macrokey_s[0];
      }
   }
#endif

   /* For backward compatibility */
   dashboard = booleanDefault("dashboard", dashboard);

   if (dashboard)
      dashboardStyle = 1;

#ifdef SHOW_DEFAULTS
   {
      char            ints[10];
      sprintf(ints, "%d", dashboardStyle);
      show_defaults("display", "dashboardStyle", ints,
      "dashboard style: 0 -- text, 1 -- [LAB], 2 -- COW, 3 -- KRP (new COW).");
   }
#endif
   dashboardStyle = intDefault("dashboardStyle", dashboardStyle);
   if (dashboardStyle > 3)
      dashboardStyle = 3;

#ifdef SHOW_DEFAULTS
   show_defaults("display", "showPlayerStatus", plshowstatus ? "on" : "off",
		 "Player list: also show players not alive.");
#endif
   plshowstatus = booleanDefault("showPlayerStatus", plshowstatus);

#ifdef SHOW_DEFAULTS
   show_defaults("display", "cloakChars", cloakChars,
	 "Pair of characters to use to show cloakers on the galactic map.");
#endif
   s = getdefault("cloakChars");
   if (s) {
      strncpy(cloakChars, s, 2);
   }
#ifdef SHOW_DEFAULTS
   show_defaults("display", "showInd", showInd ? "on" : "off",
		 "Highlight independent planets with an X.");
#endif
   showInd = booleanDefault("showInd", showInd);

#ifdef SHOW_DEFAULTS
   show_defaults("display", "stippleBorder", stippleBorder ? "on" : "off",
		 "Use a textured alert border (implied by forcemono).");
#endif
   stippleBorder = booleanDefault("stippleBorder", stippleBorder);

#ifdef SHOW_DEFAULTS
   show_defaults("display", "showPlanetOwner", showPlanetOwner ? "on" : "off",
    "Show planet owner on galactic map with a team letter next to planet.");
#endif
   showPlanetOwner = booleanDefault("showPlanetOwner", showPlanetOwner);

#ifdef SHOW_DEFAULTS
   {
      char            value[10];
      sprintf(value, "%d", enemyPhasers);
      show_defaults("display", "enemyPhasers", value,
	"Specify degree between double phaser lines (0 means single line)");
   }
#endif
   enemyPhasers = intDefault("enemyPhasers", enemyPhasers);

#ifdef SHOW_DEFAULTS
   show_defaults("display", "newInfo", newInfo ? "on" : "off",
		 "Smaller 'i' info window.");
#endif
   newInfo = booleanDefault("newInfo", newInfo);

#ifdef DROP_FIX
#ifdef SHOW_DEFAULTS
   show_defaults("UDP-droppedPackets", "droppedPacketsFix", drop_fix ? "on" : "off",
		 "Heuristics to minimize affects of dropped packets.");
#endif

   drop_fix = booleanDefault("droppedPacketsFix", drop_fix);
#endif

#ifdef TTS
#ifdef SHOW_DEFAULTS
   show_defaults("tts", "tts", "off",
		 "Tactical Text Solution for the Tactical Tunnel Syndrome.");
   {
      char            buf[10];
      sprintf(buf, "%d", TTS_TIME);
      show_defaults("tts", "tts_time", buf,
		    "How long text remains on tactical.");

      show_defaults("tts", "tts_len", "40",
		    "Length of text displayed on tactical.");

      sprintf(buf, "%d", WINSIDE / 2 - 16);
      show_defaults("tts", "tts_loc", buf,
		    "Y location of text on local.");
   }
#endif
   if (!bw) {
      tts = booleanDefault("tts", 0);
      tts_time = intDefault("tts_time", TTS_TIME);
      tts_len = intDefault("tts_len", 40);
      tts_loc = intDefault("tts_loc", WINSIDE / 2 - 16);
   }
#endif

#ifdef SHOW_DEFAULTS
   show_defaults("input", "enableFastQuit", "off",
		 "Enable fast quit key 'q'");
#endif
   fastQuitOk = booleanDefault("enableFastQuit", 0);

   if ((s = getdefault("logFile")))
      logFileName = strdup(s);
#ifdef SHOW_DEFAULTS
   show_defaults("logging", "logFile", DEFAULT_RECORDFILE,
		 "Message logging file.");
#endif

#ifdef PHASER_SHRINK
   shrink_phasers = booleanDefault("shrinkPhasers", shrink_phasers);
#ifdef SHOW_DEFAULTS
   show_defaults("display", "shrinkPhasers", shrink_phasers ? "on" : "off",
		 "Shrink phaser lines over fuse-length of phaser");
#endif
   shrink_phasers_amount = intDefault("shrinkPhasersAmount",
				      shrink_phasers_amount);
   if (!shrink_phasers_amount)
      shrink_phasers_amount = 10;
#ifdef SHOW_DEFAULTS
   {
      char            buf[10];
      sprintf(buf, "%d", shrink_phasers_amount);
      show_defaults("display", "shrinkPhasersAmount", buf,
	  "Degree of shrinkage [ranges from 5-15, 5 being most shrinkage]");
   }
#endif
#endif

#ifdef SHOW_DEFAULTS
   show_defaults("display", "fastClear", W_FastClear ? "on" : "off",
	   "Fast clear (blank entire tactical each time), good for xterms");
#endif
   W_FastClear = booleanDefault("fastClear", W_FastClear);

#ifdef SHOW_DEFAULTS
   show_defaults("display", "shiftedMouse", extended_mouse ? "on" : "off",
		 "Use shift and/or ctrl + mouse for more mouse functions");
#endif
   extended_mouse = booleanDefault("shiftedMouse", extended_mouse);

   sendOptionsPacket();

   if(defaults_file)
      printf("done.\n");
}

/* totally re-do the defaults! */
void
ReReadDefaults()
{
   extern char    *defaultsFile;

#ifdef PING
   /* this could take a while */
   if (!no_ping)
      stopPing();
#endif

   FreeDefaults();
   initDefaults(defaultsFile);
   readDefaults();
   W_Initialize(display);
   updateWindows();
   mapAll();


   /* assume we're alive when we do this */
   if (showStats)
      W_MapWindow(statwin);

#ifdef PING
   if (!no_ping)
      startPing();
#endif
}

#ifdef LOGMESG
/* log the message sent here into a file */
void
LogMessage(message)
    char           *message;
{
   time_t          curtime;
   struct tm      *tmstruct;
   int             hour;
   static int      lasthour;

   if (!logMess)
      return;

   if (logFile == NULL) {
      /* need to open the logfile */

      logFile = fopen(logFileName, "w+");
      if (logFile == NULL) {
	 perror(logFileName);
	 fprintf(stderr, "No message logging being done.\n");
	 logMess = 0;
	 return;
      }
   }
   time(&curtime);
   tmstruct = localtime(&curtime);
   if (!(hour = tmstruct->tm_hour % 12))
      hour = 12;
   if (lasthour != hour) {
      fprintf(logFile, "%s", ctime(&curtime));
      lasthour = hour;
   }
   fprintf(logFile, "%02d:%02d:%02d %s\n", hour, tmstruct->tm_min,
	   tmstruct->tm_sec, message);
   fflush(logFile);
}
#endif
