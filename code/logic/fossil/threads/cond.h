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
#ifndef FOSSIL_THREADS_COND_H
#define FOSSIL_THREADS_COND_H

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_WIN32) && defined(FOSSIL_THREADS_BUILD_DLL)
#  define FOSSIL_THREADS_API __declspec(dllexport)
#elif defined(_WIN32) && defined(FOSSIL_THREADS_USE_DLL)
#  define FOSSIL_THREADS_API __declspec(dllimport)
#else
#  define FOSSIL_THREADS_API
#endif

/* ---------- Types ---------- */

typedef struct fossil_threads_cond {
    void *handle;  /* CONDITION_VARIABLE* on Win, pthread_cond_t* on POSIX */
    int   valid;
} fossil_threads_cond_t;

/* ---------- Lifecycle ---------- */

// *****************************************************************************
// Function prototypes
// *****************************************************************************

FOSSIL_THREADS_API int  fossil_threads_cond_init(fossil_threads_cond_t *c);
FOSSIL_THREADS_API void fossil_threads_cond_dispose(fossil_threads_cond_t *c);

/* ---------- Wait / signal ---------- */

/* Wait on condition with given mutex locked. Atomically unlocks mutex while waiting. */
FOSSIL_THREADS_API int fossil_threads_cond_wait(
    fossil_threads_cond_t *c,
    fossil_threads_mutex_t *m
);

/* Wait with timeout in milliseconds. Returns 0 if signaled, 1 if timeout, or error. */
FOSSIL_THREADS_API int fossil_threads_cond_timedwait(
    fossil_threads_cond_t *c,
    fossil_threads_mutex_t *m,
    unsigned int ms
);

/* Wake one waiting thread. */
FOSSIL_THREADS_API int fossil_threads_cond_signal(fossil_threads_cond_t *c);

/* Wake all waiting threads. */
FOSSIL_THREADS_API int fossil_threads_cond_broadcast(fossil_threads_cond_t *c);

/* Error codes */
enum {
    FOSSIL_THREADS_COND_OK        = 0,
    FOSSIL_THREADS_COND_EINVAL    = 22,
    FOSSIL_THREADS_COND_ETIMEDOUT = 110,
    FOSSIL_THREADS_COND_EINTERNAL = 199
};

#ifdef __cplusplus
}
#include <stdexcept>
#include <vector>
#include <string>

namespace fossil {

namespace threads {



} // namespace threads

} // namespace fossil

#endif

#endif /* FOSSIL_THREADS_COND_H */
