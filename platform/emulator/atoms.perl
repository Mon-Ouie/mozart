#
# Generate declarations (file atoms.hh) and
# initialization (atoms.cc) for some often
# used atomes and names
#



%atoms = (
          ( AtomNil,       "nil"),
          ( AtomCons,      "|"),
          ( AtomPair,      "#"),
          ( AtomVoid,      "_"),

          ( AtomBool,      "bool"),
          ( AtomSup,       "sup"),
          ( AtomCompl,     "compl"),

          ( AtomEmpty,     ""),
          ( AtomUpper,     "upper"),
          ( AtomLower,     "lower"),
          ( AtomDigit,     "digit"),
          ( AtomCharSpace, "space"),
          ( AtomPunct,     "punct"),
          ( AtomOther,     "other"),

          ( AtomSucceeded,  "succeeded"),
          ( AtomAlt,        "alternatives"),
          ( AtomEntailed,   "entailed"),
          ( AtomSuspended,  "suspended"),
          ( AtomBlocked,    "blocked"),
          ( AtomMerged,     "merged"),
          ( AtomFailed,     "failed"),

          ( AtomDebugCallC, "call/c"),
          ( AtomDebugCallF, "call/f"),
          ( AtomDebugCondC, "conditional/c"),
          ( AtomDebugCondF, "conditional/f"),
          ( AtomDebugLockC, "lock/c"),
          ( AtomDebugLockF, "lock/f"),
          ( AtomDebugNameC, "name generation/c"),
          ( AtomDebugNameF, "name generation/f"),

          ( AtomUnify,      "unify"),
          ( AtomException,  "exception"),

          ( AtomExport,     "export"),
          ( AtomManager,    "manager"),
          ( AtomBoot,       "Boot"),

          ( AtomNew,        "new"),
          ( AtomApply,      "apply"),
          ( AtomApplyList,  "applyList"),

          ( AtomMin,        "min"),
          ( AtomMax,        "max"),
          ( AtomMid,        "mid"),
          ( AtomNaive,      "naive"),
          ( AtomSize,       "size"),
          ( AtomNbSusps,    "nbSusps"),

          ( AtomLow,        "low"),

          # For system set and get
          ( AtomActive,      "active"),
          ( AtomAtoms,       "atoms"),
          ( AtomBuiltins,    "builtins"),
          ( AtomCache,       "cache"),
          ( AtomCommitted,   "committed"),
          ( AtomCloned,      "cloned"),
          ( AtomCode,        "code"),
          ( AtomCopy,        "copy"),
          ( AtomCreated,     "created"),
          ( AtomDebug,       "debug"),
          ( AtomDepth,       "depth"),
          ( AtomFeed,        "feed"),
          ( AtomForeign,     "foreign"),
          ( AtomFree,        "free"),
          ( AtomFreelist,    "freelist"),
          ( AtomGC,          "gc"),
          ( AtomHigh,        "high"),
          ( AtomHints,       "hints"),
          ( AtomIdle,        "idle"),
          ( AtomInt,         "int"),
          ( AtomInvoked,     "invoked"),
          ( AtomLimits,      "limits"),
          ( AtomLoad,        "load"),
          ( AtomLocation,    "location"),
          ( AtomMedium,      "medium"),
          ( AtomNames,       "names"),
          ( AtomOn,          "on"),
          ( AtomPropagate,   "propagate"),
          ( AtomPropagators, "propagators"),
          ( AtomRun,         "run"),
          ( AtomRunnable,    "runnable"),
          ( AtomShowSuspension, "showSuspension"),
          ( AtomStopOnToplevelFailure, "stopOnToplevelFailure"),
          ( AtomSystem,      "system"),
          ( AtomThread,      "thread"),
          ( AtomThreshold,   "threshold"),
          ( AtomTolerance,   "tolerance"),
          ( AtomTotal,       "total"),
          ( AtomUser,        "user"),
          ( AtomVariables,   "variables"),
          ( AtomWidth,       "width"),
          ( AtomHeap,        "heap"),
          ( AtomDetailed,    "detailed"),
          ( AtomBrowser,     "browser"),
          ( AtomApplet,      "applet"),

          ( AtomKinded,      "kinded"),
          ( AtomDet,         "det"),
          ( AtomRecord,      "record"),
          ( AtomFSet,        "fset"),

          ( AtomDebugIP,     "debugIP"),
          ( AtomDebugPerdio, "debugPerdio"),


          # Handlers
          ( AtomTempBlocked,    "tempBlocked"),
          ( AtomPermBlocked,    "permBlocked"),
          ( AtomTempMe,         "tempMe"),
          ( AtomPermMe,         "permMe"),
          ( AtomTempAllOthers,  "tempAllOthers"),
          ( AtomPermAllOthers,  "permAllOthers"),
          ( AtomTempSomeOther,  "tempSomeOther"),
          ( AtomPermSomeOther,  "permSomeOther"),
          ( AtomEntityNormal,   "entityNormal"),
          ( AtomTemp,           "temp"),
          ( AtomTempHome,       "tempHome"),
          ( AtomTempForeign,    "tempForeign"),
          ( AtomPerm,           "perm"),
          ( AtomPermHome,       "permHome"),
          ( AtomPermForeign,    "permForeign"),
          ( AtomContinue,       "continue"),
          ( AtomRetry,          "retry"),
          ( AtomYes,            "yes"),
          ( AtomNo,             "no"),
          ( AtomPerSite,        "perSite"),
          ( AtomPerThread,      "perThread"),
          ( AtomHandler,         "handler"),
          ( AtomWatcher,        "watcher"),
          ( AtomAny,            "any"),
          ( AtomAll,            "all"),

          ( E_ERROR,            "error"),
          ( E_KERNEL,           "kernel"),
          ( E_OBJECT,           "object"),
          ( E_TK,               "tk"),
          ( E_OS,               "os"),
          ( E_SYSTEM,           "system")
          );


%names = (
          ( NameUnit,          "unit"),
          ( NameGroupVoid,     "group(void)"),
          ( NameNonExportable, "nonExportable"),

          ( NameOoAttr,        "ooAttr"),
          ( NameOoFreeFeatR,   "ooFreeFeatR"),
          ( NameOoFreeFlag,    "ooFreeFlag"),
          ( NameOoDefaultVar,  "ooDefaultVar"),
          ( NameOoRequiredArg, "ooRequiredArg"),
          ( NameOoFastMeth,    "ooFastMeth"),
          ( NameOoUnFreeFeat,  "ooUnFreeFeat"),
          ( NameOoDefaults,    "ooDefaults"),
          ( NameOoPrintName,   "ooPrintName"),
          ( NameOoLocking,     "ooLocking"),
          ( NameOoFallback,    "ooFallback"),
          ( NameOoId,          "ooId")
          );


$option = $ARGV[0];

if ("$option" eq "-body") {

    print "#include\"value.hh\"\n";

    print "TaggedRef\n";
    foreach $key (keys %atoms) { print "   $key,\n"; }
    foreach $key (keys %names) { print "   $key,\n"; }
    print "dummyend;\n";

    print "void initAtomsAndNames() {\n";

    while (($key,$val) = each %atoms) {
        printf("   %-20s = oz_atom(\"%s\");\n",$key,$val);
    }

    print "\n\n";

    while (($key,$val) = each %names) {
        printf("   %-20s = oz_uniqueName(\"%s\");\n",$key,$val);
    }

    print "}\n\n";

} elsif ("$option" eq "-header") {

    print "void initAtomsAndNames();\n\n\n";
    print "extern TaggedRef\n";
    foreach $key (keys %atoms) { print "   $key,\n"; }
    foreach $key (keys %names) { print "   $key,\n"; }
    print "dummyend;\n";

} else {

    die "usage: $ARGV[0] -body|-header\n";

}
