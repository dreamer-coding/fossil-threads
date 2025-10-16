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
#ifndef FOSSIL_THREADS_BARRIER_H
#define FOSSIL_THREADS_BARRIER_H

#include "mutex.h"
#include "cond.h"

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

// ===========================================================
// Barrier Type
// ===========================================================
typedef struct fossil_threads_barrier_t {
    fossil_threads_mutex_t mutex;     // internal lock
    fossil_threads_cond_t cond;       // condition variable
    size_t threshold;         // number of threads required
    size_t count;             // current count
    size_t cycle;             // used for cyclic barriers
    bool cyclic;              // whether barrier resets automatically
    bool destroyed;           // internal safety flag
} fossil_threads_barrier_t;

// ===========================================================
// API
// ===========================================================

/**
 * @brief Initialize a barrier for `threshold` threads.
 *
 * @param barrier Pointer to a fossil_threads_barrier_t
 * @param threshold Number of threads to synchronize (must be >= 1)
 * @param cyclic Whether to reset barrier automatically after release
 * @return 0 on success, nonzero on failure
 */
int fossil_threads_barrier_init(fossil_threads_barrier_t* barrier, size_t threshold, bool cyclic);

/**
 * @brief Wait at the barrier until the required number of threads arrive.
 *
 * @param barrier Pointer to barrier
 * @return 0 if successful, nonzero on error or if barrier is destroyed
 */
int fossil_threads_barrier_wait(fossil_threads_barrier_t* barrier);

/**
 * @brief Wait at the barrier with timeout in milliseconds.
 *
 * @param barrier Pointer to barrier
 * @param timeout_ms Timeout in milliseconds
 * @return 0 if barrier released, ETIMEDOUT if timeout occurred
 */
int fossil_threads_barrier_wait_timeout(fossil_threads_barrier_t* barrier, unsigned long timeout_ms);

/**
 * @brief Reset the barrier manually (only for cyclic barriers).
 */
void fossil_threads_barrier_reset(fossil_threads_barrier_t* barrier);

/**
 * @brief Destroy the barrier and free internal resources.
 */
void fossil_threads_barrier_destroy(fossil_threads_barrier_t* barrier);

#ifdef __cplusplus
}
#include <stdexcept>
#include <vector>
#include <string>

namespace fossil {

    namespace threads {

        /**
         * @brief C++ RAII wrapper for fossil_threads_barrier_t.
         *
         * This class provides a safe, exception-aware C++ interface for
         * managing barriers using the Fossil Threads API.
         * It disables copy construction and assignment to prevent
         * accidental resource duplication.
         */
        class Barrier {
        public:
            /**
             * @brief Constructs and initializes the barrier.
             *
             * Throws std::runtime_error if initialization fails.
             *
             * @param threshold Number of threads to synchronize (must be >= 1)
             * @param cyclic Whether to reset barrier automatically after release
             */
            Barrier(size_t threshold, bool cyclic) {
                int rc = fossil_threads_barrier_init(&barrier_, threshold, cyclic);
                if (rc != 0) {
                    throw std::runtime_error("Failed to initialize barrier");
                }
            }

            /**
             * @brief Destructor. Destroys the barrier and releases resources.
             */
            ~Barrier() {
                fossil_threads_barrier_destroy(&barrier_);
            }

            /**
             * @brief Deleted copy constructor.
             */
            Barrier(const Barrier&) = delete;

            /**
             * @brief Deleted copy assignment operator.
             */
            Barrier& operator=(const Barrier&) = delete;

            /**
             * @brief Wait at the barrier until the required number of threads arrive.
             *
             * @return 0 if successful, nonzero on error or if barrier is destroyed
             */
            int wait() {
                return fossil_threads_barrier_wait(&barrier_);
            }

            /**
             * @brief Wait at the barrier with timeout in milliseconds.
             *
             * @param timeout_ms Timeout in milliseconds
             * @return 0 if barrier released, ETIMEDOUT if timeout occurred
             */
            int wait_timeout(unsigned long timeout_ms) {
                return fossil_threads_barrier_wait_timeout(&barrier_, timeout_ms);
            }

            /**
             * @brief Reset the barrier manually (only for cyclic barriers).
             */
            void reset() {
                fossil_threads_barrier_reset(&barrier_);
            }

            /**
             * @brief Returns a pointer to the underlying native handle.
             *
             * @return Pointer to the fossil_threads_barrier_t structure.
             */
            fossil_threads_barrier_t* native_handle() { return &barrier_; }

        private:
            fossil_threads_barrier_t barrier_; /**< The underlying barrier structure. */
        };

} // namespace threads

} // namespace fossil

#endif

#endif /* FOSSIL_THREADS_COND_H */
