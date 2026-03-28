#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>

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

/* -----------------------------------------------------------------------
 *  Player definitions
 * --------------------------------------------------------------------- */
typedef struct
{
    const char *name;
    bool is_keeper;
    int  bowl;
    int  field;
    int  bat;
    bool btype;
    int  battype;
} player_def;

/* ---- India ---- */
static const player_def india_squad[TEAM_SIZE] = {
    { "R. Pant",      true,  -1, 88, 78, false, BTYPE_TOP    },
    { "V. Kohli",     false,  25, 72, 95, false, BTYPE_TOP    },
    { "R. Sharma",    false,  20, 70, 90, false, BTYPE_TOP    },
    { "S. Gill",      false,  18, 68, 82, false, BTYPE_TOP    },
    { "S. Iyer",      false,  30, 65, 76, false, BTYPE_MIDDLE },
    { "H. Pandya",    false,  72, 70, 74, false, BTYPE_MIDDLE },
    { "R. Jadeja",    false,  78, 82, 68, true,  BTYPE_MIDDLE },
    { "A. Patel",     false,  80, 70, 52, true,  BTYPE_MIDDLE },
    { "J. Bumrah",    false,  97, 60, 22, false, BTYPE_TAIL   },
    { "M. Shami",     false,  89, 58, 20, false, BTYPE_TAIL   },
    { "Y. Chahal",    false,  85, 58, 18, true,  BTYPE_TAIL   },
};

/* ---- Australia ---- */
static const player_def aus_squad[TEAM_SIZE] = {
    { "M. Wade",       true,  -1, 85, 80, false, BTYPE_TOP    },
    { "D. Warner",     false,  15, 72, 88, false, BTYPE_TOP    },
    { "A. Finch",      false,  18, 70, 84, false, BTYPE_TOP    },
    { "S. Smith",      false,  30, 68, 87, false, BTYPE_TOP    },
    { "G. Maxwell",    false,  68, 72, 82, true,  BTYPE_MIDDLE },
    { "M. Stoinis",    false,  70, 68, 74, false, BTYPE_MIDDLE },
    { "T. Head",       false,  40, 70, 78, false, BTYPE_MIDDLE },
    { "P. Cummins",    false,  88, 65, 48, false, BTYPE_MIDDLE },
    { "J. Hazlewood",  false,  87, 60, 20, false, BTYPE_TAIL   },
    { "M. Starc",      false,  92, 62, 24, false, BTYPE_TAIL   },
    { "A. Zampa",      false,  83, 58, 18, true,  BTYPE_TAIL   },
};

/* ---- Sri Lanka ---- */
static const player_def sl_squad[TEAM_SIZE] = {
    { "K. Sangakkara", true,  -1, 88, 92, false, BTYPE_TOP    },
    { "T. Dilshan",    false,  55, 72, 85, false, BTYPE_TOP    },
    { "K. Jayawardene",false,  20, 70, 90, false, BTYPE_TOP    },
    { "A. Mathews",    false,  60, 68, 82, false, BTYPE_MIDDLE },
    { "T. Perera",     false,  65, 70, 78, false, BTYPE_MIDDLE },
    { "D. Chandimal",  false,  25, 65, 74, false, BTYPE_MIDDLE },
    { "W. Hasaranga",  false,  86, 72, 58, true,  BTYPE_MIDDLE },
    { "S. Lakmal",     false,  80, 62, 22, false, BTYPE_TAIL   },
    { "L. Malinga",    false,  94, 60, 15, false, BTYPE_TAIL   },
    { "N. Pradeep",    false,  80, 58, 12, false, BTYPE_TAIL   },
    { "A. Dananjaya",  false,  82, 58, 14, true,  BTYPE_TAIL   },
};

/* ---- Pakistan ---- */
static const player_def pak_squad[TEAM_SIZE] = {
    { "M. Rizwan",    true,  -1, 85, 84, false, BTYPE_TOP    },
    { "Babar Azam",   false,  18, 72, 96, false, BTYPE_TOP    },
    { "F. Zaman",     false,  15, 68, 85, false, BTYPE_TOP    },
    { "M. Hafeez",    false,  70, 70, 82, true,  BTYPE_TOP    },
    { "S. Masood",    false,  20, 65, 76, false, BTYPE_MIDDLE },
    { "I. Wasim",     false,  75, 68, 70, false, BTYPE_MIDDLE },
    { "Shadab Khan",  false,  83, 72, 58, true,  BTYPE_MIDDLE },
    { "H. Ali",       false,  78, 64, 42, true,  BTYPE_MIDDLE },
    { "S. Afridi",    false,  88, 62, 22, false, BTYPE_TAIL   },
    { "Naseem Shah",  false,  90, 60, 15, false, BTYPE_TAIL   },
    { "H. Rauf",      false,  88, 58, 12, false, BTYPE_TAIL   },
};

/* ---- England ---- */
static const player_def eng_squad[TEAM_SIZE] = {
    { "J. Buttler",   true,  -1, 88, 90, false, BTYPE_TOP    },
    { "J. Roy",       false,  18, 70, 85, false, BTYPE_TOP    },
    { "D. Malan",     false,  20, 68, 82, false, BTYPE_TOP    },
    { "J. Root",      false,  55, 72, 91, true,  BTYPE_TOP    },
    { "E. Morgan",    false,  25, 65, 84, false, BTYPE_MIDDLE },
    { "B. Stokes",    false,  78, 75, 80, false, BTYPE_MIDDLE },
    { "M. Ali",       false,  82, 70, 62, true,  BTYPE_MIDDLE },
    { "C. Jordan",    false,  74, 65, 48, false, BTYPE_MIDDLE },
    { "J. Archer",    false,  90, 62, 22, false, BTYPE_TAIL   },
    { "M. Wood",      false,  87, 60, 18, false, BTYPE_TAIL   },
    { "A. Rashid",    false,  86, 62, 20, true,  BTYPE_TAIL   },
};

/* ---- New Zealand ---- */
static const player_def nz_squad[TEAM_SIZE] = {
    { "T. Latham",     true,  -1, 82, 82, false, BTYPE_TOP    },
    { "M. Guptill",    false,  18, 70, 86, false, BTYPE_TOP    },
    { "K. Williamson", false,  35, 72, 94, false, BTYPE_TOP    },
    { "R. Taylor",     false,  25, 68, 88, false, BTYPE_TOP    },
    { "J. Neesham",    false,  70, 70, 76, false, BTYPE_MIDDLE },
    { "D. Conway",     false,  15, 65, 80, false, BTYPE_MIDDLE },
    { "M. Santner",    false,  80, 68, 58, true,  BTYPE_MIDDLE },
    { "K. Jamieson",   false,  84, 65, 42, false, BTYPE_MIDDLE },
    { "T. Boult",      false,  91, 62, 18, false, BTYPE_TAIL   },
    { "M. Henry",      false,  83, 60, 15, false, BTYPE_TAIL   },
    { "I. Sodhi",      false,  82, 58, 14, true,  BTYPE_TAIL   },
};

/* ---- West Indies ---- */
static const player_def wi_squad[TEAM_SIZE] = {
    { "N. Pooran",    true,  -1, 85, 84, false, BTYPE_TOP    },
    { "C. Gayle",     false,  55, 68, 92, false, BTYPE_TOP    },
    { "E. Lewis",     false,  15, 70, 82, false, BTYPE_TOP    },
    { "S. Hope",      false,  12, 68, 80, false, BTYPE_TOP    },
    { "R. Chase",     false,  72, 66, 74, true,  BTYPE_MIDDLE },
    { "K. Pollard",   false,  70, 72, 82, false, BTYPE_MIDDLE },
    { "D. Bravo",     false,  75, 70, 70, false, BTYPE_MIDDLE },
    { "F. Allen",     false,  78, 65, 56, false, BTYPE_MIDDLE },
    { "K. Roach",     false,  85, 62, 18, false, BTYPE_TAIL   },
    { "S. Cottrell",  false,  82, 60, 15, false, BTYPE_TAIL   },
    { "H. Walsh Jr.", false,  80, 58, 12, true,  BTYPE_TAIL   },
};

/* ---- South Africa ---- */
static const player_def sa_squad[TEAM_SIZE] = {
    { "Q. de Kock",    true,  -1, 85, 88, false, BTYPE_TOP    },
    { "H. Amla",       false,  15, 70, 90, false, BTYPE_TOP    },
    { "T. Bavuma",     false,  18, 68, 80, false, BTYPE_TOP    },
    { "A. de Villiers",false,  25, 74, 96, false, BTYPE_TOP    },
    { "D. Miller",     false,  22, 70, 84, false, BTYPE_MIDDLE },
    { "F. du Plessis", false,  28, 72, 86, false, BTYPE_MIDDLE },
    { "A. Phehlukwayo",false,  74, 68, 60, false, BTYPE_MIDDLE },
    { "K. Rabada",     false,  92, 64, 30, false, BTYPE_MIDDLE },
    { "C. Morris",     false,  80, 62, 28, false, BTYPE_TAIL   },
    { "L. Ngidi",      false,  84, 60, 15, false, BTYPE_TAIL   },
    { "T. Shamsi",     false,  86, 60, 12, true,  BTYPE_TAIL   },
};

/* ---- Afghanistan ---- */
static const player_def afg_squad[TEAM_SIZE] = {
    { "M. Shahzad",   true,  -1, 82, 78, false, BTYPE_TOP    },
    { "H. Zazai",     false,  15, 68, 82, false, BTYPE_TOP    },
    { "R. Gurbaz",    false,  18, 66, 78, false, BTYPE_TOP    },
    { "A. Afghan",    false,  60, 68, 72, true,  BTYPE_MIDDLE },
    { "M. Nabi",      false,  78, 70, 70, false, BTYPE_MIDDLE },
    { "N. Mangal",    false,  20, 65, 68, false, BTYPE_MIDDLE },
    { "G. Naib",      false,  65, 64, 60, false, BTYPE_MIDDLE },
    { "R. Khan",      false,  98, 72, 32, true,  BTYPE_MIDDLE },
    { "Mujeeb Ur R.", false,  90, 62, 18, true,  BTYPE_TAIL   },
    { "F. Farooqi",   false,  87, 60, 12, false, BTYPE_TAIL   },
    { "Naveen-ul-H.", false,  83, 58, 10, false, BTYPE_TAIL   },
};

/* -----------------------------------------------------------------------
 *  Team registry
 * --------------------------------------------------------------------- */
#define NUM_TEAMS 9

typedef struct
{
    const char       *name;
    const char       *short_name;
    const player_def *squad;
} team_entry;

static const team_entry team_registry[NUM_TEAMS] = {
    { "India",       "IND", india_squad },
    { "Australia",   "AUS", aus_squad   },
    { "Sri Lanka",   "SRI", sl_squad    },
    { "Pakistan",    "PAK", pak_squad   },
    { "England",     "ENG", eng_squad   },
    { "New Zealand", "NZ",  nz_squad    },
    { "West Indies", "WI",  wi_squad    },
    { "South Africa","SA",  sa_squad    },
    { "Afghanistan", "AFG", afg_squad   },
};

static void load_team(player team[], const player_def defs[], int n)
{
    for (int i = 0; i < n; i++)
    {
        team[i].id             = i;
        strncpy(team[i].name, defs[i].name, sizeof(team[i].name) - 1);
        team[i].name[sizeof(team[i].name) - 1] = '\0';
        team[i].is_keeper      = defs[i].is_keeper;
        team[i].bowling_skill  = defs[i].bowl;
        team[i].fielding_skill = defs[i].field;
        team[i].batting_skill  = defs[i].bat;
        team[i].bowler_type    = defs[i].btype;
        team[i].batsmen_type   = defs[i].battype;
        team[i].played         = PLAYER_DNB;
        team[i].overs_bowled   = 0;
        team[i].runs_conceded  = 0;
        team[i].wickets_taken  = 0;
        team[i].runs_scored    = 0;
        team[i].balls_faced    = 0;
    }
}

/* -----------------------------------------------------------------------
 *  Team selection (interactive)
 * --------------------------------------------------------------------- */
static int pick_team(const char *prompt, int exclude_idx)
{
    printf("\n\033[1m\033[36m  %s\033[0m\n", prompt);
    for (int i = 0; i < NUM_TEAMS; i++)
    {
        if (i == exclude_idx) continue;
        printf("    \033[33m[%d]\033[0m  %s\n", i + 1, team_registry[i].name);
    }
    int choice = 0;
    while (choice < 1 || choice > NUM_TEAMS || choice - 1 == exclude_idx)
    {
        printf("  Enter number: ");
        fflush(stdout);
        if (scanf("%d", &choice) != 1) choice = 0;
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        if (choice < 1 || choice > NUM_TEAMS || choice - 1 == exclude_idx)
            printf("  \033[31mInvalid choice.\033[0m\n");
    }
    return choice - 1;
}

/* -----------------------------------------------------------------------
 *  Per-innings play
 * --------------------------------------------------------------------- */
const char *team1_name;
const char *team2_name;

static void play_innings(int innings_num, const char *sched_name)
{
    innings_over = 0;
    reset_pitch();
    init_batting_order();
    select_next_bowler(bowling_team, TEAM_SIZE);

    const char *bat_name  = (batting_team == team1) ? team1_name : team2_name;
    const char *bowl_name = (bowling_team == team1) ? team1_name : team2_name;

    FILE *lf = fopen(LOG_FILE, (innings_num == 0) ? "w" : "a");
    if (lf)
    {
        fprintf(lf,
            "\n"
            "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
            "  INNINGS %d  |  Batting: %-14s  |  Sched: %s\n"
            "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n",
            innings_num + 1, bat_name, sched_name);
        fclose(lf);
    }

    printf("\n\033[1m\033[36m========================================\033[0m\n");
    printf("\033[1m  INNINGS %d  |  \033[33m%s\033[0m\033[1m bat  |  \033[32m%s\033[0m\033[1m bowl\033[0m\n",
           innings_num + 1, bat_name, bowl_name);
    printf("\033[2m  Scheduling: %s\033[0m\n", sched_name);
    printf("\033[1m\033[36m========================================\033[0m\n");

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
    printf("\n\033[1mInnings %d:  %d/%d  (%d.%d overs)\033[0m\n",
           innings_num + 1, runs, wickets, overs, balls);

    /* Batting card */
    printf("\n\033[1m\033[36m  ── BATTING  (%s) ──────────────────────────────────────────────\033[0m\n", bat_name);
    print_batting_card(batting_team, TEAM_SIZE);

    /* Bowling card */
    printf("\n\033[1m\033[33m  ── BOWLING  (%s) ──────────────────────────────────────────────\033[0m\n", bowl_name);
    print_bowling_card(bowling_team, TEAM_SIZE);
    printf("\n");
}

/* -----------------------------------------------------------------------
 *  TUI file viewer (scroll with arrow keys, q to quit)
 * --------------------------------------------------------------------- */
static void tui_view_file(const char *path)
{
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    FILE *fp = fopen(path, "r");
    if (!fp)
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        printf("\n  \033[31mCannot open %s\033[0m\n", path);
        return;
    }

    char **lines = NULL;
    int nlines = 0, cap = 0;
    char buf[512];
    while (fgets(buf, sizeof(buf), fp))
    {
        int len = (int)strlen(buf);
        if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
        if (nlines >= cap) { cap = cap ? cap * 2 : 256; lines = realloc(lines, cap * sizeof(char*)); }
        lines[nlines++] = strdup(buf);
    }
    fclose(fp);

    int top = 0, rows = 35;

    while (1)
    {
        printf("\033[2J\033[H");
        printf("\033[1m\033[36m  ─── %s  (%d lines) ───────────────────────────  \033[2m↑↓ scroll  q quit\033[0m\n\n", path, nlines);
        int end = top + rows;
        if (end > nlines) end = nlines;
        for (int i = top; i < end; i++)
            printf("  \033[2m%4d\033[0m  %s\n", i + 1, lines[i]);
        /* fill blank lines if near end */
        for (int i = end - top; i < rows; i++) printf("\n");
        printf("\n\033[1m\033[33m  [↑/↓] scroll   [q] quit\033[0m\n");
        fflush(stdout);

        unsigned char ch = 0;
        read(STDIN_FILENO, &ch, 1);
        if (ch == 'q' || ch == 'Q') break;
        if (ch == 27)
        {
            unsigned char seq[2] = {0,0};
            read(STDIN_FILENO, &seq[0], 1);
            read(STDIN_FILENO, &seq[1], 1);
            if (seq[0] == '[')
            {
                if (seq[1] == 'A') { if (top > 0) top--; }
                if (seq[1] == 'B') { if (top + rows < nlines) top++; }
                if (seq[1] == '5') { top -= rows; if (top<0) top=0; } /* PgUp */
                if (seq[1] == '6') { top += rows; if (top+rows>nlines) top=nlines-rows; if(top<0)top=0; }
            }
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    printf("\033[2J\033[H");

    for (int i = 0; i < nlines; i++) free(lines[i]);
    free(lines);
}

static void prompt_view_file(const char *label, const char *path)
{
    printf("  \033[1m\033[33m[%s]\033[0m  \033[2m%s\033[0m  \033[36m— ENTER to view, any key to skip\033[0m  ",
           label, path);
    fflush(stdout);

    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    unsigned char ch = 0;
    read(STDIN_FILENO, &ch, 1);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    printf("\n");

    if (ch == '\n' || ch == '\r')
        tui_view_file(path);
}

/* -----------------------------------------------------------------------
 *  Arg parsing
 * --------------------------------------------------------------------- */
static void print_usage(const char *prog)
{
    fprintf(stderr,
        "\nUsage: %s [SCHED] [-TEAM1] [-TEAM2]\n\n"
        "  Scheduling (R=Round-Robin  P=Priority  S=SJF):\n"
        "    -R  -P  -S        same policy for both innings\n"
        "    -RP -SR -PS ...   different per innings (1st then 2nd)\n\n"
        "  Teams (optional; prompts interactively if omitted):\n"
        "    -IND  -AUS  -SRI  -PAK  -ENG\n"
        "    -NZ   -WI   -SA   -AFG\n\n"
        "  Examples:\n"
        "    %s -R               (Round-Robin both, pick teams)\n"
        "    %s -SP -IND -AUS   (SJF inn1, Priority inn2)\n\n",
        prog, prog, prog);
}

static int parse_sched_char(char c)
{
    if (c == 'R') return SCHED_RoR;
    if (c == 'P') return SCHED_PRIORITY;
    if (c == 'S') return SCHED_SJF;
    return -1;
}

static int find_team_code(const char *arg)
{
    const char *s = (arg[0] == '-') ? arg + 1 : arg;
    for (int i = 0; i < NUM_TEAMS; i++)
        if (strcasecmp(s, team_registry[i].short_name) == 0)
            return i;
    return -1;
}

/* -----------------------------------------------------------------------
 *  main
 * --------------------------------------------------------------------- */
int main(int argc, char *argv[])
{
    int sched1 = -1, sched2 = -1;
    int team1_idx = -1, team2_idx = -1;

    for (int i = 1; i < argc; i++)
    {
        char *a = argv[i];

        /* Try team first — exact match wins over sched heuristic */
        int ti = find_team_code(a);
        if (ti >= 0)
        {
            if (team1_idx < 0) team1_idx = ti;
            else               team2_idx = ti;
            continue;
        }

        /* scheduling flag: -X or -XY where X,Y in {R,P,S} */
        if (a[0] == '-' && (a[1]=='R' || a[1]=='P' || a[1]=='S')
            && (a[2]=='\0' || ((a[2]=='R'||a[2]=='P'||a[2]=='S') && a[3]=='\0')))
        {
            int s1 = parse_sched_char(a[1]);
            int s2 = (a[2] != '\0') ? parse_sched_char(a[2]) : s1;
            sched1 = s1; sched2 = s2;
            continue;
        }

        fprintf(stderr,"Unknown arg: %s\n",a);
        print_usage(argv[0]);
        return 1;
    }

    if (sched1 < 0) { fprintf(stderr,"No scheduling flag given.\n"); print_usage(argv[0]); return 1; }
    if (sched2 < 0) sched2 = sched1;

    static const char *sched_label[] = { "Round-Robin", "Priority", "SJF" };

    /* ---- Bright yellow MOSS banner ---- */
    printf("\033[1m\033[93m");
    printf(" __       __   ______    ______    ______  \n");
    printf("|  \\     /  \\ /      \\  /      \\  /      \\ \n");
    printf("| $$\\   /  $$|  $$$$$$\\|  $$$$$$\\|  $$$$$$\\\n");
    printf("| $$$\\ /  $$$| $$  | $$| $$___\\$$| $$___\\$$\n");
    printf("| $$$$\\  $$$$| $$  | $$ \\$$    \\  \\$$    \\ \n");
    printf("| $$\\$$ $$ $$| $$  | $$ _\\$$$$$$\\ _\\$$$$$$\\\n");
    printf("| $$ \\$$$| $$| $$__/ $$|  \\__| $$|  \\__| $$\n");
    printf("| $$  \\$ | $$ \\$$    $$ \\$$    $$ \\$$    $$\n");
    printf(" \\$$      \\$$  \\$$$$$$   \\$$$$$$   \\$$$$$$ \n");
    printf("\033[0m");
    printf("\033[2m  Multithreaded Over-Scheduled Simulation with Synchronization\033[0m\n\n");

    /* team selection */
    if (team1_idx < 0) team1_idx = pick_team("Select Team 1 (bats first):", -1);
    if (team2_idx < 0) team2_idx = pick_team("Select Team 2 (bats second):", team1_idx);
    if (team1_idx == team2_idx) { fprintf(stderr,"Teams must be different.\n"); return 1; }

    team1_name = team_registry[team1_idx].name;
    team2_name = team_registry[team2_idx].name;

    printf("\n\033[1m\033[36m  %s  vs  %s\033[0m\n", team1_name, team2_name);
    printf("  Innings 1 scheduling: \033[33m%s\033[0m\n", sched_label[sched1]);
    printf("  Innings 2 scheduling: \033[33m%s\033[0m\n\n", sched_label[sched2]);

    srand((unsigned)time(NULL));

    if (system("mkdir -p " LOG_DIR) != 0)
        fprintf(stderr, "warning: could not create " LOG_DIR "\n");

    init_pitch();
    init_scoreboard();
    init_scheduler();
    init_fielders();
    gantt_init();

    load_team(team1, team_registry[team1_idx].squad, TEAM_SIZE);
    load_team(team2, team_registry[team2_idx].squad, TEAM_SIZE);

    /* ---- Innings 1 ---- */
    set_scheduling_policy(sched1);
    batting_team = team1;
    bowling_team = team2;
    reset_players(batting_team, TEAM_SIZE);
    reset_players(bowling_team, TEAM_SIZE);
    play_innings(0, sched_label[sched1]);
    int inn1_runs, inn1_wkts, inn1_overs, inn1_balls;
    get_score(&inn1_runs, &inn1_wkts, &inn1_overs, &inn1_balls);

    /* ---- Innings 2 ---- */
    set_scheduling_policy(sched2);
    set_target(inn1_runs + 1);
    reset_for_second_innings();
    batting_team = team2;
    bowling_team = team1;
    reset_players(batting_team, TEAM_SIZE);
    reset_players(bowling_team, TEAM_SIZE);
    select_next_bowler(bowling_team, TEAM_SIZE);
    play_innings(1, sched_label[sched2]);
    int inn2_runs, inn2_wkts, inn2_overs, inn2_balls;
    get_score(&inn2_runs, &inn2_wkts, &inn2_overs, &inn2_balls);

    /* ---- Match result ---- */
    printf("\n\033[1m\033[36m  ══════════════════════════════════════\033[0m\n");
    printf("\033[1m              MATCH RESULT\033[0m\n");
    printf("\033[1m\033[36m  ══════════════════════════════════════\033[0m\n");
    printf("  %-15s  \033[1m%d/%d\033[0m  (%d.%d ov)\n", team1_name, inn1_runs, inn1_wkts, inn1_overs, inn1_balls);
    printf("  %-15s  \033[1m%d/%d\033[0m  (%d.%d ov)\n", team2_name, inn2_runs, inn2_wkts, inn2_overs, inn2_balls);
    printf("\033[1m\033[36m  ──────────────────────────────────────\033[0m\n");

    if (inn2_runs > inn1_runs)
    {
        int wl = 10 - inn2_wkts;
        if (wl < 0) wl = 0;
        printf("\033[1m\033[92m  ★  %s win by %d wicket%s!\033[0m\n",
               team2_name, wl, wl==1?"":"s");
    }
    else if (inn1_runs > inn2_runs)
    {
        int rm = inn1_runs - inn2_runs;
        printf("\033[1m\033[92m  ★  %s win by %d run%s!\033[0m\n",
               team1_name, rm, rm==1?"":"s");
    }
    else
        printf("\033[1m\033[93m  ★  Match tied!\033[0m\n");

    printf("\033[1m\033[36m  ══════════════════════════════════════\033[0m\n");

    /* gantt sched label e.g. "R/P" */
    char gantt_sched[8];
    char c1 = (sched1==SCHED_RoR)?'R':(sched1==SCHED_PRIORITY)?'P':'S';
    char c2 = (sched2==SCHED_RoR)?'R':(sched2==SCHED_PRIORITY)?'P':'S';
    snprintf(gantt_sched, sizeof(gantt_sched), "%c/%c", c1, c2);

    /* build match title for gantt */
    char mtitle[64];
    snprintf(mtitle, sizeof(mtitle), "%s vs %s", team1_name, team2_name);

    gantt_print(gantt_sched, mtitle,
                inn1_runs, inn1_wkts,
                inn2_runs, inn2_wkts);

    /* ---- TUI file viewers ---- */
    printf("\n\033[1m  Output files — press ENTER on any to open:\033[0m\n\n");
    prompt_view_file("MATCH LOG",   LOG_FILE);
    prompt_view_file("GANTT CHART", GANTT_TXT);
    printf("\n");

    return 0;
}