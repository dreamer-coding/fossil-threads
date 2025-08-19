/*
 * -----------------------------------------------------------------------------
 * Project: Fossil Logic
 *
 * This file is part of the Fossil Logic project, which aims to develop high-
 * performance, cross-platform applications and libraries. The code contained
 * herein is subject to the terms and conditions defined in the project license.
 *
 * Author: Michael Gene Brockus (Dreamer)
 *
 * Copyright (C) 2024 Fossil Logic. All rights reserved.
 * -----------------------------------------------------------------------------
 */
#include "fossil/threads/mutex.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <pthread.h>
#endif

static void fossil__mutex_zero(fossil_threads_mutex_t *m) {
    if (m) memset(m, 0, sizeof(*m));
}

/* ---------- Lifecycle ---------- */

int fossil_threads_mutex_init(fossil_threads_mutex_t *m) {
    if (!m) return FOSSIL_THREADS_MUTEX_EINVAL;
    fossil__mutex_zero(m);

#if defined(_WIN32)
    CRITICAL_SECTION *cs = (CRITICAL_SECTION*)malloc(sizeof(CRITICAL_SECTION));
    if (!cs) return FOSSIL_THREADS_MUTEX_EINTERNAL;
    InitializeCriticalSection(cs);
    m->handle = cs;
#else
    pthread_mutex_t *pm = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if (!pm) return FOSSIL_THREADS_MUTEX_EINTERNAL;
    if (pthread_mutex_init(pm, NULL) != 0) {
        free(pm);
        return FOSSIL_THREADS_MUTEX_EINTERNAL;
    }
    m->handle = pm;
#endif

    m->valid = 1;
    return FOSSIL_THREADS_MUTEX_OK;
}

void fossil_threads_mutex_dispose(fossil_threads_mutex_t *m) {
    if (!m || !m->valid) return;

#if defined(_WIN32)
    CRITICAL_SECTION *cs = (CRITICAL_SECTION*)m->handle;
    if (cs) {
        DeleteCriticalSection(cs);
        free(cs);
    }
#else
    pthread_mutex_t *pm = (pthread_mutex_t*)m->handle;
    if (pm) {
        pthread_mutex_destroy(pm);
        free(pm);
    }
#endif
    fossil__mutex_zero(m);
}

/* ---------- Locking ---------- */

int fossil_threads_mutex_lock(fossil_threads_mutex_t *m) {
    if (!m || !m->valid) return FOSSIL_THREADS_MUTEX_EINVAL;

#if defined(_WIN32)
    EnterCriticalSection((CRITICAL_SECTION*)m->handle);
    return FOSSIL_THREADS_MUTEX_OK;
#else
    return pthread_mutex_lock((pthread_mutex_t*)m->handle);
#endif
}

int fossil_threads_mutex_unlock(fossil_threads_mutex_t *m) {
    if (!m || !m->valid) return FOSSIL_THREADS_MUTEX_EINVAL;

#if defined(_WIN32)
    LeaveCriticalSection((CRITICAL_SECTION*)m->handle);
    return FOSSIL_THREADS_MUTEX_OK;
#else
    return pthread_mutex_unlock((pthread_mutex_t*)m->handle);
#endif
}

int fossil_threads_mutex_trylock(fossil_threads_mutex_t *m) {
    if (!m || !m->valid) return FOSSIL_THREADS_MUTEX_EINVAL;

#if defined(_WIN32)
    if (TryEnterCriticalSection((CRITICAL_SECTION*)m->handle)) {
        return FOSSIL_THREADS_MUTEX_OK;
    } else {
        return FOSSIL_THREADS_MUTEX_EBUSY;
    }
#else
    int rc = pthread_mutex_trylock((pthread_mutex_t*)m->handle);
    if (rc == 0) return FOSSIL_THREADS_MUTEX_OK;
    if (rc == EBUSY) return FOSSIL_THREADS_MUTEX_EBUSY;
    return rc;
#endif
}
