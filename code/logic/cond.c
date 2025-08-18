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
#include "fossil/threads/cond.h"
#include <string.h>
#include <stdlib.h>

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <pthread.h>
#  include <errno.h>
#  include <time.h>
#endif

static void fossil__cond_zero(fossil_threads_cond_t *c) {
    if (c) memset(c, 0, sizeof(*c));
}

/* ---------- Lifecycle ---------- */

int fossil_threads_cond_init(fossil_threads_cond_t *c) {
    if (!c) return FOSSIL_THREADS_COND_EINVAL;
    fossil__cond_zero(c);

#if defined(_WIN32)
    CONDITION_VARIABLE *cv = (CONDITION_VARIABLE*)malloc(sizeof(CONDITION_VARIABLE));
    if (!cv) return FOSSIL_THREADS_COND_EINTERNAL;
    InitializeConditionVariable(cv);
    c->handle = cv;
#else
    pthread_cond_t *pc = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
    if (!pc) return FOSSIL_THREADS_COND_EINTERNAL;
    if (pthread_cond_init(pc, NULL) != 0) {
        free(pc);
        return FOSSIL_THREADS_COND_EINTERNAL;
    }
    c->handle = pc;
#endif
    c->valid = 1;
    return FOSSIL_THREADS_COND_OK;
}

void fossil_threads_cond_dispose(fossil_threads_cond_t *c) {
    if (!c || !c->valid) return;

#if defined(_WIN32)
    CONDITION_VARIABLE *cv = (CONDITION_VARIABLE*)c->handle;
    free(cv); /* Windows condition vars don’t need explicit destruction */
#else
    pthread_cond_t *pc = (pthread_cond_t*)c->handle;
    if (pc) {
        pthread_cond_destroy(pc);
        free(pc);
    }
#endif
    fossil__cond_zero(c);
}

/* ---------- Wait / signal ---------- */

int fossil_threads_cond_wait(fossil_threads_cond_t *c, fossil_threads_mutex_t *m) {
    if (!c || !m || !c->valid || !m->valid) return FOSSIL_THREADS_COND_EINVAL;

#if defined(_WIN32)
    if (!SleepConditionVariableCS(
        (CONDITION_VARIABLE*)c->handle,
        (CRITICAL_SECTION*)m->handle,
        INFINITE))
    {
        return FOSSIL_THREADS_COND_EINTERNAL;
    }
    return FOSSIL_THREADS_COND_OK;
#else
    int rc = pthread_cond_wait((pthread_cond_t*)c->handle,
                               (pthread_mutex_t*)m->handle);
    return rc == 0 ? FOSSIL_THREADS_COND_OK : rc;
#endif
}

int fossil_threads_cond_timedwait(fossil_threads_cond_t *c,
                                  fossil_threads_mutex_t *m,
                                  unsigned int ms) {
    if (!c || !m || !c->valid || !m->valid) return FOSSIL_THREADS_COND_EINVAL;

#if defined(_WIN32)
    BOOL ok = SleepConditionVariableCS(
        (CONDITION_VARIABLE*)c->handle,
        (CRITICAL_SECTION*)m->handle,
        ms
    );
    if (ok) return FOSSIL_THREADS_COND_OK;
    if (GetLastError() == ERROR_TIMEOUT) return FOSSIL_THREADS_COND_ETIMEDOUT;
    return FOSSIL_THREADS_COND_EINTERNAL;
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    ts.tv_sec += ms / 1000u;
    ts.tv_nsec += (long)(ms % 1000u) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000L;
    }

    int rc = pthread_cond_timedwait((pthread_cond_t*)c->handle,
                                    (pthread_mutex_t*)m->handle,
                                    &ts);
    if (rc == 0) return FOSSIL_THREADS_COND_OK;
    if (rc == ETIMEDOUT) return FOSSIL_THREADS_COND_ETIMEDOUT;
    return rc;
#endif
}

int fossil_threads_cond_signal(fossil_threads_cond_t *c) {
    if (!c || !c->valid) return FOSSIL_THREADS_COND_EINVAL;

#if defined(_WIN32)
    WakeConditionVariable((CONDITION_VARIABLE*)c->handle);
    return FOSSIL_THREADS_COND_OK;
#else
    return pthread_cond_signal((pthread_cond_t*)c->handle);
#endif
}

int fossil_threads_cond_broadcast(fossil_threads_cond_t *c) {
    if (!c || !c->valid) return FOSSIL_THREADS_COND_EINVAL;

#if defined(_WIN32)
    WakeAllConditionVariable((CONDITION_VARIABLE*)c->handle);
    return FOSSIL_THREADS_COND_OK;
#else
    return pthread_cond_broadcast((pthread_cond_t*)c->handle);
#endif
}
