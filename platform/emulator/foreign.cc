/*
 *  Authors:
 *    Michael Mehl (mehl@dfki.de)
 * 
 *  Contributors:
 *    Tobias Mueller (tmueller@ps.uni-sb.de)
 *    Denys Duchier (duchier@ps.uni-sb.de)
 * 
 *  Copyright:
 *    Michael Mehl (1997)
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

#include <errno.h>
#include <string.h>
#include <stdarg.h>

#include "iso-ctype.hh"

#include "runtime.hh"

#include "genvar.hh"

#include "ofgenvar.hh"

#include "gmp.h"
#include "os.hh"
#include "builtins.hh"

// forward decl
static
void term2Buffer(ostream &out, OZ_Term term, int depth=0);

/* ------------------------------------------------------------------------ *
 * tests
 * ------------------------------------------------------------------------ */

OZ_Term OZ_deref(OZ_Term term)
{
  return oz_deref(term);
}

int OZ_isAtom(OZ_Term term)
{
  term = oz_deref(term);
  return oz_isAtom(term);
}

int OZ_isCell(OZ_Term term)
{
  term = oz_deref(term);
  return oz_isCell(term);
}

int OZ_isPort(OZ_Term term)
{
  term = oz_deref(term);
  return oz_isPort(term);
}

int OZ_isChunk(OZ_Term term)
{
  term = oz_deref(term);
  return oz_isChunk(term);
}

int OZ_isCons(OZ_Term term)
{
  term = oz_deref(term);
  return oz_isCons(term);
}

int OZ_isFloat(OZ_Term term)
{
  term = oz_deref(term);
  return oz_isFloat(term);
}

int OZ_isInt(OZ_Term term)
{
  term = oz_deref(term);
  return oz_isInt(term);
}

int OZ_isNumber(OZ_Term term)
{
  term = oz_deref(term);
  return oz_isNumber(term);
}

int OZ_isSmallInt(OZ_Term term)
{
  term = oz_deref(term);
  return oz_isSmallInt(term);
}

int OZ_isBigInt(OZ_Term term)
{
  term = oz_deref(term);
  return oz_isBigInt(term);
}

int OZ_isProcedure(OZ_Term term)
{
  term = oz_deref(term);
  return oz_isProcedure(term);
}


int OZ_isLiteral(OZ_Term term)
{
  term = oz_deref(term);
  return oz_isLiteral(term);
}

int OZ_isFeature(OZ_Term term)
{
  term = oz_deref(term);
  return oz_isFeature(term);
}

int OZ_isName(OZ_Term term)
{
  term = oz_deref(term);
  return oz_isName(term);
}

int OZ_isNil(OZ_Term term)
{
  term = oz_deref(term);
  return oz_isNil(term);
}

int OZ_isObject(OZ_Term term)
{
  term = oz_deref(term);
  return oz_isObject(term);
}

int OZ_isThread(OZ_Term t)
{
  t = oz_deref(t);
  return oz_isThread(t);
}

int OZ_isString(OZ_Term term,OZ_Term *var)
{
  OZ_Term ret = oz_isList(term,1);
  if (oz_isRef(ret)) {
    if (var) *var = ret;
    return 0;
  }
  if (var) *var = 0;
  return ret!=NameFalse;
}

int OZ_isProperString(OZ_Term term,OZ_Term *var)
{
  OZ_Term ret = oz_isList(term,2);
  if (oz_isRef(ret)) {
    if (var) *var = ret;
    return 0;
  }
  if (var) *var = 0;
  return ret!=NameFalse;
}

int OZ_isList(OZ_Term term,OZ_Term *var)
{
  OZ_Term ret = oz_isList(term);
  if (oz_isRef(ret)) {
    if (var) *var = ret;
    return 0;
  }
  if (var) *var = 0;
  return ret!=NameFalse;
}

int OZ_isTrue(OZ_Term term)
{
  term = oz_deref(term);
  return literalEq(term,NameTrue);
}

int OZ_isFalse(OZ_Term term)
{
  term = oz_deref(term);
  return literalEq(term,NameFalse);
}

int OZ_isUnit(OZ_Term term)
{
  term = oz_deref(term);
  return literalEq(term,NameUnit);
}


int OZ_isPair(OZ_Term term)
{
  term = oz_deref(term);
  return oz_isPair(term);
}

int OZ_isPair2(OZ_Term term)
{
  term = oz_deref(term);
  return oz_isPair2(term);
}

int OZ_isRecord(OZ_Term term)
{
  term = oz_deref(term);
  return oz_isRecord(term);
}

int OZ_isTuple(OZ_Term term)
{
  term = oz_deref(term);
  return oz_isTuple(term);
}

int OZ_isValue(OZ_Term term)
{
  term = oz_deref(term);
  return !oz_isVariable(term);
}

int OZ_isVariable(OZ_Term term)
{
  term = oz_deref(term);
  return oz_isVariable(term);
}

OZ_Term OZ_termType(OZ_Term term)
{
  term = oz_deref(term);

  if (oz_isThread(term)) {
    return oz_atom("thread");
  }
  
  if (oz_isVariable(term)) {
    return oz_atom("variable");
  }

  if (oz_isInt(term)) {
    return oz_atom("int");
  }
    
  if (oz_isFloat(term)) {
    return oz_atom("float");
  }
    
  if (oz_isLiteral(term)) {
    return oz_atom(tagged2Literal(term)->isAtom() ? "atom" : "name");
  }

  if (oz_isTuple(term)) {
    return oz_atom("tuple");
  }
    
  if (oz_isProcedure(term)) {
    return oz_atom("procedure");
  }
    
  if (oz_isCell(term)) {
    return oz_atom("cell");
  }

  if (oz_isChunk(term)) {
    return oz_atom("chunk");
  }
    
  if (oz_isSRecord(term)) {
    return oz_atom("record");
  }

  if (oz_isSpace(term)) {
    return oz_atom("space");
  }

  if (oz_isFSetValue(term)) {
    return oz_atom("fset");
  }

  if (OZ_isForeignPointer(term)) {
    return oz_atom("foreign_pointer");
  }

  if (oz_isPromise(term)) {
    return oz_atom("promise");
  }

  OZ_warning("OZ_termType: unknown type in 0x%x\n",term);
  return 0;
}

/* -----------------------------------------------------------------
 * providing constants
 *------------------------------------------------------------------*/

int OZ_getLowPrio(void) 
{
  return OZMIN_PRIORITY;
}

int OZ_getMediumPrio(void) 
{
  return DEFAULT_PRIORITY;
}

int OZ_getHighPrio(void) 
{
  return OZMAX_PRIORITY;
}

int OZ_smallIntMin(void) 
{
  return OzMinInt;
}

int OZ_smallIntMax(void) 
{
  return OzMaxInt;
}

OZ_Term OZ_false(void)
{
  return NameFalse;
}

OZ_Term OZ_true(void)
{
  return NameTrue;
}

OZ_Term OZ_unit(void)
{
  return NameUnit;
}

/* -----------------------------------------------------------------
 * convert: C from/to Oz datastructure
 * -----------------------------------------------------------------*/

/*
 * Ints
 */

OZ_Term OZ_int(int i)
{
  return oz_int(i);
}

/*
 * BigInt: return INT_MAX/INT_MIN if too big
 */
int OZ_intToC(OZ_Term term)
{
  term = oz_deref(term);
  if (oz_isSmallInt(term)) {
    return smallIntValue(term);
  }

  return tagged2BigInt(term)->getInt();
}

OZ_Term OZ_CStringToInt(char *str)
{
  if (!str || str[0] == '\0') {
    OZ_warning("OZ_CStringToInt: empty string");
    return 0;
  }

  char *aux = str;
  int sign = 1;
  if (aux[0] == '~') {
    aux++;
    sign = -1;
  }

  MP_INT theInt;
  mpz_init(&theInt);
  if (aux[0] == '0') {
    switch (aux[1]) {
    case '\0':
      return newSmallInt(0);
    case 'x': case 'X':
      if (aux[2] == '\0' || mpz_set_str(&theInt, &aux[2], 16) == -1) {
	mpz_clear(&theInt);
	return 0;
      }
      break;
    case 'b': case 'B':
      if (aux[2] == '\0' || mpz_set_str(&theInt, &aux[2], 2) == -1) {
	mpz_clear(&theInt);
	return 0;
      }
      break;
    case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7':
      if (mpz_set_str(&theInt, &aux[1], 8) == -1) {
	mpz_clear(&theInt);
	return 0;
      }
      break;
    default:
      mpz_clear(&theInt);
      return 0;
    }
  } else if (aux[0] == '\0' || mpz_set_str(&theInt, aux, 10) == -1) {
    mpz_clear(&theInt);
    return 0;
  }

  BigInt *theBigInt = new BigInt(&theInt);
  mpz_clear(&theInt);
  if (sign > 0)
    return theBigInt->shrink();
  else
    return theBigInt->neg();
}

/*
 * parse: [~]<digit>+.<digit>*[(e|E)[~]<digit>+]
 */
char *OZ_parseFloat(char *s) {
  char *p = OZ_parseInt(s);
  if (!p || *p++ != '.') {
    return NULL;
  }
  while (iso_isdigit((unsigned char) *p)) {
    p++;
  }
  switch (*p) {
  case 'e':
  case 'E':
    p++;
    break;
  default:
    return p;
  }
  return OZ_parseInt(p);
}

/*
 * parse: [~]<digit>+
 */
char *OZ_parseInt(char *s)
{
  char *p = s;
  if (*p == '~') {
    p++;
  }
  if (!iso_isdigit((unsigned char) *p++)) {
    return 0;
  }
  while (iso_isdigit((unsigned char) *p)) {
    p++;
  }
  return p;
}


/*
 * Floats
 */

OZ_Term OZ_float(double i)
{
  return oz_float(i);
}

double OZ_floatToC(OZ_Term term)
{
  term = oz_deref(term);
  return floatValue(term);
}

/*
 * PRE: OZ_parseFloat succesful
 * see strtod(3)
 */
OZ_Term OZ_CStringToFloat(char *s)
{
  replChar(s,'~','-');

  char *end;
  double res = strtod(s,&end);

  // undo changes
  replChar(s,'-','~');

  return oz_float(res);
}


/*
 * Numbers
 */

OZ_Term OZ_CStringToNumber(char *s)
{
  if (strchr(s, '.') != NULL) {
    return OZ_CStringToFloat(s);
  }
  return OZ_CStringToInt(s);
}


/*
 * Features
 */

int OZ_featureCmp(OZ_Term term1, OZ_Term term2)
{
  term1 = oz_deref(term1);
  term2 = oz_deref(term2);
  return featureCmp(term1,term2);
}


/*
 * PRINTING
 */
static
void smallInt2buffer(ostream &out, OZ_Term term)
{
  int i = smallIntValue(term);
  if (i < 0) {
    out << '~' << -i;
  } else {
    out << i;
  }
}

static
void bigInt2buffer(ostream &out, BigInt *bb)
{
  char *str = new char[bb->stringLength()+1];
  bb->getString(str);
  if (*str == '-') *str = '~';
  out << str;
  delete str;
}


inline
char *strAndDelete(ostrstream *out)
{
  (*out) << ends;
  char *ret = ozstrdup(out->str());
  delete out;
  return ret;
}

static
void float2buffer(ostream &out, OZ_Term term)
{
  double f = floatValue(term);

  ostrstream *tmp = new ostrstream;
  (*tmp) << f;
  char *str = strAndDelete(tmp);

  // normalize float
  Bool hasDot = NO;
  Bool hasDigits = NO;
  char *s = str;
  for (char c=*s++; c ; c=*s++) {
    switch (c) {
    case 'e':
      if (!hasDot) out << '.' << '0';
      hasDot = OK;
      out << c;
      break;
    case '.':
      if (!hasDigits) out << '0';
      hasDot = OK;
      out << c;
      break;
    case '-':
      out << '~';
      break;
    case '+':
      break;
    default:
      if (!iso_isdigit((unsigned char) c)) hasDot=OK;
      hasDigits=OK;
      out << c;
      break;
    }
  }
  if (!hasDot) out << '.' << '0';
  delete str;
}

inline
void octOut(ostream &out,unsigned char c)
{
  out << (char) (((c >> 6) & '\007') + '0')
      << (char) (((c >> 3) & '\007') + '0')
      << (char) (( c       & '\007') + '0');
}


inline
void atomq2buffer(ostream &out, const char *s)
{
  char c;
  while ((c = *s)) {
    if (iso_iscntrl(c)) {
      out << '\\';
      switch (c) {
      case '\'':
	out << '\'';
	break;
      case '\a':
	out << 'a';
	break;
      case '\b':
	out << 'b';
	break;
      case '\f':
	out << 'f';
	break;
      case '\n':
	out << 'n';
	break;
      case '\r':
	out << 'r';
	break;
      case '\t':
	out << 't';
	break;
      case '\v':
	out << 'v';
	break;
      default:
	octOut(out,c);
	break;
      }
    } else if (iso_isprint(c)) {
      switch (c) {
      case '\'':
	out << '\\' << '\'';
	break;
      case '\\':
	out << '\\' << '\\';
	break;
      default:
	out << c;
	break;
      }
    } else {
      out << '\\';
      octOut(out,c);
    }
    s++;
  }
}


inline
Bool checkAtom(const char *s)
{
  const char *t = s;
  unsigned char c = *s++;
  if (!c || !iso_islower(c)) {
    return NO;
  }
  c=*s++;
  while (c) {
    if (!iso_isalnum(c) && c != '_') {
      return NO;
    }
    c=*s++;
  }
  switch (t[0]) {
  case 'a':
    return strcmp(t, "andthen") && strcmp(t, "attr")? OK: NO;
  case 'b':
    return strcmp(t, "body")? OK: NO;
  case 'c':
    return strcmp(t, "case") && strcmp(t, "catch")
	&& strcmp(t, "choice") && strcmp(t, "class")
	&& strcmp(t, "condis")? OK: NO;
  case 'd':
    return strcmp(t, "declare") && strcmp(t, "dis")
	&& strcmp(t, "div")? OK: NO;
  case 'e':
    return strcmp(t, "else") && strcmp(t, "elsecase")
	&& strcmp(t, "elseif") && strcmp(t, "elseof")
	&& strcmp(t, "end") && strcmp(t, "export")? OK: NO;
  case 'f':
    return strcmp(t, "false") && strcmp(t, "feat")
	&& strcmp(t, "finally") && strcmp(t, "from")
	&& strcmp(t, "fun") && strcmp(t, "functor")
	&& strcmp(t, "fail")? OK: NO;
  case 'i':
    return strcmp(t, "if") && strcmp(t, "import")
	&& strcmp(t, "in")? OK: NO;
  case 'l':
    return strcmp(t, "local") && strcmp(t, "lock")? OK: NO;
  case 'm':
    return strcmp(t, "meth") && strcmp(t, "mod")? OK: NO;
  case 'n':
    return strcmp(t, "not")? OK: NO;
  case 'o':
    return strcmp(t, "of") && strcmp(t, "or")
	&& strcmp(t, "orelse")? OK: NO;
  case 'p':
    return strcmp(t, "proc") && strcmp(t, "prop")? OK: NO;
  case 'r':
    return strcmp(t, "raise")? OK: NO;
  case 's':
    return strcmp(t, "self") && strcmp(t, "skip")? OK: NO;
  case 't':
    return strcmp(t, "then") && strcmp(t, "thread")
        && strcmp(t, "true")
	&& strcmp(t, "try")? OK: NO;
  case 'u':
    return strcmp(t, "unit")? OK: NO;
  case 'w':
    return strcmp(t, "with")? OK: NO;
  default:
    return OK;
  }
}

inline
void atom2buffer(ostream &out, Literal *a)
{
  const char *s = a->getPrintName();
  if (checkAtom(s)) {
    out << s;
  } else {
    out << '\'';
    atomq2buffer(out,s);
    out << '\'';
  }
}

inline
void name2buffer(ostream &out, Literal *a) {
  const char *s = a->getPrintName();

  if (literalEq(makeTaggedLiteral(a),NameTrue))  {
    out << "true";
  } else if (literalEq(makeTaggedLiteral(a),NameFalse)) {
    out << "false";
  } else if (literalEq(makeTaggedLiteral(a),NameUnit)) {
    out << "unit";
  } else if (!*s) {
    out << "<N>";
  } else {
    out << "<N: " << s << '>';
  }
}

inline
void const2buffer(ostream &out, ConstTerm *c)
{
  const char *s = c->getPrintName();

  switch (c->getType()) {
  case Co_BigInt:
    bigInt2buffer(out,(BigInt *)c);
    break;
  case Co_Thread:
    out << "<Thread #" << (((Thread*) c)->getID() & THREAD_ID_MASK) << ">" ;
    break; 
  case Co_Abstraction:
  case Co_Builtin:
    out << "<P/" << c->getArity();
    if (*s != 0) {
      out << ' ' << s;
    }
    out << '>';
    break;

  case Co_Cell:       out << "<Cell>"; break;
  case Co_Port:       out << "<Port>"; break;
  case Co_Space:      out << "<Space>"; break;
  case Co_Lock:       out << "<Lock>"; break;
  case Co_Array:      out << "<Array>"; break;
  case Co_Dictionary: out << "<Dictionary>"; break;
  case Co_BitArray:   out << "<BitArray>"; break;

  case Co_Class:
  case Co_Object:
    if (*s == '_' && *(s+1) == 0) {
      out << (isObjectClass(c) ? "<C>" : "<O>");
    } else {
      out << (isObjectClass(c) ? "<C: " : "<O: ") << s << '>';
    }
    break;

  case Co_Foreign_Pointer:
    out << "<ForeignPointer " << ((ForeignPointer *) c)->getPointer() << ">";
    break;

  default:
    out << "<Chunk>";
    break;
  }
}


inline
void feature2buffer(ostream &out, SRecord *sr, OZ_Term fea, int depth)
{
  term2Buffer(out,fea);
  out << ':';
  term2Buffer(out,sr->getFeature(fea),depth);
}

inline
Bool isNiceHash(OZ_Term t, int width) {
  if (width <= 0) return NO;

  if (!oz_isSTuple(t) || !literalEq(tagged2SRecord(t)->getLabel(),AtomPair)) 
    return NO;

  int w = tagged2SRecord(t)->getWidth();

  return ((w <= width) && (w > 1)) ? OK : NO;
}

inline
Bool isNiceList(OZ_Term l, int width) {
  if (width <= 0) return NO;

  while (oz_isCons(l) && width--> 0) {
    l = oz_deref(tail(l));
  }
  
  if (oz_isNil(l)) return OK;

  return NO;
}

inline
void record2buffer(ostream &out, SRecord *sr,int depth) {
  if (isNiceHash(makeTaggedSRecord(sr), ozconf.printWidth)) {
    int len = sr->getWidth();
    for (int i=0; i < len; i++) {
      OZ_Term arg = oz_deref(sr->getArg(i));
      if (isNiceHash(arg,ozconf.printWidth) ||
	  (oz_isCons(arg) && !isNiceList(arg,ozconf.printWidth))) {
	out << '(';
	term2Buffer(out, sr->getArg(i), depth-1);
	out << ')';
      } else {
	term2Buffer(out, sr->getArg(i), depth-1);
      }
      if (i+1!=len)
	out << '#';
    }
    return;
  }

  term2Buffer(out,sr->getLabel());
  out << '(';
  if (depth <= 0 || ozconf.printWidth <= 0) {
    out << ",,,";
  } else {
    if (sr->isTuple()) {
      int len = min(ozconf.printWidth, sr->getWidth());
      term2Buffer(out,sr->getArg(0), depth-1);
      for (int i=1; i < len; i++) {
	out << ' ';
	term2Buffer(out,sr->getArg(i),depth-1);
      }
      
      if (sr->getWidth() > ozconf.printWidth)
	out << " ,,,";
    } else {
      OZ_Term as = sr->getArityList();
      Assert(oz_isCons(as));

      int next    = 1;

      while (oz_isCons(as) && next <= ozconf.printWidth &&
	     oz_isSmallInt(head(as)) && 
	     smallIntValue(head(as)) == next) {
	term2Buffer(out, sr->getFeature(head(as)), depth-1);
	out << ' ';
	as = tail(as);
	next++;
      }
      Assert(oz_isCons(as));

      if (next <= ozconf.printWidth) {
	
	feature2buffer(out,sr,head(as),depth-1);
	next++;
	as = tail(as);
	while (next <= ozconf.printWidth && oz_isCons(as)) {
	  out << ' ';
	  feature2buffer(out,sr,head(as),depth-1);
	  as = tail(as);
	  next++;
	}
      }

      if (sr->getWidth() > ozconf.printWidth)
	out << " ,,,";
    }
  }
  out << ')';
}

inline
void list2buffer(ostream &out, LTuple *list,int depth) {
  int width = ozconf.printWidth;

  if (width > 0 && depth > 0) {

    if (isNiceList(makeTaggedLTuple(list),width)) {
      out << '[';
      OZ_Term l = makeTaggedLTuple(list);
      while (oz_isCons(l)) {
	term2Buffer(out, head(l), depth-1);
	l = oz_deref(tail(l));
	if (oz_isCons(l)) {
	  out << ' ';
	}
      }
      out << ']';
      return;
    }

    while (width-- > 0) { 
      OZ_Term a=oz_deref(list->getHead());
      if (oz_isCons(a) && !isNiceList(a,ozconf.printWidth)) {
	out << '('; term2Buffer(out,list->getHead(),depth-1); out << ')';
      } else {
	term2Buffer(out,list->getHead(),depth-1);
      }
      out << '|';
      OZ_Term t=oz_deref(list->getTail());
      if (!oz_isCons(t)) {
	term2Buffer(out,list->getTail(),depth);
	return;
      }
      list = tagged2LTuple(t);
    }
  }
  out << ",,,|,,,";
}


ostream &DynamicTable::newprint(ostream &out, int depth)
{
  // Count Atoms & Names in dynamictable:
  OZ_Term tmplit,tmpval;
  dt_index di;
  long ai;
  long nAtomOrInt=0;
  long nName=0;
  for (di=0; di<size; di++) {
    tmplit=table[di].ident;
    tmpval=table[di].value;
    if (tmpval) { 
      CHECK_DEREF(tmplit);
      if (oz_isAtom(tmplit)||oz_isInt(tmplit)) nAtomOrInt++; else nName++;
    }
  }
  // Allocate array on heap, put Atoms in array:
  OZ_Term *arr = new OZ_Term[nAtomOrInt+1]; // +1 since nAtomOrInt may be zero
  for (ai=0,di=0; di<size; di++) {
    tmplit=table[di].ident;
    tmpval=table[di].value;
    if (tmpval!=makeTaggedNULL() && (oz_isAtom(tmplit)||oz_isInt(tmplit)))
      arr[ai++]=tmplit;
  }
  // Sort the Atoms according to printName:
  inplace_quicksort(arr, arr+(nAtomOrInt-1));

  // Output the Atoms first, in order:
  for (ai=0; ai<nAtomOrInt; ai++) {
    term2Buffer(out,arr[ai],0);
    out << ':';
    term2Buffer(out,lookup(arr[ai]),depth);
    out << ' ';
  }
  // Output the Names last, unordered:
  for (di=0; di<size; di++) {
    tmplit=table[di].ident;
    tmpval=table[di].value;
    if (tmpval!=makeTaggedNULL() && !(oz_isAtom(tmplit)||oz_isInt(tmplit))) {
      term2Buffer(out,tmplit,0);
      out << ':';
      term2Buffer(out,tmpval,depth);
      out << ' ';
    }
  }
  // Deallocate array:
  delete arr;
  return out;
}

static
void fset2buffer(ostream &out, OZ_FSetValue * fs) 
{
  out << fs->toString();
}

static
void cvar2buffer(ostream &out, const char *s, GenCVariable *cv, int depth)
{
  switch(cv->getType()){
  case FDVariable:
    {
      out << s;
      out << ((GenFDVariable *) cv)->getDom().toString();
      break;
    }

  case FSetVariable:
    {
      out << s;
      out << ((GenFSetVariable *) cv)->getSet().toString();
      break;
    }

  case BoolVariable:
    {
      out << s;
      out << "{0#1}";
      break;
    }

  case OFSVariable:
    {
      GenOFSVariable* ofs = (GenOFSVariable *) cv;
      term2Buffer(out,ofs->getLabel(),0);
      out << '(';
      if (depth > 0) {
	ofs->getTable()->newprint(out,depth-1);
      } else {
	out << ",,, ";
	break;
      }
      out << "...)";
      break;
   }

  case PerdioVariable:
    {
      PerdioVar *pv = (PerdioVar *) cv;
      if (pv->isFuture()) {
	out << s;
	out << "<future>";
	break;
      }

      out << s << "<dist:";
      char *type = "";
      if (pv->isManager()) {
	type = "mgr";
      } else if (pv->isProxy()) {
	type = "pxy";
      } else {
	type = "oprxy";
      }
      out << type << ">";
      break;
    }
  default:
    out << s << "/";
    cv->printStreamV(out,depth);
    break;
  }
}

void oz_printStream(OZ_Term term, ostream &out, int depth, int width)
{
  int old;
  if (width>=0) {
    old = ozconf.printWidth;
    ozconf.printWidth=width;
  }
  if (depth<0) {
    depth = ozconf.printDepth;
  }

  term2Buffer(out,term,depth);
  flush(out);

  if (width>=0) {
    ozconf.printWidth=old;
  }
}

// for printing from gdb: no default args needed
void oz_print(OZ_Term term) {
  oz_printStream(term,cerr);
  cerr << endl;
  flush(cerr);
}

static
void term2Buffer(ostream &out, OZ_Term term, int depth)
{
  if (!term) {
    out << "<Null pointer>";
    return;
  }

  DEREF(term,termPtr,tag);
  switch(tag) {
  case UVAR:
  case SVAR:
  case CVAR:
    {
      if (!termPtr) {
	out << "<Oz_Dereferenced variable>";
	break;
      }
      const char *s = getVarName(makeTaggedRef(termPtr));
      if (isCVar(tag)) {
	cvar2buffer(out, s,tagged2CVar(term),depth);
      } else {
	out << s;
      }
      break;
    }
  case FSETVALUE:
    fset2buffer(out, tagged2FSetValue(term));
    break;
  case SRECORD:
    record2buffer(out,tagged2SRecord(term),depth);
    break;
  case LTUPLE:
    list2buffer(out,tagged2LTuple(term),depth);
    break;
  case OZCONST:
    const2buffer(out,tagged2Const(term));
    break;
  case PROMISE:
    out << "<Promise>";
    break;
  case LITERAL:
    {
      Literal *a = tagged2Literal(term);
      if (a->isAtom()) {
	atom2buffer(out,a);
      } else {
	name2buffer(out,a);
      }
      break;
    }
  case OZFLOAT:
    float2buffer(out,term);
    break;
  case SMALLINT:
    smallInt2buffer(out,term);
    break;
  default:
    out << "<Unknown Tag: " << tag << ">";
    break;
  }
}

char *OZ_toC(OZ_Term term, int depth, int width)
{
  static char *tmpString = 0;
  if (tmpString) {
    delete tmpString;
  }

  ostrstream *out = new ostrstream;

  oz_printStream(term,*out,depth,width);

  tmpString = strAndDelete(out);
  return tmpString;
}

char *toC(OZ_Term term)
{
  return OZ_toC(term,ozconf.errorPrintDepth,ozconf.errorPrintWidth);
}

int OZ_termGetSize(OZ_Term term, int depth, int width)
{
  ostrstream *out=new ostrstream;
  int old=ozconf.printWidth;

  ozconf.printWidth = width;
  term2Buffer(*out,term,depth);
  ozconf.printWidth = old;

  int ret = out->pcount ();
  delete out;
  return ret;
}

/*
 * Atoms
 */

const char *OZ_atomToC(OZ_Term term)
{
  term = oz_deref(term);

  Literal *a = tagged2Literal(term);
  return a->getPrintName();
}

OZ_Term OZ_atom(const char *s)
{
  return oz_atom(s);
}

/* -----------------------------------------------------------------
 * virtual strings
 * -----------------------------------------------------------------*/

/* convert a C string (char*) to an Oz string */
OZ_Term OZ_string(const char *s)
{
  if (!s) { return nil(); }
  const char *p=s;
  while (*p!='\0') {
    p++;
  }
  OZ_Term ret = nil();
  while (p!=s) {
    ret = cons(makeInt((unsigned char)*(--p)), ret);
  }
  return ret;
}

void string2buffer(ostream &out,OZ_Term list)
{
  OZ_Term tmp = oz_deref(list);
  for (; oz_isCons(tmp); tmp=oz_deref(tail(tmp))) {
    OZ_Term hh = oz_deref(head(tmp));
    if (!oz_isSmallInt(hh)) {
      message("no small int %s",toC(hh));
      printf(" in string %s\n",toC(list));
      return;
    }
    int i = smallIntValue(hh);
    if (i <= 0 || i > 255) {
      message("no small int %d",i);
      printf(" in string %s\n",toC(list));
      return;
    }
    out << (char) i;
  }
  if (!oz_isNil(tmp)) {
    message("no string %s\n",toC(list));
  }
}

/*
 * convert Oz string to C string
 * PRE: list is a proper string
 */
char *OZ_stringToC(OZ_Term list)
{
  static char *tmpString = 0;
  if (tmpString) {
    delete tmpString;
    tmpString = 0;
  }

  ostrstream *out = new ostrstream;

  string2buffer(*out,list);

  tmpString = strAndDelete(out);
  return tmpString;
}

void OZ_printString(OZ_Term term)
{
  ostrstream *out = new ostrstream;

  string2buffer(*out,term);

  char *tmpString = strAndDelete(out);
  printf("%s",tmpString);
  delete tmpString;
}

void OZ_printAtom(OZ_Term t)
{
  Literal *a = tagged2Literal(t);
  printf("%s",a->getPrintName());
}

void OZ_printInt(OZ_Term t)
{
  t=oz_deref(t);
  if (oz_isSmallInt(t)) {
    printf("%d",smallIntValue(t));
  } else {
    char *s=toC(t);
    printf("%s",s);
  }
}

void OZ_printFloat(OZ_Term t)
{
  char *s=toC(t);
  printf("%s",s);
}



inline
void vsatom2buffer(ostream &out, OZ_Term term)
{
  const char *s = tagged2Literal(term)->getPrintName();
  out << s;
}

void virtualString2buffer(ostream &out,OZ_Term term)
{
  OZ_Term t=oz_deref(term);
  if (oz_isCons(t)) {
    string2buffer(out,t);
    return;
  }
  if (oz_isAtom(t)) {
    if (oz_isNil(t) || oz_isPair(t)) return;
    vsatom2buffer(out,t);
    return;
  }
  if (oz_isSmallInt(t)) {
    smallInt2buffer(out,t);
    return;
  }
  if (oz_isBigInt(t)) {
    bigInt2buffer(out,tagged2BigInt(t));
    return;
  }
  if (oz_isFloat(t)) {
    float2buffer(out,t);
    return;
  }

  if (!oz_isPair(t)) {
    OZ_warning("no virtual string: %s",toC(term));
    return;
  }

  SRecord *sr=tagged2SRecord(t);
  int len=sr->getWidth();
  for (int i=0; i < len; i++) {
    virtualString2buffer(out,sr->getArg(i));
  }
}

/*
 * convert Oz virtual string to C string
 * PRE: list is a virtual string
 */
char *OZ_virtualStringToC(OZ_Term t)
{
  static char *tmpString = 0;
  if (tmpString) {
    delete tmpString;
    tmpString = 0;
  }

  ostrstream *out = new ostrstream;

  virtualString2buffer(*out,t);

  tmpString = strAndDelete(out);
  return tmpString;
}


void OZ_printVirtualString(OZ_Term term)
{
  OZ_Term t=oz_deref(term);
  if (oz_isCons(t)) {
    OZ_printString(t);
  } else if (oz_isAtom(t)) {
    if (oz_isNil(t) || oz_isPair(t)) {
      return;
    }
    OZ_printAtom(t);
  } else if (oz_isSmallInt(t)) {
      printf("%d",smallIntValue(t));
  } else if (oz_isBigInt(t)) {
    char *s=toC(t);
    if (s[0] == '~') 
      s[0] = '-';
    printf("%s",s);
  } else if (oz_isFloat(t)) {
    char *s=toC(t);
    char *p=s;
    while (*p) {
      if (*p == '~') 
	*p='-';
      p++;
    }
    printf("%s",s);
  } else {
    if (!oz_isPair(t)) {
      OZ_warning("OZ_printVirtualString: no virtual string: %s",toC(term));
      return;
    }
    SRecord *sr=tagged2SRecord(t);
    int len=sr->getWidth();
    for (int i=0; i < len; i++) {
      OZ_printVirtualString(sr->getArg(i));
    }
  }
}

OZ_Term OZ_toVirtualString(OZ_Term t,int depth, int width)
{
  switch (tagTypeOf(oz_deref(t))) {
  case SMALLINT:
  case OZFLOAT:
    return t;
  case UVAR:
  case SVAR:
  case CVAR:
  case LTUPLE:
  case SRECORD:
  case OZCONST:
    if (oz_isBigInt(t)) return t;
    return OZ_string(OZ_toC(t,depth,width));
  case LITERAL:
    if (OZ_isAtom(t)) return t;
    return OZ_string(toC(t));
  default:
    return 0;
  }
}

/* -----------------------------------------------------------------
 * tuple
 * -----------------------------------------------------------------*/

OZ_Term OZ_label(OZ_Term term)
{
  DEREF(term,termPtr,termTag);

  switch (termTag) {
  case LTUPLE:
    return AtomCons;
  case LITERAL:
    return term;
  case SRECORD:
    return tagged2SRecord(term)->getLabel();
  default:
    OZ_warning("OZ_label: no record");
    return 0;
  }
}

int OZ_width(OZ_Term term)
{
  DEREF(term,termPtr,termTag);

  switch (termTag) {
  case LTUPLE:
    return 2;
  case SRECORD:
    return tagged2SRecord(term)->getWidth();
  case LITERAL:
    return 0;
  default:
    OZ_warning("OZ_width: no record");
    return 0;
  }
}

OZ_Term OZ_tuple(OZ_Term label, int width) 
{
  label = oz_deref(label);
  if (!oz_isLiteral(label)) {
    OZ_warning("OZ_tuple: label is no literal");
    return 0;
  }

  if (width == 2 && literalEq(label,AtomCons)) {
    // have to make a list
    return makeTaggedLTuple(new LTuple());
  }

  if (width <= 0) {
    return label;
  }
  
  return makeTaggedSRecord(SRecord::newSRecord(label,width));
}

OZ_Term OZ_mkTupleC(char *label,int arity,...)
{
  if (arity == 0) {
    return OZ_atom(label);
  }
  va_list ap;
  va_start(ap,arity);

  OZ_Term tt=OZ_tuple(OZ_atom(label),arity);
  for (int i = 0; i < arity; i++) {
    OZ_putArg(tt,i,va_arg(ap,OZ_Term));
  }

  va_end(ap);
  return tt;
}

OZ_Term OZ_mkTuple(OZ_Term label,int arity,...)
{
  va_list ap;
  va_start(ap,arity);

  OZ_Term tt=OZ_tuple(label,arity);
  for (int i = 0; i < arity; i++) {
    OZ_putArg(tt,i,va_arg(ap,OZ_Term));
  }

  va_end(ap);
  return tt;
}

void OZ_putArg(OZ_Term term, int pos, OZ_Term newTerm)
{
  term=oz_deref(term);
  if (oz_isLTuple(term)) {
    switch (pos) {
    case 0:
      tagged2LTuple(term)->setHead(newTerm);
      return;
    case 1:
      tagged2LTuple(term)->setTail(newTerm);
      return;
    }
  }
  if (!oz_isSTuple(term)) {
    OZ_warning("OZ_putArg: no record");
    return;
  }
  tagged2SRecord(term)->setArg(pos,newTerm);
}

OZ_Term OZ_getArg(OZ_Term term, int pos)
{
  term=oz_deref(term);
  if (oz_isLTuple(term)) {
    switch (pos) {
    case 0:
      return tagged2LTuple(term)->getHead();
    case 1:
      return tagged2LTuple(term)->getTail();
    }
  }
  if (!oz_isSRecord(term)) {
    OZ_warning("OZ_getArg: no record");
    return 0;
  }
  if (pos < 0 || pos >= tagged2SRecord(term)->getWidth()) {
    OZ_warning("OZ_getArg: invalid index: %d",pos);
    return 0;
  }
  return tagged2SRecord(term)->getArg(pos);
}

OZ_Term OZ_nil()
{
  return nil();
}

OZ_Term OZ_cons(OZ_Term hd,OZ_Term tl)
{
  return cons(hd,tl);
}

OZ_Term OZ_head(OZ_Term term)
{
  term=oz_deref(term);
  if (!oz_isLTuple(term)) {
    OZ_warning("OZ_head: no cons");
    return 0;
  }
  return head(term);
}

OZ_Term OZ_tail(OZ_Term term)
{
  term=oz_deref(term);
  if (!oz_isLTuple(term)) {
    OZ_warning("OZ_tail: no cons");
    return 0;
  }
  return tail(term);
}

/*
 * Compute the length of a list
 */
int OZ_length(OZ_Term l)
{
  OZ_Term ret=oz_isList(l);
  if (!oz_isSmallInt(ret)) return -1;
  return smallIntValue(ret);
}


OZ_Term OZ_toList(int len, OZ_Term *tuple)
{
  OZ_Term l = nil();
  while (--len >= 0) {
    l = cons(tuple[len],l);
  }
  return l;
}

/* -----------------------------------------------------------------
 * pairs
 * -----------------------------------------------------------------*/

OZ_Term OZ_pair(int n)
{
  SRecord *sr = SRecord::newSRecord(AtomPair,n);
  return makeTaggedSRecord(sr);
}

OZ_Term OZ_pair2(OZ_Term t1,OZ_Term t2) {
  return oz_pair2(t1,t2);
}

/* -----------------------------------------------------------------
 * record
 * -----------------------------------------------------------------*/

OZ_Arity OZ_makeArity(OZ_Term list)
{
  list=packsort(list);
  if (!list) return 0;
  return aritytable.find(list);
}

/* take a label and an arity (as list) and construct a record
   the fields are not initialized */
OZ_Term OZ_record(OZ_Term label, OZ_Term arity) 
{
  OZ_Arity newArity = OZ_makeArity(arity);
  return makeTaggedSRecord(SRecord::newSRecord(label,(Arity *) newArity));
}

/* take a label and a property list and construct a record */
OZ_Term OZ_recordInit(OZ_Term label, OZ_Term propList) 
{
  OZ_Term out;
  (void) adjoinPropList(label,propList,out,NO);

  return out;
}

void OZ_putSubtree(OZ_Term term, OZ_Term feature, OZ_Term value)
{
  term=oz_deref(term);
  if (oz_isCons(term)) {
    int i2 = smallIntValue(feature);

    switch (i2) {
    case 1:
      tagged2LTuple(term)->setHead(value);
      return;
    case 2:
      tagged2LTuple(term)->setTail(value);
      return;
    }
    OZ_warning("OZ_putSubtree: invalid feature");
    return;
  }
  if (!oz_isSRecord(term)) {
    OZ_warning("OZ_putSubtree: invalid record");
    return;
  }
  if (!tagged2SRecord(term)->setFeature(feature,value)) {
    OZ_warning("OZ_putSubtree: invalid feature");
    return;
  }
}

OZ_Term OZ_adjoinAt(OZ_Term rec, OZ_Term fea, OZ_Term val)
{
  rec = oz_deref(rec);
  fea = oz_deref(fea);
  if (!oz_isFeature(fea) || !oz_isRecord(rec)) return 0;

  if (oz_isLiteral(rec)) {
    SRecord *srec = SRecord::newSRecord(rec,aritytable.find(cons(fea,nil())));
    srec->setArg(0,val);
    return makeTaggedSRecord(srec);
  } else {
    return oz_adjoinAt(makeRecord(rec),fea,val);
  }
}

OZ_Term OZ_subtree(OZ_Term term, OZ_Term fea)
{
  DEREF(term,termPtr,termTag);
  fea=oz_deref(fea);

  switch (termTag) {
  case LTUPLE:
    {
      if (!oz_isSmallInt(fea)) return 0;

      int i2 = smallIntValue(fea);

      switch (i2) {
      case 1:
	return tagged2LTuple(term)->getHead();
      case 2:
	return tagged2LTuple(term)->getTail();
      }
      return 0;
    }
  case SRECORD:
    return tagged2SRecord(term)->getFeature(fea);

  case OZCONST:
    {
      ConstTerm *ct = tagged2Const(term);
      switch (ct->getType()) {
      case Co_Object:
	return ((Object *) ct)->getFeature(fea);
      case Co_Chunk:
	return ((SChunk *) ct)->getFeature(fea);
      default:
	return 0;
      }
    }
  default:
    return 0;
  }
}


OZ_Term OZ_arityList(OZ_Term term)
{
  OZ_Term arity;
  (void) BIarityInline(term, arity);
  return arity;
}

/* -----------------------------------------------------------------
 * unification
 * -----------------------------------------------------------------*/

OZ_Return OZ_unify(OZ_Term t1, OZ_Term t2)
{
  return oz_unify(t1,t2);
}

int OZ_eq(OZ_Term t1, OZ_Term t2)
{
  return oz_eq(t1,t2);
}


OZ_Term OZ_newVariable()
{
  return oz_newVariable();
}

/* -----------------------------------------------------------------
 * IO
 * -----------------------------------------------------------------*/

void OZ_registerReadHandler(int fd,OZ_IOHandler fun,void *val) {
  am.select(fd,SEL_READ,fun,val);
}

void OZ_unregisterRead(int fd) {
  am.deSelect(fd,SEL_READ);
}

void OZ_registerWriteHandler(int fd,OZ_IOHandler fun,void *val) {
  am.select(fd,SEL_WRITE,fun,val);
}

void OZ_unregisterWrite(int fd) {
  am.deSelect(fd,SEL_WRITE);
}

void OZ_registerAcceptHandler(int fd,OZ_IOHandler fun,void *val) {
  am.acceptSelect(fd,fun,val);
}





OZ_Return OZ_readSelect(int fd,OZ_Term l,OZ_Term r) {
  return am.select(fd,SEL_READ,l,r) ? PROCEED : FAILED;
}

OZ_Return OZ_writeSelect(int fd,OZ_Term l,OZ_Term r) {
  return am.select(fd,SEL_WRITE,l,r) ? PROCEED : FAILED;
}

OZ_Return OZ_acceptSelect(int fd,OZ_Term l,OZ_Term r) {
  am.acceptSelect(fd,l,r);
  return PROCEED;
}

void OZ_deSelect(int fd) {
  am.deSelect(fd);
}


/* -----------------------------------------------------------------
 * garbage collection
 * -----------------------------------------------------------------*/

int OZ_protect(OZ_Term *t)
{
  if (!gcProtect(t)) {
    OZ_warning("protect: failed");
    return 0;
  }
  return 1;
}

int OZ_unprotect(OZ_Term *t)
{
  if (!gcUnprotect(t)) {
    OZ_warning("unprotect: failed");
    return 0;
  }
  return 1;
}

/* -----------------------------------------------------------------
 * vs
 * -----------------------------------------------------------------*/


inline
int oz_isVirtualString(OZ_Term vs, OZ_Term *var)
{
  if (oz_isRef(vs)) {
    DEREF(vs,vsPtr,vsTag);
    if (isVariableTag(vsTag))  {
      if (var) *var = makeTaggedRef(vsPtr);
      return 0;
    }
  }

  if (oz_isInt(vs) || oz_isFloat(vs) || oz_isAtom(vs))  return 1;

  if (oz_isPair(vs)) {
    SRecord *sr = tagged2SRecord(vs);
    int len = sr->getWidth();
    for (int i=0; i < len; i++) {
      if (!oz_isVirtualString(sr->getArg(i),var)) return 0;
    }
    return 1;
  }

  if (oz_isCons(vs)) {
    OZ_Term ret = oz_isList(vs,2);
    if (oz_isRef(ret)) {
      if (var) *var = ret;
      return 0;
    }
    if (var) *var = 0;
    return ret==NameFalse ? 0 : 1;
  }

  return 0;
}

int OZ_isVirtualString(OZ_Term vs, OZ_Term *var)
{
  if (var) *var = 0;

  return oz_isVirtualString(vs,var);
}


/* -----------------------------------------------------------------
 * names
 * -----------------------------------------------------------------*/

OZ_Term OZ_newName()
{
  return oz_newName();
}
/* -----------------------------------------------------------------
 * 
 * -----------------------------------------------------------------*/

int OZ_addBuiltin(const char *name, int inArity, int outArity, OZ_CFun fun)
{
  return BIadd(name,inArity,outArity,fun,OK) == NULL ? 0 : 1;
}

void OZ_addBISpec(OZ_BIspec *spec)
{
  for (int i=0; spec[i].name; i++) {
    OZ_addBuiltin(spec[i].name,spec[i].inArity,spec[i].outArity,spec[i].fun);
  }
}

// don't raise errors, because debug info is never included!
// use OZ_raiseError
OZ_Return OZ_raise(OZ_Term exc) {
  am.setException(exc,FALSE);
  return RAISE;
}

// OZ_raiseDebug(E) raises exception E, but adds debuging information
// if E is a record with feature `debug' and either (1) has label `error'
// or (2) ozconf.errorDebug is true

OZ_Return OZ_raiseDebug(OZ_Term exc) {
  int debug =
    OZ_isRecord(exc) && OZ_subtree(exc,AtomDebug) &&
    (literalEq(OZ_label(exc),E_ERROR) || ozconf.errorDebug);
  am.setException(exc,debug);
  return RAISE;
}

OZ_Return OZ_raiseC(char *label,int arity,...)
{
  if (arity == 0) {
    return OZ_raise(OZ_atom(label));
  }

  va_list ap;
  va_start(ap,arity);

  OZ_Term tt=OZ_tuple(OZ_atom(label),arity);
  for (int i = 0; i < arity; i++) {
    OZ_putArg(tt,i,va_arg(ap,OZ_Term));
  }

  va_end(ap);
  return OZ_raise(tt);
}

OZ_Return OZ_raiseError(OZ_Term exc)
{
  OZ_Term ret = OZ_record(oz_atom("error"),
			  cons(oz_int(1),
			       cons(oz_atom("debug"),nil())));
  OZ_putSubtree(ret,oz_int(1),exc);
  OZ_putSubtree(ret,oz_atom("debug"),NameUnit);

  am.setException(ret,TRUE);
  return RAISE;
}

OZ_Return OZ_raiseErrorC(char *label,int arity,...)
{
  if (arity == 0) {
    return OZ_raiseError(OZ_atom(label));
  }

  va_list ap;
  va_start(ap,arity);

  OZ_Term tt=OZ_tuple(OZ_atom(label),arity);
  for (int i = 0; i < arity; i++) {
    OZ_putArg(tt,i,va_arg(ap,OZ_Term));
  }

  va_end(ap);
  return OZ_raiseError(tt);
}
 
OZ_Term OZ_makeException(OZ_Term cat,OZ_Term key,char*label,int arity,...)
{
  OZ_Term exc=OZ_tuple(key,arity+1);
  OZ_putArg(exc,0,OZ_atom(label));

  va_list ap;
  va_start(ap,arity);

  for (int i = 0; i < arity; i++) {
    OZ_putArg(exc,i+1,va_arg(ap,OZ_Term));
  }

  va_end(ap);


  OZ_Term ret = OZ_record(cat,
			  cons(OZ_int(1),
			       cons(OZ_atom("debug"),OZ_nil())));
  OZ_putSubtree(ret,OZ_int(1),exc);
  OZ_putSubtree(ret,OZ_atom("debug"),NameUnit);
  return ret;
}

/* -----------------------------------------------------------------
 * threads
 * -----------------------------------------------------------------*/

void OZ_pushCFun(OZ_Thread thr,OZ_CFun fun,OZ_Term *args,int arity)
{
  ((Thread *)thr)->pushCFun(fun, args, arity, OK);
}

void OZ_pushCall(OZ_Thread thr,OZ_Term fun,OZ_Term *args,int arity)
{
  ((Thread *)thr)->pushCall(fun, args, arity);
}

OZ_Thread OZ_newSuspendedThread()
{
#ifdef SHOW_SUSPENSIONS
  static int xxx=0;
  printf("Suspension(%d):",xxx++);
  for(int i=0; i<arity; i++) {
    printf("%s, ",toC(args[i]));
  }
  printf("\n");
#endif

  return (OZ_Thread) oz_newSuspendedThread();
}

OZ_Thread OZ_makeSuspendedThread(OZ_CFun fun,OZ_Term *args,int arity)
{
  OZ_Thread thr=OZ_newSuspendedThread();
  OZ_pushCFun(thr,fun,args,arity);
  return thr;
}

OZ_Thread OZ_newRunnableThread()
{
  return (OZ_Thread) oz_newRunnableThread();
}

void OZ_makeRunnableThread(OZ_CFun fun, OZ_Term *args,int arity)
{
  OZ_Thread thr = OZ_newRunnableThread();
  OZ_pushCFun(thr,fun,args,arity);
}

void OZ_unifyInThread(OZ_Term val1,OZ_Term val2)
{
  int ret = oz_unify(val1,val2);
  if (ret == PROCEED) return;
  switch (ret) {
  case SUSPEND:
    {
      OZ_Thread thr = OZ_newSuspendedThread();
      am.suspendOnVarList((Thread *)thr);
      break;
    }
  case BI_REPLACEBICALL:
    {
      OZ_Thread thr = OZ_newRunnableThread();
      am.pushPreparedCalls((Thread *) thr);
      break;
    }
  case FAILED:
    {
      OZ_Thread thr = OZ_newRunnableThread();
      OZ_pushCall(thr,BI_fail,0,0);
      break;
    }
  default:
    Assert(0);
  }
}

/* Suspensions */
void OZ_addThread(OZ_Term var, OZ_Thread thr)
{
  DEREF(var, varPtr, varTag);
  if (!isVariableTag(varTag)) {
    OZ_warning("OZ_addThread(%s): var arg expected",
	       toC(var));
    return;
  }

  addSuspAnyVar(varPtr, (Thread *) thr);
}

OZ_Return OZ_suspendOnInternal(OZ_Term var)
{
  oz_suspendOn(var);
}

OZ_Return OZ_suspendOnInternal2(OZ_Term var1,OZ_Term var2)
{
  oz_suspendOn2(var1,var2);
}

OZ_Return OZ_suspendOnInternal3(OZ_Term var1,OZ_Term var2,OZ_Term var3)
{
  oz_suspendOn3(var1,var2,var3);
}

/* -----------------------------------------------------------------
 * 
 * -----------------------------------------------------------------*/

OZ_Term OZ_newCell(OZ_Term val)
{
  return oz_newCell(val);
}

OZ_Term OZ_newChunk(OZ_Term val)
{
  val=oz_deref(val);
  if (!oz_isRecord(val)) return 0;
  return oz_newChunk(val);
}

int OZ_onToplevel()
{
  return am.onToplevel() ? 1 : 0;
}

/* -----------------------------------------------------------------
 * 
 * -----------------------------------------------------------------*/

char *OZ_unixError(int aErrno) {
#ifndef SUNOS_SPARC
  return strerror(aErrno);
#else
  extern char *sys_errlist[];
  return sys_errlist[aErrno];
#endif  
}

OZ_Return OZ_typeError(int pos,char *type)
{
  oz_typeError(pos,type);
}

void OZ_main(int argc,char **argv)
{
  am.init(argc,argv);
  scheduler();
  am.exitOz(0);
}

OZ_Term OZ_newPort(OZ_Term val) 
{
  return oz_newPort(val);
}

extern OZ_Return sendPort(OZ_Term prt, OZ_Term val);
void OZ_send(OZ_Term port, OZ_Term val)
{
  port = oz_deref(port);
  if (!oz_isPort(port)) return;

  (void) sendPort(port,val);
}

// mm2: this is not longer needed in Oz 3.0, but for compatibility with
// modules compiled for Oz 2.0 OZ_raiseA has to be defined
extern "C" OZ_Return OZ_raiseA(char*, int, int);
OZ_Return OZ_raiseA(char *name, int was, int shouldBe)
{
  return oz_raise(E_ERROR,E_SYSTEM,"inconsistentArity",3,
		  OZ_atom(name),OZ_int(was),OZ_int(shouldBe));
}

// heap chunks
inline
Bool oz_isHeapChunk(TaggedRef term)
{
  term = oz_deref(term);
  return oz_isConst(term)
    ? tagged2Const(term)->getType() == Co_HeapChunk
    : FALSE;
}

int OZ_isHeapChunk(OZ_Term t)
{
  return oz_isHeapChunk(oz_deref(t));
}

OZ_Term OZ_makeHeapChunk(int s)
{
  HeapChunk * hc = new HeapChunk(s);
  return makeTaggedConst(hc);
}

#define NotHeapChunkWarning(T, F, R)                                        \
if (! OZ_isHeapChunk(T)) {                                                  \
  OZ_warning("Heap chunk expected in %s. Got 0x%x. Result undetermined.\n", \
             #F, T);                                                        \
  return R;                                                                 \
}

int OZ_getHeapChunkSize(TaggedRef t)
{
  NotHeapChunkWarning(t, OZ_getHeapChunkSize, 0);
  
  return ((HeapChunk *) tagged2Const(oz_deref(t)))->getChunkSize();
}

void * OZ_getHeapChunkData(TaggedRef t)
{
  NotHeapChunkWarning(t, OZ_getHeapChunk, NULL);
  
  return ((HeapChunk *) tagged2Const(oz_deref(t)))->getChunkData();
}

