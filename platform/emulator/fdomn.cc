/*
  Hydra Project, DFKI Saarbruecken,
  Stuhlsatzenhausweg 3, D-66123 Saarbruecken, Phone (+49) 681 302-5312
  Author: tmueller
  Last modified: $Date$ from $Author$
  Version: $Revision$
  State: $State$

  ------------------------------------------------------------------------
*/

#if defined(__GNUC__) && !defined(NOPRAGMA)
#pragma implementation "fdomn.hh"
#endif

#include "fdomn.hh"
#include "misc.hh"

#if defined(OUTLINE) || defined(FDOUTLINE)
#define inline
#include "fdomn.icc"
#undef inline
#endif

// Debugging Stuff ------------------------------------------------------------

#if defined(DEBUG_CHECK) && defined(DEBUG_FD)

Bool FDIntervals::isConsistent(void) const {
  if (high < 0) return FALSE;
  int i;
  for (i = 0; i < high; i++) {
    if (i_arr[i].left > i_arr[i].right) return FALSE;
    if ((i + 1 < high) && (i_arr[i].right >= i_arr[i + 1].left)) return FALSE;
  }
  for (i = 0; i < high - 1; i++)
    if (! ((i_arr[i].right + 1) < i_arr[i + 1].left)) return FALSE;
  return TRUE;
}

Bool FiniteDomain::isConsistent(void) const {
  if (size == 0) return TRUE;
  descr_type type = getType();
  if (type == fd_descr)
    return findSize() == size;
  else if (type == bv_descr)
    return get_bv()->findSize() == size;
  else
    return get_iv()->findSize() == size;
}

#endif

// Miscellaneous --------------------------------------------------------------

extern unsigned char numOfBitsInByte[];
extern int toTheLowerEnd[];
extern int toTheUpperEnd[];

int fd_bv_left_conv[fd_bv_conv_max_high];
int fd_bv_right_conv[fd_bv_conv_max_high];

intptr fd_iv_left_sort[MAXFDBIARGS];
intptr fd_iv_right_sort[MAXFDBIARGS];

// FDInterval -----------------------------------------------------------------

void printFromTo(ostream &ofile, int f, int t)
{
  if (f == t)
    ofile << ' ' << f;
  else if ((t - f) == 1)
    ofile << ' ' << f << ' ' << t;
  else
    ofile << ' ' << f << ".." << t;
}

void FDIntervals::print(ostream &ofile, int idnt) const
{
  ofile << indent(idnt) << '{';
  for (int i = 0; i < high; i += 1)
    printFromTo(ofile, i_arr[i].left, i_arr[i].right);
  ofile << " }";
}

void FDIntervals::printLong(ostream &ofile, int idnt) const
{
  ofile << endl << indent(idnt) << "high=" << high
        << " fd_iv_max_high=" << fd_iv_max_high << endl;
  print(ofile, idnt);
  for (int i = 0; i < high; i += 1)
    ofile << endl << indent(idnt)
          << "i_arr[" << i << "]@" << (void*) &i_arr[i]
          << " left=" << i_arr[i].left << " right=" << i_arr[i].right;
  ofile << endl;
}

void FDIntervals::printDebug(void) const
{
  print(cerr, 0);
  cerr << endl;
  cerr.flush();
}

void FDIntervals::printDebugLong(void) const
{
  printLong(cerr, 0);
  cerr << endl;
  cerr.flush();
}

void FDIntervals::initList(int list_len,
                           int * list_left, int * list_right)
{
  AssertFD(list_len <= high);

  for (int i = list_len; i--; ) {
    i_arr[i].left = list_left[i];
    i_arr[i].right = list_right[i];
  }

  AssertFD(isConsistent());
}

int FDIntervals::nextBiggerElem(int v, int max_elem) const
{
  for (int i = 0; i < high; i += 1) {
    if (i_arr[i].left <= v && i_arr[i].right < v)
      return v + 1;
  }
  error ("no bigger element in domain");
  return -1;
}

Bool FDIntervals::next(int i, int &n) const
{
  if (contains(i)) {
    n = i;
    return FALSE;
  }

  int j;
  for (j = 0;
       j < high - 1 &&
       !(i_arr[j].right < i && i < i_arr[j + 1].left);
       j += 1);

  int l = i_arr[j].right, r = i_arr[j + 1].left;

  if ((r - i) == (i - l)) {
    n = l;
    return TRUE;
  } else {
    n = ((r - i) < (i - l)) ? r : l;
    return FALSE;
  }
}

inline
LTuple * mkListEl(LTuple * &h, LTuple * a, TaggedRef el)
{
  if (h == NULL) {
    return h = new LTuple(el, AtomNil);
  } else {
    LTuple * aux = new LTuple(el, AtomNil);
    a->setTail(makeTaggedLTuple(aux));
    return aux;
  }
}

TaggedRef FDIntervals::getAsList(void) const
{
  LTuple * hd = NULL, * l_ptr = NULL;

  for (int i = 0; i < high; i += 1)
      l_ptr = (i_arr[i].left == i_arr[i].right)
        ? mkListEl(hd, l_ptr, newSmallInt(i_arr[i].left))
        : mkListEl(hd, l_ptr, mkTuple(i_arr[i].left, i_arr[i].right));

  return makeTaggedLTuple(hd);
}

// taken from stubbs&webre page 168
inline
int FDIntervals::findPossibleIndexOf(int i) const
{
  int lo, hi;
  for (lo = 0, hi = high - 1; lo < hi; ) {
    int mid = (lo + hi + 1) / 2;
    if (i < i_arr[mid].left)
      hi = mid - 1;
    else
      lo = mid;
  }
  return lo;
}

Bool FDIntervals::contains(int i) const
{
  int index = findPossibleIndexOf(i);
  return (i_arr[index].left <= i && i <= i_arr[index].right);
}

// 0 <= leq <= fd_iv_max_elem
int FDIntervals::operator <= (const int leq)
{
  int index = findPossibleIndexOf(leq);

  if (i_arr[index].left <= leq && leq <= i_arr[index].right) {
    i_arr[index].right = leq;
    index += 1;
  } else
    if (i_arr[index].right < leq) index += 1;

  AssertFD(high >= index);

  high = index;

  AssertFD(isConsistent());

  return findSize();
}

// 0 <= geq <= fd_iv_max_elem
int FDIntervals::operator >= (const int geq)
{
  int index = findPossibleIndexOf(geq);

  if (i_arr[index].left <= geq && geq <= i_arr[index].right)
    i_arr[index].left = geq;
  else
    if (i_arr[index].right < geq) index += 1;

  if (index != 0) {
    for (int t = 0, f = index; f < high; ) i_arr[t++] = i_arr[f++];
    high -= index;
  }

  AssertFD(isConsistent());

  return findSize();
}

FDIntervals * FDIntervals::operator -= (const int take_out)
{
  int index = findPossibleIndexOf(take_out);

  if (take_out < i_arr[index].left) // take_out is not in this domain
    return this;

  // take_out is in i_arr[index]
  if (i_arr[index].left == i_arr[index].right) {
    for (int i = index; i < (high - 1); i++)
      i_arr[i] = i_arr[i + 1];
    high -= 1;
  } else if (i_arr[index].left == take_out) {
    i_arr[index].left += 1;
  } else if (i_arr[index].right == take_out) {
    i_arr[index].right -= 1;
  } else {
    int new_max_high = high + 1;
    if (new_max_high <= fd_iv_max_high) {
      high = new_max_high;

      for (int i = high - 1; i > index; i -= 1)
        i_arr[i] = i_arr[i - 1];
      i_arr[index].right = take_out - 1;
      i_arr[index + 1].left = take_out + 1;
    } else {
      FDIntervals * new_iv = new(new_max_high) FDIntervals(new_max_high);
      int i;
      for (i = 0; i <= index; i += 1)
        new_iv->i_arr[i] = i_arr[i];
      new_iv->i_arr[index].right = take_out - 1;
      for (i = index; i < high; i += 1)
        new_iv->i_arr[i + 1] = i_arr[i];
      new_iv->i_arr[index + 1].left = take_out + 1;

      AssertFD(new_iv->isConsistent());

      return new_iv;
    }
  }

  AssertFD(isConsistent());

  return this;
}

FDIntervals * FDIntervals::operator += (const int put_in)
{
  int index = findPossibleIndexOf(put_in);

  if (i_arr[index].left <= put_in && put_in <= i_arr[index].right)
    return this;

  if (put_in == i_arr[index].right + 1) {
    // closing a gap
    if ((index + 1) < high && put_in == i_arr[index + 1].left - 1) {
      i_arr[index].right = i_arr[index + 1].right;
      for (int i = index + 1; (i + 1) < high; i += 1)
        i_arr[i] = i_arr[i + 1];
      high -= 1;
    } else {
      i_arr[index].right += 1;
    }
  } else if (put_in == i_arr[index].left - 1) {
    i_arr[index].left = put_in;
  } else if ((index + 1) < high && put_in == i_arr[index + 1].left - 1) {
    i_arr[index + 1].left -= 1;
  } else {
    high += 1;
    if (i_arr[index].right < put_in) index += 1;
    if (high <= fd_iv_max_high) {
      for (int i = high - 1; index < i; i -= 1)
        i_arr[i] = i_arr[i - 1];
      i_arr[index].left = i_arr[index].right = put_in;
    } else {
      FDIntervals * new_iv = new(high) FDIntervals(high);
      int i;
      for (i = 0; i < index; i += 1)
        new_iv->i_arr[i] = i_arr[i];
      for (i = high - 1; index < i; i -= 1)
        new_iv->i_arr[i] = i_arr[i - 1];
      new_iv->i_arr[index].left = new_iv->i_arr[index].right = put_in;
      AssertFD(new_iv->isConsistent());
      return new_iv;
    }
  }

  AssertFD(isConsistent());

  return this;
}

FDIntervals * FDIntervals::complement(FDIntervals * x_iv)
{
  int c_i = 0, i = 0;
  if (0 < i_arr[i].left) {
    i_arr[c_i].left = 0;
    i_arr[c_i].right = x_iv->i_arr[i].left - 1;
    c_i += 1;
  }
  for ( ; i < x_iv->high - 1; i += 1, c_i += 1) {
    i_arr[c_i].left = x_iv->i_arr[i].right + 1;
    i_arr[c_i].right = x_iv->i_arr[i + 1].left - 1;
  }
  if (i_arr[i].right < fd_iv_max_elem) {
    i_arr[c_i].left = x_iv->i_arr[i].right + 1;
    i_arr[c_i].right = fd_iv_max_elem;
  }

  AssertFD(isConsistent());

  return this;
}

FDIntervals * FDIntervals::complement(int a_high, int * x_left, int * x_right)
{
  int c_i = 0, i = 0;
  if (0 < x_left[i]) {
    i_arr[c_i].left = 0;
    i_arr[c_i].right = x_left[i] - 1;
    c_i += 1;
  }
  for ( ; i < a_high - 1; i += 1, c_i += 1) {
    i_arr[c_i].left = x_right[i] + 1;
    i_arr[c_i].right = x_left[i + 1] - 1;
  }
  if (x_right[i] < fd_iv_max_elem) {
    i_arr[c_i].left = x_right[i] + 1;
    i_arr[c_i].right = fd_iv_max_elem;
  }

  AssertFD(isConsistent());

  return this;
}

int FDIntervals::union_iv(const FDIntervals &x, const FDIntervals &y)
{
  int x_c, y_c, z_c, r;
  for (x_c = 0, y_c = 0, z_c = 0, r; x_c < x.high && y_c < y.high; ) {

    if (x.i_arr[x_c].left < y.i_arr[y_c].left) {
      i_arr[z_c].left = x.i_arr[x_c].left;
      r = x.i_arr[x_c].right;
      x_c += 1;
    } else {
      i_arr[z_c].left = y.i_arr[y_c].left;
      r = y.i_arr[y_c].right;
      y_c += 1;
    }

    for (Bool cont = TRUE; cont; )
      if (x_c < x.high &&
          x.i_arr[x_c].left <= r + 1 && r <= x.i_arr[x_c].right) {
        r = x.i_arr[x_c].right;
        x_c += 1;
      } else if (y_c < y.high &&
                 y.i_arr[y_c].left <= r + 1 && r <= y.i_arr[y_c].right) {
        r = y.i_arr[y_c].right;
        y_c += 1;
      } else {
        cont = FALSE;
      }

    for (; x_c < x.high && x.i_arr[x_c].right <= r; x_c += 1);
    for (; y_c < y.high && y.i_arr[y_c].right <= r; y_c += 1);

    i_arr[z_c].right = r;
    z_c += 1;
  }

  // copy remaining intervals
  if ((x_c + 1) < x.high && x.i_arr[x_c].left < r) x_c += 1;
  for (; x_c < x.high; x_c += 1, z_c += 1) i_arr[z_c] = x.i_arr[x_c];
  if ((y_c + 1) < y.high && y.i_arr[y_c].left < r) y_c += 1;
  for (; y_c < y.high; y_c += 1, z_c += 1) i_arr[z_c] = y.i_arr[y_c];

  AssertFD(high >= z_c);

  high = z_c;

  AssertFD(isConsistent());

  return findSize();
}

int FDIntervals::intersect_iv(FDIntervals &z, const FDIntervals &y)
{
  int x_c, y_c, z_c;
  for (x_c = 0, y_c = 0, z_c = 0; x_c < high && y_c < y.high; )
    if (i_arr[x_c].left > y.i_arr[y_c].left) {
      if (y.i_arr[y_c].right < i_arr[x_c].left) { // no overlapping
        y_c += 1;
      } else if (y.i_arr[y_c].right <= i_arr[x_c].right) { // overlapping
        z.i_arr[z_c].left = i_arr[x_c].left;
        z.i_arr[z_c++].right = y.i_arr[y_c++].right;
      } else { // subsumption
        z.i_arr[z_c++] = i_arr[x_c++];
      }
    } else {
      if (i_arr[x_c].right < y.i_arr[y_c].left) { // no overlapping
        x_c += 1;
      } else if (i_arr[x_c].right <= y.i_arr[y_c].right) { // overlapping
        z.i_arr[z_c].left = y.i_arr[y_c].left;
        z.i_arr[z_c++].right = i_arr[x_c++].right;
      } else { // subsumption
        z.i_arr[z_c++] = y.i_arr[y_c++];
      }
    }
  AssertFD(z.high >= z_c);
  z.high = z_c;

  AssertFD(z.isConsistent());
  return z.findSize();
}

int FDIntervals::subtract_iv(FDIntervals &z, const FDIntervals &y)
{
  int x_c, y_c, z_c;
  for (x_c = 0, y_c = 0, z_c = 0; x_c < high && y_c < y.high; ) {
    for (; y_c < y.high && y.i_arr[y_c].right < i_arr[x_c].left; y_c += 1);
    if (y_c >= y.high) break;

    if (y.i_arr[y_c].left <= i_arr[x_c].left &&
        i_arr[x_c].right <= y.i_arr[y_c].right) { // overlap
      x_c += 1;
    } else if (i_arr[x_c].right < y.i_arr[y_c].left) { // step over
      z.i_arr[z_c++] = i_arr[x_c++];
    } else if (i_arr[x_c].right <= y.i_arr[y_c].right) { // right cut
      z.i_arr[z_c].left = i_arr[x_c++].left;
      z.i_arr[z_c++].right = y.i_arr[y_c].left - 1;
    } else if (y.i_arr[y_c].right <= i_arr[x_c].right) {
      if (i_arr[x_c].left < y.i_arr[y_c].left) {
        z.i_arr[z_c].left = i_arr[x_c].left;
      } else {
        z.i_arr[z_c].left = y.i_arr[y_c++].right + 1;
      }
      while (y_c < y.high && y.i_arr[y_c].right < i_arr[x_c].right) {
        z.i_arr[z_c].right = y.i_arr[y_c].left - 1;
        z.i_arr[++z_c].left = y.i_arr[y_c++].right + 1;
      }
      if (y_c < y.high && y.i_arr[y_c].left <= i_arr[x_c].right) {
        z.i_arr[z_c++].right = y.i_arr[y_c].left - 1;
        x_c += 1;
      } else {
        z.i_arr[z_c++].right = i_arr[x_c++].right;
      }
    }
  }

  for (; x_c < high; x_c += 1, z_c += 1) z.i_arr[z_c] = i_arr[x_c];

  AssertFD(z.high >= z_c);
  z.high = z_c;

  AssertFD(z.isConsistent());
  return z.findSize();
}

// FDBitVector ----------------------------------------------------------------

void FDBitVector::print(ostream &ofile, int idnt) const
{
  ofile << indent(idnt) << '{';

  int len = mkRaw(fd_bv_left_conv, fd_bv_right_conv);
  for (int i = 0; i < len; i += 1) {
    ofile << ' ' << fd_bv_left_conv[i];
    if (fd_bv_left_conv[i] != fd_bv_right_conv[i])
      if (fd_bv_left_conv[i] + 1 == fd_bv_right_conv[i])
        ofile << ' ' << fd_bv_right_conv[i];
      else
        ofile << ".." << fd_bv_right_conv[i];
  }
  ofile << " }";
}

void FDBitVector::printLong(ostream &ofile, int idnt) const
{
  ofile << "  fd_bv_max_high=" << fd_bv_max_high << endl;
  print(ofile, idnt);
  for (int i = 0; i < fd_bv_max_high; i++) {
    ofile << endl << indent(idnt + 2) << '[' << i << "]:  ";
    for (int j = 31; j >= 0; j--) {
      ofile << ((b_arr[i] & (1 << j)) ? '1' : 'o');
      if (j % 8 == 0) ofile << ' ';
    }
  }
  ofile << endl;
}

void FDBitVector::printDebug(void) const
{
  print(cerr, 0);
  cerr << endl;
  cerr.flush();
}

void FDBitVector::printDebugLong(void) const
{
  printLong(cerr, 0);
  cerr << endl;
  cerr.flush();
}

void FDBitVector::initList(int list_len,
                           int * list_left, int * list_right)
{
  setEmpty();
  for (int i = list_len; i--; )
    addFromTo(list_left[i], list_right[i]);
}

int FDBitVector::findSize(void)
{
  int s, i;
  for (s = 0, i = fd_bv_max_high; i--; ) {
    s += numOfBitsInByte[unsigned(b_arr[i]) & 0xffff];
    s += numOfBitsInByte[unsigned(b_arr[i]) >> 16];
  }

  return s;
}

int FDBitVector::findMinElem(void)
{
  int v, i;
  for (v = 0, i = 0; i < fd_bv_max_high; v += 32, i += 1)
    if (b_arr[i] != 0)
      break;

  if (i < fd_bv_max_high) {
    int word = b_arr[i];

    if (!(word << 16)) {
      word >>= 16; v += 16;
    }
    if (!(word << 24)) {
      word >>= 8; v += 8;
    }
    if (!(word << 28)) {
      word >>= 4; v += 4;
    }
    if (!(word << 30)) {
      word >>= 2; v += 2;
    }
    if (!(word << 31))
      v++;
  }
  return v;
}

int FDBitVector::findMaxElem(void)
{
  int v, i;
  for (v = fd_bv_max_elem, i = fd_bv_max_high - 1; i >= 0; v -= 32, i--)
    if (b_arr[i] != 0)
      break;

  if (i >= 0) {
    int word = b_arr[i];

    if (!(word >> 16)) {
      word <<= 16; v -= 16;
    }
    if (!(word >> 24)) {
      word <<= 8; v -= 8;
    }
    if (!(word >> 28)) {
      word <<= 4; v -= 4;
    }
    if (!(word >> 30)) {
      word <<= 2; v -= 2;
    }
    if (!(word >> 31))
      v--;
  }
  return v;
}

void FDBitVector::setFromTo(int from, int to)
{
  AssertFD(0 <= from && from <= to && to <= fd_bv_max_elem);

  int low_word = div32(from), low_bit = mod32(from);
  int up_word = div32(to), up_bit = mod32(to);

  int i;
  for (i = 0; i < low_word; i++)
    b_arr[i] = 0;
  for (i = up_word + 1; i < fd_bv_max_high; i++)
    b_arr[i] = 0;

  if (low_word == up_word) {
    b_arr[low_word] = toTheLowerEnd[up_bit] & toTheUpperEnd[low_bit];
  } else {
    b_arr[low_word] = toTheUpperEnd[low_bit];
    for (i = low_word + 1; i < up_word; i++)
      b_arr[i] = int(~0);
    b_arr[up_word] = toTheLowerEnd[up_bit];
  }
}

void FDBitVector::addFromTo(int from, int to)
{
  AssertFD(0 <= from && from <= to && to <= fd_bv_max_elem);

  int low_word = div32(from), low_bit = mod32(from);
  int up_word = div32(to), up_bit = mod32(to);

  if (low_word == up_word) {
    b_arr[low_word] |= toTheLowerEnd[up_bit] & toTheUpperEnd[low_bit];
  } else {
    b_arr[low_word] |= toTheUpperEnd[low_bit];
    for (int i = low_word + 1; i < up_word; i++)
      b_arr[i] = int(~0);
    b_arr[up_word] |= toTheLowerEnd[up_bit];
  }
}

int FDBitVector::nextBiggerElem(int v, int max_elem) const
{
  for (int new_v = v + 1; new_v <= max_elem; new_v += 1)
    if (contains(new_v))
      return new_v;

  error ("no bigger element in domain");
  return -1;
}

Bool FDBitVector::next(int i, int &n) const
{
  if (contains(i)) {
    n = i;
    return FALSE;
  }

  // find lower neighbour
  int lb = mod32(i), lw = div32(i), ub = lb;
  if (!(b_arr[lw] << (32 - 1 - lb))) {
    lb = 32 - 1;
    for (lw--; !b_arr[lw] && lw >= 0; lw--);
  }
  for (; lb >= 0 && !(b_arr[lw] & (1 << lb)); lb--);
  int l = 32 * lw + lb;

  // find upper neighbour
  int uw = div32(i);
  if (!(b_arr[uw] >> ub)) {
    ub = 0;
    for (uw++ ; !b_arr[uw] && uw < fd_bv_max_high; uw++);
  }
  for (; ub < 32 && !(b_arr[uw] & (1 << ub)); ub++);
  int u = 32 * uw + ub;

  if (u - i == i - l) {
    n = l;
    return TRUE;
  }

  n = (u - i < i - l) ? u : l;
  return FALSE;
}

TaggedRef FDBitVector::getAsList(void) const
{
  LTuple * hd = NULL, * l_ptr = NULL;
  int len = mkRaw(fd_bv_left_conv, fd_bv_right_conv);

  for (int i = 0; i < len; i += 1)
    if (fd_bv_left_conv[i] == fd_bv_right_conv[i])
      l_ptr = mkListEl(hd, l_ptr, newSmallInt(fd_bv_left_conv[i]));
    else
      l_ptr = mkListEl(hd, l_ptr, mkTuple(fd_bv_left_conv[i],
                                          fd_bv_right_conv[i]));

  return makeTaggedLTuple(hd);
}

// 0 <= leq <= fd_bv_max_elem
int FDBitVector::operator <= (const int leq)
{
  int upper_word = div32(leq), upper_bit = mod32(leq);

  for (int i = upper_word + 1; i < fd_bv_max_high; i += 1)
    b_arr[i] = 0;
  b_arr[upper_word] &= toTheLowerEnd[upper_bit];

  return findSize();
}

// 0 <= geq <= fd_bv_max_elem
int FDBitVector::operator >= (const int geq)
{
  int lower_word = div32(geq), lower_bit = mod32(geq);

  for (int i = 0; i < lower_word; i += 1)
    b_arr[i] = 0;
  b_arr[lower_word] &= toTheUpperEnd[lower_bit];
  return findSize();
}

int FDBitVector::mkRaw(int * list_left, int * list_right) const
{
  int i, r, l, len;
  for (i = 0, r = 1, len = 0; i < fd_bv_max_elem + 2; i += 1)
    if (contains(i)) {
      if (r) l = i;
      r = 0;
    } else {
      if (!r) {
        r = 1;
        int d = i - l;
        if (d == 1) {
          list_left[len] = list_right[len] = l;
          len += 1;
        } else {
          list_left[len] = l;
          list_right[len] = i - 1;
          len += 1;
        }
      }
    }
  return len;
}

int FDBitVector::union_bv(const FDBitVector &x, const FDBitVector &y)
{
  for (int i = fd_bv_max_high; i--; )
    b_arr[i] = x.b_arr[i] | y.b_arr[i];
  return findSize();
}

int FDBitVector::intersect_bv(FDBitVector &z, const FDBitVector &y)
{
  if (this == &z)
    for (int i = fd_bv_max_high; i--; )
      b_arr[i] = b_arr[i] & y.b_arr[i];
  else
    for (int i = fd_bv_max_high; i--; )
      z.b_arr[i] = b_arr[i] & y.b_arr[i];

  return z.findSize();
}

int FDBitVector::operator -= (const FDBitVector &y)
{
  for (int i = fd_bv_max_high; i--; )
    b_arr[i] = b_arr[i] & ~y.b_arr[i];
  return findSize();
}

// FD -------------------------------------------------------------------------

void FiniteDomain::print(ostream &ofile, int idnt) const
{
  if (getSize() == 0)
    ofile << indent(idnt) << "{ - empty - }";
  else switch (getType()) {
  case fd_descr:
      ofile << indent(idnt) << '{';
      printFromTo(ofile, min_elem, max_elem);
      ofile << " }";
    break;
  case bv_descr:
    get_bv()->print(ofile, idnt);
    break;
  case iv_descr:
    get_iv()->print(ofile, idnt);
    break;
  default:
    error("unexpected case");
  }
  DEBUG_FD_IR(FALSE, ofile << ((getType() == fd_descr) ? 'f' :
              (getType() == bv_descr ? 'b' : 'i')) << '#' << size);
}

char * FiniteDomain::descr_type_text[3] = {"bv_descr", "iv_descr", "fd_descr"};

void FiniteDomain::printLong(ostream &ofile, int idnt) const
{
  ofile << indent(idnt) << "min_elem=" << min_elem
        << " max_elem=" << max_elem << " size=" << getSize()
        << " descr=" << get_iv() << " type=" << descr_type_text[getType()];

  switch (getType()) {
  case fd_descr:
    ofile << endl;
    print(ofile, idnt);
    ofile << endl;
    break;
  case bv_descr:
    get_bv()->printLong(ofile, idnt);
    break;
  case iv_descr:
    get_iv()->printLong(ofile, idnt);
    break;
  default:
    error("unexpected case");
  }
}

void FiniteDomain::printDebug(void) const
{
  print(cerr, 0);
  cerr << endl;
  cerr.flush();
}

void FiniteDomain::printDebugLong(void) const
{
  printLong(cerr, 0);
  cerr << endl;
  cerr.flush();
}

// expects valid intervals, ie 0 <= left <= right <= fd_iv_max_elem
int FiniteDomain::initList(int list_len,
                                int * list_left, int * list_right,
                                int list_min, int list_max)
{
  if (list_len == 0) {
    return initEmpty();
  } else if (list_len == 1) {
    min_elem = list_min;
    max_elem = list_max;
    size = max_elem - min_elem + 1;
    setType(fd_descr);
  } else {
    min_elem = list_min;
    max_elem = list_max;

    if (list_max <= fd_bv_max_elem) {
      FDBitVector * bv = provideBitVector();
      bv->initList(list_len, list_left, list_right);
      size = bv->findSize();
      setType(bv);
    } else {
      int new_len = simplify(list_len, list_left, list_right);
      FDIntervals * iv = provideIntervals(new_len);
      iv->initList(new_len, list_left, list_right);
      size = iv->findSize();
      setType(iv);
    }
  }
  if (isSingleInterval()) setType(fd_descr);
  return size;
}

static
// int intcompare(int ** i, int ** j) {return(**i - **j);}
int intcompare(const void * ii, const void  * jj) {
  int **i = (int **) ii;
  int **j = (int **) jj;
  return(**i - **j);
}

int FiniteDomain::simplify(int list_len, int * list_left, int * list_right)
{
  for (int i = list_len; i--; ) {
    fd_iv_left_sort[i] = list_left + i;
    fd_iv_right_sort[i] = list_right + i;
  }
  qsort(fd_iv_left_sort, list_len, sizeof(int **), intcompare);
  qsort(fd_iv_right_sort, list_len, sizeof(int **), intcompare);

  int len, p;
  for (len = 0, p = 0; p < list_len; len += 1) {
    if (p > len) {
      *fd_iv_left_sort[len] = *fd_iv_left_sort[p];
      *fd_iv_right_sort[len] = *fd_iv_right_sort[p];
    }
    while (++p < list_len && *fd_iv_right_sort[len] >= *fd_iv_left_sort[p])
      if (*fd_iv_right_sort[p] > *fd_iv_right_sort[len])
        *fd_iv_right_sort[len] = *fd_iv_right_sort[p];
  }
  return len;
}


int FiniteDomain::nextBiggerElem(int v) const
{
  descr_type type = getType();
  if (type == fd_descr) {
    if (v == max_elem) {
      error("no bigger element in domain");
      return -1;
    }
    return v + 1;
  } else if (type == bv_descr) {
    return get_bv()->nextBiggerElem(v, max_elem);
  } else {
    return get_iv()->nextBiggerElem(v, max_elem);
  }
}

Bool FiniteDomain::next(int i, int &n) const
{
  if (i <= min_elem) {
    n = min_elem;
    return FALSE;
  } else if (i >= max_elem) {
    n = max_elem;
    return FALSE;
  }

  descr_type type = getType();
  if (type == fd_descr) {
    n = i;
    return FALSE;
  } else if (type == bv_descr) {
    return get_bv()->next(i, n);
  } else {
    return get_iv()->next(i, n);
  }
}

TaggedRef FiniteDomain::getAsList(void) const
{
  if (size == 0) return AtomNil;

  descr_type type = getType();
  if (type == fd_descr) {
    return makeTaggedLTuple(new LTuple(mkTuple(min_elem, max_elem), AtomNil));
  } else if (type == bv_descr) {
    return get_bv()->getAsList();
  } else {
    return get_iv()->getAsList();
  }
}

int FiniteDomain::operator &= (const int i)
{
  DEBUG_FD_IR(FALSE, cout << *this << " &= " << i << " = ");
  if (contains(i)) {
    initSingleton(i);
    AssertFD(isConsistent());
    DEBUG_FD_IR(FALSE, cout << *this << endl);
    return 1;
  } else {
    initEmpty();
    AssertFD(isConsistent());
    DEBUG_FD_IR(FALSE, cout << *this << endl);
    return 0;
  }
}

int FiniteDomain::operator <= (const int leq)
{
  DEBUG_FD_IR(FALSE, cout << *this  << " <= " << leq << " = ");

  if (leq < min_elem) {
    AssertFD(isConsistent());
    DEBUG_FD_IR(FALSE, cout << "{ - empty -}" << endl);
    return initEmpty();
  } else if (leq < max_elem) {
    descr_type type = getType();
    if (type == fd_descr) {
      max_elem = min(max_elem, leq);
      size = findSize();
    } else if (type == bv_descr) {
      if (leq <= fd_bv_max_elem) {
        FDBitVector * bv = get_bv();
        size = (*bv <= leq);
        if (size > 0) max_elem = bv->findMaxElem();
      }
    } else if (leq <= fd_iv_max_elem) {
      FDIntervals * iv = get_iv();
      size = (*iv <= leq);
      if (size > 0) max_elem = iv->findMaxElem();
    }
  }
  if (isSingleInterval()) setType(fd_descr);
  AssertFD(isConsistent());
  DEBUG_FD_IR(FALSE, cout << *this << endl);
  return size;
}

int FiniteDomain::operator >= (const int geq)
{
  DEBUG_FD_IR(FALSE, cout << *this  << " >= " << geq << " = ");

  if (geq > max_elem) {
    AssertFD(isConsistent());
    DEBUG_FD_IR(FALSE, cout << "{ - empty -}" << endl);
    return initEmpty();
  } else if (geq > min_elem) {
    descr_type type = getType();
    if (type == fd_descr) {
      min_elem = max(min_elem, geq);
      size = findSize();
    } else if (type == bv_descr) {
      FDBitVector * bv = get_bv();
      size = (geq > fd_bv_max_elem) ? initEmpty() : (*bv >= geq);
      if (size > 0) min_elem = bv->findMinElem();
    } else {
      FDIntervals * iv = get_iv();
      size = (geq > fd_iv_max_elem) ? initEmpty() : (*iv >= geq);
      if (size > 0) min_elem = iv->findMinElem();
    }
  }
  if (isSingleInterval()) setType(fd_descr);
  AssertFD(isConsistent());
  DEBUG_FD_IR(FALSE, cout << *this << endl);
  return size;
}

int FiniteDomain::operator -= (const int take_out)
{
  DEBUG_FD_IR(FALSE, cout << *this << " -= " << take_out << " = ");
  if (contains(take_out)) {
    descr_type type = getType();
    if (type == fd_descr) {
      if (take_out == min_elem) {
        min_elem += 1;
      } else if (take_out == max_elem) {
        max_elem -= 1;
      } else {
        if (max_elem <= fd_bv_max_elem) {
          FDBitVector * bv = provideBitVector();
          bv->setFromTo(min_elem, max_elem);
          bv->resetBit(take_out);
          min_elem = bv->findMinElem();
          max_elem = bv->findMaxElem();
          setType(bv);
        } else {
          FDIntervals * iv = provideIntervals(2);
          iv->init(min_elem, take_out - 1, take_out + 1, max_elem);
          setType(iv);
        }
      }
    } else if (type == bv_descr) {
      FDBitVector * bv = get_bv();
      bv->resetBit(take_out);
      min_elem = bv->findMinElem();
      max_elem = bv->findMaxElem();
    } else {
      FDIntervals * iv = (*get_iv() -= take_out);
      min_elem = iv->findMinElem();
      max_elem = iv->findMaxElem();
      setType(iv);
    }
    size -= 1;
    if (isSingleInterval()) setType(fd_descr);
  }
  AssertFD(isConsistent());
  DEBUG_FD_IR(FALSE, cout << *this << endl);
  return size;
}

int FiniteDomain::operator -= (const FiniteDomain &y)
{
  DEBUG_FD_IR(FALSE, cout << *this << " -= " << y << " = ");
  if (y != fd_empty) {
    descr_type x_type = getType(), y_type = y.getType();
    if (x_type == fd_descr) {
      if (y_type == fd_descr) {
        if (y.max_elem < min_elem || max_elem < y.min_elem) {
          AssertFD(isConsistent());
          DEBUG_FD_IR(FALSE, cout << *this << endl);
          return size;
        } else if (y.min_elem <= min_elem && max_elem <= y.max_elem) {
          size = 0;
        } else if (min_elem < y.min_elem && y.max_elem < max_elem) {
          if (max_elem <= fd_bv_max_elem) {
            FDBitVector * bv = provideBitVector();
            bv->setFromTo(min_elem, y.min_elem - 1);
            bv->addFromTo(y.max_elem + 1, max_elem);
            size = bv->findSize();
            setType(bv);
          } else {
            FDIntervals * iv = provideIntervals(2);
            iv->init(min_elem, y.min_elem - 1, y.max_elem + 1, max_elem);
            size = iv->findSize();
            setType(iv);
          }
        } else if (y.max_elem < max_elem) {
          min_elem = y.max_elem + 1;
          size = findSize();
        } else {
          max_elem = y.min_elem - 1;
          size = findSize();
        }
      } else if (y_type == bv_descr) {
        FDBitVector * bv = asBitVector();
        size = (*bv -= *y.get_bv());
        min_elem = bv->findMinElem();
        max_elem = bv->findMaxElem();
        setType(bv);
      } else { // y_type == iv_descr
        FDIntervals * iv = newIntervals(1 + y.get_iv()->high);
        size = asIntervals()->subtract_iv(*iv, *y.get_iv());
        min_elem = iv->findMinElem();
        max_elem = iv->findMaxElem();
        setType(iv);
      }
    } else if (x_type == bv_descr) {
      FDBitVector * bv = get_bv();
      size = (*bv -= *y.asBitVector());
      min_elem = bv->findMinElem();
      max_elem = bv->findMaxElem();
    } else { // x_type == iv_descr
      FDIntervals * y_iv = y.asIntervals();
      FDIntervals * iv = newIntervals(get_iv()->high + y_iv->high);
      size = get_iv()->subtract_iv(*iv, *y_iv);
      min_elem = iv->findMinElem();
      max_elem = iv->findMaxElem();
      setType(iv);
    }

    if (isSingleInterval()) setType(fd_descr);
  }
  AssertFD(isConsistent());
  DEBUG_FD_IR(FALSE, cout << *this << endl);
  return size;
}

int FiniteDomain::operator += (const int put_in)
{
  DEBUG_FD_IR(FALSE, cout << *this << " += " << put_in << " = ");

  if (put_in < 0 || fd_iv_max_elem < put_in) return size;

  if (size == 0) {
    min_elem = max_elem = put_in;
    size = 1;
  } else if (!contains(put_in)) {
    descr_type type = getType();
    if (type == fd_descr) {
      if (put_in == min_elem - 1) {
        min_elem -= 1;
      } else if (put_in == max_elem + 1) {
        max_elem += 1;
      } else {
        if (max(max_elem, put_in) <= fd_bv_max_elem) {
          FDBitVector * bv = asBitVector();
          bv->setBit(put_in);
          min_elem = bv->findMinElem();
          max_elem = bv->findMaxElem();
          setType(bv);
        } else {
          FDIntervals * iv = provideIntervals(2);
          if (put_in < min_elem) {
            iv->init(put_in, put_in, min_elem, max_elem);
          } else {
            iv->init(min_elem, max_elem, put_in, put_in);
          }
          setType(iv);
        }
      }
    } else if (type == bv_descr) {
      if (put_in <= fd_bv_max_elem) {
        FDBitVector * bv = get_bv();
        bv->setBit(put_in);
        min_elem = bv->findMinElem();
        max_elem = bv->findMaxElem();
      } else {
        int c_len = get_bv()->mkRaw(fd_bv_left_conv, fd_bv_right_conv);
        FDIntervals * iv;
        if (put_in == max_elem + 1) {
          iv = provideIntervals(c_len);
          fd_bv_right_conv[c_len - 1] += 1;
          iv->initList(c_len, fd_bv_left_conv, fd_bv_right_conv);
        } else {
          iv = provideIntervals(c_len + 1);
          fd_bv_left_conv[c_len] = fd_bv_right_conv[c_len] = put_in;
          iv->initList(c_len + 1, fd_bv_left_conv, fd_bv_right_conv);
        }
        max_elem = put_in;
        setType(iv);
      }
    } else {
      FDIntervals * iv = (*get_iv() += put_in);
      min_elem = iv->findMinElem();
      max_elem = iv->findMaxElem();
      setType(iv);
    }
    size += 1;
  }
  if (isSingleInterval()) setType(fd_descr);

  AssertFD(isConsistent());
  DEBUG_FD_IR(FALSE, cout << *this << endl);
  return size;
}

FiniteDomain return_var;

FiniteDomain &FiniteDomain::operator ~ (void) const
{
  DEBUG_FD_IR(FALSE, cout << *this << " = ~ ");

  FiniteDomain &y = return_var; y.setEmpty();

  if (*this == fd_empty) {
    y.initFull();
  } else if (*this == fd_full) {
    y.initEmpty();
  } else {
    descr_type type = getType();
    if (type == fd_descr) {
      if (min_elem == 0) {
        y.min_elem = max_elem + 1;
        y.max_elem = fd_iv_max_elem;
        y.size = y.findSize();
      } else if (max_elem == fd_iv_max_elem) {
        y.max_elem = min_elem - 1;
        y.min_elem = 0;
        y.size = y.findSize();
      } else {
        FDIntervals * iv = newIntervals(2);
        iv->init(0, min_elem - 1, max_elem + 1, fd_iv_max_elem);
        y.size = iv->findSize();
        y.min_elem = 0;
        y.max_elem = fd_iv_max_elem;
        y.setType(iv);
      }
    } else {      // reserve one interval too many !!!
      FDIntervals * iv;
      if (type == bv_descr) {
        int s = get_bv()->mkRaw(fd_bv_left_conv, fd_bv_right_conv);
        int t = s + (0 < min_elem);
        iv = newIntervals(t)->complement(s, fd_bv_left_conv, fd_bv_right_conv);
      } else {
        FDIntervals * x_iv = get_iv();
        int s = x_iv->high - 1 + (0 < min_elem) + (max_elem < fd_iv_max_elem);
        iv = newIntervals(s)->complement(x_iv);
      }
      y.size = iv->findSize();
      y.min_elem = iv->findMinElem();
      y.max_elem = iv->findMaxElem();
      y.setType(iv);
      if (y.isSingleInterval()) y.setType(fd_descr);
    }
  }

  AssertFD(y.isConsistent());
  DEBUG_FD_IR(FALSE, cout << y << endl);

  return y;
}

FiniteDomain &FiniteDomain::operator | (const FiniteDomain &y) const
{
  DEBUG_FD_IR(FALSE, cout << *this << " | " << y << " =  ");

  FiniteDomain &z = return_var; z.setEmpty();

  if (*this == fd_empty) {
    z = y;
  } else if (y == fd_empty) {
    z = *this;
  } else if (max(max_elem, y.max_elem) > fd_bv_max_elem) {
    FDIntervals * x_v = asIntervals();
    FDIntervals * y_v = y.asIntervals();
    FDIntervals * z_v;
    z.setType(z_v = newIntervals(x_v->high + y_v->high));
    z.size = z_v->union_iv(*x_v, *y_v);
    z.min_elem = z_v->findMinElem();
    z.max_elem = z_v->findMaxElem();
  } else {
    FDBitVector * x_v = asBitVector();
    FDBitVector * y_v = y.asBitVector();
    FDBitVector * z_v;
    z.setType(z_v = new FDBitVector);
    z.size = z_v->union_bv(*x_v, *y_v);
    z.min_elem = z_v->findMinElem();
    z.max_elem = z_v->findMaxElem();
  }
  if (z.isSingleInterval()) z.setType(fd_descr);

  AssertFD(z.isConsistent());
  DEBUG_FD_IR(FALSE, cout << z << endl);

  return z;
}

FDBitVector * FiniteDomain::asBitVector(void) const
{
  descr_type type = getType();
  if (type == bv_descr) {
    return get_bv();
  } else if (type == fd_descr) {
    FDBitVector * bv = provideBitVector();
    if (min_elem > fd_bv_max_elem)
      bv->setEmpty();
    else
      bv->setFromTo(min_elem, min(fd_bv_max_elem, max_elem));
    return bv;
  } else {
    FDBitVector * bv = new FDBitVector;
    FDIntervals &iv = *get_iv();
    bv->setEmpty();
    for (int i = 0; i < iv.high && iv.i_arr[i].left <= fd_bv_max_elem; i++)
      bv->addFromTo(iv.i_arr[i].left, min(iv.i_arr[i].right, fd_bv_max_elem));
    return bv;
  }
}

FDIntervals * FiniteDomain::asIntervals(void) const
{
  descr_type type = getType();
  if (type == iv_descr) {
    return get_iv();
  } else if (type == bv_descr) {
    if (isSingleInterval()) {
      FDIntervals * iv = provideIntervals(1);
      iv->init(min_elem, max_elem);
      return iv;
    } else {
      int s = get_bv()->mkRaw(fd_bv_left_conv, fd_bv_right_conv);
      FDIntervals * iv = newIntervals(s);
      iv->initList(s, fd_bv_left_conv, fd_bv_right_conv);
      return iv;
    }
  } else {
    FDIntervals * iv = provideIntervals(1);
    iv->init(min_elem, max_elem);
    return iv;
  }
}

int FiniteDomain::operator &= (const FiniteDomain &y)
{
  DEBUG_FD_IR(FALSE, cout << *this << " &= " << y << " = ");

  if (*this == fd_empty || y == fd_empty) {
    initEmpty();
    AssertFD(isConsistent());
    DEBUG_FD_IR(FALSE, cout << "{ - empty -}" << endl);
    return 0;
  } else if (getType() == fd_descr && y.getType() == fd_descr) {
    if (max_elem < y.min_elem || y.max_elem < min_elem) {
      size = 0;
    } else {
      min_elem = max(min_elem, y.min_elem);
      max_elem = min(max_elem, y.max_elem);
      size = findSize();
    }
  } else if (min(max_elem, y.max_elem) > fd_bv_max_elem) {
    FDIntervals * x_i = asIntervals();
    FDIntervals * y_i = y.asIntervals();
    FDIntervals * z_i = newIntervals(x_i->high + y_i->high - 1);

    size = x_i->intersect_iv(*z_i, *y_i);
    min_elem = z_i->findMinElem();
    max_elem = z_i->findMaxElem();
    setType(z_i);
  } else {
    FDBitVector * x_b = asBitVector();
    FDBitVector * y_b = y.asBitVector();

    size = x_b->intersect_bv(*x_b, *y_b);
    min_elem = x_b->findMinElem();
    max_elem = x_b->findMaxElem();
    setType(x_b);
  }

  if (isSingleInterval()) setType(fd_descr);

  AssertFD(isConsistent());
  DEBUG_FD_IR(FALSE, cout << *this << endl);

  return size;
}

FiniteDomain &FiniteDomain::operator & (const FiniteDomain &y) const
{
  DEBUG_FD_IR(FALSE, cout << *this << " & " << y << " = ");

  FiniteDomain &z = return_var; z.setEmpty();

  if (*this == fd_empty || y == fd_empty) {
    AssertFD(z.isConsistent());
    DEBUG_FD_IR(FALSE, cout << "{ - empty -}" << endl);
    return z;
  } else if (getType() == fd_descr && y.getType() == fd_descr) {
    if (max_elem < y.min_elem || y.max_elem < min_elem) {
      z.size = 0;
    } else {
      z.min_elem = max(min_elem, y.min_elem);
      z.max_elem = min(max_elem, y.max_elem);
      z.size = z.findSize();
    }
  } else if (min(max_elem, y.max_elem) > fd_bv_max_elem) {
    FDIntervals * x_i = asIntervals();
    FDIntervals * y_i = y.asIntervals();
    FDIntervals * z_i = newIntervals(x_i->high + y_i->high - 1);

    z.size = x_i->intersect_iv(*z_i, *y_i);
    z.min_elem = z_i->findMinElem();
    z.max_elem = z_i->findMaxElem();
    z.setType(z_i);
  } else {
    FDBitVector * x_b = asBitVector();
    FDBitVector * y_b = y.asBitVector();
    FDBitVector * z_b = new FDBitVector;

    z.size = x_b->intersect_bv(*z_b, *y_b);
    z.min_elem = z_b->findMinElem();
    z.max_elem = z_b->findMaxElem();
    z.setType(z_b);
  }

  if (z.isSingleInterval()) z.setType(fd_descr);

  AssertFD(z.isConsistent());
  DEBUG_FD_IR(FALSE, cout << z << endl);

  return z;
}
