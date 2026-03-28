#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "../include/scheduler.h"
#include "../include/types.h"
#include "../include/scoreboard.h"
#include "../include/match.h"

int current_bowler_id  = 0;
int striker_id         = 0;
int non_striker_id     = 1;
int next_batsman_id    = 2;
int scheduling_policy  = SCHED_RoR;
pthread_mutex_t scheduler_mutex;

static inline bool can_bowl(player *p)
{
    return (!p->is_keeper) && (p->bowling_skill >= 0) && (p->overs_bowled < 4 * 6);
}

void init_scheduler()
{
    pthread_mutex_init(&scheduler_mutex, NULL);
    current_bowler_id = 1;
}

void set_scheduling_policy(int policy) { scheduling_policy = policy; }

int get_phase(scoreboard *m)
{
    if (m->overs < 6)  return 0;
    if (m->overs < 16) return 1;
    return 2;
}

int compute_intensity(scoreboard *m)
{
    if (m->innings == 0) return 0;
    float current_rr  = (float)m->score / (m->overs + 1);
    float required_rr = (float)(m->target - m->score) / (20 - m->overs + 1);
    return (int)(required_rr - current_rr);
}

static int schedule_rr_locked(player team[], int n)
{
    int start = current_bowler_id;
    for (int i = 1; i <= n; i++)
    {
        int idx = (start + i) % n;
        if (can_bowl(&team[idx]))
            return idx;
    }
    for (int i = 0; i < n; i++)
        if (!team[i].is_keeper && team[i].bowling_skill >= 0)
            return i;
    return 1;
}

static int schedule_sjf_locked(player team[], int n)
{
    int min_balls = INT_MAX, best = -1;
    for (int i = 0; i < n; i++)
        if (can_bowl(&team[i]) && team[i].overs_bowled < min_balls)
        {
            min_balls = team[i].overs_bowled;
            best = i;
        }
    return (best == -1) ? schedule_rr_locked(team, n) : best;
}

static int schedule_priority_locked(player team[], int n)
{
    int best = -1, best_score = INT_MIN;
    int phase     = get_phase(&match);
    int intensity = compute_intensity(&match);

    for (int i = 0; i < n; i++)
    {
        if (!can_bowl(&team[i])) continue;
        int score = team[i].bowling_skill * 2 - team[i].overs_bowled * 3;
        if (phase == 0) score += (team[i].bowler_type == 0) ? 10 : -5;
        else if (phase == 1) score += (team[i].bowler_type == 1) ? 8 : 0;
        else score += (team[i].bowler_type == 0) ? 12 : 0;
        if (intensity > 2) score += team[i].bowling_skill;
        score += rand() % 3;
        if (score > best_score) { best_score = score; best = i; }
    }
    return (best == -1) ? schedule_rr_locked(team, n) : best;
}

static int select_bowler_locked(player team[], int n)
{
    int result;
    if (scheduling_policy == SCHED_RoR)
        result = schedule_rr_locked(team, n);
    else if (scheduling_policy == SCHED_PRIORITY)
        result = schedule_priority_locked(team, n);
    else
        result = schedule_sjf_locked(team, n);
    current_bowler_id = result;
    return result;
}

int select_next_bowler(player team[], int n)
{
    pthread_mutex_lock(&scheduler_mutex);
    int r = select_bowler_locked(team, n);
    pthread_mutex_unlock(&scheduler_mutex);
    return r;
}

int schedule_rr(player team[], int n)
{
    pthread_mutex_lock(&scheduler_mutex);
    int r = schedule_rr_locked(team, n);
    pthread_mutex_unlock(&scheduler_mutex);
    return r;
}

int schedule_sjf(player team[], int n)
{
    pthread_mutex_lock(&scheduler_mutex);
    int r = schedule_sjf_locked(team, n);
    pthread_mutex_unlock(&scheduler_mutex);
    return r;
}

int schedule_priority(player team[], int n, scoreboard *m)
{
    (void)m;
    pthread_mutex_lock(&scheduler_mutex);
    int r = schedule_priority_locked(team, n);
    pthread_mutex_unlock(&scheduler_mutex);
    return r;
}

void init_batting_order()
{
    striker_id      = 0;
    non_striker_id  = 1;
    next_batsman_id = 2;
}

static int select_next_batsman_locked(player team[], int n, scoreboard *m)
{
    int best = -1, best_score = INT_MIN;
    int intensity = compute_intensity(m);

    for (int i = next_batsman_id; i < n; i++)
    {
        if (team[i].played == PLAYER_OUT) continue;
        int score = team[i].batting_skill * 2;
        if (intensity > 2  && team[i].batsmen_type == BTYPE_MIDDLE) score += 15;
        if (intensity < 0  && team[i].batsmen_type == BTYPE_TOP)    score += 10;
        if (m->wickets < 2 && team[i].batsmen_type == BTYPE_TOP)    score += 10;
        if (m->wickets >= 7 && team[i].batsmen_type == BTYPE_TAIL)  score += 5;
        score += rand() % 3;
        if (score > best_score) { best_score = score; best = i; }
    }
    return best;
}

int on_wicket()
{
    pthread_mutex_lock(&scheduler_mutex);
    int next = select_next_batsman_locked(batting_team, TEAM_SIZE, &match);
    if (next == -1)
    {
        pthread_mutex_unlock(&scheduler_mutex);
        return -1;
    }
    striker_id      = next;
    next_batsman_id = next + 1;
    pthread_mutex_unlock(&scheduler_mutex);
    return next;
}

void swap_strike()
{
    pthread_mutex_lock(&scheduler_mutex);
    int tmp        = striker_id;
    striker_id     = non_striker_id;
    non_striker_id = tmp;
    pthread_mutex_unlock(&scheduler_mutex);
}

int get_striker()     { return striker_id; }
int get_non_striker() { return non_striker_id; }

void end_over(player team[], int n)
{
    pthread_mutex_lock(&scheduler_mutex);
    int tmp        = striker_id;
    striker_id     = non_striker_id;
    non_striker_id = tmp;
    select_bowler_locked(team, n);
    pthread_mutex_unlock(&scheduler_mutex);
}

void update_match_intensity(scoreboard *m)
{
    m->match_intensity = compute_intensity(m);
}