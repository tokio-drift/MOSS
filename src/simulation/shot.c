#include <stdlib.h>
#include <stdio.h>

#include "../../include/scoreboard.h"
#include "../../include/types.h"
#include "../../include/constants.h"
#include "../../include/match.h"

static const char *ball_type_name(enum ball_type bt)
{
    switch (bt)
    {
        case YORKER:    return "Yorker";
        case BOUNCER:   return "Bouncer";
        case LENGTH:    return "Good Length";
        case FULL:      return "Full";
        case SLOWER:    return "Slower Ball";
        case OFF_BREAK: return "Off Break";
        case LEG_BREAK: return "Leg Break";
        case CARROM:    return "Carrom Ball";
        case GOOGLY:    return "Googly";
        case FLIGHTED:  return "Flighted";
        default:        return "Unknown";
    }
}

static const char *extra_name(int extra)
{
    switch (extra)
    {
        case WIDE:    return "Wide";
        case NO_BALL: return "No Ball";
        case BYE:     return "Bye";
        case LEG_BYE: return "Leg Bye";
        default:      return "Extra";
    }
}

shot_result play_shot(player *batsman, player *bowler, delivery_event ball)
{
    shot_result r = {0};

    int bat  = batsman->batting_skill;   
    int bowl = bowler->bowling_skill;    
    // Played more balls - more "set" batsman
    int set_bonus = batsman->balls_faced / 8;
    if (set_bonus > 15) set_bonus = 15;

    // ! Pressure only for chasing team
    // Increased high risk aerial shots
    int pressure = 0;
    if (match.innings == 1)
    {
        int runs_left  = match.target - match.score;
        int balls_left = (20 * 6) - (match.overs * 6 + match.balls);
        if (balls_left > 0)
        {
            int rrr = (runs_left * 6) / balls_left;
            if (rrr > 8)  pressure += 8;
            if (rrr > 11) pressure += 7;
        }
    }

    int intensity = match.match_intensity;
    if (intensity < 0) intensity = 0;  // Team is ahead - no panic 

    // ! Wicket Probability
    int wicket_prob = 5;

    // & better bowl: higer prob
    if (bowler->bowler_type == 0) // pace
    {
        wicket_prob += (ball.speed - 100) / 15;
        if      (ball.ball_type == YORKER || ball.ball_type == BOUNCER) wicket_prob += 12;
        else if (ball.ball_type == SLOWER)                               wicket_prob += 8;
        else if (ball.ball_type == FULL)                                 wicket_prob += 3;
        else                                                             wicket_prob += 5;
    }
    else // spin
    {
        if      (ball.ball_type == GOOGLY || ball.ball_type == CARROM)  wicket_prob += 12;
        else if (ball.ball_type == LEG_BREAK)                           wicket_prob += 6;
        else                                                             wicket_prob += 4;
    }    

    // bowler and batsman skill
    wicket_prob += (bowl * 10) / 100;
    wicket_prob -= (bat * 20) / 100;

    wicket_prob += pressure / 4;
    wicket_prob += intensity / 5;

    if (wicket_prob <  4) wicket_prob  = 4;
    if (wicket_prob > 45) wicket_prob  = 45;
    int rnd = rand() % 100;
    if (rnd < wicket_prob)
    {
        int aerial_chance = 55 + pressure / 2;
        if (rand() % 100 < aerial_chance)
        {
            r.wicket = false;  // Because catch may yet be dropped
            r.runs   = 0;
            r.aerial = true;
            return r;
        }
        else
        {
            r.wicket = true;   // Bowled, lbw, assumed to be clean
            r.runs   = 0;
            r.aerial = false;
            return r;
        }
    }

    // Higher run bias - more probab of scoring more
    int run_bias = bat + set_bonus;
    if (bowler->bowler_type == 0) run_bias += 5;   // Pace bowler - hgiher economy
    if (match.overs < 6)          run_bias += 8;   // Powerplay

    int nb = run_bias - 60; 
    if (nb < 0)  nb = 0;
    if (nb > 60) nb = 60;

    // dot ball probab
    int dot_threshold  = 30 - nb / 3; 
    int one_threshold  = dot_threshold + 28;
    int two_threshold  = one_threshold + 18;
    int four_threshold = two_threshold + (12 + nb / 6);

    int run_rand = rand() % 100;
    if      (run_rand < dot_threshold)  r.runs = 0;
    else if (run_rand < one_threshold)  r.runs = 1;
    else if (run_rand < two_threshold)  r.runs = 2;
    else if (run_rand < four_threshold) r.runs = 4;
    else                                r.runs = 6;

    // Lower order batter - not a lot of boundaries
    if (bat < 40 && r.runs >= 4)
    {
        if (rand() % 100 < 65)
            r.runs = rand() % 3;
    }

    // More aerial in high pressure chase, so more catch opportunities
    int aerial_prob = 15 + pressure / 2 + intensity / 4;
    if (aerial_prob > 40) aerial_prob = 40;
    r.aerial = (rand() % 100 < aerial_prob);
    r.wicket = false;
    return r;
}


// ! Logging Func
void log_event(FILE *fp,
               player *bowler,
               player *batsman,
               delivery_event ball,
               shot_result r,
               int fielder_id,
               int caught,
               bool caught_by_keeper)
{
    fprintf(fp, "Over %2d.%d | Bowler %2d | Batsman %2d | %-12s @ %3dkm/h | ",
            match.overs, match.balls,
            bowler->id, batsman->id,
            ball_type_name(ball.ball_type),
            ball.speed);

    if (ball.extra != NO_EXTRA)
    {   
        if (r.runs == 0)
            fprintf(fp, "%s\n", extra_name(ball.extra));
        else   
            fprintf(fp, "%s + %d\n", extra_name(ball.extra), r.runs);
        return;
    }

    if (r.wicket)
    {
        if (caught)
        {
            if (caught_by_keeper)
                fprintf(fp, "OUT! Caught behind by W.Keeper\n");
            else
                fprintf(fp, "OUT! Caught by Fielder %d\n", fielder_id);
        }
        else
            fprintf(fp, "OUT! Bowled/LBW\n");
    }
    else
    {
        if (r.aerial && !caught && fielder_id >= 0)
            fprintf(fp, "%d runs  (DROPPED by Fielder %d)\n", r.runs, fielder_id);
        else
            fprintf(fp, "%d runs\n", r.runs);
    }
}