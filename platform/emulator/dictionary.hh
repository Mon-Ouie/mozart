/*
 * FBPS Saarbr"ucken
 * Author: mehl
 * Last modified: $Date$ from $Author$
 * Version: $Revision$
 * State: $State$
 *
 * Values: literal, list, records
 */

#ifndef __DICTIONARYHH
#define __DICTIONARYHH

#ifdef INTERFACE
#pragma interface
#endif

/*===================================================================
 * Dictionaries
 *=================================================================== */

class OzDictionary: public ConstTermWithHome {
  friend void ConstTerm::gcConstRecurse(void);
private:
  DynamicTable *table;

public:
  OzDictionary(Board *b) : ConstTermWithHome(b,Co_Dictionary) 
  {
    table = DynamicTable::newDynamicTable();
  }
  
  OZ_Return getArg(TaggedRef key, TaggedRef &out) 
  { 
    TaggedRef ret = table->lookup(key);
    if (ret == makeTaggedNULL())
      return FAILED;
    out = ret;
    return PROCEED;
  }

  TaggedRef member(TaggedRef key) 
  { 
    TaggedRef found = table->lookup(key);
    return (found == makeTaggedNULL()) ? NameFalse : NameTrue;
  }

  void setArg(TaggedRef key, TaggedRef value)
  { 
    if (table->fullTest()) table = table->doubleDynamicTable();
    Bool valid=table->add(key,value);
    if (!valid) {
      table = table->doubleDynamicTable();
      valid = table->add(key,value);
    }
    Assert(valid);
  }

  void remove(TaggedRef key) 
  { 
    table = table->remove(key);
  }

  TaggedRef keys() 
  { 
    return table->getKeys();
  }

  TaggedRef toRecord(TaggedRef lbl) 
  {
    return table->toRecord(lbl);
  }

  OZPRINT;
  OZPRINTLONG;
};


inline
Bool isDictionary(TaggedRef term)
{
  return isConst(term) && tagged2Const(term)->getType() == Co_Dictionary;
}

inline
OzDictionary *tagged2Dictionary(TaggedRef term)
{
  Assert(isDictionary(term));
  return (OzDictionary *) tagged2Const(term);
}

#endif
