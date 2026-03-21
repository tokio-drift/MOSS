#include <stdlib.h>
#include "../../include/pitch.h"
#include "../../include/types.h"
#include "../../include/constants.h"
#include "../../include/scheduler.h"
#include "../../include/scoreboard.h"
#include "../../include/match.h"
#include "../../include/players.h"

void *bowler_thread(void *arg)
{
    player *bowler = (player *)arg;
    int legal_balls = 0;
    while (1)
    {
        // stop condition
        if (is_match_over())
            break;
        pthread_mutex_lock(&pitch_mutex);
        // wait until previous ball is consumed
        while (ball_ready == true)
            pthread_cond_wait(&ball_consumed_cond, &pitch_mutex);
        // generate ball
        delivery_event ball = generate_delivery(bowler);
        // write to pitch
        pitch_buffer[pitch_index] = ball;
        pitch_index = (pitch_index + 1) % PITCH_SIZE;
        // update state
        ball_ready = true;
        ball_consumed = false;
        // notify batsman
        pthread_cond_signal(&ball_ready_cond);
        // wait until batsman finishes shot
        while (ball_consumed == false)
            pthread_cond_wait(&ball_consumed_cond, &pitch_mutex);
        pthread_mutex_unlock(&pitch_mutex);
        // 🔹 update scoreboard (only legal ball increments)
        bool is_legal = (ball.extra != WIDE && ball.extra != NO_BALL);
        update_bowler_ball(bowler, is_legal);
        if (is_legal)
        {
            legal_balls++;
            update_ball(); // scheduler update
        }
        // 🔹 handle over completion
        if (legal_balls == 6)
        {
            pthread_mutex_lock(&scheduler_mutex);
            end_over(bowling_team, TEAM_SIZE);
            current_bowler_id = select_next_bowler(bowling_team, TEAM_SIZE);
            pthread_mutex_unlock(&scheduler_mutex);
            legal_balls = 0;
        }
    }
    pthread_exit(NULL);
}