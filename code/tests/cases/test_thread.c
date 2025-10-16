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

FOSSIL_TEST_SUITE(c_thread_fixture);

FOSSIL_SETUP(c_thread_fixture) {
    // Setup the test fixture
}

FOSSIL_TEARDOWN(c_thread_fixture) {
    // Teardown the test fixture
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Cases
// * * * * * * * * * * * * * * * * * * * * * * * *
// The test cases below are provided as samples, inspired
// by the Meson build system's approach of using test cases
// as samples for library usage.
// * * * * * * * * * * * * * * * * * * * * * * * *

/* ---------- Lifecycle ---------- */

static void *test_thread_func_return_arg(void *arg) {
    return arg;
}

static void *test_thread_func_set_flag(void *arg) {
    volatile int *f = (volatile int *)arg;
    *f = 1;
    return NULL;
}

static void *test_thread_func_store_id(void *arg) {
    unsigned long *tid = (unsigned long *)arg;
    *tid = fossil_threads_thread_id();
    return NULL;
}

FOSSIL_TEST_CASE(c_thread_create_and_join) {
    fossil_threads_thread_t thread;
    fossil_threads_thread_init(&thread);

    int arg = 42;
    void *ret = NULL;

    int rc = fossil_threads_thread_create(&thread, test_thread_func_return_arg, &arg);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    rc = fossil_threads_thread_join(&thread, &ret);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);
    ASSUME_ITS_EQUAL_I32(*(int *)ret, 42);

    fossil_threads_thread_dispose(&thread);
}

FOSSIL_TEST_CASE(c_thread_create_and_detach) {
    fossil_threads_thread_t thread;
    fossil_threads_thread_init(&thread);

    volatile int flag = 0;

    int rc = fossil_threads_thread_create(&thread, test_thread_func_set_flag, (void *)&flag);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    rc = fossil_threads_thread_detach(&thread);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    // Wait for the thread to set the flag
    for (int i = 0; i < 100 && flag == 0; ++i) {
        fossil_threads_thread_sleep_ms(1);
    }
    ASSUME_ITS_EQUAL_I32(flag, 1);

    fossil_threads_thread_dispose(&thread);
}

FOSSIL_TEST_CASE(c_thread_yield_and_sleep) {
    int rc = fossil_threads_thread_yield();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    rc = fossil_threads_thread_sleep_ms(10);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);
}

FOSSIL_TEST_CASE(c_thread_id_and_equal) {
    fossil_threads_thread_t thread1, thread2;
    fossil_threads_thread_init(&thread1);
    fossil_threads_thread_init(&thread2);

    unsigned long main_id = fossil_threads_thread_id();
    ASSUME_ITS_TRUE(main_id != 0);

    // Thread function that stores its thread id
    unsigned long thread_id = 0;
    int rc = fossil_threads_thread_create(&thread1, test_thread_func_store_id, &thread_id);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);
    rc = fossil_threads_thread_join(&thread1, NULL);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    ASSUME_ITS_TRUE(thread_id != 0);
    ASSUME_ITS_TRUE(thread_id != main_id);

    // Create a second thread to ensure thread2 is a valid, distinct thread
    unsigned long thread2_id = 0;
    rc = fossil_threads_thread_create(&thread2, test_thread_func_store_id, &thread2_id);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);
    rc = fossil_threads_thread_join(&thread2, NULL);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);
    ASSUME_ITS_TRUE(thread2_id != 0);
    ASSUME_ITS_TRUE(thread2_id != thread_id);

    // Compare main thread to itself
    ASSUME_ITS_TRUE(fossil_threads_thread_equal(&thread1, &thread1));
    ASSUME_ITS_TRUE(!fossil_threads_thread_equal(&thread1, &thread2));

    fossil_threads_thread_dispose(&thread1);
    fossil_threads_thread_dispose(&thread2);
}

FOSSIL_TEST_CASE(c_thread_create_invalid_args) {
    fossil_threads_thread_t thread;
    fossil_threads_thread_init(&thread);

    // NULL thread pointer
    int rc = fossil_threads_thread_create(NULL, test_thread_func_return_arg, NULL);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EINVAL);

    // NULL function pointer
    rc = fossil_threads_thread_create(&thread, NULL, NULL);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EINVAL);

    fossil_threads_thread_dispose(&thread);
}

FOSSIL_TEST_CASE(c_thread_join_invalid_args) {
    fossil_threads_thread_t thread;
    fossil_threads_thread_init(&thread);

    // Join before create
    int rc = fossil_threads_thread_join(&thread, NULL);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EPERM);

    // NULL thread pointer
    rc = fossil_threads_thread_join(NULL, NULL);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EINVAL);

    fossil_threads_thread_dispose(&thread);
}

FOSSIL_TEST_CASE(c_thread_detach_invalid_args) {
    fossil_threads_thread_t thread;
    fossil_threads_thread_init(&thread);

    // Detach before create
    int rc = fossil_threads_thread_detach(&thread);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EPERM);

    // NULL thread pointer
    rc = fossil_threads_thread_detach(NULL);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EINVAL);

    fossil_threads_thread_dispose(&thread);
}

FOSSIL_TEST_CASE(c_thread_dispose_safe_multiple) {
    fossil_threads_thread_t thread;
    fossil_threads_thread_init(&thread);

    fossil_threads_thread_dispose(&thread);
    fossil_threads_thread_dispose(&thread); // Should be safe, no crash

    // Create and join, then dispose multiple times
    int rc = fossil_threads_thread_create(&thread, test_thread_func_return_arg, NULL);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);
    rc = fossil_threads_thread_join(&thread, NULL);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    fossil_threads_thread_dispose(&thread);
    fossil_threads_thread_dispose(&thread);

    // Dispose on NULL pointer (should be safe/no-op)
    fossil_threads_thread_dispose(NULL);
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Pool
// * * * * * * * * * * * * * * * * * * * * * * * *
FOSSIL_TEST_GROUP(c_thread_tests) {    
    FOSSIL_TEST_ADD(c_thread_fixture, c_thread_create_and_join);
    FOSSIL_TEST_ADD(c_thread_fixture, c_thread_create_and_detach);
    FOSSIL_TEST_ADD(c_thread_fixture, c_thread_yield_and_sleep);
    FOSSIL_TEST_ADD(c_thread_fixture, c_thread_id_and_equal);
    FOSSIL_TEST_ADD(c_thread_fixture, c_thread_create_invalid_args);
    FOSSIL_TEST_ADD(c_thread_fixture, c_thread_join_invalid_args);
    FOSSIL_TEST_ADD(c_thread_fixture, c_thread_detach_invalid_args);
    FOSSIL_TEST_ADD(c_thread_fixture, c_thread_dispose_safe_multiple);

    FOSSIL_TEST_REGISTER(c_thread_fixture);
} // end of tests
