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

#include <stdlib.h>
#include <string.h>

/* =========================
   Backend selection
   ========================= */

#if defined(_WIN32)
#  define FOSSIL_FIBER_BACKEND_WINDOWS 1
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#elif defined(__has_include)
#  if __has_include(<ucontext.h>)
#    define FOSSIL_FIBER_BACKEND_UCONTEXT 1
#    include <ucontext.h>
#    include <errno.h>
#  endif
#else
/* Best-effort: try ucontext if we know POSIX */
#  if defined(__unix__) || defined(__APPLE__)
#    define FOSSIL_FIBER_BACKEND_UCONTEXT 1
#    include <ucontext.h>
#    include <errno.h>
#  endif
#endif

/* Thread-local storage for the 'current fiber' pointer */
#if defined(_WIN32)
__declspec(thread) static fossil_threads_fiber_t *fossil__current_fiber = NULL;
#else
static _Thread_local fossil_threads_fiber_t *fossil__current_fiber = NULL;
#endif

/* =========================
   Common helpers
   ========================= */

static void fossil__fiber_zero(fossil_threads_fiber_t *f) {
    if (f) memset(f, 0, sizeof(*f));
}

int fossil_threads_fiber_finished(const fossil_threads_fiber_t *fiber) {
    return fiber ? fiber->finished : 0;
}

/* =========================
   Windows backend
   ========================= */
#if defined(FOSSIL_FIBER_BACKEND_WINDOWS)

static VOID CALLBACK fossil__fiber_entry(LPVOID param) {
    fossil_threads_fiber_t *self = (fossil_threads_fiber_t*)param;
    if (self && self->func) {
        self->func(self->arg);
    }
    self->finished = 1;
    /* Convention: yield back to main fiber if present. We can’t implicitly know it here,
       so we just return; control goes to whoever switched to us next time. */
    /* When a Windows fiber function returns, control goes back to the last SwitchToFiber caller. */
}

int fossil_threads_fiber_init_self(fossil_threads_fiber_main_t *out_main) {
    if (!out_main) return FOSSIL_FIBER_EINVAL;
    fossil__fiber_zero(out_main);

    LPVOID h = ConvertThreadToFiber(NULL);
    if (!h) return FOSSIL_FIBER_EINTERNAL;

    out_main->handle = h;
    fossil__current_fiber = out_main;
    return FOSSIL_FIBER_OK;
}

int fossil_threads_fiber_create(
    fossil_threads_fiber_t *fiber,
    fossil_threads_fiber_func func,
    void *arg,
    size_t stack_size
) {
    if (!fiber || !func) return FOSSIL_FIBER_EINVAL;
    fossil__fiber_zero(fiber);

    fiber->func = func;
    fiber->arg  = arg;
    fiber->stack_size = stack_size;

    LPVOID h = CreateFiber(
        stack_size ? stack_size : 0,
        fossil__fiber_entry,
        fiber
    );
    if (!h) return FOSSIL_FIBER_EINTERNAL;

    fiber->handle = h;
    return FOSSIL_FIBER_OK;
}

int fossil_threads_fiber_resume(fossil_threads_fiber_t *to) {
    if (!to || !to->handle) return FOSSIL_FIBER_EINVAL;
    fossil__current_fiber = to;
    SwitchToFiber(to->handle);
    return FOSSIL_FIBER_OK;
}

int fossil_threads_fiber_yield_to(fossil_threads_fiber_t *to) {
    if (!to || !to->handle) return FOSSIL_FIBER_EINVAL;
    fossil__current_fiber = to;
    SwitchToFiber(to->handle);
    return FOSSIL_FIBER_OK;
}

fossil_threads_fiber_t* fossil_threads_fiber_current(void) {
    return fossil__current_fiber;
}

void fossil_threads_fiber_dispose(fossil_threads_fiber_t *fiber) {
    if (!fiber || !fiber->handle) return;
    /* You cannot delete the currently running fiber. */
    if (fossil__current_fiber == fiber) return;
    DeleteFiber(fiber->handle);
    fiber->handle = NULL;
}

#else /* =========================
          ucontext backend (POSIX, if available)
        ========================= */

#if defined(FOSSIL_FIBER_BACKEND_UCONTEXT)
typedef struct fossil__ucontext {
    ucontext_t ctx;
    void *stack_mem;
} fossil__ucontext_t;

static void fossil__posix_entry_trampoline(uintptr_t p) {
    fossil_threads_fiber_t *self = (fossil_threads_fiber_t*)(uintptr_t)p;
    if (self && self->func) self->func(self->arg);
    self->finished = 1;

    /* If a fiber returns, switch back to whoever resumed us last time.
       The ucontext API uses uc_link for this; we set it to the “caller” context. */
    /* If uc_link == NULL, exiting the context terminates the process.
       So we always ensure uc_link is set during resume. */
}

int fossil_threads_fiber_init_self(fossil_threads_fiber_main_t *out_main) {
    if (!out_main) return FOSSIL_FIBER_EINVAL;
    fossil__fiber_zero(out_main);

    fossil__ucontext_t *uc = (fossil__ucontext_t*)calloc(1, sizeof(*uc));
    if (!uc) return FOSSIL_FIBER_ENOMEM;

    if (getcontext(&uc->ctx) != 0) { free(uc); return FOSSIL_FIBER_EINTERNAL; }

    out_main->handle = uc;
    fossil__current_fiber = out_main;
    return FOSSIL_FIBER_OK;
}

int fossil_threads_fiber_create(
    fossil_threads_fiber_t *fiber,
    fossil_threads_fiber_func func,
    void *arg,
    size_t stack_size
) {
    if (!fiber || !func) return FOSSIL_FIBER_EINVAL;
    fossil__fiber_zero(fiber);

    fossil__ucontext_t *uc = (fossil__ucontext_t*)calloc(1, sizeof(*uc));
    if (!uc) return FOSSIL_FIBER_ENOMEM;

    if (getcontext(&uc->ctx) != 0) { free(uc); return FOSSIL_FIBER_EINTERNAL; }

    if (stack_size == 0) stack_size = 64 * 1024; /* default */
    uc->stack_mem = malloc(stack_size);
    if (!uc->stack_mem) { free(uc); return FOSSIL_FIBER_ENOMEM; }

    uc->ctx.uc_stack.ss_sp = uc->stack_mem;
    uc->ctx.uc_stack.ss_size = stack_size;
    uc->ctx.uc_stack.ss_flags = 0;
    uc->ctx.uc_link = NULL; /* set during resume() */

    fiber->func = func;
    fiber->arg  = arg;
    fiber->stack_size = stack_size;
    fiber->handle = uc;

#if defined(__APPLE__) && defined(__arm64__)
    /* On some newer Apple targets, ucontext may exist but be unreliable.
       If you hit issues, consider swapping to a different backend. */
#endif

    makecontext(&uc->ctx, (void (*)(void))fossil__posix_entry_trampoline, 1, (uintptr_t)fiber);
    return FOSSIL_FIBER_OK;
}

static int fossil__swap_to(fossil_threads_fiber_t *to) {
    fossil_threads_fiber_t *from = fossil__current_fiber;
    if (!to || !to->handle) return FOSSIL_FIBER_EINVAL;

    fossil__ucontext_t *to_uc = (fossil__ucontext_t*)to->handle;

    /* Ensure uc_link is the caller so return-from-fiber goes back here. */
    if (from && from->handle) {
        fossil__ucontext_t *from_uc = (fossil__ucontext_t*)from->handle;
        to_uc->ctx.uc_link = &from_uc->ctx;
    } else {
        to_uc->ctx.uc_link = NULL; /* No safe place to return; caller must not let fiber fall-through */
    }

    fossil__current_fiber = to;
    if (from && from->handle) {
        fossil__ucontext_t *from_uc = (fossil__ucontext_t*)from->handle;
        if (swapcontext(&from_uc->ctx, &to_uc->ctx) != 0) return FOSSIL_FIBER_EINTERNAL;
    } else {
        if (setcontext(&to_uc->ctx) != 0) return FOSSIL_FIBER_EINTERNAL;
    }
    return FOSSIL_FIBER_OK;
}

int fossil_threads_fiber_resume(fossil_threads_fiber_t *to) {
    return fossil__swap_to(to);
}

int fossil_threads_fiber_yield_to(fossil_threads_fiber_t *to) {
    return fossil__swap_to(to);
}

fossil_threads_fiber_t* fossil_threads_fiber_current(void) {
    return fossil__current_fiber;
}

void fossil_threads_fiber_dispose(fossil_threads_fiber_t *fiber) {
    if (!fiber || !fiber->handle) return;
    /* You cannot free the currently running fiber. */
    if (fossil__current_fiber == fiber) return;

    fossil__ucontext_t *uc = (fossil__ucontext_t*)fiber->handle;
    if (uc->stack_mem) free(uc->stack_mem);
    free(uc);
    fiber->handle = NULL;
}

#else /* =========================
         No supported backend
       ========================= */

int fossil_threads_fiber_init_self(fossil_threads_fiber_main_t *out_main) {
    (void)out_main;
    return FOSSIL_FIBER_ENOTSUP;
}
int fossil_threads_fiber_create(
    fossil_threads_fiber_t *fiber,
    fossil_threads_fiber_func func,
    void *arg, size_t stack_size
) {
    (void)fiber; (void)func; (void)arg; (void)stack_size;
    return FOSSIL_FIBER_ENOTSUP;
}
int fossil_threads_fiber_resume(fossil_threads_fiber_t *to){ (void)to; return FOSSIL_FIBER_ENOTSUP; }
int fossil_threads_fiber_yield_to(fossil_threads_fiber_t *to){ (void)to; return FOSSIL_FIBER_ENOTSUP; }
fossil_threads_fiber_t* fossil_threads_fiber_current(void){ return NULL; }
void fossil_threads_fiber_dispose(fossil_threads_fiber_t *fiber){ (void)fiber; }
#endif /* backends */
