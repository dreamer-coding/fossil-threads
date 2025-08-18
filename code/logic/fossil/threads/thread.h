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
#ifndef FOSSIL_THREADS_THREAD_H
#define FOSSIL_THREADS_THREAD_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ---------- Configuration / visibility ---------- */
#if defined(_WIN32) && defined(FOSSIL_THREADS_BUILD_DLL)
#  define FOSSIL_THREADS_API __declspec(dllexport)
#elif defined(_WIN32) && defined(FOSSIL_THREADS_USE_DLL)
#  define FOSSIL_THREADS_API __declspec(dllimport)
#else
#  define FOSSIL_THREADS_API
#endif

/* ---------- Types ---------- */

/* User thread entry point: return value is optionally retrieved by join(). */
typedef void* (*fossil_threads_thread_func)(void *arg);

/* Opaque-ish handle; fields are public for POD-ness but do not touch them. */
typedef struct fossil_threads_thread {
    void *handle;          /* OS handle: HANDLE on Win, pthread_t* heap ptr on POSIX */
    unsigned long id;      /* OS thread id */
    void *retval;          /* join() result (set by wrapper) */
    int   joinable;        /* 1 if joinable, 0 if detached */
    int   started;         /* 1 after successful create */
    int   finished;        /* 1 after thread function returns */
} fossil_threads_thread_t;

/* ---------- Lifecycle ---------- */

// *****************************************************************************
// Function prototypes
// *****************************************************************************

/* Create a joinable thread that runs 'func(arg)'. Returns 0 on success. */
FOSSIL_THREADS_API int fossil_threads_thread_create(fossil_threads_thread_t *thread, fossil_threads_thread_func func, void *arg);

/* Join a joinable thread. If retval != NULL, *retval receives the thread's return.
   After join, the thread handle is cleaned up. Returns 0 on success. */
FOSSIL_THREADS_API int fossil_threads_thread_join(fossil_threads_thread_t *thread, void **retval);

/* Detach a thread. After detaching, resources are released on exit and join() is invalid. */
FOSSIL_THREADS_API int fossil_threads_thread_detach(fossil_threads_thread_t *thread);

/* Optionally initialize a zeroed/unused struct (safe no-op if already zeroed). */
FOSSIL_THREADS_API void fossil_threads_thread_init(fossil_threads_thread_t *thread);

/* Clean up a non-running handle if any (safe for zeroed or already-joined objects). */
FOSSIL_THREADS_API void fossil_threads_thread_dispose(fossil_threads_thread_t *thread);

/* ---------- Management / utilities ---------- */

/* Yield the processor to another thread (hint). Returns 0 on success. */
FOSSIL_THREADS_API int fossil_threads_thread_yield(void);

/* Sleep for 'ms' milliseconds. Returns 0 on success. */
FOSSIL_THREADS_API int fossil_threads_thread_sleep_ms(unsigned int ms);

/* Get the current thread id as an unsigned long (best-effort portable). */
FOSSIL_THREADS_API unsigned long fossil_threads_thread_id(void);

/* Compare if two thread objects refer to the same underlying thread. */
FOSSIL_THREADS_API int fossil_threads_thread_equal(const fossil_threads_thread_t *t1, const fossil_threads_thread_t *t2);

/* Error codes (subset mirrors common errno/GetLastError patterns) */
enum {
    FOSSIL_THREADS_OK            = 0,
    FOSSIL_THREADS_EINVAL        = 22,   /* invalid argument */
    FOSSIL_THREADS_EBUSY         = 16,   /* resource busy / wrong state */
    FOSSIL_THREADS_ENOMEM        = 12,   /* allocation failed */
    FOSSIL_THREADS_EPERM         = 1,    /* not permitted / wrong usage */
    FOSSIL_THREADS_EINTERNAL     = 199   /* generic internal failure */
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

#endif /* FOSSIL_THREADS_THREAD_H */
