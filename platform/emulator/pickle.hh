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
 *     http://mozart.ps.uni-sb.de
 *
 *  See the file "LICENSE" or
 *     http://mozart.ps.uni-sb.de/LICENSE.html
 *  for information on usage and redistribution
 *  of this file, and for a DISCLAIMER OF ALL
 *  WARRANTIES.
 *
 */


#ifndef __PICKLEH
#define __PICKLEH

#include "opcodes.hh"


/* magic marker for start of saved components */
/* HACK ALERT: */
#define PERDIOMAGICSTART       31

#define PERDIOMINOR      "15"
#define PERDIOMAJOR      OZVERSION
#define PERDIOVERSION    PERDIOMAJOR "#" PERDIOMINOR

// the DIFs
// the protocol layer needs to know about some of these

typedef enum {
  DIF_UNUSED0,
  DIF_SMALLINT,
  DIF_BIGINT,
  DIF_FLOAT,
  DIF_ATOM,
  DIF_NAME,
  DIF_UNIQUENAME,
  DIF_RECORD,
  DIF_TUPLE,
  DIF_LIST,
  DIF_REF,
  DIF_REF_DEBUG,
  DIF_OWNER,
  DIF_OWNER_SEC,
  DIF_PORT,
  DIF_CELL,
  DIF_LOCK,
  DIF_VAR,
  DIF_BUILTIN,
  DIF_DICT,
  DIF_OBJECT,
  DIF_THREAD,
  DIF_SPACE,
  DIF_CHUNK,            // SITE INDEX NAME value
  DIF_PROC,             // SITE INDEX NAME ARITY globals code
  DIF_CLASS,            // SITE INDEX NAME obj class
  DIF_ARRAY,
  DIF_FSETVALUE,        // finite set constant
  DIF_ABSTRENTRY,       // AbstractionEntry (code instantiation)
  DIF_PRIMARY,
  DIF_SECONDARY,
  DIF_SITE,
  DIF_SITE_VI,
  DIF_SITE_PERM,
  DIF_PASSIVE,
  DIF_COPYABLENAME,
  DIF_LAST
} MarshalTag;


const struct {MarshalTag tag; char *name;} dif_names[] = {
  { DIF_UNUSED0,      "UNUSED0"},
  { DIF_SMALLINT,     "SMALLINT"},
  { DIF_BIGINT,       "BIGINT"},
  { DIF_FLOAT,        "FLOAT"},
  { DIF_ATOM,         "ATOM"},
  { DIF_NAME,         "NAME"},
  { DIF_UNIQUENAME,   "UNIQUENAME"},
  { DIF_RECORD,       "RECORD"},
  { DIF_TUPLE,        "TUPLE"},
  { DIF_LIST,         "LIST"},
  { DIF_REF,          "REF"},
  { DIF_REF_DEBUG,    "REF_DEBUG"},
  { DIF_OWNER,        "OWNER"},
  { DIF_OWNER_SEC,    "OWNER_SEC"},
  { DIF_PORT,         "PORT"},
  { DIF_CELL,         "CELL"},
  { DIF_LOCK,         "LOCK"},
  { DIF_VAR,          "VAR"},
  { DIF_BUILTIN,      "BUILTIN"},
  { DIF_DICT,         "DICT"},
  { DIF_OBJECT,       "OBJECT"},
  { DIF_THREAD,       "THREAD"},
  { DIF_SPACE,        "SPACE"},
  { DIF_CHUNK,        "CHUNK"},
  { DIF_PROC,         "PROC"},
  { DIF_CLASS,        "CLASS"},
  { DIF_ARRAY,        "ARRAY"},
  { DIF_FSETVALUE,    "FSETVALUE"},
  { DIF_ABSTRENTRY,   "ABSTRENTRY"},
  { DIF_PRIMARY,      "PRIMARY"},
  { DIF_SECONDARY,    "SECONDARY"},
  { DIF_SITE,         "SITE"},
  { DIF_SITE_VI,      "SITE_VI"},
  { DIF_SITE_PERM,    "SITE_PERM"},
  { DIF_PASSIVE,      "PASSIVE"},
  { DIF_COPYABLENAME, "COPYABLENAME"},
  { DIF_LAST,         "LAST"}
};


#define TAG_STRING    'S'
#define TAG_INT       'I'
#define TAG_DIF       'D'
#define TAG_OPCODE    'O'
#define TAG_LABELREF  'L'
#define TAG_LABELDEF  'l'
#define TAG_BYTE      'B'
#define TAG_COMMENT   '#'
#define TAG_CODESTART 'C'
#define TAG_CODEEND   'c'
#define TAG_TERMREF   'T'
#define TAG_TERMDEF   't'
#define TAG_EOF       -1


class MsgBuffer;

void marshalNumber(unsigned int i, MsgBuffer *bs);
int unmarshalNumber(MsgBuffer *bs);
void marshalCode(ProgramCounter,MsgBuffer*);
void marshalLabel(ProgramCounter,int,MsgBuffer*);
void marshalOpCode(int lbl, Opcode op, MsgBuffer *bs, int showLabel = 1);
void marshalCodeEnd(MsgBuffer *bs);
void marshalCodeStart(int codesize, MsgBuffer *bs);
void putComment(char *s,MsgBuffer *bs);
void putString(const char *s, MsgBuffer *bs);
void putTag(char tag, MsgBuffer *bs);
void marshalDIF(MsgBuffer *bs, MarshalTag tag) ;
void marshalByte(unsigned char c, MsgBuffer *bs);
void marshalString(const char *s, MsgBuffer *bs);
void marshalLabel(int start, int lbl, MsgBuffer *bs);
void marshalLabelDef(char *lbl, MsgBuffer *bs);
void marshalTermDef(int lbl, MsgBuffer *bs);
void marshalTermRef(int lbl, MsgBuffer *bs);

void splitversion(char *vers, char *&major, int &minordiff);

#endif /* __PICKLEH */
