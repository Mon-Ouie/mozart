/*
  Hydra Project, DFKI Saarbruecken,
  Stuhlsatzenhausweg 3, D-66123 Saarbruecken, Phone (+49) 681 302-5312
  Author: tmueller
  Last modified: $Date$ from $Author$
  Version: $Revision$
  State: $State$

  ------------------------------------------------------------------------
*/

#ifndef __AUXCOMP_HH__
#define __AUXCOMP_HH__

#include <stdio.h>
#include <math.h>
#include <limits.h>

//-----------------------------------------------------------------------------

inline
OZ_Boolean is_recalc_txl_lower(int i, int k, int a[])
{
  return (a[i] >=0) != (a[k] >= 0);
}

inline
OZ_Boolean is_recalc_txu_lower(int i, int k, int a[])
{
  return (a[i] >= 0) == (a[k] >= 0);
}

inline
OZ_Boolean is_recalc_txl_upper(int i, int k, int a[])
{
  return (a[i] >= 0) == (a[k] >= 0);
}

inline
OZ_Boolean is_recalc_txu_upper(int i, int k, int a[])
{
  return (a[i] >= 0) != (a[k] >= 0);
}

//-----------------------------------------------------------------------------

#ifndef INT_MAX
#define INT_MAX    2147483647
#define INT_MIN    (-2147483647-1)
#endif

inline int truncToIntMax(double d) { return d > INT_MAX ? INT_MAX : int(d); }
inline int doubleToInt(double d) {
  return (d > INT_MAX) ? INT_MAX : ((d < INT_MIN) ? INT_MIN : int(d));
}
inline int square(int a) { return truncToIntMax(double(a) * double(a)); }
inline int min(int a, int b) { return a < b ? a : b; }
inline int max(int a, int b) { return a > b ? a : b; }

//-----------------------------------------------------------------------------
// cache stuff for LinEqPropagator::run()

#define CACHESLOTSIZE 4

inline
int find_cache_slot_num(int sz) {
  return doubleToInt(ceil(double(sz) / double(CACHESLOTSIZE)));
}


inline
void init_cache_slot_index(int sz, int cache_num,
                           int cache_from[], int cache_to[])
{
  cache_from[0] = 0;

  int i;
  for (i = 0; i < cache_num; i += 1) {
    cache_to[i] = CACHESLOTSIZE + cache_from[i];
    if(i + 1 < cache_num) {
      cache_from[i + 1] = cache_to[i];
    }
  }

  OZ_ASSERT(i > 0);

  cache_to[i - 1] = min(sz, cache_to[i - 1]);
}

// Computes values for txu assuming a_k is positive
inline
void update_cache(int slot, int a[], OZ_FDIntVar x[],
                  int cache_from[], int cache_to[],
                  int pos_cache[], int neg_cache[])
{
  NUMBERCAST cache_val_pos = 0, cache_val_neg = 0;
  int cache_slot_to = cache_to[slot];

  for (int j = cache_from[slot]; j < cache_slot_to; j += 1)
    if (a[j] >= 0) {
      cache_val_pos += NUMBERCAST(a[j]) * x[j]->getMinElem();
      cache_val_neg += NUMBERCAST(a[j]) * x[j]->getMaxElem();
    } else {
      cache_val_pos += NUMBERCAST(a[j]) * x[j]->getMaxElem();
      cache_val_neg += NUMBERCAST(a[j]) * x[j]->getMinElem();
    }
  pos_cache[slot] = doubleToInt(cache_val_pos);
  neg_cache[slot] = doubleToInt(cache_val_neg);
}

inline
int precalc_lin(int slots, int slot_k, int cache[], int c)
{
  int sum = c;

  for (int i = slots; i--; )
    if (i != slot_k)
      sum += cache[i];
  return sum;
}

inline
int calc_txl_lin(int i, int slot,
                 int a[], OZ_FDIntVar x[],
                 int cache_from[], int cache_to[],
                 int pos_txl_cache, int neg_txl_cache)
{
  NUMBERCAST s;
  // summing up uncached values
  int cache_slot_to = cache_to[slot];
  if (a[i] >= 0) {
    s = pos_txl_cache;
    for (int j = cache_from[slot]; j < cache_slot_to; j += 1)
      if (j != i)
        s += a[j] * NUMBERCAST(a[j] >= 0 ? x[j]->getMaxElem() : x[j]->getMinElem());
  } else {
    s = neg_txl_cache;
    for (int j = cache_from[slot]; j < cache_slot_to; j += 1)
      if (j != i)
        s += a[j] * NUMBERCAST(a[j] >= 0 ? x[j]->getMinElem() : x[j]->getMaxElem());
  }
  return doubleToInt(ceil(-double(s) / a[i]));
}

inline
int calc_txu_lin(int i, int slot,
                 int a[], OZ_FDIntVar x[],
                 int cache_from[], int cache_to[],
                 int pos_txu_cache, int neg_txu_cache)
{
  NUMBERCAST s;
  // summing up uncached values
  int cache_slot_to = cache_to[slot];
  if (a[i] >= 0) {
    s = pos_txu_cache;
    for (int j = cache_from[slot]; j < cache_slot_to; j += 1)
      if (j != i)
        s += a[j] * NUMBERCAST(a[j] >= 0 ? x[j]->getMinElem() : x[j]->getMaxElem());
  } else {
    s = neg_txu_cache;
    for (int j = cache_from[slot]; j < cache_slot_to; j += 1)
      if (j != i)
        s += a[j] * NUMBERCAST(a[j] >= 0 ? x[j]->getMaxElem() : x[j]->getMinElem());
  }
  return doubleToInt(floor(-double(s) / a[i]));
}

//-----------------------------------------------------------------------------

inline
int calc_txl_nonlin(int k, int sz, int l, int smd_sz[],
                    int a[], FDIntVarArr2 &x, int c)
{
  NUMBERCAST sum = c;
  for (int i = sz; i--; ) {
    NUMBERCAST prod = a[i];
    if (i != k) {
      if ((a[i] >= 0) == (a[k] >= 0)) {
        for (int j = smd_sz[i]; j--; )
          prod *= x[i][j]->getMaxElem();
      } else {
        for (int j = smd_sz[i]; j--; )
          prod *= x[i][j]->getMinElem();
      }
      sum += prod;
    }
  }

  NUMBERCAST prod = a[k];
  for (int j = smd_sz[k]; j--; )
    if (j != l)
      prod *= x[k][j]->getMaxElem();
  return prod == 0 ? 0 : doubleToInt(ceil(- sum / prod));
}

inline
int calc_txu_nonlin(int k, int sz, int l, int smd_sz[],
                    int a[], FDIntVarArr2 &x, int c)
{
  NUMBERCAST sum = c;
  for (int i = sz; i--; ) {
    NUMBERCAST prod = a[i];
    if (i != k) {
      if ((a[i] >= 0) == (a[k] >= 0)) {
        for (int j = smd_sz[i]; j--; )
          prod *= x[i][j]->getMinElem();
      } else  {
        for (int j = smd_sz[i]; j--; )
          prod *= x[i][j]->getMaxElem();
      }
      sum += prod;
    }
  }

  NUMBERCAST prod = a[k];
  for (int j = smd_sz[k]; j--; )
    if (j != l)
      prod *= x[k][j]->getMinElem();
  return prod == 0 ? OZ_smallIntMax() : doubleToInt(floor(- sum / prod));
}

//-----------------------------------------------------------------------------

inline
NUMBERCAST calc_tx_nonlin(int k, int sz, int l, int smd_sz[],
                          int a[], FDIntVarArr2 &x, int c)
{
  NUMBERCAST sum = c;
  for (int i = sz; i--; ) {
    NUMBERCAST prod = a[i];
    if (i != k) {
      if ((!(a[k] >= 0))^(a[i] >= 0) == (a[k] >= 0)) {
        for (int j = smd_sz[i]; j--; )
          prod *= x[i][j]->getMinElem();
      } else  {
        for (int j = smd_sz[i]; j--; )
          prod *= x[i][j]->getMaxElem();
      }
      sum += prod;
    }
  }

  NUMBERCAST prod = a[k];
  if (a[k] >= 0) {
    for (int j = smd_sz[k]; j--; )
      if (j != l)
        prod *= x[k][j]->getMinElem();
    return prod == 0 ? OZ_smallIntMax() : doubleToInt(floor(- sum / prod));
  } else {
    for (int j = smd_sz[k]; j--; )
      if (j != l)
        prod *= x[k][j]->getMaxElem();
    return prod == 0 ? 0 : (- sum / prod);
  }
}

//-----------------------------------------------------------------------------

inline
double calc_tx_lin(int k, int sz, int a[], OZ_FDIntVar x[], int c)
{
  double s = c;
  for (int i = sz; i--; )
    if (k != i)
      s += NUMBERCAST(a[i]) * (a[i] >= 0 ? x[i]->getMinElem() : x[i]->getMaxElem());

  s /= -a[k];
  return s;
}

//-----------------------------------------------------------------------------

inline
int calc_txl_lin(int k, int sz, int a[], OZ_FDIntVar x[], int c)
{
  NUMBERCAST sum = 0;
  OZ_Boolean sign_k = (a[k] >=0);
  for (int i = sz; i--; )
    if (i != k)
      sum += NUMBERCAST(a[i]) * (sign_k == (a[i] >= 0)
                         ? x[i]->getMaxElem() : x[i]->getMinElem());

  return doubleToInt(ceil(-(sum + c) / NUMBERCAST(a[k])));
}


inline
int calc_txu_lin(int k, int sz, int a[], OZ_FDIntVar x[], int c)
{
  NUMBERCAST sum = 0;
  OZ_Boolean sign_k = (a[k] >=0);
  for (int i = sz; i--; )
    if (i != k)
      sum += NUMBERCAST(a[i]) * (sign_k == (a[i] >= 0)
                         ? x[i]->getMinElem() : x[i]->getMaxElem());

  return doubleToInt(floor(-(sum + c) / NUMBERCAST(a[k])));
}

//-----------------------------------------------------------------------------

inline
int check_calc_txl(int sz, int a[], OZ_FDIntVar x[], int c)
{
  NUMBERCAST sum = c;
  for (int i = sz; i--; )
    sum += NUMBERCAST(a[i]) * ((a[i] >= 0) ? x[i]->getMinElem() : x[i]->getMaxElem());
  return doubleToInt(sum);
}

inline
int check_calc_txu(int sz, int a[], OZ_FDIntVar x[], int c)
{
  NUMBERCAST sum = c;
  for (int i = sz; i--; )
    sum += NUMBERCAST(a[i]) * ((a[i] >= 0) ? x[i]->getMaxElem() : x[i]->getMinElem());
  return doubleToInt(sum);
}

//-----------------------------------------------------------------------------

class BoolMatrix3 {
private:
  int * _vector, vect_size;
  int _x_dim, _y_dim, _z_dim, _y_off, _z_off;

  int bitNum(int x, int y, int z) { return x+y*_y_off+z*_z_off; }
  static int divWord(int n) { return n >> 5; }
  static int modWord(int n) { return n & 0x1f; }

public:
  BoolMatrix3(int *vect, int vs, int x, int y, int z)
    : _vector(vect), vect_size(vs),
      _x_dim(x), _y_dim(y), _z_dim(z)
  {
    OZ_ASSERT(vect_size == requiredVectSize(x, y, z));
    _y_off = _x_dim;
    _z_off = _x_dim * _y_dim;
  }

  static int requiredVectSize(int x_dim, int y_dim, int z_dim)
  {
    int i = x_dim * y_dim * z_dim;
    return modWord(i) ? divWord(i) + 1 : divWord(i);
  }

  void init(OZ_Boolean how = OZ_FALSE)
  {
    if (how)
      for (int i = vect_size; i--; ) _vector[i] = 0xffff;
    else
      for (int i = vect_size; i--; ) _vector[i] = 0x0000;
  }
  OZ_Boolean is(int x, int y, int z)
  {
    int i = bitNum(x, y, z);
    return _vector[divWord(i)] & (0x0001 << modWord(i));
  }
  void set(int x, int y, int z)
  {
    int i = bitNum(x, y, z);
    _vector[divWord(i)] |= (0x0001 << modWord(i));
  }
  void reset(int x, int y, int z)
  {
    int i = bitNum(x, y, z);
    _vector[divWord(i)] &= ~(0x0001 << modWord(i));
  }
};

#endif // __AUXCOMP_HH__
