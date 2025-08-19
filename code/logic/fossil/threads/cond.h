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

#include "mutex.h"

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

/**
 * @brief Initialize a condition variable.
 *
 * This function initializes the given condition variable structure.
 * Must be called before using the condition variable.
 *
 * @param c Pointer to the condition variable to initialize.
 * @return FOSSIL_THREADS_COND_OK on success, or error code on failure.
 */
FOSSIL_THREADS_API int fossil_threads_cond_init(fossil_threads_cond_t *c);

/**
 * @brief Dispose of a condition variable.
 *
 * This function releases any resources associated with the condition variable.
 * After calling this, the condition variable must not be used unless re-initialized.
 *
 * @param c Pointer to the condition variable to dispose.
 */
FOSSIL_THREADS_API void fossil_threads_cond_dispose(fossil_threads_cond_t *c);

/* ---------- Wait / signal ---------- */

/**
 * @brief Wait on a condition variable.
 *
 * Atomically unlocks the given mutex and waits for the condition variable to be signaled.
 * When the wait is over, the mutex is re-locked before returning.
 *
 * @param c Pointer to the condition variable.
 * @param m Pointer to the mutex (must be locked by the calling thread).
 * @return FOSSIL_THREADS_COND_OK on success, or error code on failure.
 */
FOSSIL_THREADS_API int fossil_threads_cond_wait(
    fossil_threads_cond_t *c,
    fossil_threads_mutex_t *m
);

/**
 * @brief Wait on a condition variable with a timeout.
 *
 * Atomically unlocks the given mutex and waits for the condition variable to be signaled
 * or for the specified timeout (in milliseconds) to elapse. When the wait is over,
 * the mutex is re-locked before returning.
 *
 * @param c Pointer to the condition variable.
 * @param m Pointer to the mutex (must be locked by the calling thread).
 * @param ms Timeout in milliseconds.
 * @return FOSSIL_THREADS_COND_OK if signaled, FOSSIL_THREADS_COND_ETIMEDOUT if timeout,
 *         or error code on failure.
 */
FOSSIL_THREADS_API int fossil_threads_cond_timedwait(
    fossil_threads_cond_t *c,
    fossil_threads_mutex_t *m,
    unsigned int ms
);

/**
 * @brief Wake one thread waiting on the condition variable.
 *
 * Signals the condition variable, waking up one waiting thread (if any).
 *
 * @param c Pointer to the condition variable.
 * @return FOSSIL_THREADS_COND_OK on success, or error code on failure.
 */
FOSSIL_THREADS_API int fossil_threads_cond_signal(fossil_threads_cond_t *c);

/**
 * @brief Wake all threads waiting on the condition variable.
 *
 * Broadcasts to the condition variable, waking up all waiting threads.
 *
 * @param c Pointer to the condition variable.
 * @return FOSSIL_THREADS_COND_OK on success, or error code on failure.
 */
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

    /**
     * @brief C++ RAII wrapper for fossil_threads_cond_t.
     *
     * This class provides a safe, exception-aware C++ interface for
     * managing condition variables using the Fossil Threads API.
     * It disables copy construction and assignment to prevent
     * accidental resource duplication.
     */
    class Cond {
    public:
        /**
         * @brief Constructs and initializes the condition variable.
         *
         * Throws std::runtime_error if initialization fails.
         */
        Cond() {
            int rc = fossil_threads_cond_init(&cond_);
            if (rc != FOSSIL_THREADS_COND_OK) {
                throw std::runtime_error("Failed to initialize condition variable");
            }
        }

        /**
         * @brief Destructor. Disposes of the condition variable.
         *
         * Ensures resources are released when the object goes out of scope.
         */
        ~Cond() {
            fossil_threads_cond_dispose(&cond_);
        }

        /**
         * @brief Deleted copy constructor.
         *
         * Prevents copying of the condition variable wrapper.
         */
        Cond(const Cond&) = delete;

        /**
         * @brief Deleted copy assignment operator.
         *
         * Prevents assignment of the condition variable wrapper.
         */
        Cond& operator=(const Cond&) = delete;

        /**
         * @brief Waits on the condition variable.
         *
         * Atomically unlocks the given mutex and waits for the condition variable to be signaled.
         * The mutex is re-locked before returning.
         *
         * @param mutex Pointer to the mutex (must be locked by the calling thread).
         * @return FOSSIL_THREADS_COND_OK on success, or error code on failure.
         */
        int wait(fossil_threads_mutex_t *mutex) {
            return fossil_threads_cond_wait(&cond_, mutex);
        }

        /**
         * @brief Waits on the condition variable with a timeout.
         *
         * Atomically unlocks the given mutex and waits for the condition variable to be signaled
         * or for the specified timeout (in milliseconds) to elapse.
         * The mutex is re-locked before returning.
         *
         * @param mutex Pointer to the mutex (must be locked by the calling thread).
         * @param ms Timeout in milliseconds.
         * @return FOSSIL_THREADS_COND_OK if signaled, FOSSIL_THREADS_COND_ETIMEDOUT if timeout,
         *         or error code on failure.
         */
        int timed_wait(fossil_threads_mutex_t *mutex, unsigned int ms) {
            return fossil_threads_cond_timedwait(&cond_, mutex, ms);
        }

        /**
         * @brief Wakes one thread waiting on the condition variable.
         *
         * @return FOSSIL_THREADS_COND_OK on success, or error code on failure.
         */
        int signal() {
            return fossil_threads_cond_signal(&cond_);
        }

        /**
         * @brief Wakes all threads waiting on the condition variable.
         *
         * @return FOSSIL_THREADS_COND_OK on success, or error code on failure.
         */
        int broadcast() {
            return fossil_threads_cond_broadcast(&cond_);
        }

        /**
         * @brief Returns a pointer to the underlying native handle.
         *
         * @return Pointer to the fossil_threads_cond_t structure.
         */
        fossil_threads_cond_t* native_handle() { return &cond_; }

    private:
        fossil_threads_cond_t cond_; /**< The underlying condition variable structure. */
    };

} // namespace threads

} // namespace fossil

#endif

#endif /* FOSSIL_THREADS_COND_H */
