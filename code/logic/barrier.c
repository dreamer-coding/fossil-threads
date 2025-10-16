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
#include "fossil/threads/barrier.h"
#include <errno.h>


int fossil_threads_barrier_init(fossil_threads_barrier_t* barrier, size_t threshold, bool cyclic) {
    if (!barrier || threshold == 0) return EINVAL;
    fossil_threads_mutex_init(&barrier->mutex);
    fossil_threads_cond_init(&barrier->cond);
    barrier->threshold = threshold;
    barrier->count = 0;
    barrier->cycle = 0;
    barrier->cyclic = cyclic;
    barrier->destroyed = false;
    return 0;
}

int fossil_threads_barrier_wait(fossil_threads_barrier_t* barrier) {
    if (!barrier || barrier->destroyed) return EINVAL;
    fossil_threads_mutex_lock(&barrier->mutex);

    size_t cycle = barrier->cycle;
    if (++barrier->count == barrier->threshold) {
        barrier->cycle++;
        barrier->count = 0;
        fossil_threads_cond_broadcast(&barrier->cond);
        fossil_threads_mutex_unlock(&barrier->mutex);
        return 0;
    }

    while (cycle == barrier->cycle)
        fossil_threads_cond_wait(&barrier->cond, &barrier->mutex);

    fossil_threads_mutex_unlock(&barrier->mutex);
    return 0;
}

int fossil_threads_barrier_wait_timeout(fossil_threads_barrier_t* barrier, unsigned long timeout_ms) {
    if (!barrier || barrier->destroyed) return EINVAL;
    fossil_threads_mutex_lock(&barrier->mutex);

    size_t cycle = barrier->cycle;
    if (++barrier->count == barrier->threshold) {
        barrier->cycle++;
        barrier->count = 0;
        fossil_threads_cond_broadcast(&barrier->cond);
        fossil_threads_mutex_unlock(&barrier->mutex);
        return 0;
    }

    int result = 0;
    while (cycle == barrier->cycle && result == 0)
        result = fossil_threads_cond_timedwait(&barrier->cond, &barrier->mutex, timeout_ms);

    fossil_threads_mutex_unlock(&barrier->mutex);
    return result;
}

void fossil_threads_barrier_reset(fossil_threads_barrier_t* barrier) {
    if (!barrier) return;
    fossil_threads_mutex_lock(&barrier->mutex);
    barrier->count = 0;
    barrier->cycle++;
    fossil_threads_cond_broadcast(&barrier->cond);
    fossil_threads_mutex_unlock(&barrier->mutex);
}

void fossil_threads_barrier_destroy(fossil_threads_barrier_t* barrier) {
    if (!barrier) return;
    fossil_threads_mutex_lock(&barrier->mutex);
    barrier->destroyed = true;
    fossil_threads_cond_broadcast(&barrier->cond);
    fossil_threads_mutex_unlock(&barrier->mutex);
    fossil_threads_cond_dispose(&barrier->cond);
    fossil_threads_mutex_dispose(&barrier->mutex);
}
