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
#ifndef FOSSIL_THREADS_GHOST_H
#define FOSSIL_THREADS_GHOST_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_WIN32) && defined(FOSSIL_THREADS_BUILD_DLL)
#  define FOSSIL_THREADS_API __declspec(dllexport)
#elif defined(_WIN32) && defined(FOSSIL_THREADS_USE_DLL)
#  define FOSSIL_THREADS_API __declspec(dllimport)
#else
#  define FOSSIL_THREADS_API
#endif

/* ---------- Error codes ---------- */
enum {
    FOSSIL_GHOST_OK       = 0,
    FOSSIL_GHOST_EINVAL   = 22,
    FOSSIL_GHOST_ENOMEM   = 12,
    FOSSIL_GHOST_EBUSY    = 16,
    FOSSIL_GHOST_ENOTSUP  = 95,
    FOSSIL_GHOST_EINTERNAL= 199
};

/* ---------- Candidate (speculative) ---------- */
/* Candidate payload is opaque; 'tag' is a short string used for deterministic hashing and audit. */
typedef struct fossil_threads_ghost_candidate {
    void *data;       /* pointer to candidate state (opaque) */
    size_t size;      /* size of data or 0 if unknown */
    char tag[64];     /* small tag/id describing candidate (must be NUL-terminated) */
} fossil_threads_ghost_candidate_t;

/* Ghost thread step function:
 *   - called when executing a non-speculative step;
 *   - can also be NULL for ghosts that only operate via proposals.
 * It receives 'arg' and returns a pointer to a new state via out_state (caller-managed). */
typedef void (*fossil_threads_ghost_func)(void *arg, void **out_state);

/* Ghost thread handle */
typedef struct fossil_threads_ghost {
    char id[64];                     /* Unique ID */
    void *state;                     /* Current (collapsed) state */
    fossil_threads_ghost_candidate_t *candidates; /* last-proposed candidate array (owned by caller) */
    size_t candidate_count;
    fossil_threads_ghost_func func;  /* Step function (non-speculative) */
    void *arg;
    int finished;
    int step_index;
} fossil_threads_ghost_t;

/* ---------- Lifecycle ---------- */

// *****************************************************************************
// Function prototypes
// *****************************************************************************

/**
 * @brief Initialize the Ghost Thread system.
 *
 * Sets up internal data structures, including the ledger and scheduling queue,
 * preparing the system for creating and running ghost threads.
 *
 * @return FOSSIL_GHOST_OK on success, or an error code on failure.
 */
int fossil_threads_ghost_init(void);

/**
 * @brief Create a new ghost thread.
 *
 * Initializes a ghost thread structure with a unique ID, entry function, and user argument.
 * Records the initial state in the ledger for tracking and auditing purposes.
 *
 * @param ghost Pointer to a pre-allocated ghost thread structure.
 * @param id Unique string identifier for the ghost thread.
 * @param func Function to be called for each step of the ghost thread.
 * @param arg User-provided argument passed to the step function.
 * @return FOSSIL_GHOST_OK on success, or an error code on failure.
 */
int fossil_threads_ghost_create(
    fossil_threads_ghost_t *ghost,
    const char *id,
    fossil_threads_ghost_func func,
    void *arg
);

/* Propose N candidates for this ghost's next step.
   - candidates: pointer to an array (length count) of fossil_threads_ghost_candidate_t
   - Ownership: library stores the pointer only until collapse; caller must keep candidate array & payload valid
     until fossil_threads_ghost_collapse_by_consensus() has been called for that proposal.
*/
int fossil_threads_ghost_propose_candidates(
    fossil_threads_ghost_t *ghost,
    fossil_threads_ghost_candidate_t *candidates,
    size_t count
);

/* Deterministically collapse the most-recent proposal for a ghost using ledger-derived consensus.
   - The function selects one candidate and installs it into ghost->state.
   - Returns index of selected candidate on success (>=0), or negative error code.
*/
int fossil_threads_ghost_collapse_by_consensus(fossil_threads_ghost_t *ghost);

/**
 * @brief Execute a single step of a ghost thread.
 *
 * Calls the ghost thread's step function, updates its state, and records the
 * resulting state in the ledger. If the thread has speculative candidates,
 * they are collapsed to a single state.
 *
 * @param ghost Pointer to the ghost thread to execute.
 * @return FOSSIL_GHOST_OK on success, or an error code on failure.
 */
int fossil_threads_ghost_step(fossil_threads_ghost_t *ghost);

/**
 * @brief Add a ghost thread to the scheduling queue.
 *
 * The ghost thread will be executed in round-robin order when
 * fossil_threads_ghost_schedule() is called.
 *
 * @param ghost Pointer to the ghost thread to enqueue.
 * @return FOSSIL_GHOST_OK on success, or an error code if the queue is full or invalid.
 */
int fossil_threads_ghost_queue_add(fossil_threads_ghost_t *ghost);

/**
 * @brief Execute one scheduling round for all queued ghost threads.
 *
 * Iterates through the queue and executes one step for each ghost thread that is not finished.
 * This provides cooperative scheduling and deterministic ordering of ghost thread execution.
 *
 * @return FOSSIL_GHOST_OK on success, or an error code on failure.
 */
int fossil_threads_ghost_schedule(void);

/**
 * @brief Retrieve the current state of a ghost thread.
 *
 * Provides access to the ghost thread's latest state after execution steps.
 *
 * @param ghost Pointer to the ghost thread.
 * @param out_state Output parameter to receive the current state pointer.
 * @return FOSSIL_GHOST_OK on success, or an error code on failure.
 */
int fossil_threads_ghost_state(fossil_threads_ghost_t *ghost, void **out_state);

/**
 * @brief Check if a ghost thread has finished execution.
 *
 * @param ghost Pointer to the ghost thread.
 * @return 1 if finished, 0 otherwise.
 */
int fossil_threads_ghost_finished(fossil_threads_ghost_t *ghost);

/**
 * @brief Dispose of a ghost thread and free associated resources.
 *
 * Clears candidate states, releases memory, and marks the ghost thread as finished.
 *
 * @param ghost Pointer to the ghost thread to dispose.
 */
void fossil_threads_ghost_dispose(fossil_threads_ghost_t *ghost);

#ifdef __cplusplus
}
#include <stdexcept>
#include <vector>
#include <string>

namespace fossil {

namespace threads {

    class Ghost {
    public:
        /**
         * @brief Construct a new Ghost thread.
         *
         * Initializes a ghost thread with the specified ID, step function, and user argument.
         * Throws std::runtime_error if creation fails.
         *
         * @param id   Unique identifier for the ghost thread.
         * @param func Step function for the ghost thread.
         * @param arg  User argument passed to the step function.
         */
        Ghost(const std::string& id, fossil_threads_ghost_func func, void* arg)
            : ghost_{}
        {
            int rc = fossil_threads_ghost_create(&ghost_, id.c_str(), func, arg);
            if (rc != FOSSIL_GHOST_OK)
                throw std::runtime_error("Failed to create ghost thread");
        }

        /**
         * @brief Destructor. Disposes of the ghost thread and releases resources.
         */
        ~Ghost() {
            fossil_threads_ghost_dispose(&ghost_);
        }

        /**
         * @brief Propose a set of candidate states for the next step.
         *
         * @param candidates Vector of candidate states to propose.
         * @return FOSSIL_GHOST_OK on success, or an error code on failure.
         */
        int propose_candidates(std::vector<fossil_threads_ghost_candidate_t>& candidates) {
            return fossil_threads_ghost_propose_candidates(
                &ghost_, candidates.data(), candidates.size());
        }

        /**
         * @brief Collapse the most recent proposal by consensus.
         *
         * Selects one candidate deterministically and installs it as the new state.
         * @return Index of the selected candidate on success (>=0), or negative error code.
         */
        int collapse_by_consensus() {
            return fossil_threads_ghost_collapse_by_consensus(&ghost_);
        }

        /**
         * @brief Execute a single step of the ghost thread.
         *
         * Calls the step function and updates the ghost's state.
         * @return FOSSIL_GHOST_OK on success, or an error code on failure.
         */
        int step() {
            return fossil_threads_ghost_step(&ghost_);
        }

        /**
         * @brief Add this ghost thread to the scheduling queue.
         *
         * @return FOSSIL_GHOST_OK on success, or an error code if the queue is full or invalid.
         */
        int queue_add() {
            return fossil_threads_ghost_queue_add(&ghost_);
        }

        /**
         * @brief Execute one scheduling round for all queued ghost threads.
         *
         * Static method to step all queued ghost threads in round-robin order.
         * @return FOSSIL_GHOST_OK on success, or an error code on failure.
         */
        static int schedule() {
            return fossil_threads_ghost_schedule();
        }

        /**
         * @brief Retrieve the current state of the ghost thread.
         *
         * @return Pointer to the current state.
         * @throws std::runtime_error if retrieval fails.
         */
        void* state() {
            void* out = nullptr;
            int rc = fossil_threads_ghost_state(&ghost_, &out);
            if (rc != FOSSIL_GHOST_OK)
                throw std::runtime_error("Failed to get ghost state");
            return out;
        }

        /**
         * @brief Check if the ghost thread has finished execution.
         *
         * @return true if finished, false otherwise.
         */
        bool finished() {
            return fossil_threads_ghost_finished(&ghost_) != 0;
        }

        /**
         * @brief Get the underlying native ghost thread handle.
         *
         * @return Pointer to the native fossil_threads_ghost_t structure.
         */
        fossil_threads_ghost_t* native_handle() { return &ghost_; }

    private:
        fossil_threads_ghost_t ghost_; /**< Underlying C ghost thread structure. */
    };

} // namespace threads

} // namespace fossil

#endif

#endif /* FOSSIL_THREADS_GHOST_H */
