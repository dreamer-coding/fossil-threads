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

FOSSIL_TEST_SUITE(cpp_thread_fixture);

FOSSIL_SETUP(cpp_thread_fixture) {
    // Setup the test fixture
}

FOSSIL_TEARDOWN(cpp_thread_fixture) {
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

using fossil::threads::Thread;

static void* test_thread_funcpp_noop(void* arg) {
    (void)arg;
#if defined(_WIN32)
    return (void*)(uintptr_t)0;
#else
    return NULL;
#endif
}

static void* test_thread_funcpp_sleep(void* arg) {
    unsigned int ms = arg ? *(unsigned int*)arg : 10;
    Thread::sleep_ms(ms);
#if defined(_WIN32)
    return (void*)(uintptr_t)ms;
#else
    return (void*)(uintptr_t)ms;
#endif
}

FOSSIL_TEST_CASE(cpp_thread_zero_and_init) {
    Thread t;
    // After default construction, all bytes should be zero
    const unsigned char* bytes = (const unsigned char*)t.native_handle();
    int all_zero = 1;
    for (size_t i = 0; i < sizeof(fossil_threads_thread_t); ++i) {
        if (bytes[i] != 0) { all_zero = 0; break; }
    }
    ASSUME_ITS_TRUE(all_zero);
}

FOSSIL_TEST_CASE(cpp_thread_dispose_null_safe) {
    fossil_threads_thread_dispose(NULL);
    ASSUME_ITS_TRUE(1);
}

FOSSIL_TEST_CASE(cpp_thread_create_twice_should_fail) {
    Thread t;
    int rc = fossil_threads_thread_create(t.native_handle(), test_thread_funcpp_noop, NULL);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    rc = fossil_threads_thread_create(t.native_handle(), test_thread_funcpp_noop, NULL);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EBUSY);

    t.join();
}

FOSSIL_TEST_CASE(cpp_thread_join_twice_should_fail) {
    Thread t;
    int rc = fossil_threads_thread_create(t.native_handle(), test_thread_funcpp_noop, NULL);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    rc = t.join();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    rc = t.join();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EPERM);
}

FOSSIL_TEST_CASE(cpp_thread_detach_twice_should_fail) {
    Thread t;
    int rc = fossil_threads_thread_create(t.native_handle(), test_thread_funcpp_noop, NULL);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    rc = t.detach();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    rc = t.detach();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EPERM);
}

FOSSIL_TEST_CASE(cpp_thread_sleep_and_return_value) {
    unsigned int ms = 25;
    Thread t(test_thread_funcpp_sleep, &ms);
    void* ret = nullptr;
    int rc = t.join(&ret);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);
    ASSUME_ITS_EQUAL_I32((unsigned int)(uintptr_t)ret, ms);
}

FOSSIL_TEST_CASE(cpp_thread_equal_null_and_self) {
    Thread t;
    Thread t2;
    ASSUME_ITS_TRUE(Thread::equal(t, t));
    // Can't compare with nullptr, so compare native handles
    ASSUME_ITS_TRUE(!fossil_threads_thread_equal(t.native_handle(), NULL));
    ASSUME_ITS_TRUE(!fossil_threads_thread_equal(NULL, t.native_handle()));
    ASSUME_ITS_TRUE(fossil_threads_thread_equal(NULL, NULL));
}

FOSSIL_TEST_CASE(cpp_thread_create_and_join) {
    Thread t(test_thread_funcpp_noop, NULL);
    int rc = t.join();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);
}

FOSSIL_TEST_CASE(cpp_thread_create_and_detach) {
    Thread t(test_thread_funcpp_noop, NULL);
    int rc = t.detach();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);
}

FOSSIL_TEST_CASE(cpp_thread_yield_and_sleep) {
    Thread::yield();
    int rc = fossil_threads_thread_sleep_ms(5);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);
}

FOSSIL_TEST_CASE(cpp_thread_create_invalid_args) {
    int rc = fossil_threads_thread_create(NULL, test_thread_funcpp_noop, NULL);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EINVAL);

    Thread t;
    rc = fossil_threads_thread_create(t.native_handle(), NULL, NULL);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EINVAL);
}

FOSSIL_TEST_CASE(cpp_thread_priority_set_get) {
    Thread t;
    int rc = t.set_priority(5);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);
    ASSUME_ITS_EQUAL_I32(t.get_priority(), 5);

    rc = fossil_threads_thread_set_priority(NULL, 1);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EINVAL);
    ASSUME_ITS_EQUAL_I32(fossil_threads_thread_get_priority(NULL), FOSSIL_THREADS_EINVAL);
}

FOSSIL_TEST_CASE(cpp_thread_affinity_set_get) {
    Thread t;
    int rc = t.set_affinity(2);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);
    ASSUME_ITS_EQUAL_I32(t.get_affinity(), 2);

    rc = fossil_threads_thread_set_affinity(NULL, 1);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EINVAL);
    ASSUME_ITS_EQUAL_I32(fossil_threads_thread_get_affinity(NULL), FOSSIL_THREADS_EINVAL);
}

FOSSIL_TEST_CASE(cpp_thread_cancel_and_is_running) {
    Thread t;
    int rc = t.cancel();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EPERM);

    ASSUME_ITS_TRUE(!fossil_threads_thread_is_running(NULL));
    ASSUME_ITS_TRUE(!t.is_running());

    Thread t2(test_thread_funcpp_sleep, NULL);
    ASSUME_ITS_TRUE(t2.is_running());

    t2.join();
    ASSUME_ITS_TRUE(!t2.is_running());
}

FOSSIL_TEST_CASE(cpp_thread_get_retval) {
    Thread t;
    ASSUME_ITS_TRUE(fossil_threads_thread_get_retval(NULL) == NULL);
    ASSUME_ITS_TRUE(t.get_retval() == NULL);

    unsigned int ms = 42;
    Thread t2(test_thread_funcpp_sleep, &ms);
    t2.join();
    void* ret = t2.get_retval();
    ASSUME_ITS_EQUAL_I32((unsigned int)(uintptr_t)ret, ms);
}

FOSSIL_TEST_CASE(cpp_thread_pool_api_stubs) {
    fossil_threads_pool_t* pool = fossil_threads_pool_create(0);
    ASSUME_ITS_TRUE(pool == NULL);

    fossil_threads_pool_destroy(pool);

    int rc = fossil_threads_pool_submit(pool, test_thread_funcpp_noop, NULL);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EINVAL);

    rc = fossil_threads_pool_wait(pool);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EINVAL);

    ASSUME_ITS_EQUAL_I32((int)fossil_threads_pool_size(pool), 0);

    pool = fossil_threads_pool_create(2);
    ASSUME_ITS_TRUE(pool != NULL);

    rc = fossil_threads_pool_submit(pool, test_thread_funcpp_noop, NULL);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    rc = fossil_threads_pool_wait(pool);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    fossil_threads_pool_destroy(pool);
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Pool
// * * * * * * * * * * * * * * * * * * * * * * * *
FOSSIL_TEST_GROUP(cpp_thread_tests) {    
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_zero_and_init);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_dispose_null_safe);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_create_twice_should_fail);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_join_twice_should_fail);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_detach_twice_should_fail);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_sleep_and_return_value);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_equal_null_and_self);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_create_and_join);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_create_and_detach);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_yield_and_sleep);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_create_invalid_args);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_priority_set_get);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_affinity_set_get);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_cancel_and_is_running);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_get_retval);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_pool_api_stubs);

    FOSSIL_TEST_REGISTER(cpp_thread_fixture);
} // end of tests
