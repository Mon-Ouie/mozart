#ifndef __GENOFSVAR__H__
#define __GENOFSVAR__H__

#if defined(__GNUC__)
#pragma interface
#endif

#include "am.hh"
#include "genvar.hh"
#include "tagged.hh"
#include "term.hh"
#include "records.hh"
#include "mem.hh"
#include "misc.hh"

// TODO
// 1. Check that all TaggedRef's are dereferenced when they should be.

//-------------------------------------------------------------------------
//                           class PairList
//-------------------------------------------------------------------------

// A simple class implementing list of pairs of terms,
// used as a utility by DynamicTable and GenOFSVariable.


class Pair {
friend class PairList;

private:
  TaggedRef term1;
  TaggedRef term2;
  Pair* next;

  USEFREELISTMEMORY;

  Pair(TaggedRef t1, TaggedRef t2, Pair* list) {
     term1=t1;
     term2=t2;
     next=list;
  }
};


class PairList {
friend class Pair;

private:
  Pair* list;

public:
  PairList() { list=NULL; }

  USEFREELISTMEMORY;

  // Add a pair to the head of a list
  void addpair(TaggedRef term1, TaggedRef term2) {
      list=new Pair(term1, term2, list);
  }

  // Get the first pair of a list
  // (Return FALSE if the list is empty)
  Bool getpair(TaggedRef &t1, TaggedRef &t2) {
      if (list!=NULL) {
          t1=list->term1;
          t2=list->term2;
          return TRUE;
      } else {
          return FALSE;
      }
  }

  // Advance to next element
  // (Return FALSE if the advance cannot be done because the list is empty)
  Bool nextpair() {
      if (list!=NULL) {
          list=list->next;
          return TRUE;
      } else {
          return FALSE;
      }
  }

  Bool isempty() {
      return (list==NULL);
  }

  // Free memory for all elements of the list and the head
  void free() {
      while (list) {
          Pair* next=list->next;
          freeListDispose(list, sizeof(*list));
          list=next;
      }
      freeListDispose(this, sizeof(*this)); // IS THIS REASONABLE?
  }
};


//-------------------------------------------------------------------------
//                               class DynamicTable
//-------------------------------------------------------------------------

// A class implementing a hash table that can be expanded when
// it becomes too full.  Several operations (merge, srecordcheck) are
// added to facilitate unification of GenOFSVariables, which are built on
// top of a DynamicTable.

typedef unsigned long dt_index;


class HashElement {
friend class DynamicTable;

private:
    TaggedRef ident;
    TaggedRef value;
};


// class DynamicTable uses:
//   TaggedRef SRecord::getFeature(Literal* feature)            (srecord.hh)
//   int Literal::hash()                                        (term.hh)
//   Literal* tagged2Literal(TaggedRef ref)                     (tagged.hh)
//   Bool isLiteral(TaggedRef term)                             (tagged.hh)
//   void* heapMalloc(size_t chunk_size)                        (mem.hh)
//   void* gcRealloc(void* ptr, size_t sz)                      (gc.cc)
//   void gcTagged(TaggedRef &fromTerm, TaggedRef &toTerm)      (gc.cc)

class DynamicTable {
friend class HashElement;

public:
    dt_index numelem;
    dt_index size;
    HashElement table[1]; // 1 == size of initial table

    USEHEAPMEMORY;

    void print(ostream & = cout, int=0, int=0);
    void printLong(ostream & = cout, int=0, int=0);

    // Copy the dynamictable from 'from' to 'to' space:
    DynamicTable* gc(void); // See definition in gc.cc
    void gcRecurse(void);

    DynamicTable() { error("do not use DynamicTable"); }

    // Create an initially empty dynamictable of size s
    static DynamicTable* newDynamicTable(dt_index s=4);

    // Initialize an elsewhere-allocated dynamictable of size s
    void init(dt_index s=1);

    // Create a copy of an existing dynamictable
    DynamicTable* copyDynamicTable();

    // Test whether the current table has too little room for one new element:
    // ATTENTION: Calls to insert should be preceded by fullTest.
    Bool fullTest();

    // Return a table that is double the size of the current table and
    // that contains the same elements:
    // ATTENTION: Should be called before insert if the table is full.
    DynamicTable* doubleDynamicTable();

    // Insert val at index id
    // Return NULL if val is successfully inserted (id did not exist)
    // Return the value of the pre-existing element if id already exists
    // Test for and increase size of hash table if it becomes too full
    // ATTENTION: insert must only be done if the table has room for a new element.
    TaggedRef insert(TaggedRef id, TaggedRef val);

    // Look up val at index id
    // Return val if it is found
    // Return NULL if nothing is found
    TaggedRef lookup(TaggedRef id);

    // Return TRUE iff there are features in an external dynamictable that
    // are not in the current dynamictable
    Bool extraFeaturesIn(DynamicTable* dt);

    // Merge the current dynamictable into an external dynamictable
    // Return a pairlist containing all term pairs with the same feature
    // The external dynamictable is resized if necessary
    void merge(DynamicTable* &dt, PairList* &pairs);

    // Check an srecord against the current dynamictable
    // Return TRUE if all elements of dynamictable exist in srecord.
    // Return FALSE if there exists element of dynamictable that is not in srecord.
    // If TRUE, collect pairs of corresponding elements of dynamictable and srecord.
    // If FALSE, pair list contains a well-terminated but meaningless list.
    // Neither the srecord nor the dynamictable is modified.
    Bool srecordcheck(SRecord &sr, PairList* &pairs);

    // Return a difference list of all the features currently in the dynamic table.
    // The head is the return value and the tail is returned through an argument.
    TaggedRef getOpenArityList(TaggedRef*,Board*);
    TaggedRef getOpenArityList(TaggedRef*);

    // Return list of features in current table that are not in dt:
    TaggedRef extraFeatures(DynamicTable* &dt);

    // Return list of features in srecord that are not in current table:
    TaggedRef extraSRecFeatures(SRecord &sr);

    // Return sorted list containing all features:
    TaggedRef getArity();

private:

    // Hash and rehash until the element or an empty slot is found
    // Returns index of slot; the slot is empty or contains the element
    dt_index fullhash(TaggedRef id) {
        Assert(isPwrTwo(size));
        Assert(isLiteral(id));
        // Function 'hash' may eventually return the literal's seqNumber (see term.hh):
        dt_index i=(size-1) & ((dt_index) (tagged2Literal(id)->hash()));
        dt_index s=1;
        // Rehash if necessary using semi-quadratic probing (quadratic is not covering)
        while(table[i].ident!=makeTaggedNULL() && table[i].ident!=id) {
            i+=s;
            i&=(size-1);
            s++;
        }
        return i;
    }

    // Return true iff argument is a power of two:
    static Bool isPwrTwo(dt_index s)
    {
        Assert(s>0);
        while ((s&1)==0) s=(s>>1);
        return (s==1);
    }
};


//-------------------------------------------------------------------------
//                           class GenOFSVariable
//-------------------------------------------------------------------------


class GenOFSVariable: public GenCVariable {

private:
    TaggedRef label;
    DynamicTable* dynamictable;

public:
    GenOFSVariable(DynamicTable &dt)
    : GenCVariable(OFSVariable) {
        label=makeTaggedRef(newTaggedUVar(am.currentBoard));
        dynamictable= &dt;
    }

    GenOFSVariable()
    : GenCVariable(OFSVariable) {
        label=makeTaggedRef(newTaggedUVar(am.currentBoard));
        dynamictable=DynamicTable::newDynamicTable();
    }

    // GenOFSVariable(TaggedRef lbl)
    // : GenCVariable(OFSVariable) {
    //  Assert(isLiteral(lbl));
    //  label=lbl;
    //  dynamictable=DynamicTable::newDynamicTable();
    // }

    // Methods relevant for term copying (gc and solve)
    void gc(void);
    size_t getSize(void) { return sizeof(GenOFSVariable); }

    TaggedRef getLabel(void) {
        return label;
    }

    long getWidth(void) {
        return (long)(dynamictable->numelem);
    }

    DynamicTable* getTable(void) {
        return dynamictable;
    }

    TaggedRef getOpenArityList(TaggedRef*,Board*);
    TaggedRef getOpenArityList(TaggedRef*);

    Bool unifyOFS(TaggedRef *, TaggedRef, TaggedRef *, TaggedRef, Bool);

    // Return the feature value if feature exists, return NULL if it doesn't exist
    TaggedRef getFeatureValue(TaggedRef feature) {
        Assert(isLiteral(feature));
        return dynamictable->lookup(feature);
    }

    // Add the feature and its value
    // If the feature already exists, do not insert anything
    // Return TRUE if feature successfully inserted, FALSE if it already exists
    // ATTENTION: only use this for terms that do not have to be trailed
    Bool addFeatureValue(TaggedRef feature, TaggedRef term) {
        Assert(isLiteral(feature));
        if (dynamictable->fullTest()) dynamictable=dynamictable->doubleDynamicTable();
        TaggedRef prev=dynamictable->insert(feature,term);
        // (a future optimization: a second suspList only waiting on features)
        if (prev==makeTaggedNULL()) {
            // propagate(makeTaggedCVar(this), suspList, makeTaggedCVar(this), pc_propagator);
            am.addFeatOFSSuspensionList(makeTaggedCVar(this),suspList,feature,FALSE);
            return TRUE;
        } else {
            return FALSE;
        }
    }

    // Used in conjunction with addFeatureValue to propagate suspensions:
    void propagateFeature(void) {
      /* third argument must be ignored --> use AtomNil */
      propagate(makeTaggedCVar(this), suspList, AtomNil, pc_propagator);
    }

    int getSuspListLength(void) {
        // see suspension.{hh,cc}:
        return suspList->length();
    }

    int getNumOfFeatures(void) {
        return (int) dynamictable->numelem;
    }

    // Is X=val still valid, i.e., is val a literal and is width(ofs)==0 (see GenFDVariable::valid)
    Bool valid(TaggedRef val);

    // These procedures exist as well in the class GenFDVariable,
    // but they are not needed in GenOFSVariable:

    // void propagate (needs no redefinition from GenCVariable version)
    // void relinkSuspList (needs no redefinition from GenCVariable version)
    // void becomesSmallIntAndPropagate (meaningless for ofs)
    // void setDom (meaningless for ofs)
    // FiniteDomain &getDom (meaningless for ofs)
};


Bool isGenOFSVar(TaggedRef term);
Bool isGenOFSVar(TaggedRef term, TypeOfTerm tag);
GenOFSVariable *tagged2GenOFSVar(TaggedRef term);

#endif
