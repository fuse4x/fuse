/* Copyright (C) 2000,02 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Written by Gaël Le Mignot <address@hidden>

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

// This implementation is based on libsem http://lists.debian.org/debian-devel/2004/08/msg00612.html

#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

/* Caller must not include <semaphore.h> */

#include <pthread.h>

struct __local_sem_t
{
    unsigned int    count;
    pthread_mutex_t count_lock;
    pthread_cond_t  count_cond;
};

typedef struct fuse_sem {
    unsigned int id;
    union {
        struct __local_sem_t local;
    } __data;
} fuse_sem_t;

#define FUSE_SEM_VALUE_MAX ((int32_t)32767)

int fuse_sem_init(fuse_sem_t *sem, int pshared, unsigned int value);
int fuse_sem_destroy(fuse_sem_t *sem);
int fuse_sem_getvalue(fuse_sem_t *sem, unsigned int *value);
int fuse_sem_post(fuse_sem_t *sem);
int fuse_sem_timedwait(fuse_sem_t *sem, const struct timespec *abs_timeout);
int fuse_sem_trywait(fuse_sem_t *sem);
int fuse_sem_wait(fuse_sem_t *sem);

typedef fuse_sem_t sem_t;

#define sem_init(s, p, v)   fuse_sem_init(s, p, v)
#define sem_destroy(s)      fuse_sem_destroy(s)
#define sem_getvalue(s, v)  fuse_sem_getvalue(s, v)
#define sem_post(s)         fuse_sem_post(s)
#define sem_timedwait(s, t) fuse_sem_timedwait(s, t)
#define sem_trywait(s)      fuse_sem_trywait(s)
#define sem_wait(s)         fuse_sem_wait(s)

#define SEM_VALUE_MAX       FUSE_SEM_VALUE_MAX


#endif /* semaphore.h */
