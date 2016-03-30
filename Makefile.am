EXTRA_DIST =
CLEANFILES =
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AM_MAKEFLAGS = --no-print-directory

AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-I${top_srcdir}/src

AM_CFLAGS = ${my_CFLAGS} \
	-fvisibility=hidden \
	-ffunction-sections \
	-fdata-sections

AM_LDFLAGS = \
	-Wl,--gc-sections \
	-Wl,--as-needed

SED_PROCESS = \
	$(AM_V_GEN)$(MKDIR_P) $(dir $@) && $(SED) \
	-e 's,@VERSION\@,$(VERSION),g' \
	-e 's,@prefix\@,$(prefix),g' \
	-e 's,@exec_prefix\@,$(exec_prefix),g' \
	-e 's,@libdir\@,$(libdir),g' \
	-e 's,@includedir\@,$(includedir),g' \
	< $< > $@ || rm $@

%.pc: %.pc.in Makefile
	$(SED_PROCESS)

LIBI8X_CURRENT=0
LIBI8X_REVISION=0
LIBI8X_AGE=0

pkginclude_HEADERS = src/i8x/libi8x.h
lib_LTLIBRARIES = src/libi8x.la

src_libi8x_la_SOURCES =\
	src/libi8x-private.h \
	src/libi8x.c

EXTRA_DIST += src/libi8x.sym

src_libi8x_la_LDFLAGS = $(AM_LDFLAGS) \
	-version-info $(LIBI8X_CURRENT):$(LIBI8X_REVISION):$(LIBI8X_AGE) \
	-Wl,--version-script=$(top_srcdir)/src/libi8x.sym
src_libi8x_la_DEPENDENCIES = ${top_srcdir}/src/libi8x.sym

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = src/libi8x.pc
EXTRA_DIST += src/libi8x.pc.in
CLEANFILES += src/libi8x.pc

TESTS = src/test-libi8x

check_PROGRAMS = src/test-libi8x
src_test_libi8x_SOURCES = src/test-libi8x.c
src_test_libi8x_LDADD = src/libi8x.la
