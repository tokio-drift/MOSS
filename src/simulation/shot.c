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

shot_result play_shot(player *batsman, player *bowler, delivery_event ball)
{
    shot_result r = {0};

    int bat       = batsman->batting_skill;
    int bowl      = bowler->bowling_skill;
    int set_bonus = batsman->balls_faced / 8;
    if (set_bonus > 15) set_bonus = 15;

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
    if (intensity < 0) intensity = 0;

    int wicket_prob = 5;

    if (bowler->bowler_type == 0)
    {
        wicket_prob += (ball.speed - 100) / 15;
        if      (ball.ball_type == YORKER || ball.ball_type == BOUNCER) wicket_prob += 12;
        else if (ball.ball_type == SLOWER)                               wicket_prob += 8;
        else if (ball.ball_type == FULL)                                 wicket_prob += 3;
        else                                                             wicket_prob += 5;
    }
    else
    {
        if      (ball.ball_type == GOOGLY || ball.ball_type == CARROM)  wicket_prob += 12;
        else if (ball.ball_type == LEG_BREAK)                           wicket_prob += 6;
        else                                                             wicket_prob += 4;
    }

    wicket_prob += (bowl * 10) / 100;
    wicket_prob -= (bat  * 20) / 100;

    if (batsman->batsmen_type == BTYPE_TAIL)   wicket_prob += 8;
    if (batsman->batsmen_type == BTYPE_MIDDLE) wicket_prob += 2;

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
            r.wicket         = false;
            r.runs           = 0;
            r.aerial         = true;
            r.wicket_attempt = true;
            return r;
        }
        else
        {
            r.wicket         = true;
            r.runs           = 0;
            r.aerial         = false;
            r.wicket_attempt = false;
            return r;
        }
    }

    int run_bias = bat + set_bonus;
    if (bowler->bowler_type == 0) run_bias += 5;
    if (match.overs < 6)          run_bias += 8;

    if (batsman->batsmen_type == BTYPE_TAIL) run_bias -= 25;
    if (run_bias < 0) run_bias = 0;

    int nb = run_bias - 60;
    if (nb < 0)  nb = 0;
    if (nb > 60) nb = 60;

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

    if (batsman->batsmen_type == BTYPE_TAIL && r.runs >= 4)
    {
        if (rand() % 100 < 65)
            r.runs = rand() % 3;
    }

    int aerial_prob = 15 + pressure / 2 + intensity / 4;
    if (aerial_prob > 40) aerial_prob = 40;
    r.aerial         = (rand() % 100 < aerial_prob);
    r.wicket         = false;
    r.wicket_attempt = false;
    return r;
}

 
void log_event(FILE *fp,
               player *bowler,
               player *batsman,
               delivery_event ball,
               shot_result r,
               int fielder_id,
               int caught,
               bool caught_by_keeper,
               bool runout,
               bool runout_striker)
{
    fprintf(fp, "Over %2d.%d | %-12s | %-12s | %-12s @ %3dkm/h | ",
            match.overs, match.balls,
            bowler->name, batsman->name,
            ball_type_name(ball.ball_type),
            ball.speed);

    if (ball.extra != NO_EXTRA)
    {
        const char *en;
        switch (ball.extra)
        {
            case WIDE:    en = "Wide";    break;
            case NO_BALL: en = "No Ball"; break;
            case BYE:     en = "Bye";     break;
            case LEG_BYE: en = "Leg Bye"; break;
            default:      en = "Extra";   break;
        }
        if (r.runs == 0)
            fprintf(fp, "%s\n", en);
        else
            fprintf(fp, "%s + %d run%s\n", en, r.runs, r.runs == 1 ? "" : "s");
        return;
    }

    if (runout)
    {
        const char *dismissed = runout_striker ? batsman->name : "(non-striker)";
        if (r.runs > 0)
            fprintf(fp, "RUN OUT! %-12s  |  %d run%s scored before dismissal"
                        "  |  Fielder %d\n",
                    dismissed,
                    r.runs, r.runs == 1 ? "" : "s",
                    fielder_id);
        else
            fprintf(fp, "RUN OUT! %-12s  |  0 runs scored"
                        "  |  Fielder %d\n",
                    dismissed, fielder_id);
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
        return;
    }

    if (r.aerial && !caught && fielder_id >= 0)
        fprintf(fp, "%d run%s  (DROPPED by Fielder %d)\n",
                r.runs, r.runs == 1 ? "" : "s", fielder_id);
    else
        fprintf(fp, "%d run%s\n", r.runs, r.runs == 1 ? "" : "s");
}