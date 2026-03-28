#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "../include/gantt.h"
#include "../include/constants.h"
#include "../include/types.h"

gantt_event gantt_log[MAX_GANTT_EVENTS];
int gantt_count      = 0;
long long gantt_start_ns = 0;

void gantt_init()
{
    gantt_count = 0;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    gantt_start_ns = ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

void gantt_record(delivery_event *ball, player *bowler, player *batsman,
                  long long consumed_ns, int over, int ball_num,
                  int runs, bool wicket, int innings)
{
    if (gantt_count >= MAX_GANTT_EVENTS) return;

    long long bowled_ns = ball->bowled_at.tv_sec * 1000000000LL
                        + ball->bowled_at.tv_nsec;

    gantt_event *e   = &gantt_log[gantt_count++];
    e->bowled_ns     = bowled_ns   - gantt_start_ns;
    e->consumed_ns   = consumed_ns - gantt_start_ns;
    e->bowler_id     = bowler->id;
    e->batsman_id    = batsman->id;
    snprintf(e->bowler_name,  sizeof(e->bowler_name),  "%s", bowler->name);
    snprintf(e->batsman_name, sizeof(e->batsman_name), "%s", batsman->name);
    e->batsman_type  = batsman->batsmen_type;
    e->over          = over;
    e->ball          = ball_num;
    e->runs          = runs;
    e->wicket        = wicket;
    e->innings       = innings;
}

// col values: 0=empty  1=top  2=middle  3=tail  4=wicket
#define BAR_W 68
#define LABEL_W 14

static void render_row(int bowler_id, int innings,
                       long long t_min, long long t_range,
                       char bar[BAR_W + 1],
                       char col[BAR_W + 1])
{
    memset(bar, ' ', BAR_W);
    bar[BAR_W] = '\0';
    memset(col, 0, BAR_W + 1);

    for (int i = 0; i < gantt_count; i++)
    {
        gantt_event *e = &gantt_log[i];
        if (e->innings != innings || e->bowler_id != bowler_id) continue;

        int left  = (int)((e->bowled_ns   - t_min) * BAR_W / t_range);
        int right = (int)((e->consumed_ns - t_min) * BAR_W / t_range);
        if (left  < 0)      left  = 0;
        if (right >= BAR_W) right = BAR_W - 1;
        if (right < left)   right = left;

        char fill  = e->wicket ? 'X' : '#';
        char c_id  = e->wicket ? 4   : (char)(e->batsman_type + 1);

        for (int c = left; c <= right; c++)
        {
            bar[c] = fill;
            col[c] = c_id;
        }
    }
}

// print the bar to terminal with ANSI color; spaces are printed raw (no escape)
static void print_colored_bar(const char *bar, const char *col, FILE *f)
{
    int in_color = 0;
    for (int i = 0; i < BAR_W; i++)
    {
        if (col[i] == 0)
        {
            if (in_color) { fprintf(f, ANSI_RESET); in_color = 0; }
            fputc(' ', f);
        }
        else
        {
            const char *esc;
            switch (col[i])
            {
                case 1:  esc = ANSI_TOP;    break;
                case 2:  esc = ANSI_MIDDLE; break;
                case 3:  esc = ANSI_TAIL;   break;
                default: esc = ANSI_WICKET; break;
            }
            // only emit escape on color change
            if (!in_color || col[i] != col[i > 0 ? i - 1 : 0])
                fprintf(f, "%s", esc);
            in_color = 1;
            fputc(bar[i], f);
        }
    }
    if (in_color) fprintf(f, ANSI_RESET);
}

static int collect_bowlers(int innings, int ids[], char names[][32])
{
    int nb = 0;
    for (int i = 0; i < gantt_count; i++)
    {
        if (gantt_log[i].innings != innings) continue;
        bool found = false;
        for (int j = 0; j < nb; j++)
            if (ids[j] == gantt_log[i].bowler_id) { found = true; break; }
        if (!found && nb < TEAM_SIZE * 2)
        {
            ids[nb] = gantt_log[i].bowler_id;
            snprintf(names[nb], 32, "%s", gantt_log[i].bowler_name);
            nb++;
        }
    }
    return nb;
}

static long long gantt_tmin()
{
    if (gantt_count == 0) return 0;
    long long mn = gantt_log[0].bowled_ns;
    for (int i = 1; i < gantt_count; i++)
        if (gantt_log[i].bowled_ns < mn) mn = gantt_log[i].bowled_ns;
    return mn;
}

static long long gantt_tmax()
{
    if (gantt_count == 0) return 1;
    long long mx = gantt_log[0].consumed_ns;
    for (int i = 1; i < gantt_count; i++)
        if (gantt_log[i].consumed_ns > mx) mx = gantt_log[i].consumed_ns;
    return mx;
}

static void middle_stats(int innings, int *cnt_out, double *avg_ms_out)
{
    long long total = 0;
    int cnt = 0;
    for (int i = 0; i < gantt_count; i++)
    {
        gantt_event *e = &gantt_log[i];
        if (e->innings != innings || e->batsman_type != BTYPE_MIDDLE) continue;
        total += e->consumed_ns - e->bowled_ns;
        cnt++;
    }
    *cnt_out    = cnt;
    *avg_ms_out = cnt > 0 ? (double)total / cnt / 1e6 : 0.0;
}

// build the axis line: "0ms      Nms      2Nms     ..." across BAR_W chars
static void build_axis(long long t_range_ms, char out[BAR_W + 1])
{
    memset(out, ' ', BAR_W);
    out[BAR_W] = '\0';

    int n_ticks = 5;
    for (int t = 0; t < n_ticks; t++)
    {
        int pos = t * (BAR_W - 1) / (n_ticks - 1);
        long long tick_ms = t_range_ms * t / (n_ticks - 1);
        char label[16];
        snprintf(label, sizeof(label), "%lldms", tick_ms);
        int len = (int)strlen(label);
        // right-align last tick so it doesn't overflow
        if (t == n_ticks - 1) pos = pos - len + 1;
        if (pos < 0) pos = 0;
        for (int k = 0; k < len && pos + k < BAR_W; k++)
            out[pos + k] = label[k];
    }
}

#define SEP_W (LABEL_W + 1 + BAR_W)

static void print_sep(FILE *f)
{
    fprintf(f, "  ");
    for (int i = 0; i < SEP_W; i++) fputc('-', f);
    fputc('\n', f);
}

static void print_innings_block(int innings, long long t_min, long long t_range,
                                const char *bat_team, FILE *term, FILE *txt)
{
    int   ids[TEAM_SIZE * 2];
    char  names[TEAM_SIZE * 2][32];
    int   nb = collect_bowlers(innings, ids, names);

    fprintf(term, ANSI_BOLD ANSI_CYAN
            "\n  Innings %d  |  Batting: %s\n"
            ANSI_RESET, innings + 1, bat_team);
    fprintf(txt, "\n  Innings %d  |  Batting: %s\n", innings + 1, bat_team);

    // axis
    char axis[BAR_W + 1];
    build_axis(t_range / 1000000LL, axis);
    fprintf(term, "  %-*s|%s\n", LABEL_W, "Bowler", axis);
    fprintf(txt,  "  %-*s|%s\n", LABEL_W, "Bowler", axis);

    print_sep(term);
    print_sep(txt);

    char bar[BAR_W + 1];
    char col[BAR_W + 1];

    for (int r = 0; r < nb; r++)
    {
        render_row(ids[r], innings, t_min, t_range, bar, col);

        fprintf(term, "  %-*s|", LABEL_W, names[r]);
        print_colored_bar(bar, col, term);
        fputc('\n', term);

        fprintf(txt, "  %-*s|%s\n", LABEL_W, names[r], bar);
    }

    print_sep(term);
    print_sep(txt);

    int mc; double mavg;
    middle_stats(innings, &mc, &mavg);
    fprintf(term, ANSI_DIM "  Middle order: %d balls, avg pitch-hold %.3f ms\n" ANSI_RESET, mc, mavg);
    fprintf(txt,  "  Middle order: %d balls, avg pitch-hold %.3f ms\n", mc, mavg);
}

// total width of chart: 2 + LABEL_W + 1 + BAR_W = 2 + 14 + 1 + 68 = 85
#define HDR_W 85

static void print_hdr_line(FILE *f, const char *left, const char *mid)
{
    // plain ASCII box — no UTF-8
    fprintf(f, "  +");
    for (int i = 0; i < HDR_W - 4; i++) fputc('-', f);
    fprintf(f, "+\n");
    (void)left; (void)mid;
}

void gantt_print(const char *sched_name,
                 int t1_runs, int t1_wkts,
                 int t2_runs, int t2_wkts)
{
    FILE *term = stdout;
    FILE *txt  = fopen(GANTT_TXT, "a");
    if (!txt) txt = stderr;

    long long t_min   = gantt_tmin();
    long long t_max   = gantt_tmax();
    long long t_range = t_max - t_min;
    if (t_range <= 0) t_range = 1;

    // --- terminal header ---
    fprintf(term, "\n" ANSI_BOLD);
    fprintf(term, "  +-------------------------------------------------------------------------------+\n");
    fprintf(term, "  |  GANTT  |  Scheduling: %-12s  |  India vs Australia              |\n", sched_name);
    fprintf(term, "  +-------------------------------------------------------------------------------+\n");
    fprintf(term, ANSI_RESET);

    // --- txt header ---
    time_t now = time(NULL);
    char tbuf[32];
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(txt, "\n");
    fprintf(txt, "  +-------------------------------------------------------------------------------+\n");
    fprintf(txt, "  |  GANTT  |  Scheduling: %-12s  |  India vs Australia  |  %s  |\n", sched_name, tbuf);
    fprintf(txt, "  +-------------------------------------------------------------------------------+\n");

    // legend
    fprintf(term, "\n  Legend: "
            ANSI_TOP    "# Top " ANSI_RESET
            ANSI_MIDDLE "# Mid " ANSI_RESET
            ANSI_TAIL   "# Tail " ANSI_RESET
            ANSI_WICKET "X Wicket" ANSI_RESET "\n");
    fprintf(txt,  "\n  Legend: # Top  # Mid  # Tail  X Wicket\n");

    print_innings_block(0, t_min, t_range, "India",     term, txt);
    print_innings_block(1, t_min, t_range, "Australia", term, txt);

    fprintf(term, "\n" ANSI_BOLD
            "  Result:  India %d/%d     Australia %d/%d\n"
            ANSI_RESET, t1_runs, t1_wkts, t2_runs, t2_wkts);
    fprintf(txt,
            "\n  Result:  India %d/%d     Australia %d/%d\n"
            "  +-------------------------------------------------------------------------------+\n\n",
            t1_runs, t1_wkts, t2_runs, t2_wkts);

    if (txt != stderr) fclose(txt);
    (void)print_hdr_line;
}