/*
 *  Authors:
 *    Michael Mehl (mehl@dfki.de)
 * 
 *  Contributors:
 *    Tobias Mueller (tmueller@ps.uni-sb.de)
 *    Denys Duchier (duchier@ps.uni-sb.de)
 * 
 *  Copyright:
 *    Michael Mehl (1997)
 * 
 *  Last change:
 *    $Date$ by $Author$
 *    $Revision$
 * 
 *  This file is part of Mozart, an implementation 
 *  of Oz 3:
 *     $MOZARTURL$
 * 
 *  See the file "LICENSE" or
 *     $LICENSEURL$
 *  for information on usage and redistribution 
 *  of this file, and for a DISCLAIMER OF ALL 
 *  WARRANTIES.
 *
 */

#include "value.hh"
#include "runtime.hh"

class IONode {
public:
  int fd;
  OZ_IOHandler handler[2];
  void *readwritepair[2];
  IONode *next;
  IONode(int f, IONode *nxt): fd(f), next(nxt) {
    handler[0] = handler[1] = 0;
    readwritepair[0] = readwritepair[1] = 0;
  }
};

static
IONode *ioNodes = NULL;

static
IONode *findIONode(int fd)
{
  IONode *aux = ioNodes;
  while(aux) {
    if (aux->fd == fd) return aux;
    aux = aux->next;
  }
  ioNodes = new IONode(fd,ioNodes);
  return ioNodes;
}

static
int hasPendingSelect()
{
  IONode *aux = ioNodes;
  while(aux) {
    if (aux->handler[SEL_READ] || aux->handler[SEL_WRITE]) return OK;
    aux = aux->next;
  }
  return NO;
}

void oz_io_select(int fd, int mode, OZ_IOHandler fun, void *val)
{
  if (!oz_onToplevel()) {
    warning("select only on toplevel");
    return;
  }
  IONode *ion = findIONode(fd);
  ion->readwritepair[mode]=val;
  ion->handler[mode]=fun;
  osWatchFD(fd,mode);
}


void oz_io_acceptSelect(int fd, OZ_IOHandler fun, void *val)
{
  if (!oz_onToplevel()) {
    warning("select only on toplevel");
    return;
  }

  IONode *ion = findIONode(fd);
  ion->readwritepair[SEL_READ]=val;
  ion->handler[SEL_READ]=fun;
  osWatchAccept(fd);
}

void oz_io_awakeVar(TaggedRef var)
{
  Assert(oz_onToplevel());
  Assert(oz_isCons(var));

  OZ_unifyInThread(OZ_head(var),OZ_tail(var));
}

static
int oz_io_awake(int, void *var)
{
  oz_io_awakeVar((TaggedRef) var);
  return 1;
}

int oz_io_select(int fd, int mode,TaggedRef l,TaggedRef r)
{
  if (!oz_onToplevel()) {
    warning("select only on toplevel");
    return OK;
  }
  if (osTestSelect(fd,mode)==1) {
    OZ_unifyInThread(l,r);
    return OK;
  }
  IONode *ion = findIONode(fd);
  ion->readwritepair[mode]=(void *) oz_cons(l,r);
  (void) oz_protect((TaggedRef *) &(ion->readwritepair[mode]));

  ion->handler[mode]=oz_io_awake;
  osWatchFD(fd,mode);
  return OK;
}

void oz_io_acceptSelect(int fd,TaggedRef l,TaggedRef r)
{
  if (!oz_onToplevel()) {
    warning("acceptSelect only on toplevel");
    return;
  }

  IONode *ion = findIONode(fd);
  ion->readwritepair[SEL_READ]=(void *) oz_cons(l,r);
  (void) oz_protect((TaggedRef *) &(ion->readwritepair[SEL_READ]));

  ion->handler[SEL_READ]=oz_io_awake;
  osWatchAccept(fd);
}

void oz_io_deSelect(int fd,int mode)
{
  osClrWatchedFD(fd,mode);
  IONode *ion = findIONode(fd);
  ion->readwritepair[mode]  = 0;
  (void) oz_unprotect((TaggedRef *) &(ion->readwritepair[mode]));
  ion->handler[mode]  = 0;
}

void oz_io_deSelect(int fd)
{
  oz_io_deSelect(fd,SEL_READ);
  oz_io_deSelect(fd,SEL_WRITE);
}

// called if IOReady (signals are blocked)
void oz_io_handle()
{
  am.unsetSFlag(IOReady);
  int numbOfFDs = osFirstSelect();

  // find the nodes to awake
  for (int index = 0; numbOfFDs > 0; index++) {
    for(int mode=SEL_READ; mode <= SEL_WRITE; mode++) {

      if (osNextSelect(index, mode) ) {

	numbOfFDs--;

	IONode *ion = findIONode(index);
	if ((ion->handler[mode]) &&  /* Perdio: handlers may do a deselect for other fds*/
	    (ion->handler[mode])(index, ion->readwritepair[mode])) {
	  ion->readwritepair[mode] = 0;
	  (void) oz_unprotect((TaggedRef *)&(ion->readwritepair[mode]));
	  ion->handler[mode] = 0;
	  osClrWatchedFD(index,mode);
	}
      }
    }
  }
}

//
// called from signal handler
void oz_io_check()
{
  int numbOfFDs = osCheckIO();
  if (!am.isCritical() && (numbOfFDs > 0)) {
    am.setSFlag(IOReady);
  }
}
