#include "dictionary.hh"
#include "builtins.hh"

OZ_Term site_dict = 0;

void SitePropertyInit()
{
  if (site_dict==0) {
    site_dict = makeTaggedConst(new OzDictionary(oz_rootBoard()));
    OZ_protect(&site_dict);
  }
}

#define INIT if (site_dict==0) SitePropertyInit()

OZ_BI_define(BIsitePropertyGet,1,1)
{
  OZ_expectType(0,"Feature",OZ_isFeature);
  INIT;
  TaggedRef out;
  if (tagged2Dictionary(site_dict)->getArg(OZ_in(0),out)!=PROCEED)
    return oz_raise(E_SYSTEM,E_KERNEL,"SitePropertyGet",1,OZ_in(0));
  OZ_RETURN(out);
}
OZ_BI_end

OZ_BI_define(BIsitePropertyPut,2,0)
{
  OZ_expectType(0,"Feature",OZ_isFeature);
  INIT;
  tagged2Dictionary(site_dict)->setArg(OZ_in(0),OZ_in(1));
  return PROCEED;
}
OZ_BI_end

