#ifndef _RECORDER_H_
#define _RECORDER_H_

#if __STDC__ || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

#define RECORDPACKET(p) ( \
(p) != SP_RSA_KEY && \
(p) != SP_MOTD && \
(p) != SP_MOTD_PIC )

#define RECORD_UPDATE 127
     
/* Where to send record related messages */
#ifdef RECORD_DEBUG
#define RECORDFD stderr
#else 
#define RECORDFD stdout
#endif

enum PlaybackMode { CONT, FRAME, SCAN };
enum AlertMode { PB_NONE, PB_RED, PB_YELLOW, PB_GREEN, PB_DEATH };

extern void startRecorder P_((void));
extern void stopRecorder P_((void));
extern void recordPacket P_((char *bufptr, int size));
extern void recordUpdate P_((void));
extern void startPlayback P_(());
extern int readPlayback P_((FILE *f, char *buff, int size));
extern void playback_input P_((void));
extern void playback_add_updates P_((char *buf));
extern void playback_clock P_((void));
extern void playback_status P_((char *buf));
extern int playback_keyaction P_((W_Event *data));
extern void check_record_filename P_((void));

extern int teamReq, recordGame, playback, playback_update, paradise_compat,
  maxRecord, update_dashboard, pb_paused, not_first_entry, recordIndiv,
  confirmOverwrite;
extern long int packets_recorded, updates_recorded;
extern FILE *recordFile;
extern char *recordFileName;
extern enum PlaybackMode pb_mode;
extern enum AlertMode pb_alert, pb_alert_scan;

#undef P_

#endif
