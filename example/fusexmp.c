/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags --libs` -DHAVE_SETXATTR fusexmp.c -o fusexmp
*/

#define FUSE_USE_VERSION 26

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#elif __APPLE__
#define _GNU_SOURCE
#endif

#if defined(_POSIX_C_SOURCE)
typedef unsigned char  u_char;
typedef unsigned short u_short;
typedef unsigned int   u_int;
typedef unsigned long  u_long;
#endif

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/xattr.h>
#include <sys/attr.h>
#include <sys/param.h>

#define G_PREFIX   "org"
#define G_KAUTH_FILESEC_XATTR G_PREFIX ".apple.system.Security"
#define A_PREFIX   "com"
#define A_KAUTH_FILESEC_XATTR A_PREFIX ".apple.system.Security"
#define XATTR_APPLE_PREFIX   "com.apple."


static int xmp_getattr(const char *path, struct stat *stbuf)
{
	int res;

	res = lstat(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_access(const char *path, int mask)
{
	int res;

	res = access(path, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
	int res;

	res = readlink(path, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			   off_t offset, struct fuse_file_info *fi)
{
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;

	dp = opendir(path);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;

	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(path, mode);
	else
		res = mknod(path, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
	int res;

	res = mkdir(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_unlink(const char *path)
{
	int res;

	res = unlink(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rmdir(const char *path)
{
	int res;

	res = rmdir(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
	int res;

	res = symlink(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rename(const char *from, const char *to)
{
	int res;

	res = rename(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_exchange(const char *path1, const char *path2, unsigned long options)
{
	int res;

	res = exchangedata(path1, path2, options);
	if (res == -1) {
		return -errno;
	}

	return 0;
}

static int xmp_link(const char *from, const char *to)
{
	int res;

	res = link(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_fsetattr_x(const char *path, struct setattr_x *attr,
					struct fuse_file_info *fi)
{
	int res;
	uid_t uid = -1;
	gid_t gid = -1;

	if (SETATTR_WANTS_MODE(attr)) {
		res = lchmod(path, attr->mode);
		if (res == -1) {
			return -errno;
		}
	}

	if (SETATTR_WANTS_UID(attr)) {
		uid = attr->uid;
	}

	if (SETATTR_WANTS_GID(attr)) {
		gid = attr->gid;
	}

	if ((uid != -1) || (gid != -1)) {
		res = lchown(path, uid, gid);
		if (res == -1) {
			return -errno;
		}
	}

	if (SETATTR_WANTS_SIZE(attr)) {
		if (fi) {
			res = ftruncate(fi->fh, attr->size);
		} else {
			res = truncate(path, attr->size);
		}
		if (res == -1) {
			return -errno;
		}
	}

	if (SETATTR_WANTS_MODTIME(attr)) {
		struct timeval tv[2];
		if (!SETATTR_WANTS_ACCTIME(attr)) {
			gettimeofday(&tv[0], NULL);
		} else {
			tv[0].tv_sec = attr->acctime.tv_sec;
			tv[0].tv_usec = attr->acctime.tv_nsec / 1000;
		}
		tv[1].tv_sec = attr->modtime.tv_sec;
		tv[1].tv_usec = attr->modtime.tv_nsec / 1000;
		res = utimes(path, tv);
		if (res == -1) {
			return -errno;
		}
	}

	if (SETATTR_WANTS_CRTIME(attr)) {
		struct attrlist attributes;

		attributes.bitmapcount = ATTR_BIT_MAP_COUNT;
		attributes.reserved = 0;
		attributes.commonattr = ATTR_CMN_CRTIME;
		attributes.dirattr = 0;
		attributes.fileattr = 0;
		attributes.forkattr = 0;
		attributes.volattr = 0;

		res = setattrlist(path, &attributes, &attr->crtime,
				  sizeof(struct timespec), FSOPT_NOFOLLOW);

		if (res == -1) {
			return -errno;
		}
	}

	if (SETATTR_WANTS_CHGTIME(attr)) {
		struct attrlist attributes;

		attributes.bitmapcount = ATTR_BIT_MAP_COUNT;
		attributes.reserved = 0;
		attributes.commonattr = ATTR_CMN_CHGTIME;
		attributes.dirattr = 0;
		attributes.fileattr = 0;
		attributes.forkattr = 0;
		attributes.volattr = 0;

		res = setattrlist(path, &attributes, &attr->chgtime,
				  sizeof(struct timespec), FSOPT_NOFOLLOW);

		if (res == -1) {
			return -errno;
		}
	}

	if (SETATTR_WANTS_BKUPTIME(attr)) {
		struct attrlist attributes;

		attributes.bitmapcount = ATTR_BIT_MAP_COUNT;
		attributes.reserved = 0;
		attributes.commonattr = ATTR_CMN_BKUPTIME;
		attributes.dirattr = 0;
		attributes.fileattr = 0;
		attributes.forkattr = 0;
		attributes.volattr = 0;

		res = setattrlist(path, &attributes, &attr->bkuptime,
				  sizeof(struct timespec), FSOPT_NOFOLLOW);

		if (res == -1) {
			return -errno;
		}
	}

	if (SETATTR_WANTS_FLAGS(attr)) {
		res = lchflags(path, attr->flags);
		if (res == -1) {
			return -errno;
		}
	}

	return 0;
}

static int xmp_setattr_x(const char *path, struct setattr_x *attr)
{
	return xmp_fsetattr_x(path, attr, (struct fuse_file_info *)0);
}

static int xmp_getxtimes(const char *path, struct timespec *bkuptime,
				   struct timespec *crtime)
{
	int res = 0;
	struct attrlist attributes;

	attributes.bitmapcount = ATTR_BIT_MAP_COUNT;
	attributes.reserved	   = 0;
	attributes.commonattr  = 0;
	attributes.dirattr	   = 0;
	attributes.fileattr	   = 0;
	attributes.forkattr	   = 0;
	attributes.volattr	   = 0;

	struct xtimeattrbuf {
		uint32_t size;
		struct timespec xtime;
	} __attribute__ ((packed));

	struct xtimeattrbuf buf;

	attributes.commonattr = ATTR_CMN_BKUPTIME;
	res = getattrlist(path, &attributes, &buf, sizeof(buf), FSOPT_NOFOLLOW);
	if (res == 0) {
		(void)memcpy(bkuptime, &(buf.xtime), sizeof(struct timespec));
	} else {
		(void)memset(bkuptime, 0, sizeof(struct timespec));
	}

	attributes.commonattr = ATTR_CMN_CRTIME;
	res = getattrlist(path, &attributes, &buf, sizeof(buf), FSOPT_NOFOLLOW);
	if (res == 0) {
		(void)memcpy(crtime, &(buf.xtime), sizeof(struct timespec));
	} else {
		(void)memset(crtime, 0, sizeof(struct timespec));
	}

	return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
	int res;

	res = chmod(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
	int res;

	res = lchown(path, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
	int res;

	res = truncate(path, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2])
{
	int res;
	struct timeval tv[2];

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	res = utimes(path, tv);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	int res;

	res = open(path, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
			struct fuse_file_info *fi)
{
	int fd;
	int res;

	(void) fi;
	fd = open(path, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
			 off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;

	(void) fi;
	fd = open(path, O_WRONLY);
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	int res;

	res = statvfs(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) fi;
	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
			 struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

static int xmp_setxattr(const char *path, const char *name, const char *value,
				  size_t size, int flags, uint32_t position)
{
	int res;

	if (!strncmp(name, XATTR_APPLE_PREFIX, sizeof(XATTR_APPLE_PREFIX) - 1)) {
		flags &= ~(XATTR_NOSECURITY);
	}

	if (!strcmp(name, A_KAUTH_FILESEC_XATTR)) {

		char new_name[MAXPATHLEN];

		memcpy(new_name, A_KAUTH_FILESEC_XATTR, sizeof(A_KAUTH_FILESEC_XATTR));
		memcpy(new_name, G_PREFIX, sizeof(G_PREFIX) - 1);

		res = setxattr(path, new_name, value, size, position, XATTR_NOFOLLOW);

	} else {
		res = setxattr(path, name, value, size, position, XATTR_NOFOLLOW);
	}

	if (res == -1) {
		return -errno;
	}

	return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value, size_t size,
				  uint32_t position)
{
	int res;

	if (strcmp(name, A_KAUTH_FILESEC_XATTR) == 0) {

		char new_name[MAXPATHLEN];

		memcpy(new_name, A_KAUTH_FILESEC_XATTR, sizeof(A_KAUTH_FILESEC_XATTR));
		memcpy(new_name, G_PREFIX, sizeof(G_PREFIX) - 1);

		res = getxattr(path, new_name, value, size, position, XATTR_NOFOLLOW);

	} else {
		res = getxattr(path, name, value, size, position, XATTR_NOFOLLOW);
	}

	if (res == -1) {
		return -errno;
	}

	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	ssize_t res = listxattr(path, list, size, XATTR_NOFOLLOW);
	if (res > 0) {
		if (list) {
			size_t len = 0;
			char *curr = list;
			do {
				size_t thislen = strlen(curr) + 1;
				if (strcmp(curr, G_KAUTH_FILESEC_XATTR) == 0) {
					memmove(curr, curr + thislen, res - len - thislen);
					res -= thislen;
					break;
				}
				curr += thislen;
				len += thislen;
			} while (len < res);
		}
	}

	if (res == -1) {
		return -errno;
	}

	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	int res;

	if (strcmp(name, A_KAUTH_FILESEC_XATTR) == 0) {

		char new_name[MAXPATHLEN];

		memcpy(new_name, A_KAUTH_FILESEC_XATTR, sizeof(A_KAUTH_FILESEC_XATTR));
		memcpy(new_name, G_PREFIX, sizeof(G_PREFIX) - 1);

		res = removexattr(path, new_name, XATTR_NOFOLLOW);

	} else {
		res = removexattr(path, name, XATTR_NOFOLLOW);
	}

	if (res == -1) {
		return -errno;
	}

	return 0;
}

static void* xmp_init(struct fuse_conn_info *conn)
{
	FUSE_ENABLE_SETVOLNAME(conn);
	FUSE_ENABLE_XTIMES(conn);

	return NULL;
}


static struct fuse_operations xmp_oper = {
	.init		= xmp_init,
	.getattr	= xmp_getattr,
	.access		= xmp_access,
	.readlink	= xmp_readlink,
	.readdir	= xmp_readdir,
	.mknod		= xmp_mknod,
	.mkdir		= xmp_mkdir,
	.symlink	= xmp_symlink,
	.unlink		= xmp_unlink,
	.rmdir		= xmp_rmdir,
	.rename		= xmp_rename,
	.link		= xmp_link,
	.chmod		= xmp_chmod,
	.chown		= xmp_chown,
	.truncate	= xmp_truncate,
	.utimens	= xmp_utimens,
	.open		= xmp_open,
	.read		= xmp_read,
	.write		= xmp_write,
	.statfs		= xmp_statfs,
	.release	= xmp_release,
	.fsync		= xmp_fsync,
#ifdef HAVE_SETXATTR
	.setxattr	= xmp_setxattr,
	.getxattr	= xmp_getxattr,
	.listxattr	= xmp_listxattr,
	.removexattr	= xmp_removexattr,
#endif
	.exchange	 = xmp_exchange,
	.getxtimes	 = xmp_getxtimes,
	.setattr_x	 = xmp_setattr_x,
	.fsetattr_x	 = xmp_fsetattr_x,
};

int main(int argc, char *argv[])
{
	umask(0);
	return fuse_main(argc, argv, &xmp_oper, NULL);
}
