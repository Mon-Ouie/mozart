DESCRIPTION="Mozart/Oz: a multiparadigm concurrent constraint programming language"
MOZART_TAG="1.2.5.20030125"
SRC_URI="ftp://ftp.mozart-oz.org/pub/mozart/store/1.2.5-2003-01-25/mozart-1.2.5.20030125-src.tar.gz
HOMEPAGE="http://www.mozart-oz.org/"
KEYWORDS="x86 ppc sparc hppa"
DEPEND="virtual/glibc
        virtual/emacs
        sys-apps/bash
        sys-devel/perl
        sys-devel/make
        sys-apps/sh-utils
        sys-devel/gcc
        sys-devel/binutils
        sys-apps/sed
        >=sys-devel/flex-2.5.3
        >=sys-devel/bison-1.25
        sys-devel/m4
        dev-libs/gmp
        sys-libs/zlib
        sys-libs/gdbm
"
RDEPEND="virtual/glibc
        virtual/emacs
        sys-apps/bash
        sys-apps/sh-utils
        sys-devel/gcc
        sys-devel/binutils
        dev-libs/gmp
        sys-libs/zlib
        sys-libs/gdbm
"

S="${WORKDIR}/mozart-${MOZART_TAG}"

src_compile() {
    
    ./configure --prefix=/opt/mozart || die "configure failed"
    make depend || die "make depend failed"
    make bootstrap || die "make bootstrap failed"
}

src_install() {

    # install the system itself
    make install PREFIX=${D}/opt/mozart

    # update env
    insinto /etc/env.d
    doins ${FILESDIR}/99mozart

}
