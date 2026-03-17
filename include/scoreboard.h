#ifndef SCOREBOARD_H
#define SCOREBOARD_H
#include <pthread.h>
#include <x86_64-linux-gnu/bits/pthreadtypes.h>
#include <stdbool.h>

extern int total_runs;
extern int total_wickets;
extern int overs_completed;
extern int balls_in_over;
extern int target;              // only used in 2nd innings
extern bool is_second_innings;
extern pthread_mutex_t score_mutex;
void init_scoreboard();
// Update runs (normal runs or extras)
void add_runs(int runs);
// Add a wicket
void add_wicket();
// Advance ball count (handles over progression)
void next_ball(bool is_legal_delivery);
// Set target at end of first innings
void set_target(int runs);
// Reset scoreboard for second innings
void reset_for_second_innings();
// Check if match is over
bool is_match_over();
// Get current score snapshot
void get_score(int *runs, int *wickets, int *overs, int *balls);
// Check if target is reached (2nd innings)
bool target_chased();
// Print current score (for debugging / logs)
void print_scoreboard();
// Ball-by-ball logging
void log_ball_event(int over, int ball, int runs, bool wicket);
#endif