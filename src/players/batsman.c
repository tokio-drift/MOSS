#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "../../include/pitch.h"
#include "../../include/types.h"
#include "../../include/constants.h"
#include "../../include/scheduler.h"
#include "../../include/scoreboard.h"
#include "../../include/match.h"
#include "../../include/fielder.h"
#include "../../include/players.h"
#include "../../include/gantt.h"

void *batsman_thread(void *arg)
{
    int slot = *(int *)arg;

    if (slot != 0)
    {
        while (!innings_over)
            sched_yield();
        pthread_exit(NULL);
    }

    srand((unsigned)(time(NULL) ^ (uintptr_t)pthread_self()));

    FILE *log_fp = fopen(LOG_FILE, "a");
    if (!log_fp) log_fp = stderr;

    while (!innings_over)
    {
        delivery_event ball = pitch_read();
        if (innings_over) break;

        struct timespec consumed_ts;
        clock_gettime(CLOCK_MONOTONIC, &consumed_ts);
        long long consumed_ns = consumed_ts.tv_sec * 1000000000LL + consumed_ts.tv_nsec;

        pthread_mutex_lock(&scheduler_mutex);
        int sid      = striker_id;
        int bowler_i = current_bowler_id;
        pthread_mutex_unlock(&scheduler_mutex);

        player *batsman = &batting_team[sid];
        player *bowler  = &bowling_team[bowler_i];

        if (ball.extra != NO_EXTRA)
        {
            pthread_mutex_lock(&score_mutex);
            int extra_runs = 1;
            int bat_runs   = 0;
            if (rand() % 100 < 10) bat_runs = 1;

            int total_runs = extra_runs + bat_runs;  // Calculate total once
            match.score  += total_runs;
            match.extras += extra_runs;
            update_bowler_runs(bowler, total_runs);
            if (bat_runs > 0)
                update_batsman_stats(batsman, bat_runs, false);
            pthread_mutex_unlock(&score_mutex);

            log_event(log_fp, bowler, batsman, ball,
                      (shot_result){.runs=total_runs, .wicket=false, .aerial=false},
                      -1, 0, false);
            fflush(log_fp);
            gantt_record(&ball, bowler, batsman, consumed_ns,
                         match.overs, match.balls, total_runs, false, match.innings);
            continue;
        }

        shot_result r  = play_shot(batsman, bowler, ball);
        int fielder_id = -1;
        int caught     = 0;
        bool caught_by_keeper = false;

        if (r.aerial)
        {
            bool edge_to_keeper = (!r.wicket && rand() % 100 < 20) ||
                                  (r.wicket  && rand() % 100 < 35);
            if (edge_to_keeper)
                fielder_id = 0;
            else
                fielder_id = select_fielder(bowling_team, TEAM_SIZE);

            notify_fielder(fielder_id, r.aerial);

            pthread_mutex_lock(&fielder_mutex);
            player *f = &bowling_team[fielder_id];
            if (!catch_taken && attempt_catch(f, r.aerial))
            {
                catch_taken       = true;
                r.wicket          = true;
                r.runs            = 0;
                caught            = 1;
                caught_by_keeper  = f->is_keeper;
            }
            else
            {
                catch_taken       = false;
                r.wicket          = false;
                /* Keep original r.runs from play_shot() - DON'T overwrite */
            }
            pthread_mutex_unlock(&fielder_mutex);
        }

        pthread_mutex_lock(&score_mutex);

        if (is_match_over())
        {
            pthread_mutex_unlock(&score_mutex);
            break;
        }

        if (batsman->played == PLAYER_DNB) {
            batsman->played = PLAYER_BATTING;
        }

        update_batsman_stats(batsman, r.runs, true);
        update_bowler_runs(bowler, r.runs);
        match.score += r.runs;

        if (r.wicket && match.wickets < 10)
        {
            mark_batsman_out(batsman);
            match.wickets++;
            update_bowler_wicket(bowler);
            on_wicket();
        }
        else
        {
            r.wicket = false;
        }

        next_ball(true);
        pthread_mutex_unlock(&score_mutex);

        if (!r.wicket && (r.runs % 2 == 1))
            swap_strike();

        gantt_record(&ball, bowler, batsman, consumed_ns,
                     match.overs, match.balls, r.runs, r.wicket, match.innings);

        log_event(log_fp, bowler, batsman, ball, r, fielder_id, caught, caught_by_keeper);
        fflush(log_fp);
        reset_fielder_state();
    }

    if (log_fp != stderr) fclose(log_fp);
    pthread_exit(NULL);
}