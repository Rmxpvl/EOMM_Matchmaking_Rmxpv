/*
 * match_history.c
 *
 * EOMM Match History — persistent in-memory store for completed matches.
 *
 * Why store matches?
 *   The core simulation works with Player* pointers that change every game.
 *   To replay, query, or export any match after the simulation we need a
 *   snapshot that is independent of those live pointers.  StoredMatch uses
 *   player IDs (integers) instead of pointers, so every record remains valid
 *   for the entire lifetime of the program and can be serialised to disk.
 *
 * Memory safety rules followed throughout this file:
 *   • Every malloc/realloc is checked; failure returns early without leaking.
 *   • history_free() zeroes count and capacity before returning so the caller
 *     cannot accidentally use a dangling pointer.
 *   • history_export_json() treats a NULL history or filename as a no-op.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/eomm_system.h"

/* =========================================================
 * history_create
 * ========================================================= */

/*
 * Allocate a new MatchHistory with the given initial_capacity.
 *
 * Why pre-allocate?  Calling realloc on every single match would be O(n)
 * individual allocations.  Starting with a reasonable capacity means realloc
 * is only called when the array doubles — amortised O(1) per insert.
 *
 * Returns NULL on any allocation failure so the caller can decide whether
 * to abort or continue without history.
 */
MatchHistory *history_create(int initial_capacity) {
    if (initial_capacity <= 0) initial_capacity = 64;

    MatchHistory *h = (MatchHistory *)malloc(sizeof(MatchHistory));
    if (!h) return NULL;

    h->matches = (StoredMatch *)malloc((size_t)initial_capacity
                                       * sizeof(StoredMatch));
    if (!h->matches) {
        free(h);
        return NULL;
    }

    h->count    = 0;
    h->capacity = initial_capacity;
    return h;
}

/* =========================================================
 * history_add_match
 * ========================================================= */

/*
 * Append a snapshot of a completed match to the history.
 *
 * Why copy IDs rather than pointers?
 *   Player pointers become invalid if the player array is ever reallocated or
 *   freed.  Storing integer IDs makes every StoredMatch self-contained.
 *
 * If the internal array is full the capacity is doubled via realloc.
 * On realloc failure the function returns silently — the match is lost but
 * the existing history is unaffected (no data corruption).
 */
void history_add_match(MatchHistory *h, const Match *m,
                       int match_id, int timestamp) {
    if (!h || !m) return;

    /* Grow if needed */
    if (h->count >= h->capacity) {
        int new_cap = h->capacity * 2;
        StoredMatch *tmp = (StoredMatch *)realloc(
            h->matches, (size_t)new_cap * sizeof(StoredMatch));
        if (!tmp) return; /* allocation failed; skip this match silently */
        h->matches  = tmp;
        h->capacity = new_cap;
    }

    StoredMatch *s = &h->matches[h->count];
    s->match_id  = match_id;
    s->timestamp = timestamp;
    s->winner    = m->winner;

    s->team_a_power  = 0.0f;
    s->team_b_power  = 0.0f;
    s->troll_count_a = 0;
    s->troll_count_b = 0;

    for (int i = 0; i < TEAM_SIZE; i++) {
        /* team_a */
        if (m->team_a[i] != NULL) {
            s->team_a_ids[i] = m->team_a[i]->id;
            s->team_a_power += m->team_a[i]->visible_mmr
                               * m->team_a[i]->hidden_factor;
            if (m->team_a[i]->is_troll_pick) s->troll_count_a++;
        } else {
            s->team_a_ids[i] = -1; /* sentinel: slot was empty */
        }

        /* team_b */
        if (m->team_b[i] != NULL) {
            s->team_b_ids[i] = m->team_b[i]->id;
            s->team_b_power += m->team_b[i]->visible_mmr
                               * m->team_b[i]->hidden_factor;
            if (m->team_b[i]->is_troll_pick) s->troll_count_b++;
        } else {
            s->team_b_ids[i] = -1;
        }
    }

    h->count++;
}

/* =========================================================
 * history_export_json
 * ========================================================= */

/*
 * Write the full match history to a JSON file.
 *
 * Output format (one object per match):
 * {
 *   "matches": [
 *     {
 *       "match_id": 1,
 *       "timestamp": 1710000000,
 *       "winner": 1,
 *       "team_a_power": 1234.5,
 *       "team_b_power": 1300.0,
 *       "troll_count_a": 0,
 *       "troll_count_b": 1,
 *       "team_a": [
 *         {"id": 3, "name": "Player0004"},
 *         ...
 *       ],
 *       "team_b": [
 *         {"id": 7, "name": "Player0008"},
 *         ...
 *       ]
 *     },
 *     ...
 *   ]
 * }
 *
 * player names are looked up by ID; if no matching player is found the name
 * is emitted as an empty string.
 *
 * Why JSON?  It is the simplest format that both a Python backend and a
 * JavaScript frontend can consume without a binary parser.
 */
void history_export_json(const MatchHistory *h, const Player *players,
                         int n_players, const char *filename) {
    if (!h || !filename) return;

    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "[history] Could not open '%s' for writing.\n", filename);
        return;
    }

    /*
     * Build a direct-indexed name table so each player can be looked up in
     * O(1) by ID instead of scanning the whole player array per slot.
     *
     * Player IDs are sequential integers in [0, n_players-1], so we
     * allocate an array of that size and populate it in one pass.
     * Any ID outside this range falls back to an empty string.
     */
    char (*name_table)[PLAYER_NAME_LEN] = NULL;
    if (players != NULL && n_players > 0) {
        name_table = (char (*)[PLAYER_NAME_LEN])
            calloc((size_t)n_players, PLAYER_NAME_LEN);
        if (name_table) {
            for (int k = 0; k < n_players; k++) {
                if (players[k].id >= 0 && players[k].id < n_players) {
                    strncpy(name_table[players[k].id], players[k].name,
                            PLAYER_NAME_LEN - 1);
                    name_table[players[k].id][PLAYER_NAME_LEN - 1] = '\0';
                }
            }
        }
    }

    fprintf(fp, "{\n  \"matches\": [\n");

    for (int i = 0; i < h->count; i++) {
        const StoredMatch *s = &h->matches[i];
        if (i > 0) fprintf(fp, ",\n");

        fprintf(fp, "    {\n");
        fprintf(fp, "      \"match_id\": %d,\n",    s->match_id);
        fprintf(fp, "      \"timestamp\": %d,\n",   s->timestamp);
        fprintf(fp, "      \"winner\": %d,\n",      s->winner);
        fprintf(fp, "      \"team_a_power\": %.2f,\n", s->team_a_power);
        fprintf(fp, "      \"team_b_power\": %.2f,\n", s->team_b_power);
        fprintf(fp, "      \"troll_count_a\": %d,\n",  s->troll_count_a);
        fprintf(fp, "      \"troll_count_b\": %d,\n",  s->troll_count_b);

        /* Look up a player name by ID using the pre-built table (O(1)). */
        #define GET_NAME(pid) \
            ((name_table != NULL && (pid) >= 0 && (pid) < n_players) \
             ? name_table[(pid)] : "")

        /* team_a */
        fprintf(fp, "      \"team_a\": [\n");
        for (int t = 0; t < TEAM_SIZE; t++) {
            fprintf(fp, "        {\"id\": %d, \"name\": \"%s\"}%s\n",
                    s->team_a_ids[t], GET_NAME(s->team_a_ids[t]),
                    (t < TEAM_SIZE - 1) ? "," : "");
        }
        fprintf(fp, "      ],\n");

        /* team_b */
        fprintf(fp, "      \"team_b\": [\n");
        for (int t = 0; t < TEAM_SIZE; t++) {
            fprintf(fp, "        {\"id\": %d, \"name\": \"%s\"}%s\n",
                    s->team_b_ids[t], GET_NAME(s->team_b_ids[t]),
                    (t < TEAM_SIZE - 1) ? "," : "");
        }
        fprintf(fp, "      ]\n");

        #undef GET_NAME

        fprintf(fp, "    }");
    }

    fprintf(fp, "\n  ]\n}\n");
    fclose(fp);

    free(name_table);

    printf("[history] Exported %d matches to '%s'.\n", h->count, filename);
}

/* =========================================================
 * history_free
 * ========================================================= */

/*
 * Release all memory owned by the history.
 *
 * After this call h->matches is NULL, h->count and h->capacity are 0.
 * The MatchHistory struct itself is also freed — the caller must not
 * dereference h afterwards.
 */
void history_free(MatchHistory *h) {
    if (!h) return;
    free(h->matches);
    h->matches  = NULL;
    h->count    = 0;
    h->capacity = 0;
    free(h);
}
