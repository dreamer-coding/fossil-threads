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
#include <fossil/pizza/framework.h>
#include "fossil/threads/framework.h"


// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Utilities
// * * * * * * * * * * * * * * * * * * * * * * * *
// Setup steps for things like test fixtures and
// mock objects are set here.
// * * * * * * * * * * * * * * * * * * * * * * * *

FOSSIL_TEST_SUITE(c_mutex_fixture);

FOSSIL_SETUP(c_mutex_fixture) {
    // Setup the test fixture
}

FOSSIL_TEARDOWN(c_mutex_fixture) {
    // Teardown the test fixture
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Cases
// * * * * * * * * * * * * * * * * * * * * * * * *
// The test cases below are provided as samples, inspired
// by the Meson build system's approach of using test cases
// as samples for library usage.
// * * * * * * * * * * * * * * * * * * * * * * * *

FOSSIL_TEST_CASE(c_thread_mutex_init_dispose) {
    fossil_threads_mutex_t m;
    int rc = fossil_threads_mutex_init(&m);
    ASSUME_ITS_EQUAL_I32(rc, 0);
    fossil_threads_mutex_dispose(&m);
}

FOSSIL_TEST_CASE(c_thread_mutex_lock_unlock) {
    fossil_threads_mutex_t m;
    int rc = fossil_threads_mutex_init(&m);
    ASSUME_ITS_EQUAL_I32(rc, 0);

    rc = fossil_threads_mutex_lock(&m);
    ASSUME_ITS_EQUAL_I32(rc, 0);

    rc = fossil_threads_mutex_unlock(&m);
    ASSUME_ITS_EQUAL_I32(rc, 0);

    fossil_threads_mutex_dispose(&m);
}

FOSSIL_TEST_CASE(c_thread_mutex_trylock) {
    fossil_threads_mutex_t m;
    int rc = fossil_threads_mutex_init(&m);
    ASSUME_ITS_EQUAL_I32(rc, 0);

    // Lock the mutex
    rc = fossil_threads_mutex_lock(&m);
    ASSUME_ITS_EQUAL_I32(rc, 0);

    // Try to lock again, should get EBUSY
    int rc2 = fossil_threads_mutex_trylock(&m);
    ASSUME_ITS_EQUAL_I32(rc2, FOSSIL_THREADS_MUTEX_EBUSY);

    rc = fossil_threads_mutex_unlock(&m);
    ASSUME_ITS_EQUAL_I32(rc, 0);

    fossil_threads_mutex_dispose(&m);
}

FOSSIL_TEST_CASE(c_thread_mutex_init_null) {
    int rc = fossil_threads_mutex_init(NULL);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_MUTEX_EINVAL);
}

FOSSIL_TEST_CASE(c_thread_mutex_dispose_twice) {
    fossil_threads_mutex_t m;
    int rc = fossil_threads_mutex_init(&m);
    ASSUME_ITS_EQUAL_I32(rc, 0);
    fossil_threads_mutex_dispose(&m);
    fossil_threads_mutex_dispose(&m); // Should be safe, no crash
}

FOSSIL_TEST_CASE(c_thread_mutex_lock_invalid) {
    int rc = fossil_threads_mutex_lock(NULL);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_MUTEX_EINVAL);

    fossil_threads_mutex_t m;
    memset(&m, 0, sizeof(m)); // Not initialized
    rc = fossil_threads_mutex_lock(&m);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_MUTEX_EINVAL);
}

FOSSIL_TEST_CASE(c_thread_mutex_unlock_invalid) {
    int rc = fossil_threads_mutex_unlock(NULL);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_MUTEX_EINVAL);

    fossil_threads_mutex_t m;
    memset(&m, 0, sizeof(m)); // Not initialized
    rc = fossil_threads_mutex_unlock(&m);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_MUTEX_EINVAL);
}

FOSSIL_TEST_CASE(c_thread_mutex_trylock_invalid) {
    int rc = fossil_threads_mutex_trylock(NULL);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_MUTEX_EINVAL);

    fossil_threads_mutex_t m;
    memset(&m, 0, sizeof(m)); // Not initialized
    rc = fossil_threads_mutex_trylock(&m);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_MUTEX_EINVAL);
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Pool
// * * * * * * * * * * * * * * * * * * * * * * * *
FOSSIL_TEST_GROUP(c_mutex_tests) {
    FOSSIL_TEST_ADD(c_mutex_fixture, c_thread_mutex_init_dispose);
    FOSSIL_TEST_ADD(c_mutex_fixture, c_thread_mutex_lock_unlock);
    FOSSIL_TEST_ADD(c_mutex_fixture, c_thread_mutex_trylock);
    FOSSIL_TEST_ADD(c_mutex_fixture, c_thread_mutex_init_null);
    FOSSIL_TEST_ADD(c_mutex_fixture, c_thread_mutex_dispose_twice);
    FOSSIL_TEST_ADD(c_mutex_fixture, c_thread_mutex_lock_invalid);
    FOSSIL_TEST_ADD(c_mutex_fixture, c_thread_mutex_unlock_invalid);
    FOSSIL_TEST_ADD(c_mutex_fixture, c_thread_mutex_trylock_invalid);

    FOSSIL_TEST_REGISTER(c_mutex_fixture);
} // end of tests
