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
#include "fossil/threads/ghost.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ---------- Simple in-memory ledger ---------- */
typedef struct fossil__ledger_entry {
    char ghost_id[64];
    void *state_snapshot;
} fossil__ledger_entry_t;

#define FOSSIL__LEDGER_MAX 1024
static fossil__ledger_entry_t fossil__ledger[FOSSIL__LEDGER_MAX];
static size_t fossil__ledger_count = 0;

int fossil_threads_ghost_init(void) {
    fossil__ledger_count = 0;
    memset(fossil__ledger, 0, sizeof(fossil__ledger));
    return FOSSIL_GHOST_OK;
}

int fossil_threads_ghost_create(
    fossil_threads_ghost_t *ghost,
    const char *id,
    fossil_threads_ghost_func func,
    void *arg
) {
    if (!ghost || !id || !func) return FOSSIL_GHOST_EINVAL;

    memset(ghost, 0, sizeof(*ghost));
    strncpy(ghost->id, id, sizeof(ghost->id) - 1);
    ghost->func = func;
    ghost->arg  = arg;
    ghost->finished = 0;

    /* Record initial state in ledger */
    if (fossil__ledger_count < FOSSIL__LEDGER_MAX) {
        strncpy(fossil__ledger[fossil__ledger_count].ghost_id, id, 64);
        fossil__ledger[fossil__ledger_count].state_snapshot = NULL;
        fossil__ledger_count++;
    }

    return FOSSIL_GHOST_OK;
}

/* Execute the ghost thread step */
int fossil_threads_ghost_step(fossil_threads_ghost_t *ghost) {
    if (!ghost || ghost->finished) return FOSSIL_GHOST_EINVAL;

    /* Call the user function for the step */
    if (ghost->func) ghost->func(ghost->arg);

    /* Record post-step state to ledger */
    if (fossil__ledger_count < FOSSIL__LEDGER_MAX) {
        strncpy(fossil__ledger[fossil__ledger_count].ghost_id, ghost->id, 64);
        fossil__ledger[fossil__ledger_count].state_snapshot = ghost->state;
        fossil__ledger_count++;
    }

    ghost->finished = 1; /* simple model: one step per ghost thread */
    return FOSSIL_GHOST_OK;
}

int fossil_threads_ghost_state(fossil_threads_ghost_t *ghost, void **out_state) {
    if (!ghost || !out_state) return FOSSIL_GHOST_EINVAL;
    *out_state = ghost->state;
    return FOSSIL_GHOST_OK;
}

int fossil_threads_ghost_finished(fossil_threads_ghost_t *ghost) {
    if (!ghost) return 1;
    return ghost->finished;
}

void fossil_threads_ghost_dispose(fossil_threads_ghost_t *ghost) {
    if (!ghost) return;
    ghost->state = NULL;
    ghost->func = NULL;
    ghost->arg = NULL;
    ghost->finished = 1;
}
