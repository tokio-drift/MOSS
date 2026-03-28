#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#include "../include/gantt.h"

player team1[TEAM_SIZE];
player team2[TEAM_SIZE];

player *batting_team;
player *bowling_team;

volatile int innings_over = 0;

typedef struct
{
    const char *name;
    bool is_keeper;
    int  bowl;
    int  field;
    int  bat;
    bool btype;      // false=pace, true=spin
    int  battype;    // BTYPE_TOP / MIDDLE / TAIL
} player_def;

static const player_def india_squad[TEAM_SIZE] = {
    { "R. Pant",    true,  -1, 88, 78, false, BTYPE_TOP    },
    { "V. Kohli",   false,  25, 72, 92, false, BTYPE_TOP    },
    { "R. Sharma",  false,  20, 70, 88, false, BTYPE_TOP    },
    { "S. Gill",    false,  18, 68, 82, false, BTYPE_TOP    },
    { "S. Iyer",    false,  30, 65, 76, false, BTYPE_MIDDLE },
    { "H. Pandya",  false,  72, 70, 74, false, BTYPE_MIDDLE },
    { "R. Jadeja",  false,  78, 82, 68, true,  BTYPE_MIDDLE },
    { "A. Patel",   false,  80, 70, 52, true,  BTYPE_MIDDLE },
    { "J. Bumrah",  false,  97, 60, 22, false, BTYPE_TAIL   },
    { "M. Shami",   false,  89, 58, 20, false, BTYPE_TAIL   },
    { "Y. Chahal",  false,  85, 58, 18, true,  BTYPE_TAIL   },
};

static const player_def aus_squad[TEAM_SIZE] = {
    { "M. Wade",    true,  -1, 85, 80, false, BTYPE_TOP    },
    { "D. Warner",  false,  15, 72, 88, false, BTYPE_TOP    },
    { "A. Finch",   false,  18, 70, 84, false, BTYPE_TOP    },
    { "S. Smith",   false,  30, 68, 87, false, BTYPE_TOP    },
    { "G. Maxwell", false,  68, 72, 82, true,  BTYPE_MIDDLE },
    { "M. Stoinis", false,  70, 68, 74, false, BTYPE_MIDDLE },
    { "T. Head",    false,  40, 70, 78, false, BTYPE_MIDDLE },
    { "P. Cummins", false,  88, 65, 48, false, BTYPE_MIDDLE },
    { "J. Hazlewood",false, 87, 60, 20, false, BTYPE_TAIL   },
    { "M. Starc",   false,  92, 62, 24, false, BTYPE_TAIL   },
    { "A. Zampa",   false,  83, 58, 18, true,  BTYPE_TAIL   },
};

static void load_team(player team[], const player_def defs[], int n)
{
    for (int i = 0; i < n; i++)
    {
        team[i].id            = i;
        strncpy(team[i].name, defs[i].name, sizeof(team[i].name) - 1);
        team[i].name[sizeof(team[i].name) - 1] = '\0';
        team[i].is_keeper     = defs[i].is_keeper;
        team[i].bowling_skill = defs[i].bowl;
        team[i].fielding_skill= defs[i].field;
        team[i].batting_skill = defs[i].bat;
        team[i].bowler_type   = defs[i].btype;
        team[i].batsmen_type  = defs[i].battype;

        team[i].played        = PLAYER_DNB;
        team[i].overs_bowled  = 0;
        team[i].runs_conceded = 0;
        team[i].wickets_taken = 0;
        team[i].runs_scored   = 0;
        team[i].balls_faced   = 0;
    }
}

static void play_innings(int innings_num, const char *sched_name)
{
    innings_over = 0;
    reset_pitch();
    init_batting_order();
    select_next_bowler(bowling_team, TEAM_SIZE);

    const char *bat_name = (batting_team == team1) ? "India" : "Australia";

    FILE *lf = fopen(LOG_FILE, (innings_num == 0) ? "w" : "a");
    if (lf)
    {
        fprintf(lf,
                "\n"
                "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
                "  INNINGS %d  |  Batting: %s  |  Sched: %s\n"
                "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n",
                innings_num + 1, bat_name, sched_name);
        fclose(lf);
    }

    printf("\n========================================\n");
    printf("  INNINGS %d  |  Batting: %s\n", innings_num + 1, bat_name);
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

    while (1)
    {
        pthread_mutex_lock(&score_mutex);
        int over = is_match_over();
        pthread_mutex_unlock(&score_mutex);
        if (over) break;
        struct timespec ts = {0, 1000000};
        nanosleep(&ts, NULL);
    }

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

static void print_usage(const char *prog)
{
    fprintf(stderr, "Usage: %s -R|-P|-S\n", prog);
    fprintf(stderr, "  -R  Round-Robin bowler scheduling\n");
    fprintf(stderr, "  -P  Priority-based bowler scheduling\n");
    fprintf(stderr, "  -S  Shortest-Job-First bowler scheduling\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        print_usage(argv[0]);
        return 1;
    }

    int policy;
    const char *sched_name;

    if      (strcmp(argv[1], "-R") == 0) { policy = SCHED_RoR;      sched_name = "Round-Robin"; }
    else if (strcmp(argv[1], "-P") == 0) { policy = SCHED_PRIORITY;  sched_name = "Priority";    }
    else if (strcmp(argv[1], "-S") == 0) { policy = SCHED_SJF;       sched_name = "SJF";         }
    else
    {
        fprintf(stderr, "Unknown flag: %s\n", argv[1]);
        print_usage(argv[0]);
        return 1;
    }

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
    printf("\nIndia vs Australia  |  Scheduling: %s\n", sched_name);

    if (system("mkdir -p " LOG_DIR) != 0)
        fprintf(stderr, "warning: could not create " LOG_DIR "\n");

    init_pitch();
    init_scoreboard();
    init_scheduler();
    init_fielders();
    gantt_init();
    set_scheduling_policy(policy);

    load_team(team1, india_squad, TEAM_SIZE);
    load_team(team2, aus_squad,   TEAM_SIZE);

    // innings 1
    batting_team = team1;
    bowling_team = team2;
    reset_players(batting_team, TEAM_SIZE);
    reset_players(bowling_team, TEAM_SIZE);
    play_innings(0, sched_name);
    int inn1_runs, inn1_wkts, inn1_overs, inn1_balls;
    get_score(&inn1_runs, &inn1_wkts, &inn1_overs, &inn1_balls);

    // innings 2
    set_target(inn1_runs + 1);
    reset_for_second_innings();
    batting_team = team2;
    bowling_team = team1;
    reset_players(batting_team, TEAM_SIZE);
    reset_players(bowling_team, TEAM_SIZE);
    select_next_bowler(bowling_team, TEAM_SIZE);
    play_innings(1, sched_name);
    int inn2_runs, inn2_wkts, inn2_overs, inn2_balls;
    get_score(&inn2_runs, &inn2_wkts, &inn2_overs, &inn2_balls);

    printf("\n========================================\n");
    printf("           MATCH RESULT\n");
    printf("========================================\n");
    printf("India:     %d/%d (%d.%d overs)\n", inn1_runs, inn1_wkts, inn1_overs, inn1_balls);
    printf("Australia: %d/%d (%d.%d overs)\n", inn2_runs, inn2_wkts, inn2_overs, inn2_balls);
    printf("----------------------------------------\n");

    if (inn2_runs > inn1_runs)
    {
        int wl = 10 - inn2_wkts;
        if (wl < 0) wl = 0;
        printf("Australia wins by %d wicket%s!\n", wl, wl == 1 ? "" : "s");
    }
    else if (inn1_runs > inn2_runs)
    {
        int rm = inn1_runs - inn2_runs;
        printf("India wins by %d run%s!\n", rm, rm == 1 ? "" : "s");
    }
    else
        printf("Match tied!\n");

    printf("========================================\n");

    gantt_print(sched_name,
                inn1_runs, inn1_wkts,
                inn2_runs, inn2_wkts);

    printf("\nLog: %s  |  Gantt: %s\n", LOG_FILE, GANTT_TXT);
    return 0;
}