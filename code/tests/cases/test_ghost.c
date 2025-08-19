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

FOSSIL_TEST_SUITE(c_ghost_fixture);

FOSSIL_SETUP(c_ghost_fixture) {
    // Setup the test fixture
}

FOSSIL_TEARDOWN(c_ghost_fixture) {
    // Teardown the test fixture
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Cases
// * * * * * * * * * * * * * * * * * * * * * * * *
// The test cases below are provided as samples, inspired
// by the Meson build system's approach of using test cases
// as samples for library usage.
// * * * * * * * * * * * * * * * * * * * * * * * *

FOSSIL_TEST_CASE(c_thread_ghost_init) {
    int rc = fossil_threads_ghost_init();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);
}

static void dummy_func(void *arg, void **out_state) {
    if (out_state) *out_state = arg;
}

FOSSIL_TEST_CASE(c_thread_ghost_create_and_state) {
    fossil_threads_ghost_t ghost;
    int rc = fossil_threads_ghost_init();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);

    int dummy_state = 42;
    rc = fossil_threads_ghost_create(&ghost, "ghost1", dummy_func, &dummy_state);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);

    void *state = NULL;
    rc = fossil_threads_ghost_state(&ghost, &state);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);
    ASSUME_ITS_EQUAL_PTR(state, NULL); // Not stepped yet

    fossil_threads_ghost_dispose(&ghost);
}

FOSSIL_TEST_CASE(c_thread_ghost_step_and_state) {
    fossil_threads_ghost_t ghost;
    int rc = fossil_threads_ghost_init();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);

    int dummy_state = 123;
    rc = fossil_threads_ghost_create(&ghost, "ghost2", dummy_func, &dummy_state);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);

    rc = fossil_threads_ghost_step(&ghost);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);

    void *state = NULL;
    rc = fossil_threads_ghost_state(&ghost, &state);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);
    ASSUME_ITS_EQUAL_PTR(state, &dummy_state);

    fossil_threads_ghost_dispose(&ghost);
}

FOSSIL_TEST_CASE(c_thread_ghost_propose_and_collapse) {
    fossil_threads_ghost_t ghost;
    int rc = fossil_threads_ghost_init();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);

    int state1 = 1, state2 = 2;
    fossil_threads_ghost_candidate_t candidates[2] = {
        { "A", (size_t)state1, { 0 } },
        { "B", (size_t)state2, { 0 } }
    };

    rc = fossil_threads_ghost_create(&ghost, "ghost3", NULL, NULL);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);

    rc = fossil_threads_ghost_propose_candidates(&ghost, candidates, 2);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);

    int chosen = fossil_threads_ghost_collapse_by_consensus(&ghost);
    ASSUME_ITS_TRUE(chosen == 0 || chosen == 1);

    void *state = NULL;
    rc = fossil_threads_ghost_state(&ghost, &state);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);
    ASSUME_ITS_TRUE(state == &state1 || state == &state2);

    fossil_threads_ghost_dispose(&ghost);
}

FOSSIL_TEST_CASE(c_thread_ghost_queue_and_schedule) {
    fossil_threads_ghost_t ghost;
    int rc = fossil_threads_ghost_init();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);

    int dummy_state = 99;
    rc = fossil_threads_ghost_create(&ghost, "ghost4", dummy_func, &dummy_state);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);

    rc = fossil_threads_ghost_queue_add(&ghost);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);

    rc = fossil_threads_ghost_schedule();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);

    void *state = NULL;
    rc = fossil_threads_ghost_state(&ghost, &state);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);
    ASSUME_ITS_EQUAL_PTR(state, &dummy_state);

    fossil_threads_ghost_dispose(&ghost);
}

FOSSIL_TEST_CASE(c_thread_ghost_finished_and_dispose) {
    fossil_threads_ghost_t ghost;
    int rc = fossil_threads_ghost_init();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);

    rc = fossil_threads_ghost_create(&ghost, "ghost5", NULL, NULL);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);

    int finished = fossil_threads_ghost_finished(&ghost);
    ASSUME_ITS_EQUAL_I32(finished, 0);

    fossil_threads_ghost_dispose(&ghost);

    finished = fossil_threads_ghost_finished(&ghost);
    ASSUME_ITS_EQUAL_I32(finished, 1);
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Pool
// * * * * * * * * * * * * * * * * * * * * * * * *
FOSSIL_TEST_GROUP(c_ghost_tests) {
    FOSSIL_TEST_ADD(c_ghost_fixture, c_thread_ghost_init);
    FOSSIL_TEST_ADD(c_ghost_fixture, c_thread_ghost_create_and_state);
    FOSSIL_TEST_ADD(c_ghost_fixture, c_thread_ghost_step_and_state);
    FOSSIL_TEST_ADD(c_ghost_fixture, c_thread_ghost_propose_and_collapse);
    FOSSIL_TEST_ADD(c_ghost_fixture, c_thread_ghost_queue_and_schedule);
    FOSSIL_TEST_ADD(c_ghost_fixture, c_thread_ghost_finished_and_dispose);

    FOSSIL_TEST_REGISTER(c_ghost_fixture);
} // end of tests
