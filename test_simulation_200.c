#include <stdio.h>
#include "eomm_system.h"

// Simulation settings
#define TOTAL_GAMES 200

int main() {
    int mmr = 1500; // Starting MMR
    int win_count = 0;
    int loss_count = 0;
    int current_streak = 0;
    int max_streak = 0;

    // Seed random number generator
    srand(time(NULL));

    // Run simulation for TOTAL_GAMES
    for (int game = 1; game <= TOTAL_GAMES; game++) {
        int outcome = rand() % 2; // Random win (1) or loss (0)
        if (outcome) { // Win
            win_count++;
            current_streak++;
            mmr += 10; // Increase MMR on win
        } else { // Loss
            loss_count++;
            current_streak = 0;
            mmr -= 10; // Decrease MMR on loss
        }
        // Update max streak
        if (current_streak > max_streak) {
            max_streak = current_streak;
        }
        // Output details for this game
        printf("Game %d: %s | MMR: %d | Current Streak: %d | Max Streak: %d\n",
               game, outcome ? "Win" : "Loss", mmr, current_streak, max_streak);
    }

    // Final Output
    printf("\nSummary: Total Wins: %d, Total Losses: %d, Final MMR: %d\n", win_count, loss_count, mmr);
    return 0;
}