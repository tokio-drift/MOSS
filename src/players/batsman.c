// src/players/batsman.c
//
// DESIGN: Each batsman thread represents a SLOT (0 = striker, 1 = non-striker),
// NOT a specific player.  Only the striker slot calls pitch_read() and plays shots.
// When a wicket falls the striker's player pointer is updated via striker_id;
// the thread keeps looping with the new player without any respawn needed.

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

/*
 * arg = pointer to int: slot index
 *   slot 0 → striker
 *   slot 1 → non-striker  (just waits so we can join it cleanly)
 */
void *batsman_thread(void *arg)
{
    int slot = *(int *)arg;

    if (slot != 0)
    {
        /* Non-striker: nothing to do except wait for innings to end */
        while (!innings_over)
            sched_yield();
        pthread_exit(NULL);
    }

    /* ---- Striker thread ---- */
    srand((unsigned)(time(NULL) ^ (uintptr_t)pthread_self()));

    FILE *log_fp = fopen("../logs/log.txt", "a");
    if (!log_fp) log_fp = stderr;

    while (!innings_over)
    {
        delivery_event ball = pitch_read();

        if (innings_over) break;

        /* Identify current striker and bowler */
        pthread_mutex_lock(&scheduler_mutex);
        int sid      = striker_id;
        int bowler_i = current_bowler_id;
        pthread_mutex_unlock(&scheduler_mutex);

        player *batsman = &batting_team[sid];
        player *bowler  = &bowling_team[bowler_i];

        /* -------- EXTRAS -------- */
        if (ball.extra != NO_EXTRA)
        {
            pthread_mutex_lock(&score_mutex);
            int extra_runs = 1;
            int bat_runs   = 0;
            int chance = rand() % 100;
            if      (chance < 5) bat_runs = 1;
            else if (chance < 7) bat_runs = 4;

            match.score   += extra_runs + bat_runs;
            match.extras  += extra_runs;
            update_bowler_runs(bowler, extra_runs + bat_runs);
            if (bat_runs > 0)
                update_batsman_stats(batsman, bat_runs, false);
            pthread_mutex_unlock(&score_mutex);

            log_event(log_fp, bowler, batsman, ball,
                      (shot_result){.runs = bat_runs, .wicket = false, .aerial = false},
                      -1, 0);
            continue;
        }

        /* -------- SHOT -------- */
        shot_result r = play_shot(batsman, bowler, ball);

        int fielder_id = -1;
        int caught     = 0;

        if (r.aerial)
        {
            fielder_id = select_fielder(bowling_team, TEAM_SIZE);
            notify_fielder(fielder_id, r.aerial);

            pthread_mutex_lock(&fielder_mutex);
            player *f = &bowling_team[fielder_id];
            if (!catch_taken && attempt_catch(f, r.aerial))
            {
                catch_taken = true;
                r.wicket    = true;
                caught      = 1;
            }
            else
            {
                r.wicket = false;
                r.runs   = rand() % 3;
            }
            pthread_mutex_unlock(&fielder_mutex);
        }

        /* -------- SCORE UPDATE -------- */
        pthread_mutex_lock(&score_mutex);

        /* Re-check: match may have ended between pitch_read and here */
        if (is_match_over())
        {
            pthread_mutex_unlock(&score_mutex);
            break;
        }

        update_batsman_stats(batsman, r.runs, true);
        update_bowler_runs(bowler, r.runs);
        match.score += r.runs;

        if (r.wicket && match.wickets < 10)
        {
            mark_batsman_out(batsman);
            match.wickets++;
            update_bowler_wicket(bowler);
            int new_sid = on_wicket();
            (void)new_sid;
        }
        else
        {
            r.wicket = false; /* cancel wicket if already 10 down */
        }

        next_ball(true);

        pthread_mutex_unlock(&score_mutex);

        /* -------- STRIKE ROTATION (odd runs) -------- */
        if (!r.wicket && (r.runs % 2 == 1))
            swap_strike();

        log_event(log_fp, bowler, batsman, ball, r, fielder_id, caught);
        fflush(log_fp);
        reset_fielder_state();
    }

    fclose(log_fp);
    pthread_exit(NULL);
}