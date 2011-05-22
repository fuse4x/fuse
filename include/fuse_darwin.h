/*
 * Copyright (C) 2006-2008 Google. All Rights Reserved.
 * Amit Singh <singh@>
 */

#ifdef __APPLE__

#ifndef _FUSE_DARWIN_H_
#define _FUSE_DARWIN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <fuse_param.h>
#include <fuse_ioctl.h>
#include <fuse_version.h>


/* Notifications */

#define LIBFUSE_BUNDLE_IDENTIFIER "com.google.filesystems.libfuse"

#define LIBFUSE_UNOTIFICATIONS_OBJECT                 \
    LIBFUSE_BUNDLE_IDENTIFIER ".unotifications"

#define LIBFUSE_UNOTIFICATIONS_NOTIFY_OSISTOONEW      \
    LIBFUSE_BUNDLE_IDENTIFIER ".osistoonew"

#define LIBFUSE_UNOTIFICATIONS_NOTIFY_OSISTOOOLD      \
    LIBFUSE_BUNDLE_IDENTIFIER ".osistooold"

#define LIBFUSE_UNOTIFICATIONS_NOTIFY_RUNTIMEVERSIONMISMATCH \
    LIBFUSE_BUNDLE_IDENTIFIER ".runtimeversionmismatch"

#define LIBFUSE_UNOTIFICATIONS_NOTIFY_VERSIONMISMATCH \
    LIBFUSE_BUNDLE_IDENTIFIER ".versionmismatch"

#ifdef __cplusplus
}
#endif

#endif /* _FUSE_DARWIN_H_ */

#endif /* __APPLE__ */
