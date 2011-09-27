/*
 * Copyright (C) 2006-2008 Google. All Rights Reserved.
 * Copyright (C) 2011 Anatol Pomozov. All Rights Reserved.
 */

#ifndef _FUSE_MOUNT_H_
#define _FUSE_MOUNT_H_

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef KERNEL
#include <unistd.h>
#endif

#include <fuse_param.h>
#include <fuse_version.h>

/*
 * Shared between the kernel and user spaces. This is 64-bit invariant.
 */
struct fuse_mount_args {
    char     mntpath[MAXPATHLEN]; // path to the mount point
    char     fsname[MAXPATHLEN];  // file system description string
    char     fstypename[MFSTYPENAMELEN]; // file system type name
    char     volname[MAXPATHLEN]; // volume name
    uint64_t altflags;            // see mount-time flags below
    uint32_t blocksize;           // fictitious block size of our "storage"
    uint32_t daemon_timeout;      // timeout in seconds for upcalls to daemon
    uint32_t fsid;                // optional custom value for part of fsid[0]
    uint32_t fssubtype;           // file system sub type id (type is "fuse4x")
    uint32_t iosize;              // maximum size for reading or writing
    uint32_t rdev;                // dev_t for the /dev/fuse4xN in question
};
typedef struct fuse_mount_args fuse_mount_args;

/* File system type name. */

#define FUSE_FSTYPENAME_PREFIX FUSE4X_FS_TYPE "_"

/* Courtesy of the Finder, this is 1 less than what you think it should be. */
#define FUSE_FSTYPENAME_MAXLEN (MFSTYPENAMELEN - sizeof(FUSE4X_FS_TYPE) - 2)

/* mount-time flags */
enum {
    FUSE_MOPT_ALLOW_OTHER         = 1ULL << 0,
    FUSE_MOPT_ALLOW_ROOT          = 1ULL << 1,
    FUSE_MOPT_AUTO_XATTR          = 1ULL << 2,
    FUSE_MOPT_BLOCKSIZE           = 1ULL << 3,
    FUSE_MOPT_DAEMON_TIMEOUT      = 1ULL << 4,
    FUSE_MOPT_DEFAULT_PERMISSIONS = 1ULL << 5,
    FUSE_MOPT_DEFER_PERMISSIONS   = 1ULL << 6,
    FUSE_MOPT_DIRECT_IO           = 1ULL << 7,
    FUSE_MOPT_EXTENDED_SECURITY   = 1ULL << 8,
    FUSE_MOPT_FSID                = 1ULL << 9,
    FUSE_MOPT_FSSUBTYPE           = 1ULL << 10,
    FUSE_MOPT_INIT_TIMEOUT        = 1ULL << 11,
    FUSE_MOPT_IOSIZE              = 1ULL << 12,
    FUSE_MOPT_JAIL_SYMLINKS       = 1ULL << 13,
    FUSE_MOPT_NEGATIVE_VNCACHE    = 1ULL << 15,
    FUSE_MOPT_NO_APPLEDOUBLE      = 1ULL << 17,
    FUSE_MOPT_NO_APPLEXATTR       = 1ULL << 18,
    FUSE_MOPT_NO_ATTRCACHE        = 1ULL << 19,
    FUSE_MOPT_NO_READAHEAD        = 1ULL << 20,
    FUSE_MOPT_NO_SYNCONCLOSE      = 1ULL << 21,
    FUSE_MOPT_NO_SYNCWRITES       = 1ULL << 22,
    FUSE_MOPT_NO_UBC              = 1ULL << 23,
    FUSE_MOPT_NO_VNCACHE          = 1ULL << 24,
    FUSE_MOPT_USE_INO             = 1ULL << 25,
    FUSE_MOPT_AUTO_CACHE          = 1ULL << 27,
    FUSE_MOPT_NATIVE_XATTR        = 1ULL << 28,
    FUSE_MOPT_SPARSE              = 1ULL << 29,
    FUSE_MOPT_QUIET               = 1ULL << 30,
    FUSE_MOPT_LOCALVOL            = 1ULL << 31,
};

#define FUSE_MINOR_MASK                 0x00FFFFFFUL
#define FUSE_CUSTOM_FSID_DEVICE_MAJOR   255

#endif /* _FUSE_MOUNT_H_ */
