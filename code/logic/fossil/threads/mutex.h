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

#include <stddef.h>
#include <stdbool.h>

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
#include <thread>
#include <chrono>
#include <atomic>
#include <type_traits>

namespace fossil {

    namespace threads {

        /**
         * @brief A thin C++ RAII wrapper around the C mutex API provided by Fossil Threads.
         *
         * This class owns a fossil_threads_mutex_t and exposes a C++-friendly interface:
         *  - RAII initialization and disposal in ctor/dtor
         *  - non-copyable, movable
         *  - lock/unlock/try_lock operations that translate error codes into exceptions
         *  - timed try-lock helpers implemented via repeated non-blocking attempts
         *  - a small LockGuard nested class for scoped locking
         *
         * The underlying C mutex implementation may be platform-specific (e.g. CRITICAL_SECTION
         * on Windows or pthread_mutex_t on POSIX). This wrapper does not attempt to emulate
         * features not provided by the underlying implementation (for example, fairness or
         * recursive locking).
         */
        class Mutex {
        public:
            /**
             * @brief Construct and initialize the underlying C mutex.
             *
             * Calls fossil_threads_mutex_init to initialize the internal handle. If the C API
             * returns an error, a std::runtime_error is thrown. After successful construction
             * the object is considered initialized and may be used with lock/unlock/try_lock.
             *
             * Throws:
             *   std::runtime_error on initialization failure.
             */
            Mutex()
                : m_{nullptr, 0}, initialized_{false}
            {
                int rc = fossil_threads_mutex_init(&m_);
                if (rc != FOSSIL_THREADS_MUTEX_OK) {
                    throw std::runtime_error("Failed to initialize mutex");
                }
                initialized_.store(true, std::memory_order_release);
            }

            /**
             * @brief Destructor disposes the underlying C mutex if it is initialized.
             *
             * Safe to call on a default-constructed or already-moved-from object. Disposal
             * uses fossil_threads_mutex_dispose and the initialized_ flag is cleared to avoid
             * double-dispose.
             */
            ~Mutex() {
                // allow safe double-dispose
                if (initialized_.load(std::memory_order_acquire)) {
                    fossil_threads_mutex_dispose(&m_);
                    initialized_.store(false, std::memory_order_release);
                }
            }

            /**
             * @brief Move constructor.
             *
             * Transfers ownership of the underlying C mutex handle from other to this object.
             * After the move, other is placed into a non-initialized state. This operation is
             * noexcept to allow usage in containers that require noexcept move construction.
             *
             * Parameters:
             *   other - the Mutex to move from (will be left in a non-initialized state).
             */
            Mutex(Mutex&& other) noexcept
                : m_(std::exchange(other.m_, fossil_threads_mutex_t{nullptr, 0})),
                initialized_(other.initialized_.load(std::memory_order_acquire))
            {
                other.initialized_.store(false, std::memory_order_release);
            }

            /**
             * @brief Move assignment operator.
             *
             * Disposes any currently-owned mutex (if initialized) and then takes ownership of
             * the mutex owned by other. The moved-from object is left in a non-initialized
             * state. This function is noexcept.
             *
             * Parameters:
             *   other - the Mutex to move-assign from.
             *
             * Returns:
             *   *this
             */
            Mutex& operator=(Mutex&& other) noexcept {
                if (this != &other) {
                    if (initialized_.load(std::memory_order_acquire)) {
                        fossil_threads_mutex_dispose(&m_);
                    }
                    m_ = std::exchange(other.m_, fossil_threads_mutex_t{nullptr, 0});
                    initialized_.store(other.initialized_.load(std::memory_order_acquire),
                                    std::memory_order_release);
                    other.initialized_.store(false, std::memory_order_release);
                }
                return *this;
            }

            // Non-copyable: copying a mutex (shallow or deep) is error-prone and not allowed.
            Mutex(const Mutex&) = delete;
            Mutex& operator=(const Mutex&) = delete;

            /**
             * @brief Lock the mutex, blocking until the lock is acquired.
             *
             * Calls the underlying fossil_threads_mutex_lock and translates non-zero return
             * values into std::runtime_error exceptions. If the Mutex object is not
             * initialized, a runtime_error is thrown as well.
             *
             * Throws:
             *   std::runtime_error if the mutex is uninitialized or if the underlying C API fails.
             */
            void lock() {
                if (!initialized_.load(std::memory_order_acquire)) {
                    throw std::runtime_error("Lock on uninitialized mutex");
                }
                int rc = fossil_threads_mutex_lock(&m_);
                if (rc != FOSSIL_THREADS_MUTEX_OK) {
                    throw std::runtime_error("Failed to lock mutex");
                }
            }

            /**
             * @brief Unlock the mutex.
             *
             * Calls fossil_threads_mutex_unlock. On error, throws std::runtime_error. The caller
             * must ensure the mutex was previously locked by the calling thread where required
             * by the underlying implementation.
             *
             * Throws:
             *   std::runtime_error if the mutex is uninitialized or if the underlying C API fails.
             */
            void unlock() {
                if (!initialized_.load(std::memory_order_acquire)) {
                    throw std::runtime_error("Unlock on uninitialized mutex");
                }
                int rc = fossil_threads_mutex_unlock(&m_);
                if (rc != FOSSIL_THREADS_MUTEX_OK) {
                    throw std::runtime_error("Failed to unlock mutex");
                }
            }

            /**
             * @brief Attempt to lock the mutex without blocking.
             *
             * Returns true if the lock was acquired, false if the mutex was already locked.
             * On any other error from the underlying API, an exception is thrown.
             *
             * Throws:
             *   std::runtime_error if the mutex is uninitialized or if the underlying call fails.
             */
            bool try_lock() {
                if (!initialized_.load(std::memory_order_acquire)) {
                    throw std::runtime_error("Try_lock on uninitialized mutex");
                }
                int rc = fossil_threads_mutex_trylock(&m_);
                if (rc == FOSSIL_THREADS_MUTEX_OK) return true;
                if (rc == FOSSIL_THREADS_MUTEX_EBUSY) return false;
                throw std::runtime_error("Failed to try_lock mutex");
            }

            /**
             * @brief Attempt to lock the mutex, waiting up to rel_time duration.
             *
             * This implementation performs repeated non-blocking try_lock attempts until the
             * deadline is reached, yielding the thread for short intervals between attempts.
             * If rel_time is zero or negative, this function falls back to try_lock().
             *
             * Parameters:
             *   rel_time - maximum duration to wait before giving up.
             *
             * Returns:
             *   true if lock acquired, false if timeout elapsed without acquiring the lock.
             *
             * Throws:
             *   std::runtime_error on underlying API errors or if the Mutex is uninitialized.
             */
            bool try_lock_for(const std::chrono::steady_clock::duration& rel_time) {
                if (!initialized_.load(std::memory_order_acquire)) {
                    throw std::runtime_error("Timed try_lock on uninitialized mutex");
                }

                if (rel_time <= std::chrono::steady_clock::duration::zero()) {
                    return try_lock();
                }

                auto deadline = std::chrono::steady_clock::now() + rel_time;
                while (std::chrono::steady_clock::now() < deadline) {
                    int rc = fossil_threads_mutex_trylock(&m_);
                    if (rc == FOSSIL_THREADS_MUTEX_OK) return true;
                    if (rc != FOSSIL_THREADS_MUTEX_EBUSY) {
                        throw std::runtime_error("Failed to try_lock mutex");
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                return false;
            }

            /**
             * @brief Convenience overload: try_lock_for with milliseconds.
             *
             * Delegates to try_lock_for(duration).
             */
            bool try_lock_for(const std::chrono::milliseconds& ms) {
                return try_lock_for(std::chrono::duration_cast<std::chrono::steady_clock::duration>(ms));
            }

            /**
             * @brief Convenience overload: try_lock_for with seconds.
             *
             * Delegates to try_lock_for(duration).
             */
            bool try_lock_for(const std::chrono::seconds& s) {
                return try_lock_for(std::chrono::duration_cast<std::chrono::steady_clock::duration>(s));
            }

            /**
             * @brief Attempt to lock the mutex until the specified time point.
             *
             * Repeatedly attempts try_lock until the provided time point is reached. If the
             * time point is in the past or present, it falls back to a single try_lock call.
             *
             * Parameters:
             *   tp - absolute time point (steady_clock) after which the attempt should stop.
             *
             * Returns:
             *   true if lock acquired before tp, false if timeout.
             *
             * Throws:
             *   std::runtime_error on underlying API errors or if the Mutex is uninitialized.
             */
            bool try_lock_until(const std::chrono::steady_clock::time_point& tp) {
                if (!initialized_.load(std::memory_order_acquire)) {
                    throw std::runtime_error("Timed try_lock on uninitialized mutex");
                }

                if (std::chrono::steady_clock::now() >= tp) {
                    return try_lock();
                }

                while (std::chrono::steady_clock::now() < tp) {
                    int rc = fossil_threads_mutex_trylock(&m_);
                    if (rc == FOSSIL_THREADS_MUTEX_OK) return true;
                    if (rc != FOSSIL_THREADS_MUTEX_EBUSY) {
                        throw std::runtime_error("Failed to try_lock mutex");
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                return false;
            }

            /**
             * @brief A small RAII lock helper that locks the given Mutex on construction
             * and unlocks it on destruction.
             *
             * Use this to ensure exception-safe scoped locking:
             *   {
             *       Mutex::LockGuard g(mutex);
             *       // critical section
             *   } // mutex unlocked automatically
             *
             * The destructor swallows exceptions from unlock to avoid throwing during stack unwinding.
             */
            class LockGuard {
            public:
                /**
                 * @brief Construct and lock the supplied Mutex.
                 *
                 * Parameters:
                 *   m - reference to a Mutex to lock for the lifetime of the guard.
                 *
                 * Throws:
                 *   std::runtime_error if locking fails.
                 */
                explicit LockGuard(Mutex& m) : m_(m) { m_.lock(); }

                /**
                 * @brief Destructor unlocks the associated Mutex. noexcept to avoid throwing
                 * during unwinding; any exceptions from unlock are caught and ignored.
                 */
                ~LockGuard() noexcept { try { m_.unlock(); } catch (...) {} }

                LockGuard(const LockGuard&) = delete;
                LockGuard& operator=(const LockGuard&) = delete;
            private:
                Mutex& m_;
            };

        private:
            fossil_threads_mutex_t m_;         /**< Underlying C mutex handle and state */
            std::atomic<bool> initialized_;    /**< True when m_ has been successfully initialized */
        };

        static_assert(!std::is_copy_constructible_v<Mutex>, "Mutex must not be copyable");
        static_assert(std::is_nothrow_move_constructible_v<Mutex>, "Mutex should be noexcept-movable");

        } // namespace threads

} // namespace fossil

#endif

#endif /* FOSSIL_THREADS_MUTEX_H */
