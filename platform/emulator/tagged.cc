/*
 *  Authors:
 *    Ralf Scheidhauer (Ralf.Scheidhauer@ps.uni-sb.de)
 *
 *  Contributors:
 *    optional, Contributor's name (Contributor's email address)
 *
 *  Copyright:
 *    Organization or Person (Year(s))
 *
 *  Last change:
 *    $Date$ by $Author$
 *    $Revision$
 *
 *  This file is part of Mozart, an implementation
 *  of Oz 3:
 *     $MOZARTURL$
 *
 *  See the file "LICENSE" or
 *     $LICENSEURL$
 *  for information on usage and redistribution
 *  of this file, and for a DISCLAIMER OF ALL
 *  WARRANTIES.
 *
 */

#if defined(INTERFACE)
#pragma implementation "tagged.hh"
#endif

#include "tagged.hh"

#ifdef DEBUG_CHECK
char *TypeOfTermString[1<<tagSize];
#endif


void initTagged()
{
  void *p = (void *) malloc(100);
  void *p1 = tagValueOf(makeTaggedMiscp(p));
  if (p != p1) {
    fprintf(stderr,"\n*******\nError, wrong configuration\n");
    fprintf(stderr,"Try defining\n\n");
    fprintf(stderr,"\t const long int mallocBase = 0x%lx;\n\n",
            (long int)p - (long int)p1);
    fprintf(stderr,"in \"tagged.hh\"\n\n");
    exit(1);
  }

  free(p);

#ifdef DEBUG_CHECK
  char **tts = TypeOfTermString;

  tts[0]        = "REF";       //  0
  tts[UVAR]     = "UVAR";      //  1
  tts[LTUPLE]   = "LTUPLE";    //  2
  tts[SMALLINT] = "SMALLINT";  //  3
  tts[4]        = "REF";       //  4
  tts[CVAR]     = "CVAR";      //  5
  tts[FSETVALUE]= "FSETVALUE"; //  6
  tts[PROMISE]  = "PROMISE";   //  7
  tts[8]        = "REF";       //  8
  tts[SVAR]     = "SVAR";      //  9
  tts[OZCONST]  = "OZCONST";   // 10
  tts[OZFLOAT]  = "OZFLOAT";   // 11
  tts[12]       = "REF";       // 12
  tts[GCTAG]    = "GCTAG";     // 13
  tts[SRECORD]  = "SRECORD";   // 14
  tts[LITERAL]  = "ATOM";      // 15
#endif
}
