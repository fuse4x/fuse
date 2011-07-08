/*
 * Copyright (C) fuse4x.org 2011
 *
 * Derived from mount_bsd.c from the fuse distribution.
 *
 *	FUSE: Filesystem in Userspace
 *	Copyright (C) 2005-2006 Csaba Henk <csaba.henk@creo.hu>
 *
 *	This program can be distributed under the terms of the GNU LGPLv2.
 *	See the file COPYING.LIB.
*/

#undef _POSIX_C_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/utsname.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <paths.h>

#include <CoreFoundation/CoreFoundation.h>

#include "fuse_i.h"
#include "fuse_opt.h"
#include "fuse_darwin.h"
#include "fuse_mount.h"
#include "fuse_common_compat.h"

struct mount_options {
	struct fuse_mount_args fuse_args; // mount flags for fuse4x kext
	int standard_args; // standard XNU mount flags, see mount.h
	bool quiet;
	bool allow_recursion;
};

enum {
	KEY_RDONLY,
	KEY_SYNCHRONOUS,
	KEY_NOEXEC,
	KEY_NOSUID,
	KEY_NODEV,
	KEY_UNION,
	KEY_ASYNC,
	KEY_QUARANTINE,
	KEY_LOCAL,
	KEY_QUOTA,
	KEY_ROOTFS,
	KEY_DONTBROWSE,
	KEY_IGNORE_OWNERSHIP,
	KEY_AUTOMOUNTED,
	KEY_JOURNALED,
	KEY_NOUSERXATTR,
	KEY_DEFWRITE,
	KEY_MULTILABEL,
	KEY_NOATIME,

	KEY_ALLOW_OTHER,
	KEY_ALLOW_RECURSION,
	KEY_ALLOW_ROOT,
	KEY_AUTO_XATTR,
	KEY_BLOCKSIZE,
	KEY_DAEMON_TIMEOUT,
	KEY_DEFAULT_PERMISSIONS,
	KEY_DEFER_PERMISSIONS,
	KEY_DIRECT_IO,
	KEY_EXTENDED_SECURITY,
	KEY_FSID,
	KEY_FSNAME,
	KEY_FSSUBTYPE,
	KEY_FSTYPENAME,
	KEY_INIT_TIMEOUT,
	KEY_IOSIZE,
	KEY_JAIL_SYMLINKS,
	KEY_NEGATIVE_VNCACHE,
	KEY_NO_APPLEDOUBLE,
	KEY_NO_APPLEXATTR,
	KEY_NO_ATTRCACHE,
	KEY_NO_LOCALCACHES,
	KEY_NO_READAHEAD,
	KEY_NO_SYNCONCLOSE,
	KEY_NO_SYNCWRITES,
	KEY_NO_UBC,
	KEY_NO_VNCACHE,
	KEY_USE_INO,
	KEY_VOLNAME,
	KEY_PING_DISKARB,
	KEY_AUTO_CACHE,
	KEY_NATIVE_XATTR,
	KEY_SPARSE,
	KEY_QUIET,
	KEY_VOLICON,

	KEY_IGNORED
};

static const struct fuse_opt fuse_mount_opts[] = {
	// Standard flags for XNU
	// See http://fxr.watson.org/fxr/source/bsd/sys/mount.h?v=xnu-1456.1.26#L279
	FUSE_OPT_KEY("-r", KEY_RDONLY),
	FUSE_OPT_KEY("rdonly", KEY_RDONLY),
	FUSE_OPT_KEY("sync", KEY_SYNCHRONOUS),
	FUSE_OPT_KEY("noexec", KEY_NOEXEC),
	FUSE_OPT_KEY("nosuid", KEY_NOSUID),
	FUSE_OPT_KEY("nodev", KEY_NODEV),
	FUSE_OPT_KEY("union", KEY_UNION),
	FUSE_OPT_KEY("async", KEY_ASYNC),
	FUSE_OPT_KEY("quarantine", KEY_QUARANTINE),
	FUSE_OPT_KEY("local", KEY_LOCAL),
	FUSE_OPT_KEY("quota", KEY_QUOTA),
	FUSE_OPT_KEY("rootfs", KEY_ROOTFS),
	FUSE_OPT_KEY("nobrowse", KEY_DONTBROWSE),
	FUSE_OPT_KEY("noowners", KEY_IGNORE_OWNERSHIP),
	FUSE_OPT_KEY("automounted", KEY_AUTOMOUNTED),
	FUSE_OPT_KEY("journaled", KEY_JOURNALED),
	FUSE_OPT_KEY("nouserxattr", KEY_NOUSERXATTR),
	FUSE_OPT_KEY("defwrite", KEY_DEFWRITE),
	FUSE_OPT_KEY("multilabel", KEY_MULTILABEL),
	FUSE_OPT_KEY("noatime", KEY_NOATIME),

	// fuse4x specific mount flags
	FUSE_OPT_KEY("allow_other", KEY_ALLOW_OTHER),
	FUSE_OPT_KEY("allow_recursion", KEY_ALLOW_RECURSION),
	FUSE_OPT_KEY("allow_root", KEY_ALLOW_ROOT),
	FUSE_OPT_KEY("auto_xattr", KEY_AUTO_XATTR),
	FUSE_OPT_KEY("blocksize=", KEY_BLOCKSIZE),
	FUSE_OPT_KEY("daemon_timeout=", KEY_DAEMON_TIMEOUT),
	FUSE_OPT_KEY("default_permissions", KEY_DEFAULT_PERMISSIONS),
	FUSE_OPT_KEY("defer_permissions", KEY_DEFER_PERMISSIONS),
	FUSE_OPT_KEY("direct_io", KEY_DIRECT_IO),
	FUSE_OPT_KEY("extended_security", KEY_EXTENDED_SECURITY),
	FUSE_OPT_KEY("fsid=", KEY_FSID),
	FUSE_OPT_KEY("fsname=", KEY_FSNAME),
	FUSE_OPT_KEY("fssubtype=", KEY_FSSUBTYPE),
	FUSE_OPT_KEY("fstypename=", KEY_FSTYPENAME),
	FUSE_OPT_KEY("init_timeout=", KEY_INIT_TIMEOUT),
	FUSE_OPT_KEY("iosize=", KEY_IOSIZE),
	FUSE_OPT_KEY("jail_symlinks", KEY_JAIL_SYMLINKS),
	FUSE_OPT_KEY("negative_vncache", KEY_NEGATIVE_VNCACHE),
	FUSE_OPT_KEY("noappledouble", KEY_NO_APPLEDOUBLE),
	FUSE_OPT_KEY("noapplexattr", KEY_NO_APPLEXATTR),
	FUSE_OPT_KEY("noattrcache", KEY_NO_ATTRCACHE),
	FUSE_OPT_KEY("nolocalcaches", KEY_NO_LOCALCACHES),
	FUSE_OPT_KEY("noreadahead", KEY_NO_READAHEAD),
	FUSE_OPT_KEY("nosynconclose", KEY_NO_SYNCONCLOSE),
	FUSE_OPT_KEY("nosyncwrites", KEY_NO_SYNCWRITES),
	FUSE_OPT_KEY("noubc", KEY_NO_UBC),
	FUSE_OPT_KEY("novncache", KEY_NO_VNCACHE),
	FUSE_OPT_KEY("use_ino", KEY_USE_INO),
	FUSE_OPT_KEY("volname=", KEY_VOLNAME),
	FUSE_OPT_KEY("volicon=", KEY_VOLICON),
	FUSE_OPT_KEY("ping_diskarb", KEY_PING_DISKARB),
	FUSE_OPT_KEY("auto_cache", KEY_AUTO_CACHE),
	FUSE_OPT_KEY("native_xattr", KEY_NATIVE_XATTR),
	FUSE_OPT_KEY("sparse", KEY_SPARSE),
	FUSE_OPT_KEY("quiet", KEY_QUIET),

	FUSE_OPT_KEY("subtype=", KEY_IGNORED),
	FUSE_OPT_END
};


#define STANDARD_MOUNT_OPT(name) case KEY_ ## name: mo->standard_args |= MNT_ ## name; return 0;
#define FUSE_MOUNT_OPT(name) case KEY_ ## name: mo->fuse_args.altflags |= FUSE_MOPT_ ## name; return 0;

#define FUSE_MOUNT_OPT_PARSE_U32(key, param_name) \
	case KEY_ ## key: \
		if (sscanf(arg, #param_name "=%u", &(param)) < 0) { \
			perror("fuse4x " #param_name " parameter:"); \
			return -1; \
		} \
		mo->fuse_args.param_name = (uint32_t)param; \
		mo->fuse_args.altflags |= FUSE_MOPT_ ## key; \
		return 0;

#define FUSE_MOUNT_OPT_PARSE_STRING(key, param_name, length) \
	case KEY_ ## key: \
		if (strlcpy(mo->fuse_args.param_name, arg+strlen(#param_name "="), length) >= length) { \
			fprintf(stderr, "fuse4x: " #param_name " parameter too long\n"); \
			return -1; \
		}; \
		return 0;


static int fuse_mount_opt_proc(void *data, const char *arg, int key,
		struct fuse_args *outargs)
{
	struct mount_options *mo = data;
	unsigned int param;

	switch (key) {

	STANDARD_MOUNT_OPT(RDONLY)
	STANDARD_MOUNT_OPT(SYNCHRONOUS)
	STANDARD_MOUNT_OPT(NOEXEC)
	STANDARD_MOUNT_OPT(NOSUID)
	STANDARD_MOUNT_OPT(NODEV)
	STANDARD_MOUNT_OPT(UNION)
	STANDARD_MOUNT_OPT(ASYNC)
	STANDARD_MOUNT_OPT(QUARANTINE)
	STANDARD_MOUNT_OPT(LOCAL)
	STANDARD_MOUNT_OPT(QUOTA)
	STANDARD_MOUNT_OPT(ROOTFS)
	STANDARD_MOUNT_OPT(DONTBROWSE)
	STANDARD_MOUNT_OPT(IGNORE_OWNERSHIP)
	STANDARD_MOUNT_OPT(AUTOMOUNTED)
	STANDARD_MOUNT_OPT(JOURNALED)
	STANDARD_MOUNT_OPT(NOUSERXATTR)
	STANDARD_MOUNT_OPT(DEFWRITE)
	STANDARD_MOUNT_OPT(MULTILABEL)
	STANDARD_MOUNT_OPT(NOATIME)

	FUSE_MOUNT_OPT(ALLOW_OTHER)
	FUSE_MOUNT_OPT(ALLOW_ROOT)
	FUSE_MOUNT_OPT(AUTO_XATTR)
	FUSE_MOUNT_OPT(DEFAULT_PERMISSIONS)
	FUSE_MOUNT_OPT(DEFER_PERMISSIONS)
	FUSE_MOUNT_OPT(DIRECT_IO)
	FUSE_MOUNT_OPT(EXTENDED_SECURITY)
	FUSE_MOUNT_OPT(JAIL_SYMLINKS)
	FUSE_MOUNT_OPT(NEGATIVE_VNCACHE)
	FUSE_MOUNT_OPT(NO_APPLEDOUBLE)
	FUSE_MOUNT_OPT(NO_APPLEXATTR)
	FUSE_MOUNT_OPT(NO_ATTRCACHE)
	FUSE_MOUNT_OPT(NO_READAHEAD)
	FUSE_MOUNT_OPT(NO_SYNCONCLOSE)
	FUSE_MOUNT_OPT(NO_SYNCWRITES)
	FUSE_MOUNT_OPT(NO_UBC)
	FUSE_MOUNT_OPT(NO_VNCACHE)
	FUSE_MOUNT_OPT(USE_INO)
	FUSE_MOUNT_OPT(PING_DISKARB)
	FUSE_MOUNT_OPT(AUTO_CACHE)
	FUSE_MOUNT_OPT(NATIVE_XATTR)
	FUSE_MOUNT_OPT(SPARSE)

	case KEY_QUIET:
		mo->quiet = true;
		mo->fuse_args.altflags |= FUSE_MOPT_QUIET;
		return 0;

	case KEY_ALLOW_RECURSION:
		mo->allow_recursion = true;
		return 0;

	case KEY_NO_LOCALCACHES:
		mo->fuse_args.altflags |= (FUSE_MOPT_NO_ATTRCACHE | FUSE_MOPT_NO_READAHEAD | FUSE_MOPT_NO_UBC | FUSE_MOPT_NO_VNCACHE);
		return 0;

	case KEY_VOLICON: {
		char volicon_arg[MAXPATHLEN + 32];
		char *volicon_path = strchr(arg, '=');
		if (!volicon_path) {
			return -1;
		}
		if (snprintf(volicon_arg, sizeof(volicon_arg),
						"-omodules=volicon,iconpath%s", volicon_path) <= 0) {
			return -1;
		}
		if (fuse_opt_add_arg(outargs, volicon_arg) == -1) {
			return -1;
		}

		return 0;
	}

	FUSE_MOUNT_OPT_PARSE_U32(BLOCKSIZE, blocksize)
	FUSE_MOUNT_OPT_PARSE_U32(DAEMON_TIMEOUT, daemon_timeout)
	FUSE_MOUNT_OPT_PARSE_U32(INIT_TIMEOUT, init_timeout)
	FUSE_MOUNT_OPT_PARSE_U32(IOSIZE, iosize)
	FUSE_MOUNT_OPT_PARSE_U32(FSID, fsid)
	FUSE_MOUNT_OPT_PARSE_U32(FSSUBTYPE, fssubtype)

	FUSE_MOUNT_OPT_PARSE_STRING(FSNAME, fsname, MAXPATHLEN)
	FUSE_MOUNT_OPT_PARSE_STRING(FSTYPENAME, fstypename, MFSTYPENAMELEN)
	FUSE_MOUNT_OPT_PARSE_STRING(VOLNAME, volname, MAXPATHLEN)

	case KEY_IGNORED:
		return 0;
	}

	return 1;
}

static int post_notification(char *name, char const *udata_keys[], char const *udata_values[], CFIndex nfNum)
{
	CFNotificationCenterRef distributedCenter = CFNotificationCenterGetDistributedCenter();

	if (!distributedCenter) {
		return -1;
	}

	CFStringRef nfName = CFStringCreateWithCString(kCFAllocatorDefault, name, kCFStringEncodingUTF8);
	CFStringRef nfObject = CFStringCreateWithCString(kCFAllocatorDefault, LIBFUSE_UNOTIFICATIONS_OBJECT,
			kCFStringEncodingUTF8);

	CFMutableDictionaryRef nfUdata = CFDictionaryCreateMutable(kCFAllocatorDefault, nfNum,
			&kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

	if (!nfName || !nfObject || !nfUdata) {
		goto out;
	}

	CFIndex i = 0;
	for (; i < nfNum; i++) {
		CFStringRef aKey = CFStringCreateWithCString(kCFAllocatorDefault, udata_keys[i], kCFStringEncodingUTF8);
		CFStringRef aValue = CFStringCreateWithCString(kCFAllocatorDefault, udata_values[i], kCFStringEncodingUTF8);

		CFDictionarySetValue(nfUdata, aKey, aValue);

		CFRelease(aKey);
		CFRelease(aValue);
	}

	CFNotificationCenterPostNotification(distributedCenter, nfName, nfObject, nfUdata, false);

out:
	if (nfName)
		CFRelease(nfName);

	if (nfObject)
		CFRelease(nfObject);

	if (nfUdata)
		CFRelease(nfUdata);

	return 0;
}

static bool check_os_kernel_version(void)
{
	struct utsname u;

	if (uname(&u) < 0) {
		perror("fuse4x uname():");
		return false;
	}

	char *c = strchr(u.release, '.');
	if (c == NULL) {
		return false;
	}

	*c = '\0';

	errno = 0;
	long major = strtol(u.release, NULL, 10);
	if ((errno == EINVAL) || (errno == ERANGE)) {
		return false;
	}

	return major >= 10;
}

static bool check_kext_version(bool quiet_mode)
{
	if (!check_os_kernel_version()) {
		// TODO: Check that OS at lease 10.6
		if (!quiet_mode) {
			CFUserNotificationDisplayNotice(
				(CFTimeInterval)0,
				kCFUserNotificationCautionAlertLevel,
				NULL,
				NULL,
				NULL,
				CFSTR("Operating System Too Old"),
				CFSTR("The installed fuse4x version is too new for the operating system. Please downgrade your fuse4x installation to one that is compatible with the currently running operating system."),
				CFSTR("OK")
			);
		}
		post_notification(LIBFUSE_UNOTIFICATIONS_NOTIFY_OSISTOOOLD, NULL, NULL, 0);
		fprintf(stderr, "fuse4x is not supported on this MacOSX version.\n");
		return false;
	}

	/* Check for user<->kernel match. */
	char version[MAXHOSTNAMELEN + 1];
	size_t version_len = MAXHOSTNAMELEN;

	int result = sysctlbyname(SYSCTL_FUSE4X_VERSION_NUMBER, version, &version_len, NULL, (size_t)0);

	if (result != 0) {
		// Let's try to load the kext
		pid_t pid = fork();
		if (pid < 0) {
			perror("fork");
			result = -1;
		} else if (pid == 0) {
			if (execl(FUSE4X_LOAD_PROG, FUSE4X_LOAD_PROG, NULL) == -1) {
				/* We can only get here if the exec failed */
				perror("execl");
				_exit(errno);
			}
		} else {
			if (waitpid(pid, &result, 0) == -1) {
				perror("waitpid");
				result = -1;
			}
		}

		if (result != 0) {
			if (!quiet_mode) {
				CFUserNotificationDisplayNotice(
					(CFTimeInterval)0,
					kCFUserNotificationCautionAlertLevel,
					NULL,
					NULL,
					NULL,
					CFSTR("Fuse4x kernel extension error"),
					CFSTR("The fuse4x kernel extension was not loaded."),
					CFSTR("OK")
				);
			}
			post_notification(LIBFUSE_UNOTIFICATIONS_NOTIFY_INVALID_KEXT, NULL, NULL, 0);
			fprintf(stderr, "fuse4x kernel extension was not loaded. Please check /var/log/system.log for more information.\n");
			return false;
		}

		result = sysctlbyname(SYSCTL_FUSE4X_VERSION_NUMBER, version, &version_len, NULL, (size_t)0);
	}

	if (strncmp(FUSE4X_VERSION, version, version_len)) {
		if (!quiet_mode) {
			CFUserNotificationDisplayNotice(
				(CFTimeInterval)0,
				kCFUserNotificationCautionAlertLevel,
				NULL,
				NULL,
				NULL,
				CFSTR("Fuse4x runtime version mismatch"),
				CFSTR("The fuse4x library version this program is using is incompatible with the loaded fuse4x kernel extension."),
				CFSTR("OK")
			);
		}
		post_notification(LIBFUSE_UNOTIFICATIONS_NOTIFY_VERSIONMISMATCH, NULL, NULL, 0);
		fprintf(stderr, "fuse4x client library version is incompatible with the kernel extension (kext='%s', library='%s').\n", version, FUSE4X_VERSION);
		return false;
	}

	return true;
}

void fuse_kern_unmount(const char *mountpoint, int fd)
{
	if (!mountpoint)
		return;

	if (fd != -1)
		close(fd);

	unmount(mountpoint, MNT_FORCE);
	return;
}

void fuse_unmount_compat22(const char *mountpoint)
{
	unmount(mountpoint, 0);

	return;
}

static bool check_mountpoint(const char *mountpoint, struct mount_options *opts)
{
	if (!mountpoint) {
		fprintf(stderr, "fuse4x: missing or invalid mount point\n");
		return -1;
	}

	struct stat statb;
	if ((realpath(mountpoint, opts->fuse_args.mntpath) != NULL) &&
		(stat(opts->fuse_args.mntpath, &statb) == 0)) {

		if (!S_ISDIR(statb.st_mode)) {
			fprintf(stderr, "fuse4x: mountpoint %s is not a directory\n", opts->fuse_args.mntpath);
			return false;
		}
	} else {
		fprintf(stderr, "fuse4x: failed to stat() on mountpoint %s - %s\n", opts->fuse_args.mntpath, strerror(errno));
		return false;
	}

	struct statfs statfsb;
	if (statfs(opts->fuse_args.mntpath, &statfsb) < 0) {
	fprintf(stderr, "fuse4x: failed to statfs() on mountpoint %s - %s\n", opts->fuse_args.mntpath, strerror(errno));
		return false;
	}
	if ((strcmp(statfsb.f_fstypename, FUSE_FSTYPENAME_PREFIX) == 0) && !opts->allow_recursion) {
		fprintf(stderr, "fuse4x: mount point %s is itself on a fuse4x volume\n", opts->fuse_args.mntpath);
		return false;
	}

	return true;
}

int fuse_kern_mount(const char *mountpoint, struct fuse_args *args)
{
	struct mount_options opts;

	memset(&opts, 0, sizeof(opts));

	opts.fuse_args.blocksize = FUSE_DEFAULT_BLOCKSIZE;
	opts.fuse_args.daemon_timeout = FUSE_DEFAULT_DAEMON_TIMEOUT;
	opts.fuse_args.init_timeout = FUSE_DEFAULT_INIT_TIMEOUT;
	opts.fuse_args.iosize = FUSE_DEFAULT_IOSIZE;


	if (args && fuse_opt_parse(args, &opts, fuse_mount_opts, fuse_mount_opt_proc) == -1) {
		return -1;
	}

	if (!check_mountpoint(mountpoint, &opts)) {
		return -1;
	}

	if (!check_kext_version(opts.quiet)) {
		return -1;
	}

	// MNT_LOCAL flag should be duplicated both as a standrad mount flag and a fuse4x flag
	// See https://groups.google.com/d/topic/fuse4x/WuKrqpNvAgw/discussion
	if (opts.standard_args & MNT_LOCAL)
		opts.fuse_args.altflags |= FUSE_MOPT_LOCALVOL;

	if ((opts.fuse_args.altflags & FUSE_MOPT_ALLOW_OTHER) &&
		(opts.fuse_args.altflags & FUSE_MOPT_ALLOW_ROOT)) {

		fprintf(stderr, "fuse4x: allow_other and allow_root are mutually exclusive\n");
		return -1;
	}

	if ((opts.fuse_args.altflags & FUSE_MOPT_NEGATIVE_VNCACHE) &&
		(opts.fuse_args.altflags & FUSE_MOPT_NO_VNCACHE)) {
		fprintf(stderr, "fuse4x: 'negative_vncache' can't be used with 'novncache'\n");
		return -1;
	}

	// 'nosyncwrites' must not appear with either 'noubc' or 'noreadahead'.
	if ((opts.fuse_args.altflags & FUSE_MOPT_NO_SYNCWRITES) &&
		(opts.fuse_args.altflags & (FUSE_MOPT_NO_UBC | FUSE_MOPT_NO_READAHEAD))) {
		fprintf(stderr, "fuse4x: disabling local caching can't be used with 'nosyncwrites'\n");
		return -1;
	}

	// 'nosynconclose' only allowed if 'nosyncwrites' is also there.
	if ((opts.fuse_args.altflags & FUSE_MOPT_DEFAULT_PERMISSIONS) &&
		(opts.fuse_args.altflags & FUSE_MOPT_DEFER_PERMISSIONS)) {
		fprintf(stderr, "fuse4x: 'default_permissions' can't be used with 'defer_permissions'\n");
		return -1;
	}

	if ((opts.fuse_args.altflags & FUSE_MOPT_AUTO_XATTR) &&
		(opts.fuse_args.altflags & FUSE_MOPT_NATIVE_XATTR)) {
		fprintf(stderr, "fuse4x: 'auto_xattr' can't be used with 'native_xattr'\n");
		return -1;
	}

	if (opts.fuse_args.fsid & ~FUSE_MINOR_MASK) {
		fprintf(stderr, "fuse4x: invalid 'fsid'\n");
		return -1;
	}

	if (opts.fuse_args.daemon_timeout < FUSE_MIN_DAEMON_TIMEOUT)
		opts.fuse_args.daemon_timeout = FUSE_MIN_DAEMON_TIMEOUT;

	if (opts.fuse_args.daemon_timeout > FUSE_MAX_DAEMON_TIMEOUT)
		opts.fuse_args.daemon_timeout = FUSE_MAX_DAEMON_TIMEOUT;

	if (opts.fuse_args.init_timeout < FUSE_MIN_INIT_TIMEOUT)
		opts.fuse_args.init_timeout = FUSE_MIN_INIT_TIMEOUT;

	if (opts.fuse_args.init_timeout > FUSE_MAX_INIT_TIMEOUT)
		opts.fuse_args.init_timeout = FUSE_MAX_INIT_TIMEOUT;


	int device_no = -1;
	char devpath[MAXPATHLEN];
	int fd;

	unsigned int i = 0;
	for (; i < FUSE4X_NDEVICES; i++) {
		snprintf(devpath, MAXPATHLEN - 1, _PATH_DEV FUSE4X_DEVICE_BASENAME "%d", i);
		fd = open(devpath, O_RDWR);
		if (fd >= 0) {
			device_no = i;

			struct stat sb;
			if (fstat(fd, &sb) < 0) {
				perror("fuse4x fstat:");
				return -1;
			}
			opts.fuse_args.rdev = sb.st_rdev;

			break;
		}
	}
	if (device_no == -1) {
		fprintf(stderr, "fuse4x: failed to open device file\n");
		return -1;
	}

	if (!*opts.fuse_args.fsname)
		snprintf(opts.fuse_args.fsname, MAXPATHLEN, "%s@fuse%d", getprogname(), device_no);

	if (!*opts.fuse_args.volname)
		snprintf(opts.fuse_args.volname, MAXPATHLEN, "fuse4x volume %d (%s)", device_no, getprogname());

	int result = mount(FUSE4X_FS_TYPE, opts.fuse_args.mntpath, opts.standard_args, &opts.fuse_args);

	char const *udata_keys[]   = { kFUSEMountPathKey };
	char const *udata_values[] = { opts.fuse_args.mntpath };

	if (result < 0) {
		fprintf(stderr, "fuse4x failed to mount %s to %s : %s\n", devpath, opts.fuse_args.mntpath, strerror(errno));
		if (!opts.quiet) {
			CFUserNotificationDisplayNotice(
				(CFTimeInterval)0,
				kCFUserNotificationCautionAlertLevel,
				NULL,
				NULL,
				NULL,
				CFSTR("Fuse4x failed to mount"),
				CFSTR("The fuse4x failed to mount path."), // TODO Add mountpath
				CFSTR("OK")
			);
		}
		post_notification(LIBFUSE_UNOTIFICATIONS_NOTIFY_FAILEDTOMOUNT, udata_keys, udata_values, 1);
		return -1;
	} else {
		post_notification(LIBFUSE_UNOTIFICATIONS_NOTIFY_MOUNTED, udata_keys, udata_values, 1);
	}

	return fd;
}
