void fd_start(void) {;}

#ifndef NDEBUG
#include <stdio.h>
#endif

#include <mozart_cpi.hh>

#define FailOnEmpty(X) if((X) == 0) goto failure;

//-------------
// class PropXY

class PropXY : public OZ_Propagator {
protected:
  OZ_Term _x, _y;
public:
  PropXY(OZ_Term x, OZ_Term y) : _x(x), _y(y) {}

  virtual size_t sizeOf() {
    return sizeof(PropXY);
  }

  virtual void updateHeapRefs(OZ_Boolean) {
    OZ_updateHeapTerm(_x);
    OZ_updateHeapTerm(_y);
  }

  virtual OZ_Term getParameters() const {
    return OZ_cons(_x, OZ_cons(_y, OZ_nil()));
  }
};

//-----------------
// class LessEqProp

class LessEqProp : public PropXY {
private:
  static OZ_PropagatorProfile profile;

public:
  LessEqProp(OZ_Term x, OZ_Term y)
    : PropXY(x, y) {};

  virtual OZ_Return propagate();

  virtual OZ_PropagatorProfile *getProfile() const {
    return &profile;
  }
};

OZ_PropagatorProfile LessEqProp::profile;

OZ_Return LessEqProp::propagate()
{
  if (mayBeEqualVars() && OZ_isEqualVars(_x, _y))
    return OZ_ENTAILED;

  OZ_FDIntVar x(_x), y(_y);

  FailOnEmpty(*x <= y->getMaxElem());
  FailOnEmpty(*y >= x->getMinElem());

  if (x->getMaxElem() <= y->getMinElem()) {
    x.leave();
    y.leave();
    return OZ_ENTAILED;
  }
  if (x->getMinElem() > y->getMaxElem())
    goto failure;

  x.leave();
  y.leave();
  return OZ_SLEEP;

failure:
  x.fail();
  y.fail();
  return OZ_FAILED;
}

OZ_C_proc_begin(fd_lessEq, 2)
{
  OZ_EXPECTED_TYPE(OZ_EM_FD","OZ_EM_FD);

  OZ_Expect pe;

  OZ_EXPECT(pe, 0, expectIntVar);
  OZ_EXPECT(pe, 1, expectIntVar);

  return pe.impose(new LessEqProp(OZ_args[0],
                                  OZ_args[1]));
}
OZ_C_proc_end



//------------------
// class GreaterProp

class GreaterProp : public PropXY {
private:
  static OZ_PropagatorProfile profile;

public:
  GreaterProp(OZ_Term x, OZ_Term y)
    : PropXY(x, y) {};

  virtual OZ_Return propagate();

  virtual OZ_PropagatorProfile *getProfile() const {
    return &profile;
  }
};

OZ_PropagatorProfile GreaterProp::profile;

OZ_Return GreaterProp::propagate()
{
  if (mayBeEqualVars() && OZ_isEqualVars(_x, _y))
    return OZ_FAILED;

  OZ_FDIntVar x(_x), y(_y);

  FailOnEmpty(*y <= x->getMaxElem() - 1);
  FailOnEmpty(*x >= y->getMinElem() + 1);

  if (x->getMinElem() > y->getMaxElem()) {
    x.leave();
    y.leave();
    return OZ_ENTAILED;
  }
  if (x->getMaxElem() <= y->getMinElem())
    goto failure;

  x.leave();
  y.leave();
  return OZ_SLEEP;

failure:
  x.fail();
  y.fail();
  return OZ_FAILED;
}

OZ_C_proc_begin(fd_greater, 2)
{
  OZ_EXPECTED_TYPE(OZ_EM_FD","OZ_EM_FD);

  OZ_Expect pe;

  OZ_EXPECT(pe, 0, expectIntVar);
  OZ_EXPECT(pe, 1, expectIntVar);

  return pe.impose(new GreaterProp(OZ_args[0],
                                   OZ_args[1]));
}
OZ_C_proc_end

//------------------------
// class ReifiedLessEqProp

class ReifiedLessEqProp : public OZ_Propagator {
private:
  static OZ_PropagatorProfile profile;
  OZ_Term _x, _y, _r;
public:
  ReifiedLessEqProp(OZ_Term x, OZ_Term y, OZ_Term r)
    : _x(x), _y(y), _r(r) {}

  virtual OZ_Return propagate();

  virtual size_t sizeOf() {
    return sizeof(ReifiedLessEqProp);
  }

  virtual void updateHeapRefs(OZ_Boolean) {
    OZ_updateHeapTerm(_x);
    OZ_updateHeapTerm(_y);
    OZ_updateHeapTerm(_r);
  }

  virtual OZ_Term getParameters() const {
    return OZ_cons(_x, OZ_cons(_y, OZ_cons(_r,
                                           OZ_nil())));
  }

  virtual OZ_PropagatorProfile *getProfile() const {
    return &profile;
  }
};

OZ_PropagatorProfile ReifiedLessEqProp::profile;

OZ_Return ReifiedLessEqProp::propagate()
{
  OZ_FDIntVar r(_r);

  if(*r == fd_singl) {
    r.leave();
    return replaceBy((r->getSingleElem() == 1)
                     ? new LessEqProp(_x, _y)
                     : new GreaterProp(_x, _y));
  }

  OZ_FDIntVar x, y;
  x.readEncap(_x); y.readEncap(_y);
  int r_val = 0;

  // entailed by store?
  if (x->getMaxElem() <= y->getMinElem()) {
    r_val = 1;
    goto quit;
  }

  if (0 == (*x <= y->getMaxElem())) goto quit;
  if (0 == (*y >= x->getMinElem())) goto quit;

  r.leave(); x.leave(); y.leave();
  return OZ_SLEEP;

quit:
  if(0 == (*r &= r_val)) {
    r.fail(); x.fail(); y.fail();
    return OZ_FAILED;
  }

  r.leave(); x.leave(); y.leave();
  return OZ_ENTAILED;
}


OZ_C_proc_begin(fd_reifiedLessEq, 3)
{
  OZ_EXPECTED_TYPE(OZ_EM_FD","OZ_EM_FD","OZ_EM_FD);

  OZ_Expect pe;

  OZ_EXPECT(pe, 0, expectIntVar);
  OZ_EXPECT(pe, 1, expectIntVar);
  OZ_EXPECT(pe, 2, expectIntVar);

  return pe.impose(new ReifiedLessEqProp(OZ_args[0],
                                         OZ_args[1],
                                         OZ_args[2]));
}
OZ_C_proc_end


//-----------------
// function fd_init

OZ_C_proc_begin(fd_init, 0)
{
#ifndef NDEBUG
  printf("fd_start=0x%p\n", (void *) fd_start);
  fflush(stdout);
#endif

  return PROCEED;
}
OZ_C_proc_end

OZ_BI_proto(fd_init);
OZ_BI_proto(fd_lessEq);
OZ_BI_proto(fd_greater);
OZ_BI_proto(fd_reifiedLesseq);

extern "C"
{

  OZ_C_proc_interface *oz_init_module(void)
  {
    static OZ_C_proc_interface i_table[] = {
      {"init", 0, 0, fd_init},
      {"lessEq", 2, 0, fd_lessEq},
      {"greater", 2, 0, fd_greater},
      {"reifiedLessEq", 3, 0, fd_reifiedLessEq},
      {0,0,0,0}
    };

    printf("reified propagators loaded\n");
    return i_table;
  }
}
