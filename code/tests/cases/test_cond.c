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

FOSSIL_TEST_SUITE(c_cond_fixture);

FOSSIL_SETUP(c_cond_fixture) {
    // Setup the test fixture
}

FOSSIL_TEARDOWN(c_cond_fixture) {
    // Teardown the test fixture
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Cases
// * * * * * * * * * * * * * * * * * * * * * * * *
// The test cases below are provided as samples, inspired
// by the Meson build system's approach of using test cases
// as samples for library usage.
// * * * * * * * * * * * * * * * * * * * * * * * *

FOSSIL_TEST_CASE(c_cond_init_and_dispose) {
    fossil_threads_cond_t cond;
    int rc = fossil_threads_cond_init(&cond);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_COND_OK);
    ASSUME_ITS_EQUAL_I32(cond.valid, 1);

    fossil_threads_cond_dispose(&cond);
    ASSUME_ITS_EQUAL_I32(cond.valid, 0);
}

FOSSIL_TEST_CASE(c_cond_init_null) {
    int rc = fossil_threads_cond_init(NULL);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_COND_EINVAL);
}

FOSSIL_TEST_CASE(c_cond_signal_and_broadcast_invalid) {
    fossil_threads_cond_t cond;
    memset(&cond, 0, sizeof(cond));
    int rc1 = fossil_threads_cond_signal(NULL);
    int rc2 = fossil_threads_cond_signal(&cond);
    int rc3 = fossil_threads_cond_broadcast(NULL);
    int rc4 = fossil_threads_cond_broadcast(&cond);
    ASSUME_ITS_EQUAL_I32(rc1, FOSSIL_THREADS_COND_EINVAL);
    ASSUME_ITS_EQUAL_I32(rc2, FOSSIL_THREADS_COND_EINVAL);
    ASSUME_ITS_EQUAL_I32(rc3, FOSSIL_THREADS_COND_EINVAL);
    ASSUME_ITS_EQUAL_I32(rc4, FOSSIL_THREADS_COND_EINVAL);
}

FOSSIL_TEST_CASE(c_cond_wait_invalid) {
    fossil_threads_cond_t cond;
    fossil_threads_mutex_t mutex;
    memset(&cond, 0, sizeof(cond));
    memset(&mutex, 0, sizeof(mutex));
    int rc1 = fossil_threads_cond_wait(NULL, NULL);
    int rc2 = fossil_threads_cond_wait(&cond, NULL);
    int rc3 = fossil_threads_cond_wait(NULL, &mutex);
    int rc4 = fossil_threads_cond_wait(&cond, &mutex);
    ASSUME_ITS_EQUAL_I32(rc1, FOSSIL_THREADS_COND_EINVAL);
    ASSUME_ITS_EQUAL_I32(rc2, FOSSIL_THREADS_COND_EINVAL);
    ASSUME_ITS_EQUAL_I32(rc3, FOSSIL_THREADS_COND_EINVAL);
    ASSUME_ITS_EQUAL_I32(rc4, FOSSIL_THREADS_COND_EINVAL);
}

FOSSIL_TEST_CASE(c_cond_timedwait_invalid) {
    fossil_threads_cond_t cond;
    fossil_threads_mutex_t mutex;
    memset(&cond, 0, sizeof(cond));
    memset(&mutex, 0, sizeof(mutex));
    int rc1 = fossil_threads_cond_timedwait(NULL, NULL, 100);
    int rc2 = fossil_threads_cond_timedwait(&cond, NULL, 100);
    int rc3 = fossil_threads_cond_timedwait(NULL, &mutex, 100);
    int rc4 = fossil_threads_cond_timedwait(&cond, &mutex, 100);
    ASSUME_ITS_EQUAL_I32(rc1, FOSSIL_THREADS_COND_EINVAL);
    ASSUME_ITS_EQUAL_I32(rc2, FOSSIL_THREADS_COND_EINVAL);
    ASSUME_ITS_EQUAL_I32(rc3, FOSSIL_THREADS_COND_EINVAL);
    ASSUME_ITS_EQUAL_I32(rc4, FOSSIL_THREADS_COND_EINVAL);
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Pool
// * * * * * * * * * * * * * * * * * * * * * * * *
FOSSIL_TEST_GROUP(c_cond_tests) {
    FOSSIL_TEST_ADD(c_cond_fixture, c_cond_init_and_dispose);
    FOSSIL_TEST_ADD(c_cond_fixture, c_cond_init_null);
    FOSSIL_TEST_ADD(c_cond_fixture, c_cond_signal_and_broadcast_invalid);
    FOSSIL_TEST_ADD(c_cond_fixture, c_cond_wait_invalid);
    FOSSIL_TEST_ADD(c_cond_fixture, c_cond_timedwait_invalid);

    FOSSIL_TEST_REGISTER(c_cond_fixture);
} // end of tests
