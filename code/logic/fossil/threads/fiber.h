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

/* ---------- Types ---------- */

/* Fiber entry point */
typedef void (*fossil_threads_fiber_func)(void *arg);

/* Fiber handle */
typedef struct fossil_threads_fiber {
    void *handle;        /* OS or library specific */
    fossil_threads_fiber_func func;
    void *arg;
    int finished;
} fossil_threads_fiber_t;

/* ---------- Lifecycle ---------- */

// *****************************************************************************
// Function prototypes
// *****************************************************************************

/* Convert current thread to fiber context (required before creating fibers). */
FOSSIL_THREADS_API int fossil_threads_fiber_self(fossil_threads_fiber_t *out);

/* Create a new fiber with given stack size (0 = default). */
FOSSIL_THREADS_API int fossil_threads_fiber_create(
    fossil_threads_fiber_t *fiber,
    fossil_threads_fiber_func func,
    void *arg,
    size_t stack_size
);

/* Switch execution to the given fiber. */
FOSSIL_THREADS_API int fossil_threads_fiber_switch(
    fossil_threads_fiber_t *to
);

/* Get the currently running fiber handle. */
FOSSIL_THREADS_API fossil_threads_fiber_t* fossil_threads_fiber_current(void);

/* Destroy fiber (stack freed). Cannot destroy current fiber. */
FOSSIL_THREADS_API void fossil_threads_fiber_dispose(fossil_threads_fiber_t *fiber);

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
