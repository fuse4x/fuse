/*
 * Copyright (C) 2006-2008 Google. All Rights Reserved.
 * Copyright (C) 2011 Anatol Pomozov. All Rights Reserved.
 */

#ifdef __APPLE__

#ifndef _FUSE_DARWIN_H_
#define _FUSE_DARWIN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <fuse_version.h>

/* User data keys. */

#define kFUSEDevicePathKey "kFUSEDevicePath"
#define kFUSEMountPathKey  "kFUSEMountPath"


/* Notifications */

#define LIBFUSE_BUNDLE_IDENTIFIER "org.fuse4x.filesystems.libfuse"

#define LIBFUSE_UNOTIFICATIONS_OBJECT                 \
    LIBFUSE_BUNDLE_IDENTIFIER ".unotifications"

#define LIBFUSE_UNOTIFICATIONS_NOTIFY_OSISTOONEW      \
    LIBFUSE_BUNDLE_IDENTIFIER ".osistoonew"

#define LIBFUSE_UNOTIFICATIONS_NOTIFY_OSISTOOOLD      \
    LIBFUSE_BUNDLE_IDENTIFIER ".osistooold"

#define LIBFUSE_UNOTIFICATIONS_NOTIFY_VERSIONMISMATCH \
    LIBFUSE_BUNDLE_IDENTIFIER ".versionmismatch"

#define LIBFUSE_UNOTIFICATIONS_NOTIFY_INVALID_KEXT \
    LIBFUSE_BUNDLE_IDENTIFIER ".invalidkext"

#define LIBFUSE_UNOTIFICATIONS_NOTIFY_FAILEDTOMOUNT \
    LIBFUSE_BUNDLE_IDENTIFIER ".failedtomount"

#define LIBFUSE_UNOTIFICATIONS_NOTIFY_MOUNTED \
    LIBFUSE_BUNDLE_IDENTIFIER ".mounted"


#ifdef __cplusplus
}
#endif

#endif /* _FUSE_DARWIN_H_ */

#endif /* __APPLE__ */
