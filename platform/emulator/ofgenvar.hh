#ifndef __GENOFSVAR__H__
#define __GENOFSVAR__H__

#if defined(INTERFACE)
#pragma interface
#endif

#include "am.hh"
#include "genvar.hh"
#include "tagged.hh"
#include "value.hh"
#include "mem.hh"

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

// Three possibilities for an entry:
// ident: fea value: val   filled entry
// ident: fea value: 0     empty entry (emptied by removeC)
// ident: 0   value: 0     empty entry (empty from the start)
// Full emptiness check: table[i].value==makeTaggedNULL()
// Hash termination condition: table[i].ident==makeTaggedNULL() || table[i].ident==id || s==0

typedef unsigned long dt_index;

// Maximum size of completely full table
#define FILLLIMIT 4
// Maximum fill factor of bigger tables
// (if changing this, must also change ofgenvar.cc)
#define FILLFACTOR 0.75

// Return true iff argument is a power of two
extern Bool isPwrTwo(dt_index s);
// Round up to nearest power of two
extern dt_index ceilPwrTwo(dt_index s);

class HashElement {
friend class DynamicTable;

private:
    TaggedRef ident;
    TaggedRef value;
};


// class DynamicTable uses:
//   TaggedRef SRecord::getFeature(Literal* feature)		(srecord.hh)
//   int Literal::hash()					(term.hh)
//   Literal* tagged2Literal(TaggedRef ref)			(tagged.hh)
//   Bool isLiteral(TaggedRef term)				(tagged.hh)
//   void* heapMalloc(size_t chunk_size)	 		(mem.hh)
//   void* gcRealloc(void* ptr, size_t sz)			(gc.cc)
//   void gcTagged(TaggedRef &fromTerm, TaggedRef &toTerm)	(gc.cc)

class DynamicTable {
friend class HashElement;

public:
    dt_index numelem;
    dt_index size;
    HashElement table[1]; // 1 == placeholder.  Actual size set when allocated.

    USEHEAPMEMORY;

    void print(ostream & = cout, int=0, int=0);
    void printLong(ostream & = cout, int=0, int=0);
    void ofs2buffer(int depth);

    // Copy the dynamictable from 'from' to 'to' space:
    DynamicTable* gc(void); // See definition in gc.cc
    void gcRecurse(void);

    DynamicTable() { error("use newDynamicTable instead of new DynamicTable"); }

    // Create an initially empty dynamictable of size s
    static DynamicTable* newDynamicTable(dt_index s=4);

    // Initialize an elsewhere-allocated dynamictable of size s
    void init(dt_index s);

    // Test whether the current table has too little room for one new element:
    // ATTENTION: Calls to insert should be preceded by fullTest.
    Bool fullTest();

    // Return a table that is double the size of the current table and
    // that contains the same elements:
    // ATTENTION: Should be called before insert if the table is full.
    DynamicTable* doubleDynamicTable();

    // Return a copy of the current table that has size newSize and all contents
    // of the current table.  The current table's contents MUST fit in the copy!
    DynamicTable* copyDynamicTable(dt_index newSize);

    // Insert val at index id 
    // Return value is valid iff 'valid'==TRUE.  Otherwise, nothing is done.
    // Return NULL if val is successfully inserted (id did not exist) 
    // Return the value of the pre-existing element if id already exists
    // User should test for and increase size of hash table if it becomes too full
    // ATTENTION: insert must only be done if the table has room for a new element.
    TaggedRef insert(TaggedRef id, TaggedRef val, Bool *valid);

    // Look up val at index id
    // Return val if it is found
    // Return NULL if nothing is found
    TaggedRef lookup(TaggedRef id);

    // Destructively update index id with new value val, if index id already has a value
    // Return TRUE if index id successfully updated, else FALSE if index id does not
    // exist in table
    Bool update(TaggedRef id, TaggedRef val);

    // Remove index id from table.  To reclaim memory, if the table becomes too sparse then
    // return a smaller table that contains all its entries.  Otherwise, return same table.
    DynamicTable *remove(TaggedRef id);

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

    // Return list of features in current table that are not in dt
    TaggedRef extraFeatures(DynamicTable* &dt);

    // Return list of features in srecord that are not in current table
    TaggedRef extraSRecFeatures(SRecord &sr);

    // Return TRUE if current table has features that are not in arity argument:
    Bool hasExtraFeatures(int tupleArity);
    Bool hasExtraFeatures(Arity *recordArity);

    // Return sorted list (with given tail) containing all features
    TaggedRef getArityList(TaggedRef tail=AtomNil);

private:
    dt_index fullhash(TaggedRef id, Bool *valid);
};


//-------------------------------------------------------------------------
//                           class GenOFSVariable
//-------------------------------------------------------------------------


class GenOFSVariable: public GenCVariable {

    friend class GenCVariable;
    friend inline void addSuspOFSVar(TaggedRef, SuspList *);
    friend inline void addSuspOFSVar(TaggedRef, Thread *);

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

    // With new table of given size (must be pwr. of 2) in given space:
    GenOFSVariable(Board* hm, dt_index size)
    : GenCVariable(OFSVariable,hm) {
        label=makeTaggedRef(newTaggedUVar(hm));
        dynamictable=DynamicTable::newDynamicTable(size);
    }

    // With new table of given size (must be pwr. of 2):
    GenOFSVariable(dt_index size)
    : GenCVariable(OFSVariable) {
        label=makeTaggedRef(newTaggedUVar(am.currentBoard));
        dynamictable=DynamicTable::newDynamicTable(size);
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

    // Return a sorted difference list of all the features currently in the OFS
    // The head is the return value and the tail is returned through an argument.
    TaggedRef getOpenArityList(TaggedRef*,Board*);
    TaggedRef getOpenArityList(TaggedRef*);

    // Return a sorted list of all features currently in the OFS
    TaggedRef getArityList();

    void addDetSusp(Thread *susp);
    Bool unifyOFS(TaggedRef *, TaggedRef, TaggedRef *, TaggedRef, Bool);

    // Return the feature value if feature exists, return NULL if it doesn't exist
    TaggedRef getFeatureValue(TaggedRef feature) {
	Assert(isFeature(feature));
        return dynamictable->lookup(feature);
    }

    // Add the feature and its value
    // If the feature already exists, do not insert anything
    // Return TRUE if feature successfully inserted, FALSE if it already exists
    // ATTENTION: only use this for terms that do not have to be trailed
    Bool addFeatureValue(TaggedRef feature, TaggedRef term) {
	Assert(isFeature(feature));
        Bool valid;
        if (dynamictable->fullTest()) dynamictable=dynamictable->doubleDynamicTable();
        TaggedRef prev=dynamictable->insert(feature,term,&valid);
        if (!valid) {
            dynamictable=dynamictable->doubleDynamicTable();
            prev=dynamictable->insert(feature,term,&valid);
        }
        Assert(valid);
        // (future optimization: a second suspList only waiting on features)
        if (prev==makeTaggedNULL()) {
            // propagate(makeTaggedCVar(this), suspList, makeTaggedCVar(this), pc_propagator);
            am.addFeatOFSSuspensionList(makeTaggedCVar(this),suspList,feature,FALSE);
	    return TRUE;
        } else {
	    return FALSE;
	}
    }

    // Destructively update feature's value, if feature exists
    // Return TRUE if feature exists, FALSE if it does not
    Bool setFeatureValue(TaggedRef feature, TaggedRef term) {
	Assert(isFeature(feature));
        return dynamictable->update(feature,term);
    }

    // Remove the feature from the OFS
    // Reclaims memory if table becomes too sparse
    void removeFeature(TaggedRef feature) {
        dynamictable=dynamictable->remove(feature);
    }

    // Used in conjunction with addFeatureValue to propagate suspensions:
    void propagateFeature(void) {
      /* third argument must be ignored --> use AtomNil */
      propagate(makeTaggedCVar(this), suspList, pc_propagator);
    }

    int getSuspListLength(void) {
        // see suspension.{hh,cc}:
        return suspList->length();
    }

    int getNumOfFeatures(void) {
        return (int) dynamictable->numelem;
    }

    // Is X=val still valid, i.e., is val a feature and is width(ofs)==0 (see GenFDVariable::valid)
    Bool valid(TaggedRef val);

    // Hooks for indexing: the following two methods should return
    // OK iff "this" disentails its label being "l" or its
    // arity being "tupleArity"/"recordArity"
    // note: "tupleArity" may be zero
    // for now they always return NO, leading to suspension
    Bool disentailed(Literal *l, int tupleArity);
    Bool disentailed(Literal *l, Arity *recordArity);

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
/* a simple sorting routine using atomcmp */
void inplace_quicksort(TaggedRef* first, TaggedRef* last);

inline
void addSuspOFSVar(TaggedRef v, SuspList * el)
{
  GenOFSVariable * ofs = tagged2GenOFSVar(v);

  ofs->suspList = addSuspToList(ofs->suspList, el, ofs->home);
}

inline
void addSuspOFSVar(TaggedRef v, Thread * el)
{
  GenOFSVariable * ofs = tagged2GenOFSVar(v);

  ofs->suspList = addSuspToList(ofs->suspList, el, ofs->home);
}


#endif
