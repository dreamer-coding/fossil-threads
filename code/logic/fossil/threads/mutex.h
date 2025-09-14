/**
 * -----------------------------------------------------------------------------
 * Project: Fossil Logic
 *
 * This file is part of the Fossil Logic project, which aims to develop
 * high-performance, cross-platform applications and libraries. The code
 * contained herein is licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License. You may obtain
 * a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 * Author: Michael Gene Brockus (Dreamer)
 * Date: 04/05/2014
 *
 * Copyright (C) 2014-2025 Fossil Logic. All rights reserved.
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

/* 
 * Initializes a mutex object.
 * 
 * Parameters:
 *   m - Pointer to a fossil_threads_mutex_t structure to initialize.
 * 
 * Returns:
 *   0 on success, or a nonzero error code on failure.
 * 
 * Notes:
 *   - The mutex must be disposed with fossil_threads_mutex_dispose when no longer needed.
 *   - The mutex is initially unlocked.
 */
FOSSIL_THREADS_API int fossil_threads_mutex_init(fossil_threads_mutex_t *m);

/* 
 * Disposes (destroys) a mutex object.
 * 
 * Parameters:
 *   m - Pointer to a fossil_threads_mutex_t structure to dispose.
 * 
 * Notes:
 *   - Safe to call on a zeroed or already-disposed mutex.
 *   - After disposal, the mutex should not be used unless re-initialized.
 */
FOSSIL_THREADS_API void fossil_threads_mutex_dispose(fossil_threads_mutex_t *m);

/* ---------- Locking ---------- */

/* 
 * Locks the mutex, blocking the calling thread until the mutex becomes available.
 * 
 * Parameters:
 *   m - Pointer to a fossil_threads_mutex_t structure to lock.
 * 
 * Returns:
 *   0 on success, or a nonzero error code on failure.
 * 
 * Notes:
 *   - If the mutex is already locked by another thread, this call will block.
 *   - Recursive locking is not supported unless implemented by the platform.
 */
FOSSIL_THREADS_API int fossil_threads_mutex_lock(fossil_threads_mutex_t *m);

/* 
 * Unlocks the mutex.
 * 
 * Parameters:
 *   m - Pointer to a fossil_threads_mutex_t structure to unlock.
 * 
 * Returns:
 *   0 on success, or a nonzero error code on failure.
 * 
 * Notes:
 *   - Only the thread that locked the mutex should unlock it.
 *   - Unlocking an unlocked mutex results in undefined behavior.
 */
FOSSIL_THREADS_API int fossil_threads_mutex_unlock(fossil_threads_mutex_t *m);

/* 
 * Attempts to lock the mutex without blocking.
 * 
 * Parameters:
 *   m - Pointer to a fossil_threads_mutex_t structure to try to lock.
 * 
 * Returns:
 *   0 if the mutex was successfully locked.
 *   FOSSIL_THREADS_MUTEX_EBUSY if the mutex is already locked.
 *   Other nonzero error code on failure.
 * 
 * Notes:
 *   - This function never blocks.
 *   - Useful for implementing timed or non-blocking synchronization.
 */
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
#include <utility>

namespace fossil {

namespace threads {

    class Mutex {
    public:
        /**
         * @brief Constructs a Mutex object and initializes the underlying mutex.
         * 
         * Throws std::runtime_error if initialization fails.
         */
        Mutex() {
            int rc = fossil_threads_mutex_init(&m_);
            if (rc != FOSSIL_THREADS_MUTEX_OK) {
            throw std::runtime_error("Failed to initialize mutex");
            }
        }

        /**
         * @brief Destroys the Mutex object and disposes the underlying mutex.
         */
        ~Mutex() {
            fossil_threads_mutex_dispose(&m_);
        }

        /**
         * @brief Locks the mutex, blocking if necessary.
         * 
         * Throws std::runtime_error if locking fails.
         */
        void lock() {
            int rc = fossil_threads_mutex_lock(&m_);
            if (rc != FOSSIL_THREADS_MUTEX_OK) {
            throw std::runtime_error("Failed to lock mutex");
            }
        }

        /**
         * @brief Unlocks the mutex.
         * 
         * Throws std::runtime_error if unlocking fails.
         */
        void unlock() {
            int rc = fossil_threads_mutex_unlock(&m_);
            if (rc != FOSSIL_THREADS_MUTEX_OK) {
            throw std::runtime_error("Failed to unlock mutex");
            }
        }

        /**
         * @brief Attempts to lock the mutex without blocking.
         * 
         * @return true if the mutex was successfully locked, false if it was already locked.
         * 
         * Throws std::runtime_error if an error occurs other than the mutex being busy.
         */
        bool try_lock() {
            int rc = fossil_threads_mutex_trylock(&m_);
            if (rc == FOSSIL_THREADS_MUTEX_OK) return true;
            if (rc == FOSSIL_THREADS_MUTEX_EBUSY) return false;
            throw std::runtime_error("Failed to try_lock mutex");
        }

        /**
         * @brief Deleted copy constructor to prevent copying.
         */
        Mutex(const Mutex&) = delete;

        /**
         * @brief Deleted copy assignment operator to prevent copying.
         */
        Mutex& operator=(const Mutex&) = delete;

    private:
        fossil_threads_mutex_t m_;
    };

} // namespace threads

} // namespace fossil

#endif

#endif /* FOSSIL_THREADS_MUTEX_H */
