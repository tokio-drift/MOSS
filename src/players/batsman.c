#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../../include/pitch.h"
#include "../../include/types.h"
#include "../../include/constants.h"
#include "../../include/scheduler.h"
#include "../../include/scoreboard.h"
#include "../../include/match.h"
#include "../../include/fielder.h"
#include "../../include/players.h"


// Batsman Thread
void *batsman_thread(void *arg)
{
    srand(time(NULL) ^ pthread_self());
    FILE *log_fp = fopen("./logs/log.txt", "a");

    while (1)
    {
        if (is_match_over())
            break;

        // Read ball from pitch
        delivery_event ball = pitch_read();
        pthread_mutex_lock(&scheduler_mutex);
        int striker = striker_id;
        int bowler_id = current_bowler_id;
        pthread_mutex_unlock(&scheduler_mutex);

        player *batsman = &batting_team[striker];
        player *bowler = &bowling_team[bowler_id];

        // Extras handler
        if (ball.extra != NO_EXTRA)
            {
                pthread_mutex_lock(&score_mutex);
                int extra_runs = 1;
                int bat_runs = 0;

                // Rare to get a run on an extra, but not impossible
                int chance = rand() % 100;
                if (chance < 5)
                    bat_runs = 1;
                else if (chance < 7)
                    bat_runs = 4;
            
                match.score += extra_runs + bat_runs;
                match.extras += extra_runs;
                update_bowler_runs(bowler, extra_runs + bat_runs);
                if (bat_runs > 0)
                    update_batsman_stats(batsman, bat_runs, false);
    
                pthread_mutex_unlock(&score_mutex);        
                log_event(log_fp, bowler, batsman, ball,
                          (shot_result){.runs = bat_runs, .wicket = false},
                          -1, 0);
                continue;
            }


        shot_result r = play_shot(batsman, bowler, ball);

        int fielder_id = -1;
        int caught = 0;

        // Fielding logic
        if (r.aerial)
        {
            fielder_id = select_fielder(bowling_team, TEAM_SIZE);
            notify_fielder(fielder_id, r.aerial);
            pthread_mutex_lock(&fielder_mutex);
            player *f = &bowling_team[fielder_id];
            if (!catch_taken && attempt_catch(f, r.aerial))
            {
                catch_taken = true;
                r.wicket = true;   // Wicket confirmed here, catch taken
                caught = 1;
            }
            else
            {
                // Dropped Catch
                r.wicket = false;
                // ! Arbitrarily assigning runs right now - will fix sometime later if I get the run gen logic separated
                int drop_runs = rand() % 3; 
                r.runs = drop_runs;
            }
        
            pthread_mutex_unlock(&fielder_mutex);
        }

        // Scoreboard update
        pthread_mutex_lock(&score_mutex);
        update_batsman_stats(batsman, r.runs, true);
        update_bowler_runs(bowler, r.runs);
        match.score += r.runs;

        if (r.wicket)
        {
            mark_batsman_out(batsman);
            update_bowler_wicket(bowler);
            on_wicket();
        }

        next_ball(true);
        pthread_mutex_unlock(&score_mutex);

        // Strike Rotation
        if (!r.wicket && (r.runs % 2 == 1))
        {
            pthread_mutex_lock(&scheduler_mutex);
            swap_strike();
            pthread_mutex_unlock(&scheduler_mutex);
        }

        // Logging
        log_event(log_fp, bowler, batsman, ball, r, fielder_id, caught);
        reset_fielder_state();
    }

    fclose(log_fp);
    pthread_exit(NULL);
}