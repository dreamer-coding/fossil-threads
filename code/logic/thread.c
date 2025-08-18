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
#include "fossil/threads/thread.h"

#include <string.h> /* memset */
#include <stdint.h>

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <process.h> /* _beginthreadex, _endthreadex */
#else
#  include <pthread.h>
#  include <time.h>
#  include <unistd.h>
#  include <sched.h>
#endif

/* ---------- Internal wrapper ---------- */

typedef struct fossil__thread_start_ctx {
    fossil_threads_thread_func func;
    void *arg;
    fossil_threads_thread_t *owner;
} fossil__thread_start_ctx;

static void fossil__thread_zero(fossil_threads_thread_t *t) {
    if (t) memset(t, 0, sizeof(*t));
}

void fossil_threads_thread_init(fossil_threads_thread_t *t) {
    if (t) fossil__thread_zero(t);
}

void fossil_threads_thread_dispose(fossil_threads_thread_t *t) {
    if (!t) return;
#if defined(_WIN32)
    if (t->handle) {
        /* If still joinable and running, we do not TerminateThread: caller must join/detach. */
        /* If finished or detached, just close. */
        CloseHandle((HANDLE)t->handle);
    }
#else
    if (t->handle) {
        /* handle is a heap-allocated pthread_t* */
        pthread_t *pth = (pthread_t*)t->handle;
        /* We cannot reliably free a live joinable thread without joining; caller contract. */
        free(pth);
    }
#endif
    fossil__thread_zero(t);
}

#if defined(_WIN32)

/* Windows thread start trampoline */
static unsigned __stdcall fossil__thread_start(void *param) {
    fossil__thread_start_ctx *ctx = (fossil__thread_start_ctx*)param;
    void *ret = NULL;
    if (ctx && ctx->func) {
        ret = ctx->func(ctx->arg);
    }
    if (ctx && ctx->owner) {
        ctx->owner->retval = ret;
        ctx->owner->finished = 1;
    }
    /* _endthreadex expects an unsigned; stash pointer->unsigned best-effort (ignored anyway) */
    unsigned code = (unsigned)(uintptr_t)ret;
    _endthreadex(code);
    return code;
}

int fossil_threads_thread_create(
    fossil_threads_thread_t *thread,
    fossil_threads_thread_func func,
    void *arg
) {
    if (!thread || !func) return FOSSIL_THREADS_EINVAL;
    if (thread->started) return FOSSIL_THREADS_EBUSY;

    fossil__thread_zero(thread);
    fossil__thread_start_ctx *ctx = (fossil__thread_start_ctx*)malloc(sizeof(*ctx));
    if (!ctx) return FOSSIL_THREADS_ENOMEM;

    ctx->func = func;
    ctx->arg = arg;
    ctx->owner = thread;

    uintptr_t handle = _beginthreadex(
        NULL,              /* default security */
        0,                 /* default stack */
        fossil__thread_start,
        ctx,
        0,                 /* run immediately */
        (unsigned*)&thread->id
    );
    if (!handle) {
        free(ctx);
        return FOSSIL_THREADS_EINTERNAL;
    }

    /* We keep ctx alive only until the thread has started and copied owner pointer.
       That's already true here; but the thread still needs ctx for calling func.
       So we transfer ownership: thread exit will free it implicitly via _endthreadex stack unwind.
       To be safe, we attach ctx to the owner via handle if needed; not necessary here. */

    thread->handle = (void*)handle;
    thread->joinable = 1;
    thread->started = 1;
    return FOSSIL_THREADS_OK;
}

int fossil_threads_thread_join(
    fossil_threads_thread_t *thread,
    void **retval
) {
    if (!thread || !thread->started) return FOSSIL_THREADS_EINVAL;
    if (!thread->joinable) return FOSSIL_THREADS_EPERM;

    HANDLE h = (HANDLE)thread->handle;
    if (!h) return FOSSIL_THREADS_EINTERNAL;

    DWORD w = WaitForSingleObject(h, INFINITE);
    if (w != WAIT_OBJECT_0) return FOSSIL_THREADS_EINTERNAL;

    if (retval) *retval = thread->retval;

    CloseHandle(h);
    thread->handle = NULL;
    thread->joinable = 0;
    return FOSSIL_THREADS_OK;
}

int fossil_threads_thread_detach(
    fossil_threads_thread_t *thread
) {
    if (!thread || !thread->started) return FOSSIL_THREADS_EINVAL;
    if (!thread->joinable) return FOSSIL_THREADS_EPERM;

    HANDLE h = (HANDLE)thread->handle;
    if (!h) return FOSSIL_THREADS_EINTERNAL;

    /* On Windows, there's no explicit 'detach'; closing our handle releases our reference. */
    CloseHandle(h);
    thread->handle = NULL;
    thread->joinable = 0;
    return FOSSIL_THREADS_OK;
}

int fossil_threads_thread_yield(void) {
    /* SwitchToThread returns nonzero if it yielded; Sleep(0) as fallback */
    if (SwitchToThread()) return FOSSIL_THREADS_OK;
    Sleep(0);
    return FOSSIL_THREADS_OK;
}

int fossil_threads_thread_sleep_ms(unsigned int ms) {
    Sleep(ms);
    return FOSSIL_THREADS_OK;
}

unsigned long fossil_threads_thread_id(void) {
    return (unsigned long)GetCurrentThreadId();
}

#else /* POSIX implementation */

static void* fossil__thread_start(void *param) {
    fossil__thread_start_ctx *ctx = (fossil__thread_start_ctx*)param;
    void *ret = NULL;
    if (ctx && ctx->func) {
        ret = ctx->func(ctx->arg);
    }
    if (ctx && ctx->owner) {
        ctx->owner->retval = ret;
        ctx->owner->finished = 1;
    }
    free(ctx);
    return ret;
}

int fossil_threads_thread_create(
    fossil_threads_thread_t *thread,
    fossil_threads_thread_func func,
    void *arg
) {
    if (!thread || !func) return FOSSIL_THREADS_EINVAL;
    if (thread->started) return FOSSIL_THREADS_EBUSY;

    fossil__thread_zero(thread);

    fossil__thread_start_ctx *ctx = (fossil__thread_start_ctx*)malloc(sizeof(*ctx));
    if (!ctx) return FOSSIL_THREADS_ENOMEM;
    ctx->func = func;
    ctx->arg = arg;
    ctx->owner = thread;

    pthread_t *pth = (pthread_t*)malloc(sizeof(pthread_t));
    if (!pth) { free(ctx); return FOSSIL_THREADS_ENOMEM; }

    int rc = pthread_create(pth, NULL, fossil__thread_start, ctx);
    if (rc != 0) {
        free(ctx);
        free(pth);
        return rc; /* propagate pthread error code */
    }

    thread->handle = (void*)pth;
    thread->joinable = 1;
    thread->started = 1;

    /* Best-effort thread id -> cast to integer width; not stable across all libcs */
    thread->id = (unsigned long)*pth;
    return FOSSIL_THREADS_OK;
}

int fossil_threads_thread_join(
    fossil_threads_thread_t *thread,
    void **retval
) {
    if (!thread || !thread->started) return FOSSIL_THREADS_EINVAL;
    if (!thread->joinable) return FOSSIL_THREADS_EPERM;

    pthread_t *pth = (pthread_t*)thread->handle;
    if (!pth) return FOSSIL_THREADS_EINTERNAL;

    void *ret = NULL;
    int rc = pthread_join(*pth, &ret);
    if (rc != 0) return rc;

    if (retval) *retval = (ret != NULL) ? ret : thread->retval;

    free(pth);
    thread->handle = NULL;
    thread->joinable = 0;
    return FOSSIL_THREADS_OK;
}

int fossil_threads_thread_detach(
    fossil_threads_thread_t *thread
) {
    if (!thread || !thread->started) return FOSSIL_THREADS_EINVAL;
    if (!thread->joinable) return FOSSIL_THREADS_EPERM;

    pthread_t *pth = (pthread_t*)thread->handle;
    if (!pth) return FOSSIL_THREADS_EINTERNAL;

    int rc = pthread_detach(*pth);
    if (rc != 0) return rc;

    free(pth);
    thread->handle = NULL;
    thread->joinable = 0;
    return FOSSIL_THREADS_OK;
}

int fossil_threads_thread_yield(void) {
#if defined(_POSIX_PRIORITY_SCHEDULING) || defined(__APPLE__)
    return sched_yield() == 0 ? FOSSIL_THREADS_OK : FOSSIL_THREADS_EINTERNAL;
#else
    /* Fallback: nanosleep(0) behaves as yield-ish */
    struct timespec ts = {0, 0};
    return nanosleep(&ts, NULL) == 0 ? FOSSIL_THREADS_OK : FOSSIL_THREADS_EINTERNAL;
#endif
}

int fossil_threads_thread_sleep_ms(unsigned int ms) {
    struct timespec ts;
    ts.tv_sec  = ms / 1000u;
    ts.tv_nsec = (long)(ms % 1000u) * 1000000L;
    while (nanosleep(&ts, &ts) == -1) {
        /* interrupted by signal, continue sleeping */
        continue;
    }
    return FOSSIL_THREADS_OK;
}

unsigned long fossil_threads_thread_id(void) {
    /* Best effort: cast pthread_self to integer width */
    return (unsigned long)pthread_self();
}

#endif /* _WIN32 / POSIX */

/* ---------- Utilities ---------- */

int fossil_threads_thread_equal(
    const fossil_threads_thread_t *t1,
    const fossil_threads_thread_t *t2
) {
    if (t1 == t2) return 1;
    if (!t1 || !t2) return 0;

#if defined(_WIN32)
    /* Thread IDs are unique system-wide for the lifetime */
    if (t1->id && t2->id) return (t1->id == t2->id);
    return (t1->handle == t2->handle);
#else
    /* Compare pthread_t values if available */
    if (t1->handle && t2->handle) {
        const pthread_t *p1 = (const pthread_t*)t1->handle;
        const pthread_t *p2 = (const pthread_t*)t2->handle;
        return pthread_equal(*p1, *p2) != 0;
    }
    return (t1->id != 0 && t1->id == t2->id);
#endif
}
