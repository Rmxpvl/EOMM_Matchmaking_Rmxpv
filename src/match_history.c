/*
 * match_history.c
 *
 * Match history tracking and JSON export for EOMM simulation.
 * Uses standard C with no external dependencies.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/eomm_system.h"

/* Create a new match history with initial capacity */
MatchHistory *history_create(int initial_capacity) {
    if (initial_capacity < 1) initial_capacity = 10;
    
    MatchHistory *h = (MatchHistory *)malloc(sizeof(MatchHistory));
    if (!h) return NULL;
    
    h->matches = (StoredMatch *)malloc((size_t)initial_capacity * sizeof(StoredMatch));
    if (!h->matches) {
        free(h);
        return NULL;
    }
    
    h->count = 0;
    h->capacity = initial_capacity;
    return h;
}

/* Add a match to history (with automatic reallocation if needed) */
void history_add_match(MatchHistory *h, const Match *m,
                       int match_id, int timestamp) {
    if (!h || !m) return;
    
    /* Grow array if needed */
    if (h->count >= h->capacity) {
        int new_cap = h->capacity * 2;
        StoredMatch *new_matches = (StoredMatch *)realloc(h->matches, 
                                                           (size_t)new_cap * sizeof(StoredMatch));
        if (!new_matches) return;
        h->matches = new_matches;
        h->capacity = new_cap;
    }
    
    /* Store match snapshot */
    StoredMatch *stored = &h->matches[h->count++];
    stored->match_id = match_id;
    stored->timestamp = timestamp;
    stored->winner = m->winner;
    
    /* Store player IDs (not pointers) */
    for (int i = 0; i < TEAM_SIZE; i++) {
        stored->team_a_ids[i] = (m->team_a[i] != NULL) ? m->team_a[i]->id : -1;
        stored->team_b_ids[i] = (m->team_b[i] != NULL) ? m->team_b[i]->id : -1;
    }
    
    /* Calculate team power */
    stored->team_a_power = 0.0f;
    stored->team_b_power = 0.0f;
    stored->troll_count_a = 0;
    stored->troll_count_b = 0;
    
    for (int i = 0; i < TEAM_SIZE; i++) {
        if (m->team_a[i]) {
            stored->team_a_power += effective_mmr(m->team_a[i]) * calculate_actual_winrate(m->team_a[i]);
            if (m->team_a[i]->is_troll_pick) stored->troll_count_a++;
        }
        if (m->team_b[i]) {
            stored->team_b_power += effective_mmr(m->team_b[i]) * calculate_actual_winrate(m->team_b[i]);
            if (m->team_b[i]->is_troll_pick) stored->troll_count_b++;
        }
    }
}

/* Export entire history as JSON */
void history_export_json(const MatchHistory *h, const Player *players,
                         int n_players, const char *filename) {
    if (!h || !players || !filename) return;
    
    FILE *f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "{\n  \"matches\": [\n");
    
    for (int i = 0; i < h->count; i++) {
        const StoredMatch *m = &h->matches[i];
        
        fprintf(f, "    {\n");
        fprintf(f, "      \"match_id\": %d,\n", m->match_id);
        fprintf(f, "      \"timestamp\": %d,\n", m->timestamp);
        fprintf(f, "      \"winner\": %d,\n", m->winner);
        fprintf(f, "      \"team_a_power\": %.1f,\n", m->team_a_power);
        fprintf(f, "      \"team_b_power\": %.1f,\n", m->team_b_power);
        fprintf(f, "      \"troll_count_a\": %d,\n", m->troll_count_a);
        fprintf(f, "      \"troll_count_b\": %d,\n", m->troll_count_b);
        
        fprintf(f, "      \"team_a\": [\n");
        for (int j = 0; j < TEAM_SIZE; j++) {
            int pid = m->team_a_ids[j];
            fprintf(f, "        {\"id\": %d, \"name\": \"", pid);
            
            if (pid >= 0 && pid < n_players) {
                fprintf(f, "%s", players[pid].name);
            } else {
                fprintf(f, "Unknown");
            }
            
            fprintf(f, "\"}%s\n", j < TEAM_SIZE - 1 ? "," : "");
        }
        fprintf(f, "      ],\n");
        
        fprintf(f, "      \"team_b\": [\n");
        for (int j = 0; j < TEAM_SIZE; j++) {
            int pid = m->team_b_ids[j];
            fprintf(f, "        {\"id\": %d, \"name\": \"", pid);
            
            if (pid >= 0 && pid < n_players) {
                fprintf(f, "%s", players[pid].name);
            } else {
                fprintf(f, "Unknown");
            }
            
            fprintf(f, "\"}%s\n", j < TEAM_SIZE - 1 ? "," : "");
        }
        fprintf(f, "      ]\n");
        
        fprintf(f, "    }%s\n", i < h->count - 1 ? "," : "");
    }
    
    fprintf(f, "  ]\n");
    fprintf(f, "}\n");
    fclose(f);
}

/* Free all match history memory */
void history_free(MatchHistory *h) {
    if (!h) return;
    if (h->matches) free(h->matches);
    h->count = 0;
    h->capacity = 0;
    free(h);
}