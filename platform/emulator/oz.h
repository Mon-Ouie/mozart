/*
  Hydra Project, DFKI Saarbruecken,
  Stuhlsatzenhausweg 3, D-66123 Saarbruecken, Phone (+49) 681 302-5312
  Author: scheidhr
  Last modified: $Date$ from $Author$
  Version: $Revision$
  State: $State$
  */

#ifndef __OZ_H__
#define __OZ_H__

/* ------------------------------------------------------------------------ *
 * 0. intro
 * ------------------------------------------------------------------------ */

/* calling convention "cdecl" under win32 */
#ifdef __WATCOMC__
#define ozcdecl __cdecl
#else
#ifdef __BORLANDC__
#define ozcdecl __export __cdecl
#else
#define ozcdecl
#endif
#endif


#ifdef __cplusplus
#define _FUNDECL(fun,arglist) ozcdecl fun arglist
extern "C" {
#else
#define _FUNDECL(fun,ignore) ozcdecl fun ()
#endif


/* Tell me whether this version of Oz supports dynamic linking */

#if defined(sun) || defined(linux) || defined(sgi)
#define OZDYNLINKING
#endif

/* ------------------------------------------------------------------------ *
 * I. type declarations
 * ------------------------------------------------------------------------ */

typedef unsigned int OZ_Term;

typedef enum {
  FAILED,
  PROCEED,
  SUSPEND,
  SLEEP,
  SCHEDULED,
  RAISE
} OZ_Return;

typedef void *OZ_Thread;
typedef void *OZ_Arity;

typedef OZ_Return _FUNDECL((*OZ_CFun),(int, OZ_Term *));


/* for tobias */
typedef int OZ_Boolean;
#define OZ_FALSE 0
#define OZ_TRUE 1


/* ------------------------------------------------------------------------ *
 * II. function prototypes
 * ------------------------------------------------------------------------ */

extern void     _FUNDECL(OZ_main, (int argc,char **argv));


extern OZ_Term  _FUNDECL(OZ_deref,(OZ_Term term));

/* tests */
extern int _FUNDECL(OZ_isAtom,(OZ_Term));
extern int _FUNDECL(OZ_isBigInt,(OZ_Term));
extern int _FUNDECL(OZ_isCell,(OZ_Term));
extern int _FUNDECL(OZ_isChunk,(OZ_Term));
extern int _FUNDECL(OZ_isCons,(OZ_Term));
extern int _FUNDECL(OZ_isFalse,(OZ_Term));
extern int _FUNDECL(OZ_isFeature,(OZ_Term));
extern int _FUNDECL(OZ_isFloat,(OZ_Term));
extern int _FUNDECL(OZ_isInt,(OZ_Term));
extern int _FUNDECL(OZ_isLiteral,(OZ_Term));
extern int _FUNDECL(OZ_isName,(OZ_Term));
extern int _FUNDECL(OZ_isNil,(OZ_Term));
extern int _FUNDECL(OZ_isObject,(OZ_Term));
extern int _FUNDECL(OZ_isPair,(OZ_Term));
extern int _FUNDECL(OZ_isPair2,(OZ_Term));
extern int _FUNDECL(OZ_isProcedure,(OZ_Term));
extern int _FUNDECL(OZ_isRecord,(OZ_Term));
extern int _FUNDECL(OZ_isSmallInt,(OZ_Term));
extern int _FUNDECL(OZ_isTrue,(OZ_Term));
extern int _FUNDECL(OZ_isTuple,(OZ_Term));
extern int _FUNDECL(OZ_isValue,(OZ_Term));
extern int _FUNDECL(OZ_isVariable,(OZ_Term));

extern int _FUNDECL(OZ_isList,(OZ_Term, OZ_Term *));
extern int _FUNDECL(OZ_isString,(OZ_Term, OZ_Term *));
extern int _FUNDECL(OZ_isVirtualString,(OZ_Term, OZ_Term *));

#define OZ_assertList(t)                        \
  {                                             \
    OZ_Term var;                                \
    if (!OZ_isList(t,&var)) {                   \
      if (var == 0) return FAILED;              \
      OZ_suspendOn(var);                        \
    }                                           \
  }

#define OZ_assertString(t)                      \
  {                                             \
    OZ_Term var;                                \
    if (!OZ_isString(t,&var)) {                 \
      if (var == 0) return FAILED;              \
      OZ_suspendOn(var);                        \
    }                                           \
  }
#define OZ_assertVirtualString(t)               \
  {                                             \
    OZ_Term var;                                \
    if (!OZ_isVirtualString(t,&var)) {          \
      if (var == 0) return FAILED;              \
      OZ_suspendOn(var);                        \
    }                                           \
  }

/*
 * mm2: should we support this ?
 */
extern OZ_Term _FUNDECL(OZ_termType,(OZ_Term));

/* convert: C from/to Oz datastructure */

extern char*   _FUNDECL(OZ_atomToC,(OZ_Term));
extern OZ_Term _FUNDECL(OZ_atom,(char *));
extern int     _FUNDECL(OZ_featureCmp,(OZ_Term,OZ_Term));

extern int     _FUNDECL(OZ_smallIntMin,(void));
extern int     _FUNDECL(OZ_smallIntMax,(void));
extern OZ_Term _FUNDECL(OZ_false,(void));
extern OZ_Term _FUNDECL(OZ_true,(void));
extern OZ_Term _FUNDECL(OZ_int,(int));
extern int     _FUNDECL(OZ_getMinPrio,(void));
extern int     _FUNDECL(OZ_getDefaultPrio,(void));
extern int     _FUNDECL(OZ_getPropagatorPrio,(void));
extern int     _FUNDECL(OZ_getMaxPrio,(void));
extern int     _FUNDECL(OZ_intToC,(OZ_Term));
extern OZ_Term _FUNDECL(OZ_CStringToInt,(char *str));
extern char *  _FUNDECL(OZ_parseInt,(char *s));

extern OZ_Term _FUNDECL(OZ_float,(double));
extern double  _FUNDECL(OZ_floatToC,(OZ_Term));

extern OZ_Term _FUNDECL(OZ_CStringToFloat,(char *s));
extern char *  _FUNDECL(OZ_parseFloat,(char *s));

extern OZ_Term _FUNDECL(OZ_CStringToNumber,(char *));

extern char *  _FUNDECL(OZ_toC,(OZ_Term, int, int));
extern int     _FUNDECL(OZ_termGetSize,(OZ_Term, int, int));

extern OZ_Term _FUNDECL(OZ_string,(char *));
extern char *  _FUNDECL(OZ_stringToC,(OZ_Term t));

extern void    _FUNDECL(OZ_printVirtualString,(OZ_Term t));
#define OZ_printVS(t) OZ_printVirtualString(t)
extern char *  _FUNDECL(OZ_virtualStringToC,(OZ_Term t));


/* tuples */
extern OZ_Term  _FUNDECL(OZ_label,(OZ_Term));
extern int      _FUNDECL(OZ_width,(OZ_Term));
extern OZ_Term _FUNDECL(OZ_tuple,(OZ_Term, int));
#define OZ_tupleC(s,n) OZ_tuple(OZ_atom(s),n)
extern OZ_Term  _FUNDECL(OZ_mkTuple,(OZ_Term label,int arity,...));
extern OZ_Term  _FUNDECL(OZ_mkTupleC,(char *label,int arity,...));

extern void     _FUNDECL(OZ_putArg,(OZ_Term, int, OZ_Term));
extern OZ_Term  _FUNDECL(OZ_getArg,(OZ_Term, int));
extern OZ_Term  _FUNDECL(OZ_nil,());
extern OZ_Term  _FUNDECL(OZ_cons,(OZ_Term ,OZ_Term));
extern OZ_Term  _FUNDECL(OZ_head,(OZ_Term));
extern OZ_Term  _FUNDECL(OZ_tail,(OZ_Term));
extern int      _FUNDECL(OZ_length,(OZ_Term list));
extern OZ_Term  _FUNDECL(OZ_toList,(int, OZ_Term *));


extern OZ_Term  _FUNDECL(OZ_pair,(int));
extern OZ_Term  _FUNDECL(OZ_pair2,(OZ_Term t1,OZ_Term t2));

#define OZ_pairA(s1,t)      OZ_pair2(OZ_atom(s1),t)
#define OZ_pairAI(s1,i)     OZ_pair2(OZ_atom(s1),OZ_int(i))
#define OZ_pairAA(s1,s2)    OZ_pair2(OZ_atom(s1),OZ_atom(s2))
#define OZ_pairAS(s1,s2)    OZ_pair2(OZ_atom(s1),OZ_string(s2))


/* records */
extern OZ_Arity _FUNDECL(OZ_makeArity,(OZ_Term list));
extern OZ_Term  _FUNDECL(OZ_record,(OZ_Term, OZ_Term));
extern OZ_Term  _FUNDECL(OZ_recordInit,(OZ_Term, OZ_Term));
extern void     _FUNDECL(OZ_putSubtree,(OZ_Term, OZ_Term, OZ_Term));
extern OZ_Term  _FUNDECL(OZ_subtree,(OZ_Term, OZ_Term));
extern OZ_Term  _FUNDECL(OZ_arityList,(OZ_Term));

/* unification */
extern OZ_Return _FUNDECL(OZ_unify,(OZ_Term, OZ_Term));
extern int       _FUNDECL(OZ_eq,(OZ_Term, OZ_Term));

#define OZ_unifyFloat(t1,f)      OZ_unify(t1, OZ_float(f))
#define OZ_unifyInt(t1,i)        OZ_unify(t1, OZ_int(i))
#define OZ_unifyAtom(t1,s)       OZ_unify(t1, OZ_atom(s))

/* create a new oz variable */
extern OZ_Term OZ_newVariable();

extern OZ_Term _FUNDECL(OZ_newChunk,(OZ_Term));

/* cell */
extern OZ_Term _FUNDECL(OZ_newCell,(OZ_Term));
/* exchangeCell, deepFeed */

/* name */
extern OZ_Term OZ_newName ();

/* print warning */
extern void _FUNDECL(OZ_warning,(char * ...));

/* generate the unix error string from an errno (see perror(3)) */
char * _FUNDECL(OZ_unixError,(int err));

/* check for toplevel */
extern int OZ_onToplevel ();

extern int _FUNDECL(OZ_addBuiltin,(char *, int, OZ_CFun));

/* replace new builtins */
struct OZ_BIspec {
  char *name;
  int arity;
  OZ_CFun fun;
};

/* add specification to builtin table */
void _FUNDECL(OZ_addBISpec,(OZ_BIspec *spec));

/* IO */

extern OZ_Return _FUNDECL(OZ_readSelect,(int, OZ_Term, OZ_Term));
extern OZ_Return _FUNDECL(OZ_writeSelect,(int, OZ_Term, OZ_Term));
extern OZ_Return _FUNDECL(OZ_acceptSelect,(int, OZ_Term, OZ_Term));
extern void      _FUNDECL(OZ_deSelect,(int));

/* garbage collection */
extern int _FUNDECL(OZ_protect,(OZ_Term *));
extern int _FUNDECL(OZ_unprotect,(OZ_Term *));

/* raise exception */
extern OZ_Return _FUNDECL(OZ_typeError,(int pos,char *type));
extern OZ_Return _FUNDECL(OZ_raise,(OZ_Term));
extern OZ_Return _FUNDECL(OZ_raiseC,(char *label,int arity,...));

/* Suspending builtins */

void      _FUNDECL(OZ_makeRunableThread,(OZ_CFun, OZ_Term *, int));
OZ_Thread _FUNDECL(OZ_makeSuspendedThread,(OZ_CFun, OZ_Term *, int));
void      _FUNDECL(OZ_addThread,(OZ_Term, OZ_Thread));

#define OZ_makeSelfSuspendedThread() \
  OZ_makeSuspendedThread(OZ_self, OZ_args,OZ_arity)

/* for example
   OZ_Thread s = OZ_makeSuspendedThread(BIplus,OZ_args,OZ_arity);
   OZ_addThread(t1,s);
   OZ_addThread(t2,s);
   */

void _FUNDECL(OZ_suspendOnInternal,(OZ_Term));
void _FUNDECL(OZ_suspendOnInternal2,(OZ_Term,OZ_Term));
void _FUNDECL(OZ_suspendOnInternal3,(OZ_Term,OZ_Term,OZ_Term));

#define OZ_suspendOn(t1) \
   { OZ_suspendOnInternal(t1); return SUSPEND; }
#define OZ_suspendOn2(t1,t2) \
   { OZ_suspendOnInternal2(t1,t2); return SUSPEND; }
#define OZ_suspendOn3(t1,t2,t3) \
   { OZ_suspendOnInternal3(t1,t2,t3); return SUSPEND; }

/* ------------------------------------------------------------------------ *
 * III. macros
 * ------------------------------------------------------------------------ */

/* variable arity is marked as follows: */
#define VarArity -1

#if defined(__GNUC__) || defined(__cplusplus)
#define OZStringify(Name) #Name
#else
#define OZStringify(Name) "Name"
#endif


#ifdef __cplusplus

#define OZ_C_proc_proto(Name)                                                 \
    extern "C" OZ_Return ozcdecl Name(int OZ_arity, OZ_Term OZ_args[]);

#define OZ_C_proc_header(Name)                                                \
    OZ_Return ozcdecl Name(int OZ_arity, OZ_Term OZ_args[]) {

#else

#define OZ_C_proc_proto(Name)                                                 \
  OZ_Return ozcdecl Name(OZ_arity, OZ_args)

#define OZ_C_proc_header(Name)                                                \
  int OZ_arity; OZ_Term OZ_args[]; {

#endif

#define OZ_C_proc_begin(Name,Arity)                     \
    OZ_C_proc_proto(Name)                               \
    OZ_C_proc_header(Name)                              \
       OZ_CFun OZ_self = Name;                          \
       if (OZ_arity != Arity && Arity != VarArity) {    \
         return OZ_raiseC("arity",2,OZStringify(Name),  \
                          OZ_toList(OZ_arity,OZ_args)); \
         return FAILED;                                 \
       }

#define OZ_C_proc_end }


#define OZ_C_ioproc_begin(Name,Arity)                   \
OZ_C_proc_begin(Name,Arity)                             \
  if (!OZ_onToplevel()) {                               \
    return OZ_raiseC("globalState",1,OZ_atom("io"));    \
  }

#define OZ_C_ioproc_end }



/* access arguments */
#define OZ_getCArg(N) OZ_args[N]

/* useful macros and functions (mm 9.2.93) */

#define OZ_declareArg(ARG,VAR) \
     OZ_Term VAR = OZ_getCArg(ARG);

#define OZ_nonvarArg(ARG)                       \
{                                               \
  if (OZ_isVariable(OZ_getCArg(ARG))) {         \
    OZ_suspendOn(OZ_getCArg(ARG));              \
  }                                             \
}

#define OZ_declareNonvarArg(ARG,VAR)            \
OZ_Term VAR = OZ_getCArg(ARG);                  \
{                                               \
  if (OZ_isVariable(VAR)) {                     \
    OZ_suspendOn(VAR);                          \
  }                                             \
}

#define OZ_declareIntArg(ARG,VAR)               \
 int VAR;                                       \
 OZ_nonvarArg(ARG);                             \
 if (! OZ_isInt(OZ_getCArg(ARG))) {             \
   return OZ_typeError(ARG,"Int");              \
 } else {                                       \
   VAR = OZ_intToC(OZ_getCArg(ARG));            \
 }

#define OZ_declareFloatArg(ARG,VAR)             \
 double VAR;                                    \
 OZ_nonvarArg(ARG);                             \
 if (! OZ_isFloat(OZ_getCArg(ARG))) {           \
   return OZ_typeError(ARG,"Float");            \
   return FAILED;                               \
 } else {                                       \
   VAR = OZ_floatToC(OZ_getCArg(ARG));          \
 }


#define OZ_declareAtomArg(ARG,VAR)              \
 char *VAR;                                     \
 OZ_nonvarArg(ARG);                             \
 if (! OZ_isAtom(OZ_getCArg(ARG))) {            \
   return OZ_typeError(ARG,"Atom");             \
 } else {                                       \
   VAR = OZ_atomToC(OZ_getCArg(ARG));           \
 }

#define OZ_declareStringArg(ARG,VAR)                    \
 char *VAR;                                             \
 {                                                      \
   OZ_Term OZ_avar;                                     \
   if (!OZ_isString(OZ_getCArg(ARG),&OZ_avar)) {        \
     if (OZ_avar == 0) {                                \
       return OZ_typeError(ARG,"String");               \
     } else {                                           \
       OZ_suspendOn(OZ_avar);                           \
     }                                                  \
   }                                                    \
   VAR = OZ_stringToC(OZ_getCArg(ARG));                 \
 }

#define OZ_declareVirtualStringArg(ARG,VAR)             \
 char *VAR;                                             \
 {                                                      \
   OZ_Term OZ_avar;                                     \
   if (!OZ_isVirtualString(OZ_getCArg(ARG),&OZ_avar)) { \
     if (OZ_avar == 0) {                                \
       return OZ_typeError(ARG,"VirtualString");        \
     } else {                                           \
       OZ_suspendOn(OZ_avar);                           \
     }                                                  \
   }                                                    \
   VAR = OZ_virtualStringToC(OZ_getCArg(ARG));          \
 }

/* ------------------------------------------------------------------------ *
 * end
 * ------------------------------------------------------------------------ */

#ifdef __cplusplus
}
#endif

/* ------------------------------------------------------------------------ *
 * oz_meta.h
 * ------------------------------------------------------------------------ */

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
  meta_unconstr     = 0,
  meta_det          = 1,
  meta_left_constr  = 2,
  meta_right_constr = 4,
  meta_fail         = 8
} mur_t;

typedef enum {
  OZ_Type_Cell,
  OZ_Type_Chunk,
  OZ_Type_Cons,
  OZ_Type_HeapChunk,
  OZ_Type_CVar,
  OZ_Type_Float,
  OZ_Type_Int,
  OZ_Type_Literal,
  OZ_Type_Procedure,
  OZ_Type_Record,
  OZ_Type_Tuple,
  OZ_Type_Var,
  OZ_Type_Unknown
} OZ_TermType;


typedef void * OZ_MetaType;

typedef mur_t _FUNDECL((* OZ_UnifyMetaDet), (OZ_Term, OZ_Term, OZ_Term, OZ_TermType, OZ_Term *));
typedef mur_t _FUNDECL((* OZ_UnifyMetaMeta), (OZ_Term, OZ_Term, OZ_Term, OZ_Term, OZ_MetaType, OZ_Term *));

typedef char * _FUNDECL((* OZ_PrintMeta), (OZ_Term, int));
typedef int    _FUNDECL((* OZ_IsSingleValue), (OZ_Term));


extern OZ_TermType _FUNDECL(OZ_typeOf,(OZ_Term t));

extern OZ_MetaType _FUNDECL(OZ_introMetaTerm,(OZ_UnifyMetaDet unify_md,
                                                OZ_UnifyMetaMeta unify_mm,
                                                OZ_PrintMeta print,
                                                OZ_IsSingleValue sgl_val,
                                                char * name));

extern OZ_Term _FUNDECL(OZ_makeMetaTerm,(OZ_MetaType t,
                                                OZ_Term d));

extern OZ_MetaType _FUNDECL(OZ_getMetaTermType,(OZ_Term v));
extern void        _FUNDECL(OZ_putMetaTermType,(OZ_Term v, OZ_MetaType t));

extern OZ_Term _FUNDECL(OZ_getMetaTermAttr,(OZ_Term v));

extern OZ_Term   _FUNDECL(OZ_makeHeapChunk,(int s));
extern char *    _FUNDECL(OZ_getHeapChunkData,(OZ_Term t));
extern int       _FUNDECL(OZ_getHeapChunkSize,(OZ_Term t));
extern int       _FUNDECL(OZ_isHeapChunk,(OZ_Term t));
extern int       _FUNDECL(OZ_isMetaTerm,(OZ_Term t));
extern int       _FUNDECL(OZ_isSingleValue,(OZ_Term t));
extern OZ_Return _FUNDECL(OZ_constrainMetaTerm,(OZ_Term v,
                                                 OZ_MetaType t,
                                                 OZ_Term d));

extern int _FUNDECL(OZ_areIdentVars,(OZ_Term v1, OZ_Term v2));

extern OZ_Return _FUNDECL(OZ_suspendMetaProp,(OZ_CFun, OZ_Term *, int));

#define OZ_MetaPropSuspend OZ_suspendMetaProp(OZ_self, OZ_args, OZ_arity)

#ifdef __cplusplus
}
#endif

#endif /* __OZ_H__ */
