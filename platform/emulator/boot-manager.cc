/*
 *  Authors:
 *    Denys Duchier (duchier@ps.uni-sb.de)
 *    Christian Schulte <schulte@ps.uni-sb.de>
 * 
 *  Copyright:
 *    Christian Schulte, 1998
 * 
 *  Last change:
 *    $Date$ by $Author$
 *    $Revision$
 * 
 *  This file is part of Mozart, an implementation 
 *  of Oz 3:
 *     http://www.mozart-oz.org
 * 
 *  See the file "LICENSE" or
 *     http://www.mozart-oz.org/LICENSE.html
 *  for information on usage and redistribution 
 *  of this file, and for a DISCLAIMER OF ALL 
 *  WARRANTIES.
 *
 */


#include "conf.h"

#include <string.h>

#include "builtins.hh"
#include "os.hh"
#include "am.hh"
#include <sys/stat.h>
#include <errno.h>

// module name -> module (used also for umarshalling)

#include "dictionary.hh"
static TaggedRef dictionary_of_modules;


/*
 * Modules that are always in the emulator: Extensions
 */

#include "modINTERNAL-if.cc"
#include "modProperty-if.cc"
#include "modOS-if.cc"
#include "modPickle-if.cc"
#include "modURL-if.cc"
#include "modApplication-if.cc"
#include "modFinalize-if.cc"
#include "modDistribution-if.cc"
#include "modWeakDictionary-if.cc"
#ifdef DENYS_EVENTS
#include "modEvent-if.cc"
#include "modTimer-if.cc"
#endif

/*
 * Modules that are always in the emulator: Base
 */

#include "modArray-if.cc"
#include "modAtom-if.cc"
#include "modBitArray-if.cc"
#include "modBool-if.cc"
#include "modCell-if.cc"
#include "modChar-if.cc"
#include "modChunk-if.cc"
#include "modClass-if.cc"
#include "modDictionary-if.cc"
#include "modException-if.cc"
#include "modFloat-if.cc"
#include "modForeignPointer-if.cc"
#include "modInt-if.cc"
#include "modList-if.cc"
#include "modLiteral-if.cc"
#include "modLock-if.cc"
#include "modName-if.cc"
#include "modNumber-if.cc"
#include "modObject-if.cc"
#include "modPort-if.cc"
#include "modProcedure-if.cc"
#include "modRecord-if.cc"
#include "modString-if.cc"
#include "modThread-if.cc"
#include "modTime-if.cc"
#include "modTuple-if.cc"
#include "modUnit-if.cc"
#include "modValue-if.cc"
#include "modVirtualString-if.cc"
#include "modBitString-if.cc"
#include "modByteString-if.cc"
#include "modInterFault-if.cc"

/*
 * Modules that are possibly dynamically loaded
 */

#ifdef MODULES_LINK_STATIC

#include "modTk-if.cc"
#include "modSpace-if.cc"
#include "modFDB-if.cc"
#include "modFDP-if.cc"
#include "modCTB-if.cc"
#include "modRecordC-if.cc"
#include "modCompat-if.cc"
#include "modSchedule-if.cc"
#include "modParser-if.cc"
#include "modFSB-if.cc"
#include "modFSP-if.cc"
#include "modCompilerSupport-if.cc"
#include "modBrowser-if.cc"
#include "modDebug-if.cc"
#include "modPID-if.cc"
#include "modFault-if.cc"
#include "modDPMisc-if.cc"
#include "modVirtualSite-if.cc"
#include "modSystem-if.cc"
#include "modProfile-if.cc"

// Here comes the faked DPB interface
extern "C"
{
  OZ_C_proc_interface * mod_int_DPB(void)
  {
    static OZ_C_proc_interface i_table[] = {
      {0,0,0,0}
    };

    return i_table;
  } /* mod_int_DPB(void) */
} /* extern "C" */

#endif

/*
 * Builtins that depend on particular configuration
 */

#ifdef MISC_BUILTINS
#include "modMisc-if.cc"
#endif

#ifdef CS_PROFILE
OZ_BI_proto(BIgetCloneDiff);

extern "C" 
{
  OZ_C_proc_interface * mod_int_CloneDiff(void) {
    static OZ_C_proc_interface i_table[] = {
      {"get",        1,      1,BIgetCloneDiff},
      {0,0,0,0}
    };
    
    return i_table;
  }
}
#endif




typedef OZ_C_proc_interface * (* init_fun_t) (void);

struct ModuleEntry {
  const char * name;
  init_fun_t   init_function;
};

/*
 * Module table for extensions
 */
static ModuleEntry ext_module_table[] = {
  // Most important stuff
  {"Property",        mod_int_Property},
  {"OS",              mod_int_OS},
  {"URL",             mod_int_URL},
  {"Pickle",          mod_int_Pickle},
  {"Application",     mod_int_Application},
  {"Finalize",        mod_int_Finalize},
  {"Distribution",    mod_int_Distribution},
  {"WeakDictionary",  mod_int_WeakDictionary},
  {"INTERNAL",        mod_int_INTERNAL},
#ifdef DENYS_EVENTS
  {"Event",	      mod_int_Event},
  {"Timer",	      mod_int_Timer},
#endif

#ifdef MODULES_LINK_STATIC
  {"Space",	      mod_int_Space},
  {"FDB",             mod_int_FDB},
  {"FSP",             mod_int_FSP},
  {"FSB",             mod_int_FSB},
  {"FDP",             mod_int_FDP},
  {"CTB",             mod_int_CTB},
  {"RecordC",         mod_int_RecordC},
  {"CompilerSupport", mod_int_CompilerSupport},
  {"Parser",          mod_int_Parser},
  {"Browser",         mod_int_Browser},
  {"Tk",              mod_int_Tk},
  {"Schedule",        mod_int_Schedule},
  {"Debug",           mod_int_Debug},
  {"DPB",             mod_int_DPB},
  {"PID",             mod_int_PID},
  {"Fault",           mod_int_Fault},
  {"DPMisc",          mod_int_DPMisc},
  {"VirtualSite",     mod_int_VirtualSite},
  {"Compat",          mod_int_Compat},
  {"System",          mod_int_System},
  {"Profile",         mod_int_Profile},
#endif

#ifdef MISC_BUILTINS
  {"Misc",         mod_int_Misc},
#endif

#ifdef CS_PROFILE
  {"CloneDiff", mod_int_CloneDiff},
#endif

  {0, 0},
};


/*
 * Module table for base
 */
static ModuleEntry base_module_table[] = {
  {"Array",		mod_int_Array},
  {"Atom",		mod_int_Atom},
  {"BitArray",		mod_int_BitArray},
  {"Bool",		mod_int_Bool},
  {"Cell",		mod_int_Cell},
  {"Char",		mod_int_Char},
  {"Chunk",		mod_int_Chunk},
  {"Class",		mod_int_Class},
  {"Dictionary",	mod_int_Dictionary},
  {"Exception",		mod_int_Exception},
  {"Float",		mod_int_Float},
  {"ForeignPointer",	mod_int_ForeignPointer},
  {"Int",		mod_int_Int},
  {"List",		mod_int_List},
  {"Literal",		mod_int_Literal},
  {"Lock",		mod_int_Lock},
  {"Name",		mod_int_Name},
  {"Number",		mod_int_Number},
  {"Object",		mod_int_Object},
  {"Port",		mod_int_Port},
  {"Procedure",		mod_int_Procedure},
  {"Record",		mod_int_Record},
  {"String",		mod_int_String},
  {"Thread",		mod_int_Thread},
  {"Time",		mod_int_Time},
  {"Tuple",		mod_int_Tuple},
  {"Unit",		mod_int_Unit},
  {"Value",		mod_int_Value},
  {"VirtualString",	mod_int_VirtualString},
  {"BitString",		mod_int_BitString},
  {"ByteString",	mod_int_ByteString},
  {"InterFault",	mod_int_InterFault},
  {0,0}
};


static 
TaggedRef ozInterfaceToRecord(OZ_C_proc_interface * I, 
			      const char * mod_name,
			      Bool isSited) {
  OZ_Term l = oz_nil();

  while (I && I->name) {
    Builtin * bi = new Builtin(mod_name,I->name,
			       I->inArity,I->outArity,I->func,isSited);
 
    l = oz_cons(oz_pair2(oz_atomNoDup(I->name),makeTaggedConst(bi)),l);
    I++;
  }

  return OZ_recordInit(AtomExport,l);
}


Builtin * cfunc2Builtin(void * f) {
  
  OzDictionary * d = tagged2Dictionary(dictionary_of_modules);

  for (int i = d->getNext(d->getFirst()); i>=0; i=d->getNext(i)) {

    TaggedRef v = d->getValue(i);
      
    if (oz_isSRecord(v)) {
	
      TaggedRef as = tagged2SRecord(v)->getArityList();
	
      while (oz_isCons(as)) {
	TaggedRef bt = tagged2SRecord(v)->getFeature(oz_head(as));
	  
	if (bt && oz_isBuiltin(bt) && 
	    (tagged2Builtin(bt)->getFun() == (OZ_CFun) f)) 
	  return tagged2Builtin(bt);
	  
	as = oz_tail(as);
	
      }
	
    }
  }

  return tagged2Builtin(BI_unknown);
}

inline
void link_module(ModuleEntry * E, Bool isSited) {
  tagged2Dictionary(dictionary_of_modules)
    ->setArg(oz_atomNoDup(E->name), 
	     ozInterfaceToRecord((E->init_function)(),
				 E->name,
				 isSited));
}

inline
ModuleEntry * find_module(ModuleEntry * mt, const char * mn) {
  for (ModuleEntry * E = mt; (E && E->name); E++)
    if (!strcmp(E->name,mn))
      return E;
  return NULL;
}

inline
void link_modules(ModuleEntry * mt, Bool isSited) {

  for (ModuleEntry * E = mt; (E && E->name); E++)
    link_module(E,isSited);
  
}



TaggedRef string2Builtin(const char * mn, const char * bn) {
  OzDictionary * d = tagged2Dictionary(dictionary_of_modules);

  TaggedRef mod; 
  
  TaggedRef mn_a = oz_atom(mn);

 retry:

  if (d->getArg(mn_a, mod) != PROCEED) {
    ModuleEntry * E = find_module(base_module_table, mn);
    if (!E) {
      OZ_warning("[BUILTIN NOT FOUND: Unknown module %s]\n", mn);
      return BI_unknown;
    }
    link_module(E,NO);
    goto retry;
  }

  mod = oz_deref(mod);
  
  Assert(oz_isSRecord(mod));

  TaggedRef bi = tagged2SRecord(mod)->getFeature(oz_atom(bn));
	  
  if (!bi || !oz_isBuiltin(bi)) { 
    OZ_warning("[BUILTIN NOT FOUND: Unknown builtin %s in module %s]\n", 
	       bn, mn);
    return BI_unknown;
  }

  return bi;
}


#define S2B_BUF_LEN 128

static char _s2b_buf[S2B_BUF_LEN + 16];

TaggedRef string2Builtin(const char * cs) {
  // NEVER USE IT FOR SOMETHING DIFFERENT THAN UNSITED BUILTINS!!!!!
  int sl = strlen(cs);

  char * s = (sl > S2B_BUF_LEN) ? new char[sl] : _s2b_buf;

  memcpy((void *) s, (const void *) cs, sl+1);
  
  char * mn = s;
  char * bn = s;

  // find seperating '.'
  while ((*bn != '\0') && (*bn != '.')) bn++;

  if (*bn == '\0') {
    OZ_warning("[BUILTIN NOT FOUND: Confused spec %s]\n", cs);
    return BI_unknown;
  }

  *bn++ = '\0';

  if (*bn == '\'') {
    bn++;
    *(s+sl-1) = '\0';
  }

  TaggedRef bi = string2Builtin(mn,bn);

  if (sl > S2B_BUF_LEN) 
    delete s;
  
  return bi;

}

#undef S2B_BUF_LEN

Builtin * string2CBuiltin(const char * Name) {
  return tagged2Builtin(string2Builtin(Name));
}


#ifdef DLOPEN_UNDERSCORE
#define USC "_"
#else
#define USC ""
#endif

OZ_BI_define(BIObtainNative, 2, 1) {
  oz_declareIN(0, is_boot_tagged);
  oz_declareVirtualStringIN(1, name);

  Bool is_boot = oz_isTrue(oz_deref(is_boot_tagged));
  
  init_fun_t init_function = 0;
  char * if_identifier;
  char * filename;
  char * mod_name = (char *) 0;

  if (is_boot) {
    // Might be something linked in statically, so try to find it in table
    
    TaggedRef module;

    if (tagged2Dictionary(dictionary_of_modules)
	->getArg(oz_atom(name), module) == PROCEED)
      OZ_RETURN(module);

    // Okay, later we will need a filename!
    int n = strlen(ozconf.emuhome);
    int m = strlen(name);
    
    mod_name = name;

    filename = new char[n + m + 64];
    
    strcpy(filename, ozconf.emuhome);
    strcat(filename, "/");
    strcat(filename, name);
    strcat(filename, ".so");
    
    // We have to set the interface identifier
    if_identifier = new char[m + 16];
    
    strcpy(if_identifier, USC "mod_int_");
    strcat(if_identifier, name);
  } else {
    // Here the identifier is always the same
    if_identifier = USC "oz_init_module";
    filename      = name;
  }
  

  void *handle;
  TaggedRef res = osDlopen(filename,&handle);

  if (res) {
    struct stat buf;
    TaggedRef file_atom = oz_atom(filename);
    if (is_boot) 
      free(if_identifier);
  retry:
    if (stat(filename,&buf)<0)
      if (errno==EINTR) {
	goto retry;
      } else {
	// file does not exist (or would need searching LD_LIBRARY_PATH
	// which we don't attempt here - too bad)
	if (is_boot) free(filename);
	return oz_raise(E_SYSTEM,AtomForeign,"dlOpen",1,file_atom);
      }
    // file presumed to exist
    if (is_boot) 
      free(filename);
    return oz_raise(E_ERROR,AtomForeign,"dlOpen",2,
		    file_atom,res);
  }
  
  init_function = (init_fun_t) osDlsym(handle,if_identifier);
  
  // oops, there is no `init_function()'
  if (init_function == 0) {
    return oz_raise(E_ERROR,AtomForeign, "cannotFindOzInitModule", 1,
		    OZ_in(1));
  } 

  if (!mod_name) {
    char * name_sym = USC "oz_module_name";
    mod_name = (char *)  osDlsym(handle,name_sym);
  }

  if (mod_name)
    mod_name = strdup(mod_name);

  OZ_RETURN(ozInterfaceToRecord((*init_function)(), mod_name, OK));

} OZ_BI_end



void initBuiltins() {

  //
  // create dictionaries for builtin modules
  //
  dictionary_of_modules =
    makeTaggedConst(new OzDictionary(oz_rootBoard()));
  OZ_protect(&dictionary_of_modules);

  //
  // populate it
  //
  link_modules(ext_module_table,  OK);

  // General stuff
  BI_wait         = string2Builtin("Value","wait");
  BI_send         = string2Builtin("Port","send");
  BI_exchangeCell = string2Builtin("Cell","exchangeFun");
  BI_assign       = string2Builtin("Object","<-");
  BI_Unify        = string2Builtin("Value","=");

  // MISC INTERNAL STUFF
  BI_controlVarHandler = string2Builtin("INTERNAL", "controlVarHandler"); 
  BI_atRedo            = string2Builtin("INTERNAL", "atRedo");
  BI_fail              = string2Builtin("INTERNAL", "fail");
  BI_skip              = string2Builtin("INTERNAL", "skip");
  BI_unknown           = string2Builtin("INTERNAL", "UNKNOWN");
  BI_PROP_LPQ          = string2Builtin("INTERNAL", "propagate");  
  BI_bindFuture        = string2Builtin("INTERNAL", "bindFuture");
  BI_waitStatus        = string2Builtin("INTERNAL", "waitStatus");
  BI_varToFuture       = string2Builtin("INTERNAL", "varToFuture");

  // to execute boot functor in am.cc
  BI_dot           = string2Builtin("Value", ".");
  BI_load          = string2Builtin("INTERNAL", "load"); 
  BI_url_load      = string2Builtin("URL", "load"); 
  BI_obtain_native = string2Builtin("INTERNAL", "native");

  // Exception stuff
  bi_raise      = string2CBuiltin("Exception.raise");
  bi_raiseError = string2CBuiltin("Exception.raiseError");
  BI_raise      = string2Builtin("Exception.raise");
}










