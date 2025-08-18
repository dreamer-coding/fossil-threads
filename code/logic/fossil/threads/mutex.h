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
#ifndef FOSSIL_THREADS_MUTEX_H
#define FOSSIL_THREADS_MUTEX_H

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

typedef struct fossil_threads_mutex {
    void *handle;   /* CRITICAL_SECTION* or pthread_mutex_t* */
    int   valid;    /* 1 if initialized */
} fossil_threads_mutex_t;

/* ---------- Lifecycle ---------- */

// *****************************************************************************
// Function prototypes
// *****************************************************************************

/* Initialize a mutex. Returns 0 on success. */
FOSSIL_THREADS_API int fossil_threads_mutex_init(fossil_threads_mutex_t *m);

/* Destroy a mutex. Safe to call on zeroed or already-destroyed. */
FOSSIL_THREADS_API void fossil_threads_mutex_dispose(fossil_threads_mutex_t *m);

/* ---------- Locking ---------- */

/* Lock the mutex, blocking until available. Returns 0 on success. */
FOSSIL_THREADS_API int fossil_threads_mutex_lock(fossil_threads_mutex_t *m);

/* Unlock the mutex. Returns 0 on success. */
FOSSIL_THREADS_API int fossil_threads_mutex_unlock(fossil_threads_mutex_t *m);

/* Try to lock without blocking. Returns 0 if locked, nonzero otherwise. */
FOSSIL_THREADS_API int fossil_threads_mutex_trylock(fossil_threads_mutex_t *m);

/* Error codes */
enum {
    FOSSIL_THREADS_MUTEX_OK       = 0,
    FOSSIL_THREADS_MUTEX_EINVAL   = 22,  /* invalid arg */
    FOSSIL_THREADS_MUTEX_EBUSY    = 16,  /* already locked (trylock only) */
    FOSSIL_THREADS_MUTEX_EINTERNAL= 199
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

#endif /* FOSSIL_THREADS_MUTEX_H */
