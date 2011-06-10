## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_srcdir)/include -DFUSERMOUNT_DIR=\"$(bindir)\" \
  -D_FILE_OFFSET_BITS=64 -D_REENTRANT -DFUSE_USE_VERSION=26

lib_LTLIBRARIES = libfuse4x.la

if !DARWIN
lib_LTLIBRARIES += libulockmgr.la
endif

if BSD
mount_source = mount_bsd.c
extra_source =
endif

if DARWIN
mount_source = mount_darwin.c
extra_source = modules/threadid.c modules/volicon.c
endif

if LINUX
mount_source = mount.c mount_util.c mount_util.h
extra_source =
endif

if ICONV
iconv_source = modules/iconv.c
else
iconv_source =
endif

libfuse4x_la_SOURCES = 		\
	fuse.c			\
	fuse_i.h		\
	fuse_kern_chan.c	\
	fuse_loop.c		\
	fuse_loop_mt.c		\
	fuse_lowlevel.c		\
	fuse_misc.h		\
	fuse_mt.c		\
	fuse_opt.c		\
	fuse_session.c		\
	fuse_signals.c		\
	cuse_lowlevel.c		\
	helper.c		\
	modules/subdir.c	\
	$(extra_source)		\
	$(iconv_source)		\
	$(mount_source)

if !DARWIN
libfuse4x_la_LDFLAGS = @libfuse_libs@ -version-number 2:8:5 \
	-Wl,--version-script,$(srcdir)/fuse_versionscript

libulockmgr_la_SOURCES = ulockmgr.c
libulockmgr_la_LDFLAGS = -pthread -version-number 1:0:1

else
libfuse4x_la_LDFLAGS = -version-number 2:8:5
endif

EXTRA_DIST = fuse_versionscript
