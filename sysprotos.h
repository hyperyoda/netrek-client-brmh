#if __STDC__ || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

/* sysprotos.h */

extern int                    fprintf P_((FILE *, const char *, ...));
extern int                    sscanf P_((const char *, const char *, ...));
extern void                   bcopy P_((const void *, void *, int));
extern void                   bzero P_((void *, size_t));
extern pid_t                  getpid P_((void));
extern char *                 strncpy P_((char *, const char *, size_t));
extern int                    fclose P_((FILE *));
extern int                    access P_((const char *, int));
extern void *                 malloc P_((size_t));
extern void                   free P_((void *));
extern int                    strncasecmp P_((char *, char *, int));
extern int                    atoi P_((const char *));
extern void                   exit P_((int));
extern int                    printf P_((const char *, ...));
extern int                    close P_((int));
extern void                   perror P_((const char *));
extern time_t                 time P_((time_t *));
extern int                    read P_((int, void *, unsigned int));
extern int                    write P_((int, const void *, unsigned int));
extern unsigned int           sleep P_((unsigned int));
extern uid_t                  getuid P_((void));
extern char *                 getenv P_((const char *));
extern int                    tolower P_((int));
extern int                    toupper P_((int));
extern int                    fflush P_((FILE *));

#undef P_
