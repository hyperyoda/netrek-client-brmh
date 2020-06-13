#ifndef _prog_version_h_
#define _prog_version_h_
/* program version number stuff */

#ifdef nodef
static char    *Program_Version = "$Revision: 1.4 $   Compiled on $Date: 2000/03/23 03:44:26 $";
#endif

#define VERSION	       "2.6.0-pre20090729"
#define CLIENTNAME     "BRMH"
//#define CLIENTOS       "*DEFINE CLIENT OS HERE*"
#define MAINTENANCE    "karthik@karthik.com"
#define URL_NETREK     "http://www.netrek.org/"
#define URL_XTREKRC    "ftp://ftp.netrek.org/pub/netrek/clients/brmh/bin/BRMH-xtrekrc"
#define URL_FTP	       "ftp://ftp.netrek.org/pub/netrek/clients/brmh/BRMH.html"
#define CLIENTCOMMENTS ""

#ifndef CLIENTOS
#error Please manually define CLIENTOS in version.h
#endif

#endif
