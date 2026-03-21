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
        ball.speed = MIN_SPEED_PACER + rand() % (MAX_SPEED_PACER - MIN_SPEED_PACER + 1);
    }
    else
    {
        ball.ball_type = (enum ball_type)(5 + rand() % 5);
        ball.speed = MIN_SPEED_SPIN + rand() % (MAX_SPEED_SPIN - MIN_SPEED_SPIN + 1);
    }

    // extras probability
    int base_wide = 8;
    int base_noball = 4;
    int skill_factor = (100 - bowler->bowling_skill) / 10;
    int fatigue = bowler->overs_bowled * 2;
    int pressure = match.match_intensity / 10;
    int wide_prob = base_wide + skill_factor + fatigue + pressure;
    int noball_prob = base_noball + skill_factor / 2 + fatigue / 2;
    if (wide_prob > 25)
        wide_prob = 25;
    if (noball_prob > 15)
        noball_prob = 15;
    int r = rand() % 100;
    if (r < wide_prob)
        ball.extra = WIDE;
    else if (r < wide_prob + noball_prob)
        ball.extra = NO_BALL;
    else
        ball.extra = NO_EXTRA;
    return ball;
}