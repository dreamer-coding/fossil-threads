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

FOSSIL_TEST_SUITE(cpp_ghost_fixture);

FOSSIL_SETUP(cpp_ghost_fixture) {
    // Setup the test fixture
}

FOSSIL_TEARDOWN(cpp_ghost_fixture) {
    // Teardown the test fixture
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Cases
// * * * * * * * * * * * * * * * * * * * * * * * *
// The test cases below are provided as samples, inspired
// by the Meson build system's approach of using test cases
// as samples for library usage.
// * * * * * * * * * * * * * * * * * * * * * * * *

FOSSIL_TEST_CASE(cpp_thread_ghost_init) {
    int rc = fossil_threads_ghost_init();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);
}

static void dummy_func(void *arg, void **out_state) {
    if (out_state) *out_state = arg;
}

FOSSIL_TEST_CASE(cpp_thread_ghost_create_and_state) {
    using fossil::threads::Ghost;
    int rc = fossil_threads_ghost_init();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);

    int dummy_state = 42;
    Ghost ghost("ghost1", dummy_func, &dummy_state);

    void *state = ghost.state();
    ASSUME_ITS_EQUAL_PTR(state, NULL); // Not stepped yet
}

FOSSIL_TEST_CASE(cpp_thread_ghost_step_and_state) {
    using fossil::threads::Ghost;
    int rc = fossil_threads_ghost_init();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);

    int dummy_state = 123;
    Ghost ghost("ghost2", dummy_func, &dummy_state);

    rc = ghost.step();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);

    void *state = ghost.state();
    ASSUME_ITS_EQUAL_PTR(state, &dummy_state);
}

FOSSIL_TEST_CASE(cpp_thread_ghost_propose_and_collapse) {
    using fossil::threads::Ghost;
    int rc = fossil_threads_ghost_init();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);

    int state1 = 1, state2 = 2;
    std::vector<fossil_threads_ghost_candidate_t> candidates = {
        { "A", &state1 },
        { "B", &state2 }
    };

    Ghost ghost("ghost3", nullptr, nullptr);

    rc = ghost.propose_candidates(candidates);
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);

    int chosen = ghost.collapse_by_consensus();
    ASSUME_ITS_TRUE(chosen == 0 || chosen == 1);

    void *state = ghost.state();
    ASSUME_ITS_TRUE(state == &state1 || state == &state2);
}

FOSSIL_TEST_CASE(cpp_thread_ghost_queue_and_schedule) {
    using fossil::threads::Ghost;
    int rc = fossil_threads_ghost_init();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);

    int dummy_state = 99;
    Ghost ghost("ghost4", dummy_func, &dummy_state);

    rc = ghost.queue_add();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);

    rc = Ghost::schedule();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);

    void *state = ghost.state();
    ASSUME_ITS_EQUAL_PTR(state, &dummy_state);
}

FOSSIL_TEST_CASE(cpp_thread_ghost_finished_and_dispose) {
    using fossil::threads::Ghost;
    int rc = fossil_threads_ghost_init();
    ASSUME_ITS_EQUAL_I32(rc, FOSSIL_GHOST_OK);

    Ghost ghost("ghost5", nullptr, nullptr);

    int finished = ghost.finished();
    ASSUME_ITS_EQUAL_I32(finished, 0);
    // Destructor will dispose ghost

    // After destruction, finished cannot be checked on disposed object.
    // So, we just check before destruction.
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Pool
// * * * * * * * * * * * * * * * * * * * * * * * *
FOSSIL_TEST_GROUP(cpp_ghost_tests) {
    FOSSIL_TEST_ADD(cpp_ghost_fixture, cpp_thread_ghost_init);
    FOSSIL_TEST_ADD(cpp_ghost_fixture, cpp_thread_ghost_create_and_state);
    FOSSIL_TEST_ADD(cpp_ghost_fixture, cpp_thread_ghost_step_and_state);
    FOSSIL_TEST_ADD(cpp_ghost_fixture, cpp_thread_ghost_propose_and_collapse);
    FOSSIL_TEST_ADD(cpp_ghost_fixture, cpp_thread_ghost_queue_and_schedule);
    FOSSIL_TEST_ADD(cpp_ghost_fixture, cpp_thread_ghost_finished_and_dispose);

    FOSSIL_TEST_REGISTER(cpp_ghost_tests);
} // end of tests
