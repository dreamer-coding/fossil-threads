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

/* ---------- Ledger (blockchain-simulated) ---------- */
typedef struct fossil__ledger_entry {
    char ghost_id[64];
    void *state_snapshot;
    size_t step_index;
} fossil__ledger_entry_t;

#define FOSSIL__LEDGER_MAX 4096
static fossil__ledger_entry_t fossil__ledger[FOSSIL__LEDGER_MAX];
static size_t fossil__ledger_count = 0;

/* ---------- Scheduler queue ---------- */
#define FOSSIL__QUEUE_MAX 256
static fossil_threads_ghost_t* fossil__queue[FOSSIL__QUEUE_MAX];
static size_t fossil__queue_count = 0;

/* ---------- Initialization ---------- */
int fossil_threads_ghost_init(void) {
    fossil__ledger_count = 0;
    fossil__queue_count = 0;
    memset(fossil__ledger, 0, sizeof(fossil__ledger));
    memset(fossil__queue, 0, sizeof(fossil__queue));
    return FOSSIL_GHOST_OK;
}

/* ---------- Create Ghost ---------- */
int fossil_threads_ghost_create(
    fossil_threads_ghost_t *ghost,
    const char *id,
    fossil_threads_ghost_func func,
    void *arg
) {
    if (!ghost || !id || !func) return FOSSIL_GHOST_EINVAL;
    memset(ghost, 0, sizeof(*ghost));
    strncpy(ghost->id, id, sizeof(ghost->id)-1);
    ghost->func = func;
    ghost->arg = arg;
    ghost->finished = 0;
    ghost->step_index = 0;

    /* Ledger initial state */
    if (fossil__ledger_count < FOSSIL__LEDGER_MAX) {
        strncpy(fossil__ledger[fossil__ledger_count].ghost_id, id, 64);
        fossil__ledger[fossil__ledger_count].state_snapshot = NULL;
        fossil__ledger[fossil__ledger_count].step_index = 0;
        fossil__ledger_count++;
    }

    return FOSSIL_GHOST_OK;
}

/* ---------- Queue management ---------- */
int fossil_threads_ghost_queue_add(fossil_threads_ghost_t *ghost) {
    if (!ghost || fossil__queue_count >= FOSSIL__QUEUE_MAX) return FOSSIL_GHOST_EBUSY;
    fossil__queue[fossil__queue_count++] = ghost;
    return FOSSIL_GHOST_OK;
}

/* ---------- Execute a single step ---------- */
int fossil_threads_ghost_step(fossil_threads_ghost_t *ghost) {
    if (!ghost || ghost->finished) return FOSSIL_GHOST_EINVAL;

    void *new_state = NULL;
    ghost->func(ghost->arg, &new_state);

    /* Collapse candidates if present */
    if (ghost->candidate_count > 0) {
        /* Simplest: pick first candidate */
        new_state = ghost->candidates[0];
        ghost->candidate_count = 0;
        free(ghost->candidates);
        ghost->candidates = NULL;
    }

    ghost->state = new_state;
    ghost->step_index++;

    /* Record in ledger */
    if (fossil__ledger_count < FOSSIL__LEDGER_MAX) {
        strncpy(fossil__ledger[fossil__ledger_count].ghost_id, ghost->id, 64);
        fossil__ledger[fossil__ledger_count].state_snapshot = ghost->state;
        fossil__ledger[fossil__ledger_count].step_index = ghost->step_index;
        fossil__ledger_count++;
    }

    return FOSSIL_GHOST_OK;
}

/* ---------- Scheduler: one round ---------- */
int fossil_threads_ghost_schedule(void) {
    if (fossil__queue_count == 0) return FOSSIL_GHOST_EINVAL;
    for (size_t i=0; i<fossil__queue_count; i++) {
        fossil_threads_ghost_t *ghost = fossil__queue[i];
        if (!ghost->finished) fossil_threads_ghost_step(ghost);
    }
    return FOSSIL_GHOST_OK;
}

/* ---------- Query / Dispose ---------- */
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
    if (ghost->candidates) free(ghost->candidates);
    ghost->state = NULL;
    ghost->func = NULL;
    ghost->arg = NULL;
    ghost->finished = 1;
}
