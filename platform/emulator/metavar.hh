#ifndef __metavar__hh__
#define __metavar__hh__


#if defined(__GNUC__)
#pragma interface
#endif

#include "genvar.hh"
#include "oz.h"

#if defined(OUTLINE)
#define inline
#endif

struct MetaTag {
friend class GenMetaVariable;

private:
  OZ_UnifyMetaDet     unify_meta_det;
  OZ_UnifyMetaMeta    unify_meta_meta;
  OZ_PrintMeta        print_meta_data;
  char *              name;
public:
  MetaTag(OZ_UnifyMetaDet unify_md,
          OZ_UnifyMetaMeta unify_mm,
          OZ_PrintMeta print,
          char * n)
  {
    unify_meta_det = unify_md;
    unify_meta_meta = unify_mm;
    print_meta_data = print;
    name = n;
  }
};

class GenMetaVariable : public GenCVariable {
friend class GenCVariable;

private:
  TaggedRef data;
  MetaTag * tag;
  Bool unifyMeta(TaggedRef *, TaggedRef, TaggedRef *, TaggedRef, Bool);

public:
  GenMetaVariable(MetaTag * t, TaggedRef tr);

  MetaTag * getTag(void) { return tag; }

  TaggedRef getData(void) { return data; }
  void setData(TaggedRef d) { data = d; }

  size_t getSize(void) { return sizeof(GenMetaVariable); }
  void gc(void);

  char * getName(void) { return tag->name; }

  char * toString(void) { return tag->print_meta_data(data); }

  Bool valid(TaggedRef v);

  void constrainVar(TaggedRef v, TaggedRef d) {
    setData(d);
    propagate(v, suspList, d, pc_propagator);
  }

  Bool isStrongerThan(TaggedRef data);
};


inline
Bool isHeapChunk(TaggedRef term);

inline
Bool isGenMetaVar(TaggedRef term);

inline
Bool isGenMetaVar(TaggedRef term, TypeOfTerm tag);

#if !defined(OUTLINE)
#include "metavar.icc"
#else
#undef inline
#endif


#endif
