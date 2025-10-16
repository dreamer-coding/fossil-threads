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

static void* test_thread_funcpp_return_arg(void* arg) {
    return arg;
}

static void* test_thread_funcpp_set_flag(void* arg) {
    volatile int* f = (volatile int*)arg;
    *f = 1;
    return nullptr;
}

static void* test_thread_funcpp_store_id(void* arg) {
    unsigned long* tid = (unsigned long*)arg;
    *tid = Thread::current_id();
    return nullptr;
}

FOSSIL_TEST_CASE(cpp_thread_create_and_join) {
    Thread thread;
    int arg = 42;
    void* ret = nullptr;

    int rc = fossil_threads_thread_create(thread.native_handle(), test_thread_funcpp_return_arg, &arg);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    rc = thread.join(&ret);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);
    ASSUME_ITS_EQUAL_I32(*(int*)ret, 42);
}

FOSSIL_TEST_CASE(cpp_thread_create_and_detach) {
    Thread thread;
    volatile int flag = 0;

    int rc = fossil_threads_thread_create(thread.native_handle(), test_thread_funcpp_set_flag, (void*)&flag);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    rc = thread.detach();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    // Wait for the thread to set the flag
    for (int i = 0; i < 100 && flag == 0; ++i) {
        Thread::sleep_ms(1);
    }
    ASSUME_ITS_EQUAL_I32(flag, 1);
}

FOSSIL_TEST_CASE(cpp_thread_yield_and_sleep) {
    int rc = fossil_threads_thread_yield();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    rc = fossil_threads_thread_sleep_ms(10);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);
}

FOSSIL_TEST_CASE(cpp_thread_id_and_equal) {
    Thread thread1, thread2;

    unsigned long main_id = Thread::current_id();
    ASSUME_ITS_TRUE(main_id != 0);

    unsigned long thread_id = 0;
    int rc = fossil_threads_thread_create(thread1.native_handle(), test_thread_funcpp_store_id, &thread_id);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);
    rc = thread1.join();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    ASSUME_ITS_TRUE(thread_id != 0);
    ASSUME_ITS_TRUE(thread_id != main_id);

    unsigned long thread2_id = 0;
    rc = fossil_threads_thread_create(thread2.native_handle(), test_thread_funcpp_store_id, &thread2_id);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);
    rc = thread2.join();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);
    ASSUME_ITS_TRUE(thread2_id != 0);
    ASSUME_ITS_TRUE(thread2_id != thread_id);

    ASSUME_ITS_TRUE(Thread::equal(thread1, thread1));
    ASSUME_ITS_TRUE(!Thread::equal(thread1, thread2));
}

FOSSIL_TEST_CASE(cpp_thread_create_invalid_args) {
    Thread thread;

    int rc = fossil_threads_thread_create(nullptr, test_thread_funcpp_return_arg, nullptr);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EINVAL);

    rc = fossil_threads_thread_create(thread.native_handle(), nullptr, nullptr);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EINVAL);
}

FOSSIL_TEST_CASE(cpp_thread_join_invalid_args) {
    Thread thread;

    int rc = fossil_threads_thread_join(thread.native_handle(), nullptr);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EPERM);

    rc = fossil_threads_thread_join(nullptr, nullptr);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EINVAL);
}

FOSSIL_TEST_CASE(cpp_thread_detach_invalid_args) {
    Thread thread;

    int rc = fossil_threads_thread_detach(thread.native_handle());
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EPERM);

    rc = fossil_threads_thread_detach(nullptr);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_EINVAL);
}

FOSSIL_TEST_CASE(cpp_thread_dispose_safe_multiple) {
    Thread thread;

    fossil_threads_thread_dispose(thread.native_handle());
    fossil_threads_thread_dispose(thread.native_handle());

    int rc = fossil_threads_thread_create(thread.native_handle(), test_thread_funcpp_return_arg, nullptr);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);
    rc = thread.join();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_THREADS_OK);

    fossil_threads_thread_dispose(thread.native_handle());
    fossil_threads_thread_dispose(thread.native_handle());

    fossil_threads_thread_dispose(nullptr);
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Pool
// * * * * * * * * * * * * * * * * * * * * * * * *
FOSSIL_TEST_GROUP(cpp_thread_tests) {    
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_create_and_join);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_create_and_detach);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_yield_and_sleep);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_id_and_equal);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_create_invalid_args);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_join_invalid_args);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_detach_invalid_args);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_dispose_safe_multiple);

    FOSSIL_TEST_REGISTER(cpp_thread_fixture);
} // end of tests
