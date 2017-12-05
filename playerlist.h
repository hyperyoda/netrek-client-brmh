/* DO NOT DELETE THIS LINE -- fcproto uses it. */
/* DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY. */
#if __STDC__ || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

/* playerlist.c */
extern void redraw_playerlist_header P_((void));
extern void playerlist P_((void));
extern void Sorted_playerlist2 P_((void));
extern void Sorted_playerlist3 P_((void));
extern void playerlist2 P_((void));
extern void playerlist3 P_((register int i));

#undef P_
#endif /* _playerlist_h_ */

/* IF YOU PUT ANYTHING HERE IT WILL GO AWAY */
