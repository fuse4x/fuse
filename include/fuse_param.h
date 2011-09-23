/*
 * Copyright (C) 2006-2008 Google. All Rights Reserved.
 * Copyright (C) 2011 Anatol Pomozov. All Rights Reserved.
 */

#ifndef _FUSE_PARAM_H_
#define _FUSE_PARAM_H_

#include <mach/vm_param.h>
#include <sys/ioctl.h>

/* User Control */

#define MACOSX_ADMIN_GROUP_NAME            "admin"

#define SYSCTL_FUSE4X_TUNABLES_ADMIN      "vfs.generic.fuse4x.tunables.admin_group"
#define SYSCTL_FUSE4X_VERSION_NUMBER      "vfs.generic.fuse4x.version.number"

/* Paths */

#define FUSE4X_KEXT_PATH                  "/System/Library/Extensions/fuse4x.kext"
#define FUSE4X_LOAD_PROG                  FUSE4X_KEXT_PATH "/Support/load_fuse4x"

/* Device Interface */

/*
 * This is the prefix ("fuse4x" by default) of the name of a FUSE device node
 * in devfs. The suffix is the device number. "/dev/fuse4x0" is the first FUSE
 * device by default. If you change the prefix from the default to something
 * else, the user-space FUSE library will need to know about it too.
 */
#define FUSE4X_DEVICE_BASENAME            "fuse4x"

/*
 * This is the number of /dev/fuse4x<n> nodes we will create. <n> goes from
 * 0 to (FUSE_NDEVICES - 1).
 */
#define FUSE4X_NDEVICES                   24

/*
 * This is the default block size of the virtual storage devices that are
 * implicitly implemented by the FUSE kernel extension. This can be changed
 * on a per-mount basis (there's one such virtual device for each mount).
 */
#define FUSE_DEFAULT_BLOCKSIZE             4096

#define FUSE_MIN_BLOCKSIZE                 512
#define FUSE_MAX_BLOCKSIZE                 MAXPHYS

#ifndef MAX_UPL_TRANSFER
#define MAX_UPL_TRANSFER 256
#endif

/*
 * This is default I/O size used while accessing the virtual storage devices.
 * This can be changed on a per-mount basis.
 *
 * Nevertheless, the I/O size must be at least as big as the block size.
 */
#define FUSE_DEFAULT_IOSIZE                (16 * PAGE_SIZE)

#define FUSE_MIN_IOSIZE                    512
#define FUSE_MAX_IOSIZE                    (MAX_UPL_TRANSFER * PAGE_SIZE)

#define FUSE_DEFAULT_INIT_TIMEOUT                  10     /* s  */
#define FUSE_MIN_INIT_TIMEOUT                      1      /* s  */
#define FUSE_MAX_INIT_TIMEOUT                      300    /* s  */
#define FUSE_INIT_WAIT_INTERVAL                    100000 /* us */

#define FUSE_DEFAULT_DAEMON_TIMEOUT                60     /* s */
#define FUSE_MIN_DAEMON_TIMEOUT                    0      /* s */
#define FUSE_MAX_DAEMON_TIMEOUT                    600    /* s */


#ifdef KERNEL

/*
 * This is the soft upper limit on the number of "request tickets" FUSE's
 * user-kernel IPC layer can have for a given mount. This can be modified
 * through the fuse.* sysctl interface.
 */
#define FUSE_DEFAULT_MAX_FREE_TICKETS      1024
#define FUSE_DEFAULT_IOV_PERMANENT_BUFSIZE (1 << 19)
#define FUSE_DEFAULT_IOV_CREDIT            16

/* User-Kernel IPC Buffer */

#define FUSE_MIN_USERKERNEL_BUFSIZE        (128  * 1024)
#define FUSE_MAX_USERKERNEL_BUFSIZE        (16   * 1024 * 1024)

#define FUSE_REASONABLE_XATTRSIZE          FUSE_MIN_USERKERNEL_BUFSIZE

#endif /* KERNEL */

#define FUSE_DEFAULT_USERKERNEL_BUFSIZE    (16   * 1024 * 1024)

#define FUSE_LINK_MAX                      LINK_MAX
#define FUSE_UIO_BACKUP_MAX                8

#define FUSE_MAXNAMLEN                     255


/* FUSEDEVIOCxxx */

/* Mark the daemon as dead. */
#define FUSEDEVIOCSETDAEMONDEAD        _IO('F', 1)

#endif /* _FUSE_PARAM_H_ */
