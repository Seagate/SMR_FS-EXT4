#
# Standard e2fsprogs prologue....
#

srcdir = .
top_srcdir = ..

top_builddir = ..
my_dir = resize
INSTALL = /usr/bin/install -c
LDFLAG_STATIC = -static

# Beginning of file MCONFIG

all::

check::

SHELL = /bin/sh

COMPRESS_EXT = gz bz2 bz Z

prefix = /usr
root_prefix = 
exec_prefix = ${prefix}
root_bindir = ${root_prefix}/bin
root_sbindir = ${root_prefix}/sbin
root_libdir = ${root_prefix}/lib
datarootdir = ${prefix}/share
bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libdir = ${exec_prefix}/lib
datadir= ${datarootdir}
localedir = $(datadir)/locale
root_sysconfdir= ${root_prefix}/etc
includedir = ${prefix}/include
mandir = ${datarootdir}/man
man1dir = $(mandir)/man1
man3dir = $(mandir)/man3
man5dir = $(mandir)/man5
man8dir = $(mandir)/man8
infodir = ${datarootdir}/info
datadir = ${datarootdir}
pkgconfigdir = $(libdir)/pkgconfig



 V =
 ifeq ($(strip $(V)),)
 #  E = @echo
 #  ES = echo
 #  Q = @
    E = @echo
    ES = echo
    Q = @
 else
    E = @\#
    ES = \#
    Q =
 endif

# E = @echo
# ES = echo
# Q = @

 CHECK=sparse
 CHECK_OPTS=-Wsparse-all -Wno-transparent-union -Wno-return-void -Wno-undef -Wno-non-pointer-null
 CPPCHECK=cppcheck
 CPPCHECK_OPTS=--force --enable=all --quiet
 ifeq ("$(C)", "2")
   CHECK_CMD=$(CHECK) $(CHECK_OPTS) -Wbitwise -D__CHECK_ENDIAN__
   CPPCHECK_CMD=$(CPPCHECK) $(CPPCHECK_OPTS)
 else
   ifeq ("$(C)", "1")
     CHECK_CMD=$(CHECK) $(CHECK_OPTS)
     CPPCHECK_CMD=$(CPPCHECK) $(CPPCHECK_OPTS)
    else
     CHECK_CMD=@true
     CPPCHECK_CMD=@true
   endif
 endif

# CHECK_CMD=@true
# CPPHECK_CMD=@true

CC = gcc
BUILD_CC = gcc
CFLAGS = -g -O2
CPPFLAGS = -I. -I$(top_builddir)/lib -I$(top_srcdir)/lib
ALL_CFLAGS = $(CPPFLAGS) $(CFLAGS) -DHAVE_CONFIG_H $(LOCAL_CFLAGS)
LDFLAGS = 
ALL_LDFLAGS = $(LDFLAGS) 
LDFLAGS_STATIC = $(LDFLAGS) -static
BUILD_CFLAGS = -g -O2  -I. -I$(top_builddir)/lib -I$(top_srcdir)/lib -DHAVE_CONFIG_H
BUILD_LDFLAGS = 
RDYNAMIC = -rdynamic
LINK_BUILD_FLAGS = 
LINK_INSTALL_FLAGS = -f
RM = /usr/bin/rm
LN = /usr/bin/ln
LN_S = ln -s
MV = /usr/bin/mv
CP = /usr/bin/cp
CHMOD = /usr/bin/chmod
AR = ar
AWK = gawk
SED = /usr/bin/sed
PERL = /usr/bin/perl
RANLIB = ranlib
STRIP = strip
LD = $(PURE) gcc
ARUPD = $(AR) r
ARGEN = $(AR) rc
LDCONFIG = /usr/bin/ldconfig
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL}
MKINSTALLDIRS = $(top_builddir)/config/mkinstalldirs

#
# Library definitions
#
LIB = $(top_builddir)/lib
LIBSS = $(LIB)/libss.a  -ldl
LIBCOM_ERR = $(LIB)/libcom_err.a  -lpthread
LIBE2P = $(LIB)/libe2p.a
LIBEXT2FS = $(LIB)/libext2fs.a
LIBUUID = $(LIB)/libuuid.a 
LIBMAGIC = -ldl
LIBQUOTA = $(LIB)/libquota.a
LIBBLKID = $(LIB)/libblkid.a  $(LIBUUID)
LIBINTL = 
SYSLIBS = 
DEPLIBSS = $(LIB)/libss.a
DEPLIBCOM_ERR = $(LIB)/libcom_err.a
DEPLIBUUID = $(LIB)/libuuid.a
DEPLIBQUOTA = $(LIB)/libquota.a
DEPLIBBLKID = $(LIB)/libblkid.a  $(DEPLIBUUID)
TESTENV = LD_LIBRARY_PATH="$(LIB):$${LD_LIBRARY_PATH}" DYLD_LIBRARY_PATH="$(LIB):$${DYLD_LIBRARY_PATH}"

STATIC_LIBSS = $(LIB)/libss.a -ldl
STATIC_LIBCOM_ERR = $(LIB)/libcom_err.a -lpthread
STATIC_LIBE2P = $(LIB)/libe2p.a
STATIC_LIBEXT2FS = $(LIB)/libext2fs.a
STATIC_LIBUUID = $(LIB)/libuuid.a 
STATIC_LIBQUOTA = $(LIB)/libquota.a
STATIC_LIBBLKID = $(LIB)/libblkid.a $(STATIC_LIBUUID)
DEPSTATIC_LIBSS = $(LIB)/libss.a
DEPSTATIC_LIBCOM_ERR = $(LIB)/libcom_err.a
DEPSTATIC_LIBUUID = $(LIB)/libuuid.a
DEPSTATIC_LIBQUOTA = $(LIB)/libquota.a
DEPSTATIC_LIBBLKID = $(LIB)/libblkid.a $(DEPSTATIC_LIBUUID)

PROFILED_LIBSS = $(LIB)/libss.a -ldl
PROFILED_LIBCOM_ERR = $(LIB)/libcom_err.a -lpthread
PROFILED_LIBE2P = $(LIB)/libe2p.a
PROFILED_LIBEXT2FS = $(LIB)/libext2fs.a
PROFILED_LIBUUID = $(LIB)/libuuid.a 
PROFILED_LIBQUOTA = $(LIB)/libquota.a
PROFILED_LIBBLKID = $(LIB)/libblkid.a $(PROFILED_LIBUUID)
DEPPROFILED_LIBSS = $(LIB)/libss.a
DEPPROFILED_LIBCOM_ERR = $(LIB)/libcom_err.a
DEPPROFILED_LIBUUID = $(LIB)/libuuid.a
DEPPROFILED_LIBQUOTA = $(LIB)/libquota.a
DEPPROFILED_LIBBLKID = $(LIB)/libblkid.a $(DEPPROFILED_LIBUUID)

#
# A fast substitution command for fixing up man pages, shell scripts, etc.
#
SUBST_CONF=$(top_builddir)/util/subst.conf
SUBSTITUTE= $(top_builddir)/util/subst -f $(SUBST_CONF)
SUBSTITUTE_UPTIME= $(top_builddir)/util/subst -t -f $(SUBST_CONF)
DEP_SUBSTITUTE= $(top_builddir)/util/subst $(SUBST_CONF)

$(top_builddir)/util/subst:
	cd $(top_builddir)/util ; $(MAKE) subst

#
# Script for installing symlinks (for shared libraries)
#
$(top_builddir)/util/install-symlink: $(top_srcdir)/util/install-symlink.in \
			      $(top_builddir)/config.status
	cd $(top_builddir); CONFIG_FILES=util/install-symlink ./config.status
	chmod +x $(top_builddir)/util/install-symlink

$(top_builddir)/util/symlinks:
	cd $(top_builddir)/util ; $(MAKE) symlinks

INSTALL_SYMLINK = /bin/sh $(top_builddir)/util/install-symlink \
			 \
			--symlinks=$(top_builddir)/util/symlinks
DEP_INSTALL_SYMLINK = $(top_builddir)/util/install-symlink \
			$(top_builddir)/util/symlinks

#
# Warning flags
#
# Run make gcc-wall to do a build with warning messages.
#
#
WFLAGS=		-std=gnu99 -D_XOPEN_SOURCE=600 -D_GNU_SOURCE \
			-pedantic $(WFLAGS_EXTRA) \
			-Wall -W -Wwrite-strings -Wpointer-arith \
			-Wcast-qual -Wcast-align -Wno-variadic-macros \
			-Wstrict-prototypes -Wmissing-prototypes \
			-Wformat-security  -Wformat-nonliteral \
			-Wmissing-format-attribute -O2 -Wstrict-aliasing \
			-Wnested-externs -Winline -DNO_INLINE_FUNCS -Wshadow \
			-UENABLE_NLS

gcc-wall-new:
	($(MAKE) CFLAGS="$(ALL_CFLAGS) $(WFLAGS)" > /dev/null) 2>&1 | sed -f $(top_srcdir)/util/gcc-wall-cleanup

gcc-wall:
	$(MAKE) clean > /dev/null
	$(MAKE) gcc-wall-new

static-check:
	($(MAKE) C=1 V=1 CFLAGS="$(ALL_CFLAGS) $(WFLAGS)") 2>&1 | sed -f $(top_srcdir)/util/static-analysis-cleanup

static-check-all:
	$(MAKE) clean > /dev/null
	$(MAKE) static-check

#
# Installation user and groups
#
BINGRP=		bin
BINOWN=		bin
BINMODE=	555
INCGRP=		bin
INCOWN=		bin
INCMODE=	444
LIBOWN=		bin
LIBGRP=		bin
LIBMODE=	444
MANGRP=		bin
MANOWN=		bin
MANMODE=	444

#
# Autoconf magic...
#

DEP_LIB_MAKEFILES = $(top_srcdir)/lib/Makefile.library \
	$(top_srcdir)/lib/Makefile.elf-lib  \
	$(top_srcdir)/lib/Makefile.bsd-lib \
	$(top_srcdir)/lib/Makefile.darwin-lib \
	$(top_srcdir)/lib/Makefile.solaris-lib \
	$(top_srcdir)/lib/Makefile.profile

$(top_builddir)/config.status: $(top_srcdir)/configure
	cd $(top_builddir); ./config.status --recheck

$(top_builddir)/MCONFIG: $(top_srcdir)/MCONFIG.in $(top_builddir)/config.status
	cd $(top_builddir); CONFIG_FILES=MCONFIG ./config.status

$(top_builddir)/lib/config.h: $(top_srcdir)/lib/config.h.in \
			      $(top_builddir)/config.status
	cd $(top_builddir); CONFIG_FILES=lib/config.h ./config.status

$(top_builddir)/lib/dirpaths.h: $(DEP_SUBSTITUTE) $(top_srcdir)/lib/dirpaths.h.in
	$(E) "	SUBST $@"
	$(Q) $(SUBSTITUTE) $(top_srcdir)/lib/dirpaths.h.in $@

$(top_builddir)/lib/substitute_sh: $(top_srcdir)/lib/substitute_sh.in \
		$(top_builddir)/config.status
	cd $(top_builddir); CONFIG_FILES=lib/substitute_sh ./config.status

$(top_builddir)/util/subst.conf: $(top_srcdir)/util/subst.conf.in \
		$(top_builddir)/config.status
	cd $(top_builddir); CONFIG_FILES=util/subst.conf ./config.status

Makefile: $(srcdir)/Makefile.in $(top_builddir)/MCONFIG \
		$(DEP_MAKEFILE) $(top_builddir)/config.status
	cd $(top_builddir); CONFIG_FILES=$(my_dir)/Makefile ./config.status

#$(top_srcdir)/configure: $(top_srcdir)/configure.in
#	cd $(top_srcdir) && autoheader && autoconf

coverage.txt: Makefile $(SRCS)
	if test -n "$(SRCS)"; then \
		gcov -s $(top_srcdir) -o . $(SRCS) > coverage.txt 2>&1 ; \
	fi

clean::
	$(RM) -f *.gcda *.gcov *.gcno coverage.txt

#
# Make depend magic...
#

.depend: Makefile $(SRCS) $(top_srcdir)/depfix.sed $(top_srcdir)/wordwrap.pl
	if test -n "$(SRCS)" ; then \
		$(CC) -M $(ALL_CFLAGS) $(DEPEND_CFLAGS) $(SRCS) | \
			$(SED) -f $(top_srcdir)/depfix.sed \
			    -e 's; $(srcdir)/; $$(srcdir)/;g' \
			    -e 's; $(top_srcdir)/; $$(top_srcdir)/;g' \
			    -e 's; $(top_builddir)/; $$(top_builddir)/;g' \
			    -e 's; \./; ;g' \
			    -e '/^#/d' \
			    -e '/^ *\\$$/d' | \
			$(PERL) $(top_srcdir)/wordwrap.pl > .depend; \
	else :; fi

depend:: .depend
	if test -n "$(SRCS)" ; then \
		sed -e '/^# +++ Dependency line eater +++/,$$d' \
			< $(srcdir)/Makefile.in | cat - .depend \
			> $(srcdir)/Makefile.in.new; \
	if cmp -s $(srcdir)/Makefile.in $(srcdir)/Makefile.in.new ; then \
		$(RM) $(srcdir)/Makefile.in.new ; \
	else \
		$(MV) $(srcdir)/Makefile.in $(srcdir)/Makefile.in.old; \
		$(MV) $(srcdir)/Makefile.in.new $(srcdir)/Makefile.in; \
	fi ; else :; fi

# End of file MCONFIG

PROGS=		resize2fs
TEST_PROGS=	test_extent
MANPAGES=	resize2fs.8

RESIZE_OBJS= extent.o resize2fs.o main.o online.o resource_track.o \
		sim_progress.o

TEST_EXTENT_OBJS= extent.o test_extent.o

SRCS= $(srcdir)/extent.c \
	$(srcdir)/resize2fs.c \
	$(srcdir)/main.c \
	$(srcdir)/online.c \
	$(srcdir)/resource_track.c \
	$(srcdir)/sim_progress.c

LIBS= $(LIBE2P) $(LIBEXT2FS) $(LIBCOM_ERR) $(LIBINTL) $(SYSLIBS)
DEPLIBS= $(LIBE2P) $(LIBEXT2FS) $(DEPLIBCOM_ERR)

STATIC_LIBS= $(STATIC_LIBE2P) $(STATIC_LIBEXT2FS) $(STATIC_LIBCOM_ERR) \
	$(LIBINTL) $(SYSLIBS)
DEPSTATIC_LIBS= $(STATIC_LIBE2P) $(STATIC_LIBEXT2FS) $(DEPSTATIC_LIBCOM_ERR) 

.c.o:
	$(E) "	CC $<"
	$(Q) $(CC) -c $(ALL_CFLAGS) $< -o $@
	$(Q) $(CHECK_CMD) $(ALL_CFLAGS) $<
	$(Q) $(CPPCHECK_CMD) $(CPPFLAGS) $<

all:: $(PROGS) $(TEST_PROGS) $(MANPAGES) 

resize2fs: $(RESIZE_OBJS) $(DEPLIBS)
	$(E) "	LD $@"
	$(Q) $(CC) $(ALL_LDFLAGS) -o resize2fs $(RESIZE_OBJS) $(LIBS) 

resize2fs.static: $(RESIZE_OBJS) $(DEPSTATIC_LIBS)
	$(E) "	LD $@"
	$(Q) $(LD) $(ALL_LDFLAGS) $(LDFLAG_STATIC) -o resize2fs.static \
		$(RESIZE_OBJS) $(STATIC_LIBS) 

resize2fs.8: $(DEP_SUBSTITUTE) $(srcdir)/resize2fs.8.in
	$(E) "	SUBST $@"
	$(Q) $(SUBSTITUTE_UPTIME) $(srcdir)/resize2fs.8.in resize2fs.8

test_extent: $(TEST_EXTENT_OBJS)
	$(E) "	LD $@"
	$(Q) $(CC) $(ALL_LDFLAGS) -o test_extent $(TEST_EXTENT_OBJS) $(LIBS) 

installdirs:
	$(E) "	MKINSTALLDIRS $(root_sbindir) $(man8dir)"
	$(Q) $(MKINSTALLDIRS) $(DESTDIR)$(root_sbindir) \
		$(DESTDIR)$(man8dir)

install: $(PROGS) $(MANPAGES) installdirs
	$(Q) for i in $(PROGS); do \
		echo "	INSTALL $(root_sbindir)/$$i"; \
		$(INSTALL_PROGRAM) $$i $(DESTDIR)$(root_sbindir)/$$i; \
	done
	$(Q) for i in $(MANPAGES); do \
		for j in $(COMPRESS_EXT); do \
			$(RM) -f $(DESTDIR)$(man8dir)/$$i.$$j; \
		done; \
		echo "	INSTALL_DATA $(man8dir)/$$i"; \
		$(INSTALL_DATA) $$i $(DESTDIR)$(man8dir)/$$i; \
	done

install-strip: install
	$(Q) for i in $(PROGS); do \
		echo "	STRIP $(root_sbindir)/$$i"; \
		$(STRIP) $(DESTDIR)$(root_sbindir)/$$i; \
	done

uninstall:
	for i in $(PROGS); do \
		$(RM) -f $(DESTDIR)$(root_sbindir)/$$i; \
	done
	for i in $(MANPAGES); do \
		$(RM) -f $(DESTDIR)$(man8dir)/$$i; \
	done

test_extent.out: test_extent $(srcdir)/test_extent.in
	$(TESTENV) ./test_extent < $(srcdir)/test_extent.in > test_extent.out

check:: test_extent.out
	$(Q) if cmp -s test_extent.out $(srcdir)/test_extent.in ; then \
		echo "Test succeeded." ; \
	else \
		echo "Test failed!" ; \
		diff test_extent.out $(srcdir)/test_extent.in ; \
		exit 1 ; \
	fi

clean::
	$(RM) -f $(PROGS) $(TEST_PROGS) $(MANPAGES) \#* *.s *.o *.a *~ core \
		resize2fs.static test_extent.out

mostlyclean: clean

distclean: clean
	$(RM) -f .depend Makefile $(srcdir)/TAGS $(srcdir)/Makefile.in.old

#
# Kludge to create a "special" e2fsprogs distribution file.
#

SRCROOT = `echo e2fsprogs-1.43-WIP | sed -e 's/-WIP//' \
			-e 's/pre-//' -e 's/-PLUS//'`
TAR=tar

$(top_srcdir)/.exclude-file:
	a=$(SRCROOT); \
	(cd $(top_srcdir)/.. ; find e2fsprogs \( -name \*~ -o -name \*.orig \
		-o -name CVS -o -name \*.rej \) -print) \
		| sed -e "s/e2fsprogs/$$a/" > $(top_srcdir)/.exclude-file
	echo "$(SRCROOT)/build" >> $(top_srcdir)/.exclude-file
	echo "$(SRCROOT)/rpm.log" >> $(top_srcdir)/.exclude-file
	echo "$(SRCROOT)/powerquest" >> $(top_srcdir)/.exclude-file
	echo "$(SRCROOT)/.exclude-file" >> $(top_srcdir)/.exclude-file
	echo $(SRCROOT)/e2fsprogs-1.43-WIP.tar.gz \
		>> $(top_srcdir)/.exclude-file
	echo $(SRCROOT)/e2fsprogs-ALL-1.43-WIP.tar.gz \
		>> $(top_srcdir)/.exclude-file

source_tar_file: $(top_srcdir)/.exclude-file
	(cd $(top_srcdir)/..; a=$(SRCROOT); rm -f $$a ; ln -sf e2fsprogs $$a ; \
		$(TAR) -c -h -v -f - \
			-X $$a/.exclude-file $$a | \
		gzip -9 > e2fsprogs-ALL-1.43-WIP.tar.gz)
	rm -f $(top_srcdir)/.exclude-file

# +++ Dependency line eater +++
# 
# Makefile dependencies follow.  This must be the last section in
# the Makefile.in file
#
extent.o: $(srcdir)/extent.c $(top_builddir)/lib/config.h \
 $(top_builddir)/lib/dirpaths.h $(srcdir)/resize2fs.h \
 $(top_srcdir)/lib/ext2fs/ext2_fs.h $(top_builddir)/lib/ext2fs/ext2_types.h \
 $(top_srcdir)/lib/ext2fs/ext2fs.h $(top_srcdir)/lib/ext2fs/ext3_extents.h \
 $(top_srcdir)/lib/et/com_err.h $(top_srcdir)/lib/ext2fs/ext2_io.h \
 $(top_builddir)/lib/ext2fs/ext2_err.h \
 $(top_srcdir)/lib/ext2fs/ext2_ext_attr.h $(top_srcdir)/lib/ext2fs/bitops.h \
 $(top_srcdir)/lib/e2p/e2p.h
resize2fs.o: $(srcdir)/resize2fs.c $(top_builddir)/lib/config.h \
 $(top_builddir)/lib/dirpaths.h $(srcdir)/resize2fs.h \
 $(top_srcdir)/lib/ext2fs/ext2_fs.h $(top_builddir)/lib/ext2fs/ext2_types.h \
 $(top_srcdir)/lib/ext2fs/ext2fs.h $(top_srcdir)/lib/ext2fs/ext3_extents.h \
 $(top_srcdir)/lib/et/com_err.h $(top_srcdir)/lib/ext2fs/ext2_io.h \
 $(top_builddir)/lib/ext2fs/ext2_err.h \
 $(top_srcdir)/lib/ext2fs/ext2_ext_attr.h $(top_srcdir)/lib/ext2fs/bitops.h \
 $(top_srcdir)/lib/e2p/e2p.h
main.o: $(srcdir)/main.c $(top_builddir)/lib/config.h \
 $(top_builddir)/lib/dirpaths.h $(top_srcdir)/lib/e2p/e2p.h \
 $(top_srcdir)/lib/ext2fs/ext2_fs.h $(top_builddir)/lib/ext2fs/ext2_types.h \
 $(srcdir)/resize2fs.h $(top_srcdir)/lib/ext2fs/ext2fs.h \
 $(top_srcdir)/lib/ext2fs/ext3_extents.h $(top_srcdir)/lib/et/com_err.h \
 $(top_srcdir)/lib/ext2fs/ext2_io.h $(top_builddir)/lib/ext2fs/ext2_err.h \
 $(top_srcdir)/lib/ext2fs/ext2_ext_attr.h $(top_srcdir)/lib/ext2fs/bitops.h \
 $(top_srcdir)/version.h
online.o: $(srcdir)/online.c $(top_builddir)/lib/config.h \
 $(top_builddir)/lib/dirpaths.h $(srcdir)/resize2fs.h \
 $(top_srcdir)/lib/ext2fs/ext2_fs.h $(top_builddir)/lib/ext2fs/ext2_types.h \
 $(top_srcdir)/lib/ext2fs/ext2fs.h $(top_srcdir)/lib/ext2fs/ext3_extents.h \
 $(top_srcdir)/lib/et/com_err.h $(top_srcdir)/lib/ext2fs/ext2_io.h \
 $(top_builddir)/lib/ext2fs/ext2_err.h \
 $(top_srcdir)/lib/ext2fs/ext2_ext_attr.h $(top_srcdir)/lib/ext2fs/bitops.h \
 $(top_srcdir)/lib/e2p/e2p.h
resource_track.o: $(srcdir)/resource_track.c $(top_builddir)/lib/config.h \
 $(top_builddir)/lib/dirpaths.h $(srcdir)/resize2fs.h \
 $(top_srcdir)/lib/ext2fs/ext2_fs.h $(top_builddir)/lib/ext2fs/ext2_types.h \
 $(top_srcdir)/lib/ext2fs/ext2fs.h $(top_srcdir)/lib/ext2fs/ext3_extents.h \
 $(top_srcdir)/lib/et/com_err.h $(top_srcdir)/lib/ext2fs/ext2_io.h \
 $(top_builddir)/lib/ext2fs/ext2_err.h \
 $(top_srcdir)/lib/ext2fs/ext2_ext_attr.h $(top_srcdir)/lib/ext2fs/bitops.h \
 $(top_srcdir)/lib/e2p/e2p.h
sim_progress.o: $(srcdir)/sim_progress.c $(top_builddir)/lib/config.h \
 $(top_builddir)/lib/dirpaths.h $(srcdir)/resize2fs.h \
 $(top_srcdir)/lib/ext2fs/ext2_fs.h $(top_builddir)/lib/ext2fs/ext2_types.h \
 $(top_srcdir)/lib/ext2fs/ext2fs.h $(top_srcdir)/lib/ext2fs/ext3_extents.h \
 $(top_srcdir)/lib/et/com_err.h $(top_srcdir)/lib/ext2fs/ext2_io.h \
 $(top_builddir)/lib/ext2fs/ext2_err.h \
 $(top_srcdir)/lib/ext2fs/ext2_ext_attr.h $(top_srcdir)/lib/ext2fs/bitops.h \
 $(top_srcdir)/lib/e2p/e2p.h
