#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "../include/scheduler.h"
#include "../include/types.h"
int current_bowler_id = 0;
int striker_id = 0;
int non_striker_id = 1;
int next_batsman_id = 2;
int scheduling_policy = SCHED_RR;
pthread_mutex_t scheduler_mutex;
void init_scheduler()
{
    pthread_mutex_init(&scheduler_mutex, NULL);
    current_bowler_id = 0;
}
void set_scheduling_policy(int policy)
{
    scheduling_policy = policy;
}
int get_phase(scoreboard *match)
{
    if (match->overs < 6)
        return 0; // powerplay
    else if (match->overs < 16)
        return 1; // middle
    else
        return 2; // death
}
int compute_intensity(scoreboard *match)
{
    if (match->which_innings == 0)
        return 0;
    float current_rr = (float)match->runs / (match->overs + 1);
    float required_rr = (float)(match->target - match->runs) /
                        (20 - match->overs + 1);
    return (int)(required_rr - current_rr);
}
int select_next_bowler(player team[], int n)
{
    pthread_mutex_lock(&scheduler_mutex);
    int result;
    if (scheduling_policy == SCHED_RR)
        result = schedule_rr(team, n);
    else if (scheduling_policy == SCHED_PRIORITY)
        result = schedule_priority(team, n, NULL);
    else
        result = schedule_sjf(team, n);
    current_bowler_id = result;
    pthread_mutex_unlock(&scheduler_mutex);
    return result;
}
int schedule_rr(player team[], int n)
{
    int start = current_bowler_id;

    for (int i = 1; i <= n; i++)
    {
        int idx = (start + i) % n;
        if (team[idx].overs_bowled < 4)
            return idx;
    }

    return start;
}
int schedule_sjf(player team[], int n)
{
    int min_overs = INT_MAX;
    int best = 0;
    for (int i = 0; i < n; i++)
    {
        if (team[i].overs_bowled < 4)
        {
            if (team[i].overs_bowled < min_overs)
            {
                min_overs = team[i].overs_bowled;
                best = i;
            }
        }
    }
    return best;
}
int schedule_priority(player team[], int n, scoreboard *match)
{
    int best = -1;
    int best_score = INT_MIN;
    int phase = get_phase(match);
    int intensity = compute_intensity(match);
    for (int i = 0; i < n; i++)
    {
        if (team[i].overs_bowled >= 4)
            continue;
        int score = 0;
        // skill
        score += team[i].skill_level * 2;
        // fatigue
        score -= team[i].overs_bowled * 3;
        // phase logic
        if (phase == 0) // powerplay
        {
            if (team[i].bowler_type == 0) // pace
                score += 10;
            else
                score -= 5;
        }
        else if (phase == 1) // middle
        {
            if (team[i].bowler_type == 1) // spin
                score += 8;
        }
        else // death
        {
            if (team[i].bowler_type == 0)
                score += 12;
        }
        // match intensity
        if (intensity > 2)
            score += team[i].skill_level * 2;
        // small randomness
        score += rand() % 3;
        if (score > best_score)
        {
            best_score = score;
            best = i;
        }
    }
    if (best == -1)
        best = 0;
    return best;
}
void init_batting_order()
{
    striker_id = 0;
    non_striker_id = 1;
    next_batsman_id = 2;
}
int select_next_batsman(player team[], int n, scoreboard *match)
{
    int best = -1;
    int best_score = INT_MIN;
    int intensity = compute_intensity(match);
    for (int i = next_batsman_id; i < n; i++)
    {
        int score = 0;
        // base skill
        score += team[i].batting_skill * 2;
        // type-based
        if (intensity > 2)
        {
            if (team[i].batsman_type == 2) // finisher
                score += 15;
        }
        else if (intensity < 0)
        {
            if (team[i].batsman_type == 1) // anchor
                score += 10;
        }
        if (match->wickets < 2)
        {
            if (team[i].batsman_type == 0) // opener
                score += 10;
        }
        if (match->wickets >= 4)
        {
            if (team[i].batsman_type == 3) // allrounder
                score += 10;
        }
        score += rand() % 3;
        if (score > best_score)
        {
            best_score = score;
            best = i;
        }
    }
    return best;
}
void on_wicket()
{
    pthread_mutex_lock(&scheduler_mutex);
    striker_id = next_batsman_id;
    next_batsman_id++;
    pthread_mutex_unlock(&scheduler_mutex);
}
void swap_strike()
{
    pthread_mutex_lock(&scheduler_mutex);
    int temp = striker_id;
    striker_id = non_striker_id;
    non_striker_id = temp;
    pthread_mutex_unlock(&scheduler_mutex);
}

int get_striker()
{
    return striker_id;
}

int get_non_striker()
{
    return non_striker_id;
}
void end_over(player team[], int n)
{
    swap_strike();
    select_next_bowler(team, n);
}
void update_match_intensity(scoreboard *match)
{
    match->match_intensity = compute_intensity(match);
}