//
// Author:
//   Leif Kornstaedt <kornstae@ps.uni-sb.de>
//
// Copyright:
//   Leif Kornstaedt, 1996, 1997
//
// Last change:
//   $Date$ by $Author$
//   $Revision$
//
// This file is part of Mozart, an implementation of Oz 3:
//    http://mozart.ps.uni-sb.de
//
// See the file "LICENSE" or
//    http://mozart.ps.uni-sb.de/LICENSE.html
// for information on usage and redistribution
// of this file, and for a DISCLAIMER OF ALL
// WARRANTIES.
//

//
// This file is included by lexers generated by Gump.
//

#include "oz.h"

static void setEmptyBuffer(yyFlexLexer *i) {
  static yy_buffer_state *p = 0;
  if (!p) {
    p = new yy_buffer_state;
    p->yy_input_file = 0;
    p->yy_ch_buf = new char[2];
    p->yy_ch_buf[0] = YY_END_OF_BUFFER_CHAR;
    p->yy_ch_buf[1] = YY_END_OF_BUFFER_CHAR;
    p->yy_buf_pos = &p->yy_ch_buf[0];
    p->yy_buf_size = 0;
    p->yy_n_chars = 0;
    p->yy_is_our_buffer = 1;
    p->yy_is_interactive = 0;
    p->yy_at_bol = 1;
    p->yy_fill_buffer = 0;
    p->yy_buffer_status = YY_BUFFER_NEW;
  }
  i->yy_switch_to_buffer(p);
}

OZ_BI_define(yy_lexer_create, 0, 1)
{
  yyFlexLexer *flexLexer = new yyFlexLexer();
  setEmptyBuffer(flexLexer);
  OZ_Term t = OZ_makeForeignPointer(flexLexer);
  OZ_RETURN(t);
}
OZ_BI_end

OZ_BI_define(yy_lexer_delete, 1, 0)
{
  OZ_declareForeignPointerIN(0, p);
  delete (yyFlexLexer *) p;
  return PROCEED;
}
OZ_BI_end

OZ_BI_define(yy_lexer_getNextMatch, 1, 1)
{
  OZ_declareForeignPointerIN(0, p);
  OZ_RETURN_INT(((yyFlexLexer *) p)->yylex());
}
OZ_BI_end

OZ_BI_define(yy_lexer_getAtom, 1, 1)
{
  OZ_declareForeignPointerIN(0, p);
  OZ_RETURN_ATOM((char *) ((yyFlexLexer *) p)->YYText());
}
OZ_BI_end

OZ_BI_define(yy_lexer_getString, 1, 1)
{
  // this does not use OZ_string because we don't necessarily want
  // to stop at the first NUL
  OZ_declareForeignPointerIN(0, p);
  yyFlexLexer *obj = (yyFlexLexer *) p;
  int i = obj->YYLeng();
  const unsigned char *yytext = (unsigned char*) obj->YYText();
  OZ_Term str = OZ_nil();
  for (i--; i >= 0; i--)
    str = OZ_cons(OZ_int(yytext[i]), str);
  OZ_RETURN(str);
}
OZ_BI_end

OZ_BI_define(yy_lexer_getLength, 1, 1)
{
  OZ_declareForeignPointerIN(0, p);
  OZ_RETURN_INT(((yyFlexLexer *) p)->YYLeng());
}
OZ_BI_end

OZ_BI_define(yy_lexer_switchToBuffer, 2, 0)
{
  OZ_declareForeignPointerIN(0, p);
  OZ_declareForeignPointerIN(1, q);
  ((yyFlexLexer *) p)->yy_switch_to_buffer((yy_buffer_state *) q);
  return PROCEED;
}
OZ_BI_end

OZ_BI_define(yy_lexer_setMode, 2, 0)
{
  OZ_declareForeignPointerIN(0, p);
  OZ_declareIntIN(1, i);
  ((yyFlexLexer *) p)->yy_start = i * 2 + 1;
  return PROCEED;
}
OZ_BI_end

OZ_BI_define(yy_lexer_currentMode, 1, 1)
{
  OZ_declareForeignPointerIN(0, p);
  OZ_RETURN_INT((((yyFlexLexer *) p)->yy_start - 1) / 2);
}
OZ_BI_end

OZ_BI_define(yy_lexer_input, 1, 1)
{
  OZ_declareForeignPointerIN(0, p);
  OZ_RETURN_INT(((yyFlexLexer *) p)->yyinput());
}
OZ_BI_end

OZ_BI_define(yy_lexer_unput, 2, 0)
{
  OZ_declareForeignPointerIN(0, p);
  OZ_declareIntIN(1, j);
  char c = j;
  ((yyFlexLexer *) p)->yyunput(1, &c);
  return PROCEED;
}
OZ_BI_end

extern "C" OZ_C_proc_interface *oz_init_module(void);

OZ_C_proc_interface *oz_init_module(void) {
  static OZ_C_proc_interface oz_interface[] = {
    {"create",0,1,yy_lexer_create},
    {"delete",1,0,yy_lexer_delete},
    {"getNextMatch",1,1,yy_lexer_getNextMatch},
    {"getAtom",1,1,yy_lexer_getAtom},
    {"getString",1,1,yy_lexer_getString},
    {"getLength",1,1,yy_lexer_getLength},
    {"switchToBuffer",2,0,yy_lexer_switchToBuffer},
    {"setMode",2,0,yy_lexer_setMode},
    {"currentMode",1,1,yy_lexer_currentMode},
    {"input",1,1,yy_lexer_input},
    {"unput",2,0,yy_lexer_unput},
    {0,0,0,0}
  };
  return oz_interface;
}
