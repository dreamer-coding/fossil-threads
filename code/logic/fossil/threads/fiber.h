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
#ifndef FOSSIL_THREADS_FIBER_H
#define FOSSIL_THREADS_FIBER_H

#include <stddef.h>

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

/* ---------- Error codes ---------- */
enum {
    FOSSIL_FIBER_OK       = 0,
    FOSSIL_FIBER_EINVAL   = 22,   /* invalid argument */
    FOSSIL_FIBER_EBUSY    = 16,   /* wrong state/busy */
    FOSSIL_FIBER_ENOMEM   = 12,   /* allocation failure */
    FOSSIL_FIBER_ENOTSUP  = 95,   /* backend not supported on this platform */
    FOSSIL_FIBER_EINTERNAL= 199   /* generic failure */
};

/* ---------- Types ---------- */
typedef void (*fossil_threads_fiber_func)(void *arg);

typedef struct fossil_threads_fiber {
    void *handle;                   /* OS/backend handle */
    fossil_threads_fiber_func func; /* user entry (POSIX backend) */
    void *arg;                      /* user data (POSIX backend) */
    int finished;                   /* set when user func returns */
    size_t stack_size;              /* for POSIX backend */
} fossil_threads_fiber_t;

/* A convenience alias for the “main/scheduler” fiber of a thread. */
typedef fossil_threads_fiber_t fossil_threads_fiber_main_t;

/* ---------- Lifecycle ---------- */

// *****************************************************************************
// Function prototypes
// *****************************************************************************

/* Convert the *current OS thread* into a fiber and store as main/scheduler.
   Must be called once per OS thread before creating/using other fibers. */
FOSSIL_THREADS_API int
fossil_threads_fiber_init_self(fossil_threads_fiber_main_t *out_main);

/* Create a new fiber with `stack_size` bytes (0 = backend default). */
FOSSIL_THREADS_API int
fossil_threads_fiber_create(
    fossil_threads_fiber_t *fiber,
    fossil_threads_fiber_func func,
    void *arg,
    size_t stack_size
);

/* Switch to the target fiber (cooperative). Returns when the target yields. */
FOSSIL_THREADS_API int
fossil_threads_fiber_resume(fossil_threads_fiber_t *to);

/* Yield from the current fiber to a specific target (commonly the main). */
FOSSIL_THREADS_API int
fossil_threads_fiber_yield_to(fossil_threads_fiber_t *to);

/* Get the currently running fiber object for this OS thread (may be NULL before init). */
FOSSIL_THREADS_API fossil_threads_fiber_t*
fossil_threads_fiber_current(void);

/* Destroy a fiber. It must not be the currently running fiber. */
FOSSIL_THREADS_API void
fossil_threads_fiber_dispose(fossil_threads_fiber_t *fiber);

/* Utility: has the fiber finished running its entry function? */
FOSSIL_THREADS_API int
fossil_threads_fiber_finished(const fossil_threads_fiber_t *fiber);

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

#endif /* FOSSIL_THREADS_FIBER_H */
