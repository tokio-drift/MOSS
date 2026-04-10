#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "../../include/pitch.h"
#include "../../include/types.h"
#include "../../include/constants.h"
#include "../../include/scheduler.h"
#include "../../include/scoreboard.h"
#include "../../include/match.h"
#include "../../include/players.h"

void *bowler_thread(void *arg)
{
    (void)arg;
    int legal_balls = 0;

    while (!innings_over)
    {
        pthread_mutex_lock(&scheduler_mutex);
        player *bowler = &bowling_team[current_bowler_id];
        pthread_mutex_unlock(&scheduler_mutex);

        delivery_event ball = generate_delivery(bowler);
        pitch_write(ball);

        if (innings_over) break;

        pthread_mutex_lock(&pitch_mutex);
        while (!ball_consumed && !innings_over)
            pthread_cond_wait(&ball_consumed_cond, &pitch_mutex);
        pthread_mutex_unlock(&pitch_mutex);

        if (innings_over) break;

        bool is_legal = (ball.extra != WIDE && ball.extra != NO_BALL);

        pthread_mutex_lock(&score_mutex);
        update_bowler_ball(bowler, is_legal);
        if (is_legal)
        {
            legal_balls++;
            update_match_intensity(&match);
        }
        pthread_mutex_unlock(&score_mutex);

        if (is_legal && legal_balls == 6)
        {
            legal_balls = 0;

            end_over(bowling_team, TEAM_SIZE);
            printf("  [Over %d complete] Bowler -> %s\n",
                   match.overs, bowler->name);
        }
        
    }

    pthread_exit(NULL);
}