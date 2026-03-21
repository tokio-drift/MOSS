#include <stdlib.h>
#include <stdio.h>
#include "../../include/types.h"
#include "../../include/constants.h"
#include "../../include/scoreboard.h"

shot_result play_shot(player *batsman, player *bowler, delivery_event ball)
{
    shot_result r;
    int base = batsman->batting_skill;

    // Defining the context factors
    int balls_faced = batsman->balls_faced;
    int set_bonus = balls_faced / 5;   // becomes "set" gradually
    int pressure = 0;
    if (match.innings == 1)
    {
        int runs_left = match.target - match.score;
        int balls_left = (20 * 6) - (match.overs * 6 + match.balls);

        if (balls_left > 0)
        {
            int rrr = (runs_left * 6) / balls_left;
            if (rrr > 8) pressure += 10;
            if (rrr > 10) pressure += 10;
        }
    }
    int powerplay = (match.overs < 6);

    // Calculating wicket probability
    int wicket_prob = 10;  // base

    // For the pacer
    if (bowler->bowler_type == 0) 
    {
        if (ball.ball_type == YORKER || ball.ball_type == BOUNCER)
            wicket_prob += 15;
        else if (ball.ball_type == SLOWER)
            wicket_prob += 10;
        else
            wicket_prob += 5;
    }
    else // For the spinner
    {
        if (ball.ball_type == GOOGLY || ball.ball_type == CARROM)
            wicket_prob += 15;
        else
            wicket_prob += 7;
    }

    wicket_prob += (ball.speed / 20);
    wicket_prob += bowler->wickets_taken * 2;
    wicket_prob += pressure;
    // better batsman reduces wicket probab
    wicket_prob -= base / 10;

    if (wicket_prob < 5) wicket_prob = 5;
    if (wicket_prob > 60) wicket_prob = 60;
    int rnd = rand() % 100;
    if (rnd < wicket_prob)
    {
        int aerial_rand = rand() % 100;
        if (aerial_rand < 30)
        {
            r.wicket = true; // Bowled or Edged (Assuming Keeper is perfect)
            r.runs = 0;
            r.aerial = false;
            return r;
        }
        else
        {
            r.wicket = false;      // Not final yet - catch may be dropped 
            r.runs = 0;
            r.aerial = true;
            return r;
        }
    }

    // Scoring logic
    int run_bias = base + set_bonus + pressure;

    if (bowler->bowler_type == 0) // pacer
        run_bias += 5;
    if (powerplay)
        run_bias += 10;
    int run_rand = rand() % 100;
    if (run_rand < 25 - run_bias / 10)
        r.runs = 0;
    else if (run_rand < 50)
        r.runs = 1;
    else if (run_rand < 65)
        r.runs = 2;
    else if (run_rand < 85)
        r.runs = 4;
    else
        r.runs = 6;

    // Tailenders
    if (batsman->batsmen_type == 2 && r.runs >= 4)
    {
        if (rand() % 100 < 60)
            r.runs = rand() % 3;
    }

    // aerial probability
    r.aerial = (rand() % 100 < (30 + pressure));
    r.wicket = false;
    return r;
}


// Logging
void log_event(FILE *fp,
               player *bowler,
               player *batsman,
               delivery_event ball,
               shot_result r,
               int fielder_id,
               int caught)
{
    fprintf(fp, "Over %d.%d | Bowler %d | Batsman %d | Ball %d @ %dkm/h | ",
            match.overs,
            match.balls,
            bowler->id,
            batsman->id,
            ball.ball_type,
            ball.speed);

    if (ball.extra != NO_EXTRA)
    {
        fprintf(fp, "EXTRA %d\n", ball.extra);
        return;
    }

    if (r.wicket)
    {
        if (caught)
            fprintf(fp, "OUT! Caught by Fielder %d\n", fielder_id);
        else
            fprintf(fp, "OUT! Bowled\n");
    }
    else
    {
        fprintf(fp, "%d runs\n", r.runs);
    }
}