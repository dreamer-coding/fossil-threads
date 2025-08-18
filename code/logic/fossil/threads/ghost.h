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
#ifndef FOSSIL_THREADS_GHOST_H
#define FOSSIL_THREADS_GHOST_H

#include <stddef.h>
#include <stdint.h>

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
    FOSSIL_GHOST_OK       = 0,
    FOSSIL_GHOST_EINVAL   = 22,
    FOSSIL_GHOST_ENOMEM   = 12,
    FOSSIL_GHOST_EBUSY    = 16,
    FOSSIL_GHOST_ENOTSUP  = 95,
    FOSSIL_GHOST_EINTERNAL= 199
};

/* ---------- Types ---------- */

/* Ghost thread entry function */
typedef void (*fossil_threads_ghost_func)(void *arg);

/* Ghost thread handle */
typedef struct fossil_threads_ghost {
    char id[64];                     /* unique global ID */
    void *state;                     /* application state */
    fossil_threads_ghost_func func;  /* entry function */
    void *arg;                       /* user data */
    int finished;                    /* has the ghost thread completed */
} fossil_threads_ghost_t;

/* ---------- Lifecycle ---------- */

// *****************************************************************************
// Function prototypes
// *****************************************************************************

/* Initialize a ghost thread system for the current process/thread */
int fossil_threads_ghost_init(void);

/* Create a ghost thread with a unique ID */
int fossil_threads_ghost_create(
    fossil_threads_ghost_t *ghost,
    const char *id,
    fossil_threads_ghost_func func,
    void *arg
);

/* Execute the next step of the ghost thread (consensus-ordered) */
int fossil_threads_ghost_step(fossil_threads_ghost_t *ghost);

/* Query current state */
int fossil_threads_ghost_state(fossil_threads_ghost_t *ghost, void **out_state);

/* Check if the ghost thread has finished */
int fossil_threads_ghost_finished(fossil_threads_ghost_t *ghost);

/* Dispose of ghost thread resources */
void fossil_threads_ghost_dispose(fossil_threads_ghost_t *ghost);

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

#endif /* FOSSIL_THREADS_GHOST_H */