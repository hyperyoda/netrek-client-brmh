
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <math.h>

int	port;

showport()
{
   printf("exiting at port %d\n", port);
   exit(1);
}

main(argc, argv)

   int	argc;
   char	**argv;
{
   char	*server;
   int	s;
   struct sockaddr_in	addr;
   struct hostent *hp;

   if(argc < 2){
      fprintf(stderr, "usage: %s <host> [port start numbern", argv[0]);
      exit(0);
   }

   signal(SIGINT, showport);

   server = argv[1];

   if(argc > 2)
      port = atoi(argv[2]);
   else 
      port = 1111;

   printf("starting at %d\n", port);

   while(1){
      if((s = socket(AF_INET, SOCK_STREAM, 0)) < 0){
	 perror("socket");
	 exit(1);
      }

      addr.sin_family = AF_INET;
      addr.sin_port = htons(port);

      if ((addr.sin_addr.s_addr = inet_addr(server)) == -1) {
	 if ((hp = gethostbyname(server)) == NULL) {
	    printf("Who is %s?\n", server);
	    exit(0);
	 } else {
	    addr.sin_addr.s_addr = *(long *) hp->h_addr;
	 }
      }

      if (connect(s, &addr, sizeof(addr)) < 0) {
	 if((port % 100)==0)
	    printf("@%d\n", port);
      }
      else{
	 printf("server listening on %d\n", port);
      }
      shutdown(s);
      close(s);
      port++;
   }
}
