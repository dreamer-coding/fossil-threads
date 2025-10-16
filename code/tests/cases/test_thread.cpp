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
    return nullptr;
}

static void* test_thread_funcpp_sleep(void* arg) {
    unsigned int ms = arg ? *(unsigned int*)arg : 10;
    Thread::sleep_ms(ms);
    return reinterpret_cast<void*>(static_cast<uintptr_t>(ms));
}

FOSSIL_TEST_CASE(cpp_thread_zero_and_init) {
    Thread thread;
    fossil_threads_thread_t* native = thread.native_handle();
    memset(native, 0xFF, sizeof(*native));
    fossil_threads_thread_init(native);
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(native);
    int all_zero = 1;
    for (size_t i = 0; i < sizeof(*native); ++i) {
        if (bytes[i] != 0) { all_zero = 0; break; }
    }
    ASSUME_ITS_TRUE(all_zero);
}

FOSSIL_TEST_CASE(cpp_thread_dispose_null_safe) {
    fossil_threads_thread_dispose(nullptr);
    ASSUME_ITS_TRUE(1);
}

FOSSIL_TEST_CASE(cpp_thread_create_twice_should_fail) {
    Thread thread;
    int rc = fossil_threads_thread_create(thread.native_handle(), test_thread_funcpp_noop, nullptr);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    rc = fossil_threads_thread_create(thread.native_handle(), test_thread_funcpp_noop, nullptr);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EBUSY);

    thread.join();
}

FOSSIL_TEST_CASE(cpp_thread_join_twice_should_fail) {
    Thread thread;
    int rc = fossil_threads_thread_create(thread.native_handle(), test_thread_funcpp_noop, nullptr);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    rc = thread.join();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    rc = thread.join();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EPERM);
}

FOSSIL_TEST_CASE(cpp_thread_detach_twice_should_fail) {
    Thread thread;
    int rc = fossil_threads_thread_create(thread.native_handle(), test_thread_funcpp_noop, nullptr);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    rc = thread.detach();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    rc = thread.detach();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EPERM);
}

FOSSIL_TEST_CASE(cpp_thread_sleep_and_return_value) {
    Thread thread;
    unsigned int ms = 25;
    void* ret = nullptr;
    int rc = fossil_threads_thread_create(thread.native_handle(), test_thread_funcpp_sleep, &ms);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    rc = thread.join(&ret);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);
    ASSUME_ITS_EQUAL_I32(static_cast<unsigned int>(reinterpret_cast<uintptr_t>(ret)), ms);
}

FOSSIL_TEST_CASE(cpp_thread_equal_null_and_self) {
    Thread thread;
    // Equal to self
    ASSUME_ITS_TRUE(fossil_threads_thread_equal(thread.native_handle(), thread.native_handle()));

    // Not equal to NULL
    ASSUME_ITS_TRUE(!fossil_threads_thread_equal(thread.native_handle(), nullptr));
    ASSUME_ITS_TRUE(!fossil_threads_thread_equal(nullptr, thread.native_handle()));
    ASSUME_ITS_TRUE(fossil_threads_thread_equal(nullptr, nullptr));
}


FOSSIL_TEST_CASE(cpp_thread_create_and_join) {
    Thread thread;
    int rc = fossil_threads_thread_create(thread.native_handle(), test_thread_funcpp_noop, nullptr);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    rc = thread.join();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);
}

FOSSIL_TEST_CASE(cpp_thread_create_and_detach) {
    Thread thread;
    int rc = fossil_threads_thread_create(thread.native_handle(), test_thread_funcpp_noop, nullptr);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    rc = thread.detach();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);
}

FOSSIL_TEST_CASE(cpp_thread_yield_and_sleep) {
    Thread::yield();
    Thread::sleep_ms(5);
    ASSUME_ITS_TRUE(1);
}

FOSSIL_TEST_CASE(cpp_thread_id_and_equal) {
    Thread thread1, thread2;
    ASSUME_ITS_TRUE(!fossil_threads_thread_equal(thread1.native_handle(), thread2.native_handle()));
    ASSUME_ITS_TRUE(fossil_threads_thread_equal(thread1.native_handle(), thread1.native_handle()));
}

FOSSIL_TEST_CASE(cpp_thread_create_invalid_args) {
    int rc = fossil_threads_thread_create(nullptr, test_thread_funcpp_noop, nullptr);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EINVAL);

    Thread thread;
    rc = fossil_threads_thread_create(thread.native_handle(), nullptr, nullptr);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EINVAL);
}

FOSSIL_TEST_CASE(cpp_thread_join_invalid_args) {
    int rc = fossil_threads_thread_join(nullptr, nullptr);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EINVAL);

    Thread thread;
    rc = thread.join();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EPERM);
}

FOSSIL_TEST_CASE(cpp_thread_detach_invalid_args) {
    int rc = fossil_threads_thread_detach(nullptr);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EINVAL);

    Thread thread;
    rc = thread.detach();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EPERM);
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
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_id_and_equal);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_create_invalid_args);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_join_invalid_args);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_detach_invalid_args);

    FOSSIL_TEST_REGISTER(cpp_thread_fixture);
} // end of tests
