## Process this file with automake to produce Makefile.in

fuseincludedir=$(includedir)/fuse

if DARWIN
darwin_headers = fuse_version.h
cuse_headers =
ulockmgr_headers =
else
darwin_headers =
cuse_headers = cuse_lowlevel.h
ulockmgr_headers = ulockmgr.h
endif

fuseinclude_HEADERS = 		\
	$(darwin_headers)		\
	fuse.h					\
	fuse_compat.h			\
	fuse_common.h			\
	fuse_common_compat.h    \
	fuse_lowlevel.h			\
	fuse_lowlevel_compat.h	\
	fuse_opt.h				\
	$(cuse_headers)

include_HEADERS = old/fuse.h $(ulockmgr_headers)

noinst_HEADERS = fuse_kernel.h
