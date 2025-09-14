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
#include "fossil/threads/ghost.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <stddef.h>  /* for size_t */

#if defined(_WIN32) || defined(_WIN64)
    #include <BaseTsd.h>
    typedef SSIZE_T ssize_t;
#else
    #include <sys/types.h>
#endif

/* ---------- Simple ledger entry ---------- */
typedef struct fossil__ledger_entry {
    char ghost_id[64];
    size_t step_index;
    /* If a proposal was made this entry records: */
    int proposal_present;
    size_t proposal_candidate_count;
    /* store candidate tags for audit (copy of tags) */
    char (*proposal_tags)[64]; /* pointer to an allocated array of tags (proposal_candidate_count x 64) */
    size_t chosen_index; /* set when collapsed (SIZE_MAX if not yet chosen) */
    void *state_snapshot; /* pointer to collapsed state (opaque) */
} fossil__ledger_entry_t;

#define FOSSIL__LEDGER_MAX 8192
static fossil__ledger_entry_t fossil__ledger[FOSSIL__LEDGER_MAX];
static size_t fossil__ledger_count = 0;

/* Scheduler queue (unchanged) */
#define FOSSIL__QUEUE_MAX 512
static fossil_threads_ghost_t* fossil__queue[FOSSIL__QUEUE_MAX];
static size_t fossil__queue_count = 0;

/* ---------- Utilities ---------- */

/* FNV-1a 64-bit (deterministic simple hash) */
static uint64_t fossil__fnv1a64(const void *data, size_t len, uint64_t seed) {
    const uint8_t *p = (const uint8_t*)data;
    uint64_t h = 14695981039346656037ULL ^ seed;
    for (size_t i = 0; i < len; ++i) {
        h ^= (uint64_t)p[i];
        h *= 1099511628211ULL;
    }
    return h;
}

/* Hash mix helper for strings */
static uint64_t fossil__hash_str_mix(uint64_t acc, const char *s) {
    return fossil__fnv1a64(s, strlen(s), acc);
}

/* Add ledger entry helper */
static fossil__ledger_entry_t* fossil__ledger_add_entry(void) {
    if (fossil__ledger_count >= FOSSIL__LEDGER_MAX) return NULL;
    fossil__ledger_entry_t *e = &fossil__ledger[fossil__ledger_count++];
    memset(e, 0, sizeof(*e));
    e->chosen_index = SIZE_MAX;
    return e;
}

/* ---------- Initialization / create ---------- */

int fossil_threads_ghost_init(void) {
    fossil__ledger_count = 0;
    fossil__queue_count = 0;
    memset(fossil__ledger, 0, sizeof(fossil__ledger));
    memset(fossil__queue, 0, sizeof(fossil__queue));
    return FOSSIL_GHOST_OK;
}

int fossil_threads_ghost_create(
    fossil_threads_ghost_t *ghost,
    const char *id,
    fossil_threads_ghost_func func,
    void *arg
) {
    if (!ghost || !id) return FOSSIL_GHOST_EINVAL;
    memset(ghost, 0, sizeof(*ghost));
    strncpy(ghost->id, id, sizeof(ghost->id)-1);
    ghost->func = func;
    ghost->arg = arg;
    ghost->finished = 0;
    ghost->step_index = 0;
    ghost->candidates = NULL;
    ghost->candidate_count = 0;

    /* create initial ledger entry */
    fossil__ledger_entry_t *e = fossil__ledger_add_entry();
    if (!e) return FOSSIL_GHOST_EINTERNAL;
    strncpy(e->ghost_id, ghost->id, 64);
    e->step_index = 0;
    e->proposal_present = 0;
    e->proposal_candidate_count = 0;
    e->proposal_tags = NULL;
    e->chosen_index = SIZE_MAX;
    e->state_snapshot = NULL;
    return FOSSIL_GHOST_OK;
}

/* ---------- Propose candidates ---------- */

int fossil_threads_ghost_propose_candidates(
    fossil_threads_ghost_t *ghost,
    fossil_threads_ghost_candidate_t *candidates,
    size_t count
) {
    if (!ghost || !candidates || count == 0) return FOSSIL_GHOST_EINVAL;

    /* attach candidate array pointer to ghost (caller must keep it until collapse) */
    ghost->candidates = candidates;
    ghost->candidate_count = count;

    /* record proposal in ledger (as a new entry) */
    fossil__ledger_entry_t *e = fossil__ledger_add_entry();
    if (!e) return FOSSIL_GHOST_EINTERNAL;

    strncpy(e->ghost_id, ghost->id, 64);
    e->step_index = ++ghost->step_index; /* increment step index to record a proposal step */
    e->proposal_present = 1;
    e->proposal_candidate_count = count;
    e->proposal_tags = (char (*)[64])malloc(count * 64);
    if (!e->proposal_tags) { /* rollback minimal */
        e->proposal_present = 0;
        return FOSSIL_GHOST_ENOMEM;
    }
    for (size_t i = 0; i < count; ++i) {
        strncpy(e->proposal_tags[i], candidates[i].tag, 63);
        e->proposal_tags[i][63] = '\0';
    }
    e->chosen_index = SIZE_MAX;
    e->state_snapshot = NULL;
    return FOSSIL_GHOST_OK;
}

/* ---------- Deterministic collapse (consensus) ---------- */
/* Select candidate index deterministically from ledger + tags using FNV-1a hashing. */
int fossil_threads_ghost_collapse_by_consensus(fossil_threads_ghost_t *ghost) {
    if (!ghost || ghost->candidate_count == 0) return FOSSIL_GHOST_EINVAL;

    /* Find the most recent ledger entry for this ghost with a proposal. */
    ssize_t found = -1;
    for (ssize_t i = (ssize_t)fossil__ledger_count - 1; i >= 0; --i) {
        if (strncmp(fossil__ledger[i].ghost_id, ghost->id, 64) == 0 &&
            fossil__ledger[i].proposal_present) {
            found = i;
            break;
        }
    }
    if (found < 0) return FOSSIL_GHOST_EINVAL;
    fossil__ledger_entry_t *entry = &fossil__ledger[found];

    /* Compute a deterministic seed hash:
       mix: global ledger count, ghost id, step_index, all proposal tags */
    uint64_t seed = 0xC0FFEE1234567890ULL; /* constant start */
    seed = fossil__fnv1a64(&fossil__ledger_count, sizeof(fossil__ledger_count), seed);
    seed = fossil__hash_str_mix(seed, entry->ghost_id);
    seed = fossil__fnv1a64(&entry->step_index, sizeof(entry->step_index), seed);

    for (size_t i = 0; i < entry->proposal_candidate_count; ++i) {
        seed = fossil__hash_str_mix(seed, entry->proposal_tags[i]);
    }

    /* Choose index using seed */
    size_t chosen = (size_t)(seed % (uint64_t)ghost->candidate_count);

    /* Install chosen candidate as collapsed state */
    fossil_threads_ghost_candidate_t *c = &ghost->candidates[chosen];
    ghost->state = c->data;

    /* mark ledger chosen index and store pointer to state (opaque) */
    entry->chosen_index = chosen;
    entry->state_snapshot = ghost->state;

    /* clear ghost's proposal pointers (caller payload remains owned by caller) */
    ghost->candidates = NULL;
    ghost->candidate_count = 0;

    return (int)chosen;
}

/* ---------- Non-speculative step ---------- */
int fossil_threads_ghost_step(fossil_threads_ghost_t *ghost) {
    if (!ghost || ghost->finished) return FOSSIL_GHOST_EINVAL;
    void *new_state = NULL;
    if (ghost->func) ghost->func(ghost->arg, &new_state);
    ghost->state = new_state;
    ghost->step_index++;

    /* ledger record */
    fossil__ledger_entry_t *e = fossil__ledger_add_entry();
    if (!e) return FOSSIL_GHOST_EINTERNAL;
    strncpy(e->ghost_id, ghost->id, 64);
    e->step_index = ghost->step_index;
    e->proposal_present = 0;
    e->proposal_candidate_count = 0;
    e->proposal_tags = NULL;
    e->chosen_index = SIZE_MAX;
    e->state_snapshot = ghost->state;
    return FOSSIL_GHOST_OK;
}

/* ---------- Scheduler ---------- */
int fossil_threads_ghost_queue_add(fossil_threads_ghost_t *ghost) {
    if (!ghost || fossil__queue_count >= FOSSIL__QUEUE_MAX) return FOSSIL_GHOST_EBUSY;
    fossil__queue[fossil__queue_count++] = ghost;
    return FOSSIL_GHOST_OK;
}

int fossil_threads_ghost_schedule(void) {
    if (fossil__queue_count == 0) return FOSSIL_GHOST_EINVAL;
    for (size_t i = 0; i < fossil__queue_count; ++i) {
        fossil_threads_ghost_t *g = fossil__queue[i];
        if (g->finished) continue;
        /* If candidate proposal present, collapse deterministically; otherwise call step(). */
        if (g->candidate_count > 0) {
            fossil_threads_ghost_collapse_by_consensus(g);
        } else if (g->func) {
            fossil_threads_ghost_step(g);
        }
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
    /* free any ledger-copied tags for all entries referencing this ghost: (simple sweep) */
    for (size_t i = 0; i < fossil__ledger_count; ++i) {
        if (strncmp(fossil__ledger[i].ghost_id, ghost->id, 64) == 0 && fossil__ledger[i].proposal_tags) {
            free(fossil__ledger[i].proposal_tags);
            fossil__ledger[i].proposal_tags = NULL;
        }
    }
    ghost->state = NULL;
    ghost->func = NULL;
    ghost->arg = NULL;
    ghost->finished = 1;
    ghost->candidate_count = 0;
    ghost->candidates = NULL;
}
