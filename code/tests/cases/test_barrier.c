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

FOSSIL_TEST_SUITE(c_barrier_fixture);

FOSSIL_SETUP(c_barrier_fixture) {
    // Setup the test fixture
}

FOSSIL_TEARDOWN(c_barrier_fixture) {
    // Teardown the test fixture
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Cases
// * * * * * * * * * * * * * * * * * * * * * * * *
// The test cases below are provided as samples, inspired
// by the Meson build system's approach of using test cases
// as samples for library usage.
// * * * * * * * * * * * * * * * * * * * * * * * *

FOSSIL_TEST_CASE(c_barrier_init_and_dispose) {
    fossil_threads_barrier_t barrier;
    int rc = fossil_threads_barrier_init(&barrier, 2, true);
    ASSUME_ITS_EQUAL_I32(rc, 0);
    // If init returns 0, the barrier is considered valid.
    ASSUME_ITS_EQUAL_I32(rc, 0);

    fossil_threads_barrier_destroy(&barrier);
    // After destroy, calling destroy again should fail.
    fossil_threads_barrier_destroy(&barrier);
}

FOSSIL_TEST_CASE(c_barrier_init_null) {
    int rc = fossil_threads_barrier_init(NULL, 2, true);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_COND_EINVAL);
}

FOSSIL_TEST_CASE(c_barrier_wait_invalid) {
    fossil_threads_barrier_t barrier;
    memset(&barrier, 0, sizeof(barrier));
    int rc1 = fossil_threads_barrier_wait(NULL);
    int rc2 = fossil_threads_barrier_wait(&barrier);
    ASSUME_ITS_EQUAL_I32(rc1, FOSSIL_THREADS_COND_EINVAL);
    ASSUME_ITS_EQUAL_I32(rc2, FOSSIL_THREADS_COND_EINVAL);
}

FOSSIL_TEST_CASE(c_barrier_wait_timeout_invalid) {
    fossil_threads_barrier_t barrier;
    memset(&barrier, 0, sizeof(barrier));
    int rc1 = fossil_threads_barrier_wait_timeout(NULL, 100);
    int rc2 = fossil_threads_barrier_wait_timeout(&barrier, 100);
    ASSUME_ITS_EQUAL_I32(rc1, FOSSIL_THREADS_COND_EINVAL);
    ASSUME_ITS_EQUAL_I32(rc2, FOSSIL_THREADS_COND_EINVAL);
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Pool
// * * * * * * * * * * * * * * * * * * * * * * * *
FOSSIL_TEST_GROUP(c_barrier_tests) {
    FOSSIL_TEST_ADD(c_barrier_fixture, c_barrier_init_and_dispose);
    FOSSIL_TEST_ADD(c_barrier_fixture, c_barrier_init_null);
    FOSSIL_TEST_ADD(c_barrier_fixture, c_barrier_wait_invalid);
    FOSSIL_TEST_ADD(c_barrier_fixture, c_barrier_wait_timeout_invalid);

    FOSSIL_TEST_REGISTER(c_barrier_fixture);
} // end of tests
