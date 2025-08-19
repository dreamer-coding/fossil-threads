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

/* 
 * Create a joinable thread that runs 'func(arg)' in a new OS thread.
 * 
 * @param thread Pointer to a fossil_threads_thread_t structure to initialize.
 * @param func   Function pointer to the thread entry point (must match fossil_threads_thread_func signature).
 * @param arg    Argument to pass to the thread function (may be NULL).
 * @return       0 on success, or error code on failure (see error codes below).
 * 
 * On success, 'thread' is initialized and represents a joinable thread.
 * The thread will execute 'func(arg)' and may be joined or detached.
 */
FOSSIL_THREADS_API int fossil_threads_thread_create(
    fossil_threads_thread_t *thread, 
    fossil_threads_thread_func func, 
    void *arg
);

/* 
 * Join a joinable thread, blocking until it finishes execution.
 * 
 * @param thread Pointer to a fossil_threads_thread_t representing a joinable thread.
 * @param retval If not NULL, receives the return value from the thread function.
 * @return       0 on success, or error code on failure.
 * 
 * After a successful join, the thread handle is cleaned up and cannot be joined or detached again.
 * If the thread was already detached or not joinable, returns an error.
 */
FOSSIL_THREADS_API int fossil_threads_thread_join(
    fossil_threads_thread_t *thread, 
    void **retval
);

/* 
 * Detach a thread, allowing its resources to be released automatically when it finishes.
 * 
 * @param thread Pointer to a fossil_threads_thread_t representing a joinable thread.
 * @return       0 on success, or error code on failure.
 * 
 * After detaching, join() is invalid for this thread.
 * Detaching a thread that is already detached or not joinable returns an error.
 */
FOSSIL_THREADS_API int fossil_threads_thread_detach(
    fossil_threads_thread_t *thread
);

/* 
 * Initialize a fossil_threads_thread_t structure to a safe, zeroed state.
 * 
 * @param thread Pointer to the thread structure to initialize.
 * 
 * This is a no-op if the structure is already zeroed.
 * It is safe to call before create(), or to reinitialize a disposed structure.
 */
FOSSIL_THREADS_API void fossil_threads_thread_init(
    fossil_threads_thread_t *thread
);

/* 
 * Dispose of a thread handle, releasing any resources if necessary.
 * 
 * @param thread Pointer to the thread structure to dispose.
 * 
 * Safe to call on zeroed, unused, or already-joined/detached structures.
 * Does not affect running threads; only cleans up handles.
 */
FOSSIL_THREADS_API void fossil_threads_thread_dispose(
    fossil_threads_thread_t *thread
);

/* ---------- Management / utilities ---------- */

/* 
 * Yield the processor, hinting the OS scheduler to switch to another thread.
 * 
 * @return 0 on success, or error code on failure.
 * 
 * This is a cooperative scheduling hint; actual behavior is OS-dependent.
 */
FOSSIL_THREADS_API int fossil_threads_thread_yield(void);

/* 
 * Sleep the calling thread for the specified number of milliseconds.
 * 
 * @param ms Number of milliseconds to sleep.
 * @return   0 on success, or error code on failure.
 * 
 * The thread is suspended for at least 'ms' milliseconds.
 * Actual sleep duration may be longer due to OS scheduling.
 */
FOSSIL_THREADS_API int fossil_threads_thread_sleep_ms(
    unsigned int ms
);

/* 
 * Get the current thread's OS-specific thread identifier.
 * 
 * @return The thread ID as an unsigned long.
 * 
 * This value is suitable for comparison and logging, but may not be unique across processes.
 */
FOSSIL_THREADS_API unsigned long fossil_threads_thread_id(void);

/* 
 * Compare two thread objects to determine if they refer to the same underlying thread.
 * 
 * @param t1 Pointer to the first thread structure.
 * @param t2 Pointer to the second thread structure.
 * @return   Nonzero if both refer to the same thread, 0 otherwise.
 * 
 * Useful for checking thread identity in portable code.
 */
FOSSIL_THREADS_API int fossil_threads_thread_equal(
    const fossil_threads_thread_t *t1, 
    const fossil_threads_thread_t *t2
);

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
#include <utility>

namespace fossil {

namespace threads {

    class Thread {
    public:
        using Func = void*(*)(void*);

        /**
         * Default constructor.
         * Initializes the native thread structure to a safe, zeroed state.
         * Does not start a thread.
         */
        Thread() {
            fossil_threads_thread_init(&native_);
        }

        /**
         * Parameterized constructor.
         * Initializes the native thread structure and starts a new thread
         * running the given function with the provided argument.
         * Throws std::runtime_error if thread creation fails.
         *
         * @param func  The thread entry point function.
         * @param arg   Argument to pass to the thread function (default: nullptr).
         */
        explicit Thread(Func func, void* arg = nullptr) {
            fossil_threads_thread_init(&native_);
            if (fossil_threads_thread_create(&native_, func, arg) != 0) {
            throw std::runtime_error("Failed to create thread");
            }
        }

        /**
         * Destructor.
         * Cleans up any resources associated with the thread.
         * If the thread is still running, this does not join or detach it.
         */
        ~Thread() {
            fossil_threads_thread_dispose(&native_);
        }

        // Disable copy construction and copy assignment to prevent
        // accidental copying of thread handles.
        Thread(const Thread&) = delete;
        Thread& operator=(const Thread&) = delete;

        /**
         * Move constructor.
         * Transfers ownership of the thread handle from another Thread object.
         * The source object is reset to a safe, zeroed state.
         *
         * @param other  The Thread object to move from.
         */
        Thread(Thread&& other) noexcept {
            native_ = other.native_;
            other.native_ = {};
        }

        /**
         * Move assignment operator.
         * Disposes of any existing thread handle, then transfers ownership
         * from another Thread object. The source object is reset.
         *
         * @param other  The Thread object to move from.
         * @return       Reference to this object.
         */
        Thread& operator=(Thread&& other) noexcept {
            if (this != &other) {
            fossil_threads_thread_dispose(&native_);
            native_ = other.native_;
            other.native_ = {};
            }
            return *this;
        }

        /**
         * Join the thread.
         * Waits for the thread to finish execution. Optionally retrieves the
         * return value from the thread function.
         *
         * @param retval  Pointer to receive the thread's return value (default: nullptr).
         * @return        0 on success, error code otherwise.
         */
        int join(void** retval = nullptr) {
            return fossil_threads_thread_join(&native_, retval);
        }

        /**
         * Detach the thread.
         * Releases resources when the thread finishes. After detaching,
         * join() is invalid.
         *
         * @return  0 on success, error code otherwise.
         */
        int detach() {
            return fossil_threads_thread_detach(&native_);
        }

        /**
         * Get the thread ID.
         * Returns the OS-specific thread identifier for this thread.
         *
         * @return  The thread ID as an unsigned long.
         */
        unsigned long id() const {
            return native_.id;
        }

        /**
         * Check if the thread is joinable.
         * Returns true if the thread can be joined, false if detached or not started.
         *
         * @return  True if joinable, false otherwise.
         */
        bool joinable() const {
            return native_.joinable != 0;
        }

        /**
         * Yield the processor to another thread.
         * Static utility to hint the OS scheduler to switch threads.
         */
        static void yield() {
            fossil_threads_thread_yield();
        }

        /**
         * Sleep for a specified duration (in milliseconds).
         * Static utility to pause the current thread.
         *
         * @param ms  Number of milliseconds to sleep.
         */
        static void sleep_ms(unsigned int ms) {
            fossil_threads_thread_sleep_ms(ms);
        }

        /**
         * Get the current thread ID.
         * Static utility to retrieve the calling thread's OS-specific ID.
         *
         * @return  The current thread ID as an unsigned long.
         */
        static unsigned long current_id() {
            return fossil_threads_thread_id();
        }

        /**
         * Compare if two thread objects refer to the same underlying thread.
         * Static utility to check thread identity.
         *
         * @param t1  First thread object.
         * @param t2  Second thread object.
         * @return    True if both refer to the same thread, false otherwise.
         */
        static bool equal(const Thread& t1, const Thread& t2) {
            return fossil_threads_thread_equal(&t1.native_, &t2.native_) != 0;
        }

        /**
         * Get the native thread handle.
         * Returns a pointer to the underlying C thread structure.
         *
         * @return  Pointer to fossil_threads_thread_t.
         */
        fossil_threads_thread_t* native_handle() { return &native_; }

        /**
         * Get the native thread handle (const version).
         * Returns a const pointer to the underlying C thread structure.
         *
         * @return  Const pointer to fossil_threads_thread_t.
         */
        const fossil_threads_thread_t* native_handle() const { return &native_; }

    private:
        fossil_threads_thread_t native_{};
    };

} // namespace threads

} // namespace fossil

#endif

#endif /* FOSSIL_THREADS_THREAD_H */
