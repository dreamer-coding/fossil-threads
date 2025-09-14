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

FOSSIL_TEST_SUITE(cpp_mutex_fixture);

FOSSIL_SETUP(cpp_mutex_fixture) {
    // Setup the test fixture
}

FOSSIL_TEARDOWN(cpp_mutex_fixture) {
    // Teardown the test fixture
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Cases
// * * * * * * * * * * * * * * * * * * * * * * * *
// The test cases below are provided as samples, inspired
// by the Meson build system's approach of using test cases
// as samples for library usage.
// * * * * * * * * * * * * * * * * * * * * * * * *

FOSSIL_TEST_CASE(cpp_thread_mutex_init_dispose) {
    fossil::threads::Mutex m;
    // If construction fails, exception will be thrown and test will fail.
    // No further action needed; destructor will dispose.
}

FOSSIL_TEST_CASE(cpp_thread_mutex_lock_unlock) {
    fossil::threads::Mutex m;
    m.lock();
    m.unlock();
}

FOSSIL_TEST_CASE(cpp_thread_mutex_trylock) {
    fossil::threads::Mutex m;
    bool locked = m.try_lock();
    ASSUME_ITS_TRUE(locked);

    // Try to lock again, should return false (already locked)
    bool locked_again = m.try_lock();
    ASSUME_ITS_FALSE(locked_again);

    if (locked) {
        m.unlock();
    }
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Pool
// * * * * * * * * * * * * * * * * * * * * * * * *
FOSSIL_TEST_GROUP(cpp_mutex_tests) {
    FOSSIL_TEST_ADD(cpp_mutex_fixture, cpp_thread_mutex_init_dispose);
    FOSSIL_TEST_ADD(cpp_mutex_fixture, cpp_thread_mutex_lock_unlock);
    FOSSIL_TEST_ADD(cpp_mutex_fixture, cpp_thread_mutex_trylock);

    FOSSIL_TEST_REGISTER(cpp_mutex_fixture);
} // end of tests
