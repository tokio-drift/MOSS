#ifndef SCOREBOARD_H
#define SCOREBOARD_H
#include <pthread.h>
#include <x86_64-linux-gnu/bits/pthreadtypes.h>
#include <stdbool.h>
#include "types.h"
#include "match.h"

extern scoreboard match;

extern pthread_mutex_t score_mutex;
//start match
void init_scoreboard();
// Update individual batsmen stats
void update_batsman_stats(player *batsman, int runs, bool is_legal);
void mark_batsman_out(player *batsman);

// Update bowler stats
void update_bowler_runs(player *bowler, int runs);
void update_bowler_ball(player *bowler, bool is_legal);
void update_bowler_wicket(player *bowler);

// Logging functions
void print_batsman(player *batsman);
void print_bowler(player *bowler);
void print_current_batsmen(player *b1, player *b2);
void print_batting_card(player team[], int n);
void print_bowling_card(player bowlers[], int n);

// innings and target 
void set_target(int runs);
void reset_for_second_innings();
bool is_match_over();
bool target_chased();
void add_runs(int runs);
void add_wicket();
void next_ball(bool is_legal_delivery);
void get_score(int *runs, int *wickets, int *overs, int *balls);
void reset_players(player team[], int n);
#endif