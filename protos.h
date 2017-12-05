#if __STDC__ || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

/* colors.c */
extern void getColorDefs P_((void));
/* data.c */
/* death.c */
extern void show_death P_((void));
extern void death P_((void));
/* defaults.c */
extern int initDefaults P_((char *deffile));
extern void FreeDefaults P_((void));
#ifndef _AIX
extern char *strdup P_((const char *str));
#endif
extern char *getalias P_((char *server));
extern char *_getdefault P_((char *str));
extern char *getdefault P_((char *str));
extern int strcmpi P_((char *str1, char *str2));
extern int booleanDefault P_((char *def, int preferred));
extern double floatDefault P_((char *def, double preferred));
extern int intDefault P_((char *def, int preferred));
extern int findDefaults P_((char *home, char *deffile));
extern int defaultShip P_((int preferred));
/* defwin.c */
/* detonate.c */
extern void detmine P_((void));
/* dmessage.c */
extern void dmessage P_((char *message, int flags, int from, int to));
extern int instr P_((char *string1, char *string2));
/* enter.c */
extern void enter P_((void));
extern void openmem P_((void));
/* entrywin.c */
extern void entrywindow P_((int *team, int *s_type));
extern int teamRequest P_((int team, int ship));
extern int numShips P_((int owner));
extern int checkBold P_((char *line));
extern void showMotd P_((W_Window motdwin, int atline));
extern void ClearMotd P_((void));
extern void newMotdPic P_((int index, int x, int y, int l, int color));
extern void newMotdPicBitmap P_((int size, int width, int height, char *bits));
extern void newMotdLine P_((char *line));
extern void getResources P_((char *prog));
extern void getTiles P_((void));
extern void redrawTeam P_((W_Window win, int teamNo, int *lastnum));
extern void redrawQuit P_((void));
extern void drawIcon P_((void));
extern void showTimeLeft P_((int time, int max));
extern void do_refit P_((int type));
extern void SaveMotd P_((void));
/* findslot.c */
extern int findslot P_((void));
/* getdefaults.c */
extern void readDefaults P_((void));
extern void ReReadDefaults P_((void));
extern void LogMessage P_((char *message));
/* getname.c */
extern void noautologin P_((void));
extern void getname P_((char *defname));
extern void try_autologin P_((char *defname));
extern void loaddude P_((void));
extern void checkpassword P_((void));
extern void makeNewGuy P_((void));
extern void adjustString P_((int ch, char *str, char *defname));
extern void displayStartup P_((char *defname));
/* getship.c */
extern void getshipdefaults P_((void));
extern void getship P_((struct ship *shipp, int s_type));
/* helpwin.c */
extern void fillhelp P_((void));
extern void update_Help_to_Keymap P_((char helpmessage[]));
/* inform.c */
extern void inform P_((W_Window ww, int x, int y, int key));
extern void destroyInfo P_((void));
/* input.c */
extern unsigned char *InitKeyMap P_((unsigned char *keyMap));
extern void initkeymaps P_((void));
extern void initinput P_((void));
extern void input P_((void));
extern int process_event P_((void));
extern void keyaction P_((W_Event *data));
extern void buttonaction P_((W_Event *data));
extern int getcourse P_((W_Window ww, int x, int y));
/* interface.c */
extern void set_speed P_((int speed));
extern void set_course P_((int dir));
extern void shield_up P_((void));
extern void shield_down P_((void));
extern void shield_tog P_((void));
extern void bomb_planet P_((void));
extern void beam_up P_((void));
extern void beam_down P_((void));
extern void repair P_((void));
extern void repair_off P_((void));
extern void repeat_message P_((void));
extern void cloak P_((void));
extern void cloak_on P_((void));
extern void cloak_off P_((void));
extern int mstime P_((void));
extern int msetime P_((void));
/* main.c */
extern unsigned long strToNetaddr P_((char *str));
extern void getUdpPort P_((void));
extern unsigned long mkaddr P_((char *m));
extern void read_servers P_((void));
extern void main P_((int argc, char **argv));
extern void printVersion P_((char *prog));
extern void printMetaInfo P_((char *prog));
extern void printUsage P_((char *prog));
extern void handle_segfault P_((void));
extern void handle_exception P_((void));
extern void get_core P_((void));
extern void gb P_((void));
extern void check_whosplaying P_((char *name, int port));
extern void addComLineArgs P_((char **args, int *count));
/* mkdefault.c */
extern void makeDefault P_((void));
/* motdwin.c */
extern void motdWinEvent P_((int key));
extern void showMotdWin P_((void));
/* newwin.c */
extern void newwin P_((char *hostmon, char *progname));
extern void mapAll P_((void));
extern void savebitmaps P_((void));
extern void updateWindows P_((void));
extern void DefineFedCursor P_((W_Window window));
extern void DefineRomCursor P_((W_Window window));
extern void DefineKliCursor P_((W_Window window));
extern void DefineOriCursor P_((W_Window window));
/* option.c */
extern void optionwindow P_((void));
extern void RefreshOptions P_((void));
extern void optionredrawtarget P_((W_Window win));
extern void optionredrawoption P_((int *ip));
extern int optionaction P_((W_Event *data));
extern void SetMenuPage P_((int pagenum));
extern void optiondone P_((void));
extern int InitOptionMenus P_((void));
extern void UpdateOptions P_((void));
/* ping.c */
extern void handlePing P_((struct ping_spacket *packet));
extern void startPing P_((void));
extern void stopPing P_((void));
extern void sendServerPingResponse P_((int number));
extern void calc_lag P_((void));
/* pingstats.c */
extern int pStatsHeight P_((void));
extern int pStatsWidth P_((void));
extern void initPStats P_((void));
extern void redrawPStats P_((void));
extern void updatePStats P_((void));
/* planetlist.c */
extern void planetlist P_((void));
/* planets.c */
extern void initPlanets P_((void));
extern void checkRedraw P_((int x, int y));
/* playerlist.c */
void redraw_playerlist_header P_((void));
void playerlist P_((void));
void Sorted_playerlist2 P_((void));
void playerlist2 P_((void));
void playerlist3 P_((register int i));
/* ranklist.c */
extern void ranklist P_((void));
/* redraw.c */
extern void intrupt P_((int *readfds));
extern void redraw P_((void));
extern void DrawLocalPlanets P_((void));
extern void local P_((void));
extern void map P_((void));
extern char *itoa P_((char *buf, int n, int w, int sp));
extern char *strcpy_return P_((register char *s1, register char *s2));
extern char *strcpyp_return P_((register char *s1, register char *s2, register int length));
extern char *itof42 P_((char *s, double f));
extern char *itof32 P_((char *s, double f));
extern char *itof22 P_((char *s, double f));
extern void setup_redraw_map P_((register int xl, register int yu, register int xr, register int yd));
/* reserved.c */
extern void makeReservedPacket P_((struct reserved_spacket *packet));
extern void encryptReservedPacket P_((struct reserved_spacket *spacket, struct reserved_cpacket *cpacket, char *server, int pno));
/* rotate.c */
extern void rotate_dir P_((unsigned char *d, int r));
extern void rotate_coord P_((int *x, int *y, int d, int cx, int cy));
/* sintab.c */
/* smessage.c */
extern void smessage P_((int ichar));
extern void smess_paste P_((void));
extern void smess_refresh P_((void));
extern void pmessage P_((char *str, int recip, int group));
extern char *getaddr P_((int who));
extern char *getaddr2 P_((int flags, int recip));
extern void emergency P_((void));
extern void army_report P_((void));
extern void message_on P_((void));
extern void message_off P_((void));
/* socket.c */
extern void resetForce P_((void));
extern void checkForce P_((void));
extern void connectToServer P_((int port));
extern void callServer P_((int port, char *server));
extern int isServerDead P_((void));
extern void socketPause P_((void));
extern int readFromServer P_((int *readfds));
extern void dotimers P_((void));
extern int doRead P_((int asock));
extern void sendShortPacket P_((int type, int state));
extern void _sendServerPacket P_((struct player_spacket *packet));
extern void handleMessage P_((struct mesg_spacket *packet));
extern void sendTeamReq P_((int team, int ship));
extern void sendLoginReq P_((char *name, char *pass, char *login, int query));
extern void sendTractorReq P_((int state, int pnum));
extern void sendRepressReq P_((int state, int pnum));
extern void sendDetMineReq P_((int torp));
extern void sendMessage P_((char *mes, int group, int indiv));
extern void sendThreshold P_((int v));
extern void sendOptionsPacket P_((void));
extern void pickSocket P_((int old));
extern int gwrite P_((int fd, char *buf, register int bytes));
extern void sendUpdatePacket P_((long speed));
extern void sendUdpReq P_((int req));
extern int openUdpConn P_((void));
extern int recvUdpConn P_((void));
extern int closeUdpConn P_((void));
extern void printUdpInfo P_((void));
extern void sendShortReq P_((int state));
/* stats.c */
extern void initStats P_((void));
extern void redrawStats P_((void));
extern void updateStats P_((void));
extern void box P_((int filled, int x, int y, int wid, int hei, W_Color color));
extern void calibrate_stats P_((void));
/* udpopt.c */
extern void udpwindow P_((void));
extern void udprefresh P_((int i));
extern void udpaction P_((W_Event *data));
extern void udpdone P_((void));
/* util.c */
extern int angdist P_((int x, int y));
extern struct obtype *gettarget P_((W_Window ww, int x, int y, int targtype));
extern struct obtype *gettarget2 P_((int x, int y, int targtype));
extern void lockPlanetOrBase P_((W_Window ww, int x, int y));
extern int ihypot P_((int x1, int y1));
extern int troop_capacity P_((void));
extern double nkills P_((void));
/* war.c */
extern void warwindow P_((void));
extern void warrefresh P_((void));
extern void fillwin P_((int menunum, char *string, int hostile, int warbits, int team));
extern void waraction P_((W_Event *data));
/* warning.c */
extern void warning P_((char *text));
/* x11window.c */
extern void set_tcp_nodelay P_((int v));
extern void W_Initialize P_((char *str));
extern void W_CloseDisplay P_((void));
extern void W_ChildCloseDisplay P_((void));
extern void W_TopWindowTitle P_((W_Window win));
extern W_Window W_MakeWindow P_((char *name, int x, int y, int width, int height, W_Window parent, int border, W_Color color));
extern void W_ChangeBorder P_((W_Window window, int color));
extern void W_MapWindow P_((W_Window window));
extern void W_UnmapWindow P_((W_Window window));
extern int W_IsMapped P_((W_Window window));
extern void W_FillArea P_((W_Window window, int x, int y, int width, int height, W_Color color));
extern void W_CacheClearArea P_((W_Window window, int x, int y, int width, int height));
extern void W_FlushClearAreaCache P_((W_Window window));
extern void W_ClearArea P_((W_Window window, int x, int y, int width, int height));
extern void W_ClearWindow P_((W_Window window));
extern int W_EventsPending P_((void));
extern int W_EventsQueued P_((void));
extern int W_ReadEvents P_((void));
extern void W_NextEvent P_((W_Event *wevent));
extern int W_SpNextEvent P_((W_Event *wevent));
extern void W_MakeLine P_((W_Window window, int x0, int y0, int x1, int y1, W_Color color));
extern void W_MakePhaserLine P_((W_Window window, int x0, int y0, int x1, int y1, W_Color color));
extern void W_CacheLine P_((W_Window window, int x0, int y0, int x1, int y1, int color));
extern void W_FlushLineCaches P_((W_Window window));
extern void W_MakeTractLine P_((W_Window window, int x0, int y0, int x1, int y1, W_Color color));
extern void W_WriteTriangle P_((W_Window window, int x, int y, int s, int t, W_Color color));
extern void W_WriteText P_((W_Window window, int x, int y, W_Color color, char *str, int len, W_Font font));
extern void W_MaskText P_((W_Window window, int x, int y, W_Color color, char *str, int len, W_Font font));
extern W_Icon W_StoreBitmap P_((int width, int height, char *data, W_Window window));
extern void W_WriteBitmap P_((int x, int y, W_Icon bit, W_Color color));
extern void W_TileWindow P_((W_Window window, W_Icon bit));
extern void W_UnTileWindow P_((W_Window window));
extern W_Window W_MakeTextWindow P_((char *name, int x, int y, int width, int height, W_Window parent, int border));
extern W_Window W_MakeScrollingWindow P_((char *name, int x, int y, int width, int height, W_Window parent, int border));
extern void W_FlushScrollingWindow P_((W_Window window));
extern W_Window W_MakeMenu P_((char *name, int x, int y, int width, int height, W_Window parent, int border));
extern void W_DefineCursorFromBitmap P_((W_Window window, unsigned char *mapbits, int width, int height, unsigned char *maskbits, int maskwidth, int maskheight));
extern void W_DefineTCrossCursor P_((W_Window window));
extern void W_DefineTrekCursor P_((W_Window window));
extern void W_DefineWarningCursor P_((W_Window window));
extern void W_DefineArrowCursor P_((W_Window window));
extern void W_DefineTextCursor P_((W_Window window));
extern void W_DefineWaitCursor P_((W_Window window));
extern void W_DefineUpDownCursor P_((W_Window window));
extern void W_DefineCursorFromGlyph P_((W_Window window, W_Font font, char *chr, char *mask_chr, W_Color color));
extern void W_DefineCursor P_((W_Window window, int width, int height, char *bits, char *mask, int xhot, int yhot));
extern void W_Beep P_((void));
extern int W_WindowWidth P_((W_Window window));
extern int W_WindowHeight P_((W_Window window));
extern int W_Socket P_((void));
extern void W_DestroyWindow P_((W_Window window));
extern void W_SetIconWindow P_((W_Window main, W_Window icon));
extern int W_CheckMapped P_((char *name));
extern void W_WarpPointer P_((W_Window window));
extern void W_FindMouse P_((int *x, int *y));
extern int W_FindMouseInWin P_((int *x, int *y, W_Window w));
extern void W_Flush P_((void));
extern void W_ReposWindow P_((W_Window window, int newx, int newy));
extern void W_ResizeWindow P_((W_Window window, int neww, int newh));
extern void W_ReinitMenu P_((W_Window window, int neww, int newh));
extern void W_ResizeMenu P_((W_Window window, int neww, int newh));
extern void W_ResizeTextWindow P_((W_Window window, int neww, int newh));
extern int W_Mono P_((void));
extern int W_TTSTextHeight P_((void));
extern int W_TTSTextWidth P_((char *s, int l));
extern void init_tts P_((void));
extern void W_EraseTTSText P_((W_Window window, int max_width, int y, int width));
extern void W_WriteTTSText P_((W_Window window, int max_width, int y, int width, char *str, int len));
extern void W_UpdateWindow P_((W_Window window));
extern void W_WriteWinBitmap P_((W_Window win, int x, int y, W_Icon bit, W_Color color));
extern void W_FreeBitmap P_((W_Icon bit));
extern void W_ChangeBackground P_((W_Window win, W_Color color));
extern char *W_FetchBuffer P_((int *l));
extern void W_FreeBuffer P_((char *m));
/* netstat.c */
extern void ns_init P_((int v));
extern void ns_record_update P_((int count));
extern void ns_do_stat P_((int v, int c));
extern double ns_get_tstat P_((void));
extern double ns_get_lstat P_((void));
extern int ns_get_nfailures P_((void));
extern char *ns_get_nfthresh_s P_((void));
extern void ns_set_nfthresh_s P_((char *s));
extern int ns_get_nfthresh P_((void));
extern void ns_set_nfthresh P_((int v));
/* lagmeter.c */
extern int lMeterHeight P_((void));
extern int lMeterWidth P_((void));
extern void redrawLMeter P_((void));
extern void updateLMeter P_((void));
extern void lMeterBox P_((int filled, int x, int y, int w, int h, W_Color color));
/* rsa_box.c */
extern void rsa_black_box P_((unsigned char *out, unsigned char *in, unsigned char *public, unsigned char *global));
/* rsa_box_0.c */
/* feature.c */
extern int feature_lines P_((void));
extern void checkFeature P_((struct feature_spacket *packet));
extern void reportFeatures P_((void));
extern void sendFeature P_((char *name, int feature_type, int value, int arg1, int arg2));
extern int feature_cmp P_((char *f, char *s));
extern void showFeatures P_((void));
/* feature_old.c */
extern void CheckFeatures P_((char *m));
extern void sendVersion P_((void));
/* macrowin.c */
extern void showMacroWin P_((void));
extern void fillmacro P_((void));
/* distsend.c */
extern struct distress *loaddistress P_((enum dist_type i, W_Event *data));
extern void rcd P_((enum dist_type i, W_Event *data));
extern int pmacro P_((int mnum, int who, W_Event *data));
extern int doMacro P_((int key, W_Event *data));
extern void macro_off P_((void));
extern int getgroup P_((int addr, int *recip));
extern void pnbtmacro P_((int c));
extern int rcd_lines P_((void));
extern void showRCDs P_((int row));
/* distress.c */
extern int itoa2 P_((int n, char s[]));
extern void HandleGenDistr P_((char *message, int from, int to, struct distress *dist));
extern void Dist2Mesg P_((struct distress *dist, char *buf));
extern int makedistress P_((struct distress *dist, char *cry, char *pm));
extern int testmacro P_((char *bufa, char *bufb, int *inda, int *indb));
extern int solvetest P_((char *bufa, int *inda));
extern int condmacro P_((char *bufa, char *bufb, int *inda, int *indb, int flag));
extern int skipmacro P_((char buf[], int index));
extern char *strcap P_((char *s));
/* spwarning.c */
extern void handleSWarning P_((struct warning_s_spacket *packet));
/* brmh-dashboard.c */
extern void db_redraw_brmh P_((int fr));
/* cow-dashboard.c */
extern void db_redraw_krp P_((int fr));
extern void db_redraw_COW P_((int fr));
/* text-dashboard.c */
extern void stline P_((int flag));
extern void updateMaxStats P_((int redraw));
extern void run_clock P_((int update));
extern void clear_clock P_((void));
extern void db_redraw P_((int fr));
/* fontbm.c */
extern void F_init P_((W_FontInfo *fonts, int lastused));
extern void F_initGC P_((W_GC *gclist_v, int lastused, W_FontInfo *fonts));
extern char *F_planetChar P_((struct planet *l));
extern W_Font F_planetFont P_((void));
extern char *F_cloakChar P_((void));
extern W_Font F_cloakFont P_((void));
extern char *F_shieldChar P_((int i));
extern W_Font F_shieldFont P_((void));
extern char *F_puckChar P_((int dir));
extern char *F_shipChar P_((int team, int ship, int dir));
extern W_Font F_puckFont P_((void));
extern W_Font F_playerFont P_((int team));
extern char *F_expSbChar P_((int frame));
extern W_Font F_explodeFont P_((void));
extern char *F_expChar P_((int frame));
extern W_Font F_smallFont P_((void));
extern char *F_expTorpChar P_((int frame));
extern char *F_expPlasmaTorpChar P_((int frame));
extern char *F_plasmaTorpEnemyChar P_((void));
extern char *F_plasmaTorpChar P_((void));
extern char *F_planetMapChar P_((struct planet *l));
extern W_Font F_planetMapFont P_((void));
extern char *F_phaserChar P_((void));
extern W_Font F_phaserFont P_((void));
/* metaserver.c */
extern int open_port P_((char *host, int port, int verbose));
extern void read_sock P_((int sock));
extern void do_metaserver P_((char *metafile));
#undef P_
