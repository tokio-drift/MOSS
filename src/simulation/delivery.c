#include <stdlib.h>
#include "../../include/types.h"
#include "../../include/constants.h"
#include "../../include/scoreboard.h"

delivery_event generate_delivery(player *bowler)
{
    delivery_event ball;

    if (bowler->bowler_type == 0) // pace
    {
        ball.ball_type = (enum ball_type)(rand() % 5);
        ball.speed     = MIN_SPEED_PACER
                         + rand() % (MAX_SPEED_PACER - MIN_SPEED_PACER + 1);
    }
    else // spin
    {
        ball.ball_type = (enum ball_type)(5 + rand() % 5);
        ball.speed     = MIN_SPEED_SPIN
                         + rand() % (MAX_SPEED_SPIN - MIN_SPEED_SPIN + 1);
    }

    // ! Extras Probability
    // & Fatigue: Tired bowler, higher chance of extra
    int skill     = bowler->bowling_skill;         
    int fatigue   = bowler->overs_bowled / 6;  

    int wide_prob   = 2 + (100 - skill) / 20 + fatigue / 3;
    int noball_prob = 1 + (100 - skill) / 30 + fatigue / 6;
    if (wide_prob   > 6) wide_prob   = 6;
    if (noball_prob > 3) noball_prob = 3;
    int r = rand() % 100;
    if      (r < wide_prob)              ball.extra = WIDE;
    else if (r < wide_prob + noball_prob) ball.extra = NO_BALL;
    else                                  ball.extra = NO_EXTRA;

    return ball;
}