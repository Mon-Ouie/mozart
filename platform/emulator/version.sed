/* -*- C++ -*- */
#include <stdio.h>
#include "../include/config.h"

#define AMVersion OZVERSION
#define AMDate "HEREGOESDATE"


#ifdef WINDOWS
char *ozplatform = "win32-i486";
#else
char *ozplatform = "OZPLATFORM";
#endif

void version()
{
  printf("Mozart playing Oz 3. Engine %s %s of %s.\n",
	 AMVersion,ozplatform,AMDate);
  fflush(stdout);
}
