
#include <stdlib.h>

#include "version.h"
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "packets.h"
#ifdef FEATURE
#include "distress.h"
#endif
#ifndef CPROTO 
#include "protos.h"
#endif
#ifdef NEED_RANDOM
#include "random.h"
#endif

#if !defined(__CENTERLINE__) && defined(UCI)
#include "sysprotos.h"
#endif

/* Use a macro to define redrawTstats() */
#define redrawTstats() db_redraw(1)
