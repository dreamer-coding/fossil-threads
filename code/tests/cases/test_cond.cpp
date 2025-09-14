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

FOSSIL_TEST_SUITE(cpp_cond_fixture);

FOSSIL_SETUP(cpp_cond_fixture) {
    // Setup the test fixture
}

FOSSIL_TEARDOWN(cpp_cond_fixture) {
    // Teardown the test fixture
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Cases
// * * * * * * * * * * * * * * * * * * * * * * * *
// The test cases below are provided as samples, inspired
// by the Meson build system's approach of using test cases
// as samples for library usage.
// * * * * * * * * * * * * * * * * * * * * * * * *

using fossil::threads::Cond;

FOSSIL_TEST_CASE(cpp_cond_raii_init_and_dispose) {
    Cond* cond = nullptr;
    try {
        cond = new Cond();
        ASSUME_ITS_TRUE(cond != nullptr);
    } catch (...) {
        ASSUME_ITS_TRUE(false); // Should not throw
    }
    delete cond;
    // No explicit check for valid, as RAII handles it
}

FOSSIL_TEST_CASE(cpp_cond_raii_signal_and_broadcast) {
    Cond cond;
    int rc1 = cond.signal();
    int rc2 = cond.broadcast();
    ASSUME_ITS_EQUAL_I32(rc1, FOSSIL_THREADS_COND_OK);
    ASSUME_ITS_EQUAL_I32(rc2, FOSSIL_THREADS_COND_OK);
}

FOSSIL_TEST_CASE(cpp_cond_raii_wait_invalid_mutex) {
    Cond cond;
    int rc1 = cond.wait(nullptr);
    int rc2 = cond.timed_wait(nullptr, 100);
    ASSUME_ITS_EQUAL_I32(rc1, FOSSIL_THREADS_COND_EINVAL);
    ASSUME_ITS_EQUAL_I32(rc2, FOSSIL_THREADS_COND_EINVAL);
}

FOSSIL_TEST_CASE(cpp_cond_raii_wait_and_timedwait) {
    Cond cond;
    fossil_threads_mutex_t mutex;
    memset(&mutex, 0, sizeof(mutex));
    // mutex is not initialized, so wait should fail
    int rc1 = cond.wait(&mutex);
    int rc2 = cond.timed_wait(&mutex, 100);
    ASSUME_ITS_EQUAL_I32(rc1, FOSSIL_THREADS_COND_EINVAL);
    ASSUME_ITS_EQUAL_I32(rc2, FOSSIL_THREADS_COND_EINVAL);
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Pool
// * * * * * * * * * * * * * * * * * * * * * * * *
FOSSIL_TEST_GROUP(cpp_cond_tests) {
    FOSSIL_TEST_ADD(cpp_cond_fixture, cpp_cond_raii_init_and_dispose);
    FOSSIL_TEST_ADD(cpp_cond_fixture, cpp_cond_raii_signal_and_broadcast);
    FOSSIL_TEST_ADD(cpp_cond_fixture, cpp_cond_raii_wait_invalid_mutex);
    FOSSIL_TEST_ADD(cpp_cond_fixture, cpp_cond_raii_wait_and_timedwait);

    FOSSIL_TEST_REGISTER(cpp_cond_fixture);
} // end of tests
