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
#define _XOPEN_SOURCE
#include "fossil/threads/fiber.h"
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <ucontext.h>
#  include <unistd.h>
#endif

/* Thread-local pointer to current fiber */
/* Portable thread-local storage macro */
#if defined(_MSC_VER)
#  define FOSSIL_TLS __declspec(thread)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#  define FOSSIL_TLS _Thread_local
#else
#  define FOSSIL_TLS __thread
#endif

static FOSSIL_TLS fossil_threads_fiber_t *fossil__current_fiber = NULL;

#if defined(_WIN32)

/* Windows fiber API is built-in */
int fossil_threads_fiber_self(fossil_threads_fiber_t *out) {
    if (!out) return -1;
    void *h = ConvertThreadToFiber(NULL);
    if (!h) return -1;
    memset(out, 0, sizeof(*out));
    out->handle = h;
    fossil__current_fiber = out;
    return 0;
}

int fossil_threads_fiber_create(
    fossil_threads_fiber_t *fiber,
    fossil_threads_fiber_func func,
    void *arg,
    size_t stack_size
) {
    if (!fiber || !func) return -1;
    memset(fiber, 0, sizeof(*fiber));

    fiber->func = func;
    fiber->arg = arg;

    fiber->handle = CreateFiber(
        stack_size ? stack_size : 0,
        (LPFIBER_START_ROUTINE)func,
        arg
    );
    if (!fiber->handle) return -1;
    return 0;
}

int fossil_threads_fiber_switch(fossil_threads_fiber_t *to) {
    if (!to || !to->handle) return -1;
    fossil__current_fiber = to;
    SwitchToFiber(to->handle);
    return 0;
}

fossil_threads_fiber_t* fossil_threads_fiber_current(void) {
    return fossil__current_fiber;
}

void fossil_threads_fiber_dispose(fossil_threads_fiber_t *fiber) {
    if (!fiber || !fiber->handle) return;
    DeleteFiber(fiber->handle);
    fiber->handle = NULL;
}

#else /* POSIX implementation using ucontext */

int fossil_threads_fiber_self(fossil_threads_fiber_t *out) {
    if (!out) return -1;
    memset(out, 0, sizeof(*out));
    out->handle = malloc(sizeof(ucontext_t));
    if (!out->handle) return -1;
    if (getcontext((ucontext_t*)out->handle) != 0) {
        free(out->handle);
        return -1;
    }
    fossil__current_fiber = out;
    return 0;
}

static void fossil__fiber_entry(uintptr_t arg) {
    fossil_threads_fiber_t *fiber = (fossil_threads_fiber_t*)arg;
    if (fiber && fiber->func) {
        fiber->func(fiber->arg);
    }
    fiber->finished = 1;
}

int fossil_threads_fiber_create(
    fossil_threads_fiber_t *fiber,
    fossil_threads_fiber_func func,
    void *arg,
    size_t stack_size
) {
    if (!fiber || !func) return -1;
    memset(fiber, 0, sizeof(*fiber));

    ucontext_t *ctx = malloc(sizeof(ucontext_t));
    if (!ctx) return -1;
    if (getcontext(ctx) != 0) {
        free(ctx);
        return -1;
    }

    fiber->func = func;
    fiber->arg  = arg;
    fiber->handle = ctx;

    if (stack_size == 0) stack_size = 64 * 1024; /* default */
    void *stack = malloc(stack_size);
    if (!stack) { free(ctx); return -1; }

    ctx->uc_stack.ss_sp   = stack;
    ctx->uc_stack.ss_size = stack_size;
    ctx->uc_link = NULL;

    makecontext(ctx, (void (*)(void))fossil__fiber_entry, 1, (uintptr_t)fiber);
    return 0;
}

int fossil_threads_fiber_switch(fossil_threads_fiber_t *to) {
    if (!to || !to->handle) return -1;
    fossil_threads_fiber_t *from = fossil__current_fiber;
    fossil__current_fiber = to;
    if (from && from->handle) {
        if (swapcontext((ucontext_t*)from->handle, (ucontext_t*)to->handle) != 0)
            return -1;
    } else {
        if (setcontext((ucontext_t*)to->handle) != 0)
            return -1;
    }
    return 0;
}

fossil_threads_fiber_t* fossil_threads_fiber_current(void) {
    return fossil__current_fiber;
}

void fossil_threads_fiber_dispose(fossil_threads_fiber_t *fiber) {
    if (!fiber || !fiber->handle) return;
    ucontext_t *ctx = (ucontext_t*)fiber->handle;
    if (ctx->uc_stack.ss_sp) free(ctx->uc_stack.ss_sp);
    free(ctx);
    fiber->handle = NULL;
}

#endif
