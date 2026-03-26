#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#include "../include/types.h"
#include "../include/constants.h"
#include "../include/match.h"
#include "../include/pitch.h"
#include "../include/scoreboard.h"
#include "../include/scheduler.h"
#include "../include/fielder.h"
#include "../include/players.h"

player team1[TEAM_SIZE];
player team2[TEAM_SIZE];

player *batting_team;
player *bowling_team;

volatile int innings_over = 0;

void init_team(player team[], int n)
{
    for (int i = 0; i < n; i++)
    {
        team[i].id             = i;
        team[i].played         = PLAYER_DNB;
        team[i].overs_bowled   = 0;
        team[i].runs_conceded  = 0;
        team[i].wickets_taken  = 0;
        team[i].runs_scored    = 0;
        team[i].balls_faced    = 0;

        if (i == 0)
        {
            // Wicketkeper: excellent fielder, decent batter, cannot ball
            team[i].is_keeper      = true;
            team[i].bowling_skill  = -1;
            team[i].fielding_skill = 80 + rand() % 20;
            team[i].batting_skill  = 45 + rand() % 35;
            team[i].bowler_type    = 0;
            team[i].batsmen_type   = 0;
        }
        else if (i < 4)
        {
            // Top order batsmen: high bat skill, moderate ball
            team[i].is_keeper      = false;
            team[i].bowling_skill  = 30 + rand() % 40;
            team[i].fielding_skill = 60 + rand() % 40;
            team[i].batting_skill  = 65 + rand() % 35;
            team[i].bowler_type    = rand() % 2;
            team[i].batsmen_type   = (i == 1) ? 0 : 1;
        }
        else if (i < 8)
        {
            // All rounders, middle order
            team[i].is_keeper      = false;
            team[i].bowling_skill  = 50 + rand() % 40;
            team[i].fielding_skill = 55 + rand() % 40;
            team[i].batting_skill  = 50 + rand() % 40;
            team[i].bowler_type    = rand() % 2;
            team[i].batsmen_type   = (rand() % 2 == 0) ? 2 : 3;
        }
        else
        {
            // Tail, great bowlers
            team[i].is_keeper      = false;
            team[i].bowling_skill  = 60 + rand() % 40;
            team[i].fielding_skill = 50 + rand() % 30;
            team[i].batting_skill  = 20 + rand() % 30;
            team[i].bowler_type    = rand() % 2;
            team[i].batsmen_type   = 2;
        }
    }
}

static void play_innings(int innings_num)
{
    innings_over = 0;
    reset_pitch();
    init_batting_order();
    select_next_bowler(bowling_team, TEAM_SIZE);

    FILE *lf = fopen("../logs/log.txt", (innings_num == 0) ? "w" : "a");
    if (lf)
    {
        fprintf(lf,
                "\n"
                "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
                "  INNINGS %d  |  Batting: Team %d\n"
                "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n",
                innings_num + 1,
                (batting_team == team1) ? 1 : 2);
        fclose(lf);
    }

    printf("\n========================================\n");
    printf("  INNINGS %d  |  Batting: Team %d\n",
           innings_num + 1, (batting_team == team1) ? 1 : 2);
    printf("========================================\n");

    int slot0 = 0, slot1 = 1;

    pthread_t bowler_tid;
    pthread_t batsman_tids[2];
    pthread_t fielder_tids[TEAM_SIZE];

    pthread_create(&bowler_tid,      NULL, bowler_thread,  NULL);
    pthread_create(&batsman_tids[0], NULL, batsman_thread, &slot0);
    pthread_create(&batsman_tids[1], NULL, batsman_thread, &slot1);
    for (int i = 0; i < TEAM_SIZE; i++)
        pthread_create(&fielder_tids[i], NULL, fielder_thread, &bowling_team[i]);

    // Poll until the innings end
    while (1)
    {
        pthread_mutex_lock(&score_mutex);
        int over = is_match_over();
        pthread_mutex_unlock(&score_mutex);
        if (over) break;
        struct timespec ts = {0, 1000000};
        nanosleep(&ts, NULL);
    }

    // ! Innings over
    innings_over = 1;
    reset_pitch();
    pthread_mutex_lock(&fielder_mutex);
    for (int i = 0; i < TEAM_SIZE; i++)
        pthread_cond_broadcast(&fielder_cond[i]);
    pthread_mutex_unlock(&fielder_mutex);

    pthread_join(batsman_tids[0], NULL);
    pthread_join(batsman_tids[1], NULL);
    pthread_join(bowler_tid,      NULL);
    for (int i = 0; i < TEAM_SIZE; i++)
        pthread_join(fielder_tids[i], NULL);

    int runs, wickets, overs, balls;
    get_score(&runs, &wickets, &overs, &balls);
    printf("\nInnings %d Result: %d/%d (%d.%d overs)\n",
           innings_num + 1, runs, wickets, overs, balls);
    printf("\n--- Batting Card ---\n");
    print_batting_card(batting_team, TEAM_SIZE);
    printf("\n--- Bowling Card ---\n");
    print_bowling_card(bowling_team, TEAM_SIZE);
}


int main()
{
    srand((unsigned)time(NULL));
    printf(" __       __   ______    ______    ______  \n");
    printf("|  \\     /  \\ /      \\  /      \\  /      \\ \n");
    printf("| $$\\   /  $$|  $$$$$$\\|  $$$$$$\\|  $$$$$$\\\n");
    printf("| $$$\\ /  $$$| $$  | $$| $$___\\$$| $$___\\$$\n");
    printf("| $$$$\\  $$$$| $$  | $$ \\$$    \\  \\$$    \\ \n");
    printf("| $$\\$$ $$ $$| $$  | $$ _\\$$$$$$\\ _\\$$$$$$\\\n");
    printf("| $$ \\$$$| $$| $$__/ $$|  \\__| $$|  \\__| $$\n");
    printf("| $$  \\$ | $$ \\$$    $$ \\$$    $$ \\$$    $$\n");
    printf(" \\$$      \\$$  \\$$$$$$   \\$$$$$$   \\$$$$$$ \n");
    init_pitch();
    init_scoreboard();
    init_scheduler();
    init_fielders();
    system("mkdir -p ../logs");
    init_team(team1, TEAM_SIZE);
    init_team(team2, TEAM_SIZE);

    // & Innings 1
    batting_team = team1;
    bowling_team = team2;
    reset_players(batting_team, TEAM_SIZE);
    reset_players(bowling_team, TEAM_SIZE);
    play_innings(0);
    int inn1_runs, inn1_wkts, inn1_overs, inn1_balls;
    get_score(&inn1_runs, &inn1_wkts, &inn1_overs, &inn1_balls);
    int team1_score = inn1_runs;

    // & Innings 2
    set_target(team1_score + 1);
    reset_for_second_innings();
    batting_team = team2;
    bowling_team = team1;
    reset_players(batting_team, TEAM_SIZE);
    reset_players(bowling_team, TEAM_SIZE);
    select_next_bowler(bowling_team, TEAM_SIZE);
    play_innings(1);
    int inn2_runs, inn2_wkts, inn2_overs, inn2_balls;
    get_score(&inn2_runs, &inn2_wkts, &inn2_overs, &inn2_balls);
    int team2_score = inn2_runs;

    printf("\n========================================\n");
    printf("           MATCH RESULT\n");
    printf("========================================\n");
    printf("Team 1: %d/%d (%d.%d overs)\n",
           team1_score, inn1_wkts, inn1_overs, inn1_balls);
    printf("Team 2: %d/%d (%d.%d overs)\n",
           team2_score, inn2_wkts, inn2_overs, inn2_balls);
    printf("----------------------------------------\n");

    if (team2_score > team1_score)
    {
        int wl = 10 - inn2_wkts;
        if (wl < 0) wl = 0;
        printf("Team 2 wins by %d wicket%s!\n", wl, wl == 1 ? "" : "s");
    }
    else if (team1_score > team2_score)
    {
        int rm = team1_score - team2_score;
        printf("Team 1 wins by %d run%s!\n", rm, rm == 1 ? "" : "s");
    }
    else
        printf("Match tied!\n");

    printf("========================================\n");
    printf("Full ball-by-ball log: ../logs/log.txt\n");
    return 0;
}