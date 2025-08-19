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

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Pool
// * * * * * * * * * * * * * * * * * * * * * * * *
FOSSIL_TEST_GROUP(c_mutex_tests) {
    FOSSIL_TEST_ADD(c_mutex_fixture, c_thread_mutex_init_dispose);
    FOSSIL_TEST_ADD(c_mutex_fixture, c_thread_mutex_lock_unlock);
    FOSSIL_TEST_ADD(c_mutex_fixture, c_thread_mutex_trylock);

    FOSSIL_TEST_REGISTER(c_mutex_fixture);
} // end of tests
