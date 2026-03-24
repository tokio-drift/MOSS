#include <stdio.h>
#include <pthread.h>
#include "../include/scoreboard.h"

scoreboard match;
pthread_mutex_t score_mutex;

void init_scoreboard()
{
    pthread_mutex_init(&score_mutex, NULL);
    match.score = 0;
    match.wickets = 0;
    match.overs = 0;
    match.balls = 0;
    match.extras = 0;
    match.target = 0;
    match.innings = 0;
    match.match_intensity = 0;
}

void update_batsman_stats(player *batsman, int runs, bool is_legal)
{
    batsman->runs_scored += runs;
    if (is_legal)
        batsman->balls_faced++;
}

void mark_batsman_out(player *batsman)
{
    batsman->played = 2; // OUT
    match.wickets++;
}

void update_bowler_runs(player *bowler, int runs)
{
    bowler->runs_conceded += runs;
}

void update_bowler_ball(player *bowler, bool is_legal)
{
    if (is_legal)
        bowler->overs_bowled += 0; // placeholder if needed later
}

void update_bowler_wicket(player *bowler)
{
    bowler->wickets_taken++;
}

void next_ball(bool is_legal_delivery)
{
    if (!is_legal_delivery)
        return;
    match.balls++;
    if (match.balls == 6)
    {
        match.overs++;
        match.balls = 0;
    }
}

void add_runs(int runs)
{
    match.score += runs;
}

void add_wicket()
{
    match.wickets++;
}

void set_target(int runs)
{
    match.target = runs;
}

void reset_for_second_innings()
{
    match.innings = 1;
    match.score = 0;
    match.wickets = 0;
    match.overs = 0;
    match.balls = 0;
    match.extras = 0;
}

bool target_chased()
{
    if (match.innings == 1 && match.score >= match.target)
        return true;
    return false;
}

bool is_match_over()
{
    // All out
    if (match.wickets >= 10)
        return true;

    // Overs finished
    if (match.overs >= 20)
        return true;
    if (target_chased())
        return true;
    return false;
}

// Utils
void get_score(int *runs, int *wickets, int *overs, int *balls)
{
    *runs = match.score;
    *wickets = match.wickets;
    *overs = match.overs;
    *balls = match.balls;
}

void reset_players(player team[], int n)
{
    for (int i = 0; i < n; i++)
    {
        team[i].runs_scored = 0;
        team[i].balls_faced = 0;
        team[i].wickets_taken = 0;
        team[i].runs_conceded = 0;
        team[i].overs_bowled = 0;
        team[i].played = 0; // DNB
    }
}