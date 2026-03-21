#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TOTAL_PLAYERS 100
#define TOTAL_GAMES 200

// Player types
#define SMURF 0
#define HARDSTUCK 1
#define NORMAL 2

// Structure to represent player statistics
typedef struct {
    int type;
    int wins;
    int losses;
    int streak;
} Player;

Player players[TOTAL_PLAYERS];

void initialize_players() {
    for (int i = 0; i < TOTAL_PLAYERS; i++) {
        if (i < 10) {
            players[i].type = SMURF;
        } else if (i < 20) {
            players[i].type = HARDSTUCK;
        } else {
            players[i].type = NORMAL;
        }
        players[i].wins = 0;
        players[i].losses = 0;
        players[i].streak = 0;
    }
}

void simulate_game(int game_number) {
    int results[TOTAL_PLAYERS];
    for (int i = 0; i < TOTAL_PLAYERS; i++) {
        results[i] = rand() % 2; // Random win/lose
    }
    
    // Calculate wins, losses and update streaks
    for (int i = 0; i < TOTAL_PLAYERS; i++) {
        if (results[i] == 1) {
            players[i].wins++;
            players[i].streak++;
        } else {
            players[i].losses++;
            players[i].streak = 0;
        }
    }
}

void output_results() {
    FILE *file = fopen("simulation_results.csv", "w");
    fprintf(file, "PlayerType,Wins,Losses,Streak\n");
    for (int i = 0; i < TOTAL_PLAYERS; i++) {
        fprintf(file, "%d,%d,%d,%d\n", players[i].type, players[i].wins, players[i].losses, players[i].streak);
    }
    fclose(file);
}

int main() {
    srand(time(NULL));
    initialize_players();
    for (int i = 0; i < TOTAL_GAMES; i++) {
        simulate_game(i);
    }
    output_results();
    return 0;
}