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
#define _POSIX_C_SOURCE 199309L
#include "fossil/threads/thread.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <process.h>
#else
#  include <pthread.h>
#  include <time.h>
#  include <unistd.h>
#  include <sched.h>
#endif


/* ============================================================================
** Thread Start Context
** --------------------------------------------------------------------------*/
typedef struct fossil__thread_start_ctx {
    fossil_threads_thread_func func;
    void *arg;
    fossil_threads_thread_t *owner;
} fossil__thread_start_ctx;

/* ============================================================================
** Internal Utilities
** --------------------------------------------------------------------------*/
static void fossil__thread_zero(fossil_threads_thread_t *t) {
    if (t) memset(t, 0, sizeof(fossil_threads_thread_t));
}

void fossil_threads_thread_init(fossil_threads_thread_t *t) {
    fossil__thread_zero(t);
    if (t) {
        t->version = 1;
        t->joinable = 1;
        t->priority = 0;
        t->affinity = -1;
        t->policy = 0;
    }
}

void fossil_threads_thread_dispose(fossil_threads_thread_t *t) {
    if (!t) return;
#if defined(_WIN32)
    if (t->handle) CloseHandle((HANDLE)t->handle);
#else
    if (t->handle) free((pthread_t*)t->handle);
#endif
    fossil__thread_zero(t);
}

/* ============================================================================
** Windows Implementation
** --------------------------------------------------------------------------*/
#if defined(_WIN32)
static unsigned __stdcall fossil__thread_start(void *param) {
    fossil__thread_start_ctx *ctx = (fossil__thread_start_ctx*)param;
    fossil_threads_thread_t *self = ctx ? ctx->owner : NULL;
    if (self) {
        self->started = 1;
        self->start_time_ns = GetTickCount64() * 1000000ULL;
    }

    void *ret = ctx && ctx->func ? ctx->func(ctx->arg) : NULL;

    if (self) {
        self->retval = ret;
        self->finished = 1;
        self->end_time_ns = GetTickCount64() * 1000000ULL;
        self->exec_time_ns = (unsigned long)(self->end_time_ns - self->start_time_ns);
        self->exit_code = (unsigned)(uintptr_t)ret;
    }

    free(ctx);
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

    unsigned thread_id = 0;
    uintptr_t handle = _beginthreadex(NULL, 0, fossil__thread_start, ctx, 0, &thread_id);
    if (!handle) {
        free(ctx);
        return FOSSIL_THREADS_EINTERNAL;
    }

    thread->handle = (void*)handle;
    thread->id = (unsigned long)thread_id;
    thread->joinable = 1;
    thread->started = 1;
    thread->start_time_ns = GetTickCount64() * 1000000ULL;

    return FOSSIL_THREADS_OK;
}

int fossil_threads_thread_join(fossil_threads_thread_t *thread, void **retval) {
    if (!thread) return FOSSIL_THREADS_EINVAL;
    if (!thread->started) return FOSSIL_THREADS_ENOTSTARTED;
    if (!thread->joinable) return FOSSIL_THREADS_EDETACHED;

    HANDLE h = (HANDLE)thread->handle;
    if (!h) return FOSSIL_THREADS_EINTERNAL;
    DWORD w = WaitForSingleObject(h, INFINITE);
    if (w != WAIT_OBJECT_0) return FOSSIL_THREADS_EINTERNAL;

    if (retval) *retval = thread->retval;
    CloseHandle(h);
    thread->handle = NULL;
    thread->joinable = 0;
    thread->finished = 1;
    thread->end_time_ns = GetTickCount64() * 1000000ULL;
    thread->exec_time_ns = (unsigned long)(thread->end_time_ns - thread->start_time_ns);
    return FOSSIL_THREADS_OK;
}

int fossil_threads_thread_detach(fossil_threads_thread_t *thread) {
    if (!thread || !thread->started) return FOSSIL_THREADS_EINVAL;
    if (!thread->joinable) return FOSSIL_THREADS_EDETACHED;
    if (thread->handle) CloseHandle((HANDLE)thread->handle);
    thread->handle = NULL;
    thread->joinable = 0;
    return FOSSIL_THREADS_OK;
}

int fossil_threads_thread_yield(void) {
    SwitchToThread();
    return FOSSIL_THREADS_OK;
}

int fossil_threads_thread_sleep_ms(unsigned int ms) {
    Sleep(ms);
    return FOSSIL_THREADS_OK;
}

unsigned long fossil_threads_thread_id(void) {
    return (unsigned long)GetCurrentThreadId();
}

/* ============================================================================
** POSIX Implementation
** --------------------------------------------------------------------------*/
#else

static void* fossil__thread_start(void *param) {
    fossil__thread_start_ctx *ctx = (fossil__thread_start_ctx*)param;
    fossil_threads_thread_t *self = ctx ? ctx->owner : NULL;
    if (self) {
        self->started = 1;
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        self->start_time_ns = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    }

    void *ret = ctx && ctx->func ? ctx->func(ctx->arg) : NULL;

    if (self) {
        self->retval = ret;
        self->finished = 1;
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        self->end_time_ns = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
        self->exec_time_ns = (unsigned long)(self->end_time_ns - self->start_time_ns);
        self->exit_code = 0;
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

    fossil__thread_start_ctx *ctx = malloc(sizeof(*ctx));
    if (!ctx) return FOSSIL_THREADS_ENOMEM;
    ctx->func = func;
    ctx->arg = arg;
    ctx->owner = thread;

    pthread_t *pth = malloc(sizeof(pthread_t));
    if (!pth) { free(ctx); return FOSSIL_THREADS_ENOMEM; }

    int rc = pthread_create(pth, NULL, fossil__thread_start, ctx);
    if (rc != 0) {
        free(ctx);
        free(pth);
        return FOSSIL_THREADS_EINTERNAL;
    }

    thread->handle = (void*)pth;
    thread->joinable = 1;
    thread->started = 1;
    memcpy(&thread->id, pth, sizeof(thread->id) < sizeof(*pth) ? sizeof(thread->id) : sizeof(*pth));

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    thread->start_time_ns = ts.tv_sec * 1000000000ULL + ts.tv_nsec;

    return FOSSIL_THREADS_OK;
}

int fossil_threads_thread_join(fossil_threads_thread_t *thread, void **retval) {
    if (!thread) return FOSSIL_THREADS_EINVAL;
    if (!thread->started) return FOSSIL_THREADS_ENOTSTARTED;
    if (!thread->joinable) return FOSSIL_THREADS_EDETACHED;

    pthread_t *pth = (pthread_t*)thread->handle;
    if (!pth) return FOSSIL_THREADS_EINTERNAL;

    void *ret = NULL;
    int rc = pthread_join(*pth, &ret);
    if (rc != 0) return FOSSIL_THREADS_EINTERNAL;

    if (retval) *retval = ret ? ret : thread->retval;
    free(pth);
    thread->handle = NULL;
    thread->joinable = 0;
    thread->finished = 1;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    thread->end_time_ns = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    thread->exec_time_ns = (unsigned long)(thread->end_time_ns - thread->start_time_ns);

    return FOSSIL_THREADS_OK;
}

int fossil_threads_thread_detach(fossil_threads_thread_t *thread) {
    if (!thread || !thread->started) return FOSSIL_THREADS_EINVAL;
    if (!thread->joinable) return FOSSIL_THREADS_EDETACHED;

    pthread_t *pth = (pthread_t*)thread->handle;
    if (!pth) return FOSSIL_THREADS_EINTERNAL;

    pthread_detach(*pth);
    free(pth);
    thread->handle = NULL;
    thread->joinable = 0;
    return FOSSIL_THREADS_OK;
}

int fossil_threads_thread_yield(void) {
#if defined(_POSIX_PRIORITY_SCHEDULING) || defined(__APPLE__)
    sched_yield();
#else
    struct timespec ts = {0, 0};
    nanosleep(&ts, NULL);
#endif
    return FOSSIL_THREADS_OK;
}

int fossil_threads_thread_sleep_ms(unsigned int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000u;
    ts.tv_nsec = (long)(ms % 1000u) * 1000000L;
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR);
    return FOSSIL_THREADS_OK;
}

unsigned long fossil_threads_thread_id(void) {
    return (unsigned long)pthread_self();
}
#endif /* platform switch */

int fossil_threads_thread_equal(
    const fossil_threads_thread_t *t1,
    const fossil_threads_thread_t *t2
) {
    if (t1 == t2) return 1;
    if (!t1 || !t2) return 0;
#if defined(_WIN32)
    if (t1->id && t2->id) return (t1->id == t2->id);
    return (t1->handle == t2->handle);
#else
    if (t1->handle && t2->handle) {
        const pthread_t *p1 = (const pthread_t*)t1->handle;
        const pthread_t *p2 = (const pthread_t*)t2->handle;
        return pthread_equal(*p1, *p2) != 0;
    }
    size_t cmp_size = sizeof(t1->id) < sizeof(t2->id) ? sizeof(t1->id) : sizeof(t2->id);
    return (memcmp(&t1->id, &t2->id, cmp_size) == 0);
#endif
}

/* Extended API stubs (priority, affinity, cancel, etc.) */

/* ============================================================================
** Common Extensions (Portable Layer)
** --------------------------------------------------------------------------*/

int fossil_threads_thread_set_priority(fossil_threads_thread_t *thread, int priority) {
    if (!thread) return FOSSIL_THREADS_EINVAL;
    thread->priority = priority;
    return FOSSIL_THREADS_OK;
}

int fossil_threads_thread_get_priority(const fossil_threads_thread_t *thread) {
    if (!thread) return FOSSIL_THREADS_EINVAL;
    return thread->priority;
}

int fossil_threads_thread_set_affinity(fossil_threads_thread_t *thread, int affinity) {
    if (!thread) return FOSSIL_THREADS_EINVAL;
    thread->affinity = affinity;
    return FOSSIL_THREADS_OK;
}

int fossil_threads_thread_get_affinity(const fossil_threads_thread_t *thread) {
    if (!thread) return FOSSIL_THREADS_EINVAL;
    return thread->affinity;
}

/* ============================================================================
** Cooperative Cancellation
** --------------------------------------------------------------------------*/
int fossil_threads_thread_cancel(fossil_threads_thread_t *thread) {
    if (!thread) return FOSSIL_THREADS_EINVAL;
    if (!thread->started) return FOSSIL_THREADS_ENOTSTARTED;
    if (thread->finished) return FOSSIL_THREADS_EFINISHED;

    thread->cancel_requested = 1;
#if defined(_WIN32)
    if (thread->handle) {
        HANDLE h = (HANDLE)thread->handle;
        DWORD exitCode = 0;
        if (!GetExitCodeThread(h, &exitCode))
            return FOSSIL_THREADS_EINTERNAL;
    }
#elif defined(__unix__) || defined(__APPLE__)
    if (thread->handle) {
        pthread_t *pt = (pthread_t*)thread->handle;
        int k = pthread_kill(*pt, 0);
        if (k != 0 && k != ESRCH)
            return FOSSIL_THREADS_EINTERNAL;
    }
#else
    return FOSSIL_THREADS_EUNSUPPORTED;
#endif
    return FOSSIL_THREADS_OK;
}

int fossil_threads_thread_is_running(const fossil_threads_thread_t *thread) {
    if (!thread) return 0;
    return thread->started && !thread->finished;
}

void* fossil_threads_thread_get_retval(const fossil_threads_thread_t *thread) {
    if (!thread || !thread->finished) return NULL;
    return thread->retval;
}

/* Thread pool API stubs (not implemented) */

/* ================================================================
 * Fossil Threads — Thread Pool
 * Cross-platform pool with cooperative task dispatch.
 * ================================================================ */

/* Task node */
typedef struct fossil_threads_pool_task {
    fossil_threads_thread_func func;
    void *arg;
    struct fossil_threads_pool_task *next;
} fossil_threads_pool_task_t;

/* Thread pool */
typedef struct fossil_threads_pool {
    size_t num_threads;
    fossil_threads_thread_t *threads;
    fossil_threads_pool_task_t *tasks_head;
    fossil_threads_pool_task_t *tasks_tail;
    size_t tasks_count;
    int stop;                /* stop flag */
#if defined(_WIN32)
    HANDLE tasks_mutex;
    HANDLE tasks_cond;
#else
    pthread_mutex_t tasks_mutex;
    pthread_cond_t tasks_cond;
#endif
} fossil_threads_pool_t;

/* ================================================================
 * Internal Worker Function
 * ================================================================ */
static void* fossil__pool_worker(void *arg) {
    fossil_threads_pool_t *pool = (fossil_threads_pool_t*)arg;
    if (!pool) return NULL;

    while (1) {
        fossil_threads_pool_task_t *task = NULL;

#if defined(_WIN32)
        WaitForSingleObject(pool->tasks_mutex, INFINITE);
        while (!pool->tasks_head && !pool->stop)
            WaitForSingleObject(pool->tasks_cond, INFINITE);

        if (pool->stop) {
            ReleaseMutex(pool->tasks_mutex);
            break;
        }

        task = pool->tasks_head;
        if (task) {
            pool->tasks_head = task->next;
            if (!pool->tasks_head) pool->tasks_tail = NULL;
            pool->tasks_count--;
        }
        ReleaseMutex(pool->tasks_mutex);
#else
        pthread_mutex_lock(&pool->tasks_mutex);
        while (!pool->tasks_head && !pool->stop)
            pthread_cond_wait(&pool->tasks_cond, &pool->tasks_mutex);

        if (pool->stop) {
            pthread_mutex_unlock(&pool->tasks_mutex);
            break;
        }

        task = pool->tasks_head;
        if (task) {
            pool->tasks_head = task->next;
            if (!pool->tasks_head) pool->tasks_tail = NULL;
            pool->tasks_count--;
        }
        pthread_mutex_unlock(&pool->tasks_mutex);
#endif

        if (task && task->func) {
            task->func(task->arg);
            free(task);
        }
    }

    return NULL;
}

/* ================================================================
 * Pool Create
 * ================================================================ */
fossil_threads_pool_t* fossil_threads_pool_create(size_t num_threads) {
    if (num_threads == 0) return NULL;

    fossil_threads_pool_t *pool = (fossil_threads_pool_t*)calloc(1, sizeof(*pool));
    if (!pool) return NULL;

    pool->num_threads = num_threads;
    pool->threads = (fossil_threads_thread_t*)calloc(num_threads, sizeof(fossil_threads_thread_t));
    if (!pool->threads) {
        free(pool);
        return NULL;
    }

#if defined(_WIN32)
    pool->tasks_mutex = CreateMutex(NULL, FALSE, NULL);
    pool->tasks_cond = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!pool->tasks_mutex || !pool->tasks_cond) {
        free(pool->threads);
        free(pool);
        return NULL;
    }
#else
    pthread_mutex_init(&pool->tasks_mutex, NULL);
    pthread_cond_init(&pool->tasks_cond, NULL);
#endif

    for (size_t i = 0; i < num_threads; ++i) {
        fossil_threads_thread_create(&pool->threads[i], fossil__pool_worker, pool);
    }

    return pool;
}

/* ================================================================
 * Pool Destroy
 * ================================================================ */
void fossil_threads_pool_destroy(fossil_threads_pool_t *pool) {
    if (!pool) return;

#if defined(_WIN32)
    WaitForSingleObject(pool->tasks_mutex, INFINITE);
    pool->stop = 1;
    SetEvent(pool->tasks_cond);
    ReleaseMutex(pool->tasks_mutex);
#else
    pthread_mutex_lock(&pool->tasks_mutex);
    pool->stop = 1;
    pthread_cond_broadcast(&pool->tasks_cond);
    pthread_mutex_unlock(&pool->tasks_mutex);
#endif

    /* Join all worker threads */
    for (size_t i = 0; i < pool->num_threads; ++i)
        fossil_threads_thread_join(&pool->threads[i], NULL);

    /* Free queued tasks */
    fossil_threads_pool_task_t *task = pool->tasks_head;
    while (task) {
        fossil_threads_pool_task_t *next = task->next;
        free(task);
        task = next;
    }

#if defined(_WIN32)
    CloseHandle(pool->tasks_mutex);
    CloseHandle(pool->tasks_cond);
#else
    pthread_mutex_destroy(&pool->tasks_mutex);
    pthread_cond_destroy(&pool->tasks_cond);
#endif

    free(pool->threads);
    free(pool);
}

/* ================================================================
 * Submit a task to the pool
 * ================================================================ */
int fossil_threads_pool_submit(
    fossil_threads_pool_t *pool,
    fossil_threads_thread_func func,
    void *arg
) {
    if (!pool || !func)
        return FOSSIL_THREADS_EINVAL;

    fossil_threads_pool_task_t *task = (fossil_threads_pool_task_t*)malloc(sizeof(*task));
    if (!task)
        return FOSSIL_THREADS_ENOMEM;

    task->func = func;
    task->arg = arg;
    task->next = NULL;

#if defined(_WIN32)
    WaitForSingleObject(pool->tasks_mutex, INFINITE);
    if (pool->stop) {
        ReleaseMutex(pool->tasks_mutex);
        free(task);
        return FOSSIL_THREADS_ECANCELLED;
    }
    if (pool->tasks_tail)
        pool->tasks_tail->next = task;
    else
        pool->tasks_head = task;
    pool->tasks_tail = task;
    pool->tasks_count++;
    SetEvent(pool->tasks_cond);
    ReleaseMutex(pool->tasks_mutex);
#else
    pthread_mutex_lock(&pool->tasks_mutex);
    if (pool->stop) {
        pthread_mutex_unlock(&pool->tasks_mutex);
        free(task);
        return FOSSIL_THREADS_ECANCELLED;
    }
    if (pool->tasks_tail)
        pool->tasks_tail->next = task;
    else
        pool->tasks_head = task;
    pool->tasks_tail = task;
    pool->tasks_count++;
    pthread_cond_signal(&pool->tasks_cond);
    pthread_mutex_unlock(&pool->tasks_mutex);
#endif

    return FOSSIL_THREADS_OK;
}

/* ================================================================
 * Wait for all queued tasks to complete
 * ================================================================ */
int fossil_threads_pool_wait(fossil_threads_pool_t *pool) {
    if (!pool)
        return FOSSIL_THREADS_EINVAL;

    for (;;) {
        int done = 0;
#if defined(_WIN32)
        WaitForSingleObject(pool->tasks_mutex, INFINITE);
        done = (pool->tasks_count == 0);
        ReleaseMutex(pool->tasks_mutex);
#else
        pthread_mutex_lock(&pool->tasks_mutex);
        done = (pool->tasks_count == 0);
        pthread_mutex_unlock(&pool->tasks_mutex);
#endif
        if (done) break;

#if defined(_WIN32)
        Sleep(1);
#else
        struct timespec ts = {0, 1000000L};
        nanosleep(&ts, NULL);
#endif
    }

    return FOSSIL_THREADS_OK;
}

/* ================================================================
 * Get pool size
 * ================================================================ */
size_t fossil_threads_pool_size(const fossil_threads_pool_t *pool) {
    if (!pool) return 0;
    return pool->num_threads;
}
