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
        delivery_event ball = generate_delivery(bowler);
            
        // write using pitch abstraction
        pitch_write(ball);
        // wait until batsman consumes
        pthread_mutex_lock(&pitch_mutex);
        while (!ball_consumed){
            if(is_match_over()) break;
            pthread_cond_wait(&ball_consumed_cond, &pitch_mutex);
        }
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