#!/bin/sh -x
#
#  Authors:
#    Christian Schulte <schulte@ps.uni-sb.de>
#    Konstantin Popov <kost@sics.se>
#
#  Contributors:
#
#  Copyright:
#    Christian Schulte, 1998
#    Konstantin Popov, 2001
#
#  Last change:
#    $Date$ by $Author$
#    $Revision$
#
#  This file is part of Mozart, an implementation
#  of Oz 3:
#     http://www.mozart-oz.org
#
#  See the file "LICENSE" or
#     http://www.mozart-oz.org/LICENSE.html
#  for information on usage and redistribution
#  of this file, and for a DISCLAIMER OF ALL
#  WARRANTIES.
#

# must be run as root;

#PLAT=$1
#PREFIX=$2
PLAT=`ozplatform`
PREFIX=`pwd`

packageroot="$PREFIX/packages/$PLAT"
build=$PREFIX/build-$PLAT
dst=$PREFIX

PATH=$packageroot/bin:$PATH

echo "Packages in: $packageroot"

use_src=mozart

case $PLAT in
    linux-i486)
        LDFLAGS=-s
    ;;
    solaris-sparc)
        LDFLAGS=-s
    ;;
    *)
        echo "Unknown platform: $PLAT" 2>& 1
        exit 1
    ;;
esac

CXXFLAGS="$CFLAGS"

with_lib_dir="$packageroot/lib"
with_inc_dir="$packageroot/include"
with_tcl="$packageroot/lib"
with_tk="$packageroot/lib"
with_gmp="$packageroot"
with_zlib="$packageroot"
with_gdbm="$packageroot"
with_regex="$packageroot"

# echo $CFLAGS $CXXFLAGS $LDFLAGS
# echo $with_lib_dir $with_inc_dir $with_tcl $with_tk
export PATH CFLAGS CXXFLAGS LDFLAGS
unset CONFIG_SITE
export with_lib_dir with_inc_dir
export with_tcl with_tk with_gmp with_zlib with_gdbm with_regex

#
set -x

#
echo executing "$use_src/misc/create-rpm $build $dst"
$use_src/misc/create-rpm $build $dst
