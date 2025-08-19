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

using fossil::threads::Thread;

static void *test_thread_funcpp_return_arg(void *arg) {
    return arg;
}

static void *test_thread_funcpp_set_flag(void *arg) {
    volatile int *f = (volatile int *)arg;
    *f = 1;
    return NULL;
}

static void *test_thread_funcpp_store_id(void *arg) {
    unsigned long *tid = (unsigned long *)arg;
    *tid = Thread::current_id();
    return NULL;
}

FOSSIL_TEST_CASE(cpp_thread_create_and_join) {
    int arg = 42;
    void *ret = nullptr;

    Thread thread(test_thread_funcpp_return_arg, &arg);

    int rc = thread.join(&ret);
    ASSUME_ITS_EQUAL_I32(rc, 0);
    ASSUME_ITS_EQUAL_I32(*(int *)ret, 42);
}

FOSSIL_TEST_CASE(cpp_thread_create_and_detach) {
    volatile int flag = 0;

    Thread thread(test_thread_funcpp_set_flag, (void *)&flag);

    int rc = thread.detach();
    ASSUME_ITS_EQUAL_I32(rc, 0);

    // Wait for the thread to set the flag
    for (int i = 0; i < 100 && flag == 0; ++i) {
        Thread::sleep_ms(1);
    }
    ASSUME_ITS_EQUAL_I32(flag, 1);
}

FOSSIL_TEST_CASE(cpp_thread_yield_and_sleep) {
    Thread::yield();
    Thread::sleep_ms(10);
}

FOSSIL_TEST_CASE(cpp_thread_id_and_equal) {
    Thread thread1, thread2;

    unsigned long main_id = Thread::current_id();
    ASSUME_ITS_TRUE(main_id != 0);

    // Thread function that stores its thread id
    unsigned long thread_id = 0;
    thread1 = Thread(test_thread_funcpp_store_id, &thread_id);
    int rc = thread1.join();
    ASSUME_ITS_EQUAL_I32(rc, 0);

    ASSUME_ITS_TRUE(thread_id != 0);
    ASSUME_ITS_TRUE(thread_id != main_id);

    // Compare thread1 to itself
    ASSUME_ITS_TRUE(Thread::equal(thread1, thread1));

    // Create a second valid thread and compare
    unsigned long thread_id2 = 0;
    thread2 = Thread(test_thread_funcpp_store_id, &thread_id2);
    int rc2 = thread2.join();
    ASSUME_ITS_EQUAL_I32(rc2, 0);

    ASSUME_ITS_TRUE(!Thread::equal(thread1, thread2));
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Pool
// * * * * * * * * * * * * * * * * * * * * * * * *
FOSSIL_TEST_GROUP(cpp_thread_tests) {    
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_create_and_join);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_create_and_detach);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_yield_and_sleep);
    FOSSIL_TEST_ADD(cpp_thread_fixture, cpp_thread_id_and_equal);

    FOSSIL_TEST_REGISTER(cpp_thread_fixture);
} // end of tests
