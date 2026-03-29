# Code Changes - Before & After Comparison

## Change #1: Fix Extra Runs Logging (CAUSE #3B)
**File**: `src/players/batsman.c`  
**Lines**: 50-70  
**Status**: ✅ FIXED

### BEFORE (BROKEN)
```c
if (ball.extra != NO_EXTRA)
{
    pthread_mutex_lock(&score_mutex);
    int extra_runs = 1;
    int bat_runs   = 0;
    if (rand() % 100 < 10) bat_runs = 1;

    match.score  += extra_runs + bat_runs;
    match.extras += extra_runs;
    update_bowler_runs(bowler, extra_runs + bat_runs);
    if (bat_runs > 0)
        update_batsman_stats(batsman, bat_runs, false);
    pthread_mutex_unlock(&score_mutex);

    log_event(log_fp, bowler, batsman, ball,
              (shot_result){.runs=bat_runs, .wicket=false, .aerial=false},
              -1, 0, false);
    fflush(log_fp);
    gantt_record(&ball, bowler, batsman, consumed_ns,
                 match.overs, match.balls, bat_runs, false, match.innings);
    continue;
}
```

**Problem**: 
- Score gets `extra_runs + bat_runs` but log and gantt only get `bat_runs`
- Results in mismatch between scorecard total and log total

### AFTER (FIXED)
```c
if (ball.extra != NO_EXTRA)
{
    pthread_mutex_lock(&score_mutex);
    int extra_runs = 1;
    int bat_runs   = 0;
    if (rand() % 100 < 10) bat_runs = 1;

    int total_runs = extra_runs + bat_runs;  // Calculate total once
    match.score  += total_runs;
    match.extras += extra_runs;
    update_bowler_runs(bowler, total_runs);
    if (bat_runs > 0)
        update_batsman_stats(batsman, bat_runs, false);
    pthread_mutex_unlock(&score_mutex);

    log_event(log_fp, bowler, batsman, ball,
              (shot_result){.runs=total_runs, .wicket=false, .aerial=false},
              -1, 0, false);
    fflush(log_fp);
    gantt_record(&ball, bowler, batsman, consumed_ns,
                 match.overs, match.balls, total_runs, false, match.innings);
    continue;
}
```

**Solution**: 
- Calculate `total_runs = extra_runs + bat_runs` once
- Use `total_runs` everywhere: score, bowler update, log, gantt
- All three tracks now match

---

## Change #2: Fix Aerial Ball Not-Caught Runs (CAUSE #3A)
**File**: `src/players/batsman.c`  
**Lines**: 88-103  
**Status**: ✅ FIXED

### BEFORE (BROKEN)
```c
pthread_mutex_lock(&fielder_mutex);
player *f = &bowling_team[fielder_id];
if (!catch_taken && attempt_catch(f, r.aerial))
{
    catch_taken       = true;
    r.wicket          = true;
    r.runs = 0;
    caught            = 1;
    caught_by_keeper  = f->is_keeper;
}
else
{
    catch_taken       = false;
    r.wicket = false;
    r.runs   = rand() % 3;  // ← BUG! Overwrites original runs
}
pthread_mutex_unlock(&fielder_mutex);
```

**Problem**:
- `play_shot()` returns correct runs (0, 4, 6, etc.)
- If ball not caught, code sets `r.runs = rand() % 3`
- This loses high-scoring shots (4s, 6s become 0-2)

### AFTER (FIXED)
```c
pthread_mutex_lock(&fielder_mutex);
player *f = &bowling_team[fielder_id];
if (!catch_taken && attempt_catch(f, r.aerial))
{
    catch_taken       = true;
    r.wicket          = true;
    r.runs            = 0;
    caught            = 1;
    caught_by_keeper  = f->is_keeper;
}
else
{
    catch_taken       = false;
    r.wicket          = false;
    /* Keep original r.runs from play_shot() - DON'T overwrite */
}
pthread_mutex_unlock(&fielder_mutex);
```

**Solution**:
- Remove the `r.runs = rand() % 3;` line
- Keep original `r.runs` from `play_shot()`
- Only set `r.runs = 0` when actually caught

---

## Change #3: Add Gantt Timestamp Range Validation (CAUSE #5)
**File**: `src/gantt.c`  
**Lines**: 68-88  
**Status**: ✅ FIXED

### BEFORE (BROKEN)
```c
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
        // ... rendering code ...
    }
}
```

**Problem**:
- If `t_range <= 0`, division causes incorrect calculations
- Bars render at wrong positions or overflow
- No bounds checking on scaling results

### AFTER (FIXED)
```c
static void render_row(int bowler_id, int innings,
                       long long t_min, long long t_range,
                       char bar[BAR_W + 1],
                       char col[BAR_W + 1])
{
    memset(bar, ' ', BAR_W);
    bar[BAR_W] = '\0';
    memset(col, 0, BAR_W + 1);

    /* Ensure minimum range to avoid division by zero */
    if (t_range <= 0) t_range = 1;

    for (int i = 0; i < gantt_count; i++)
    {
        gantt_event *e = &gantt_log[i];
        if (e->innings != innings || e->bowler_id != bowler_id) continue;

        int left  = (int)((e->bowled_ns   - t_min) * BAR_W / t_range);
        int right = (int)((e->consumed_ns - t_min) * BAR_W / t_range);
        if (left  < 0)      left  = 0;
        if (right >= BAR_W) right = BAR_W - 1;
        if (right < left)   right = left;
        // ... rendering code ...
    }
}
```

**Solution**:
- Validate `t_range` before using in division
- If `t_range <= 0`, set to 1 (minimum safe value)
- Bounds checking already present prevents overflow

---

## Files Verified (No Changes Needed)

### ✅ src/scheduler.c - CAUSE #1, #2
**Line 91-105**: `select_bowler_locked()` - Already correct ✓
```c
static int select_bowler_locked(player team[], int n)
{
    int result;
    if (scheduling_policy == SCHED_RoR)
        result = schedule_rr_locked(team, n);
    else if (scheduling_policy == SCHED_PRIORITY)
        result = schedule_priority_locked(team, n);
    else
        result = schedule_sjf_locked(team, n);
    current_bowler_id = result;
    return result;
}
```

**Line 147-163**: `select_next_batsman_locked()` - Already correct ✓
```c
static int select_next_batsman_locked(player team[], int n, scoreboard *m)
{
    int best = -1, best_score = INT_MIN;
    int intensity = compute_intensity(m);

    for (int i = next_batsman_id; i < n; i++)
    {
        if (team[i].played == PLAYER_OUT) continue;  // ← Checks for OUT status
        int score = team[i].batting_skill * 2;
        if (intensity > 2  && team[i].batsmen_type == BTYPE_MIDDLE) score += 15;
        if (intensity < 0  && team[i].batsmen_type == BTYPE_TOP)    score += 10;
        if (m->wickets < 2 && team[i].batsmen_type == BTYPE_TOP)    score += 10;
        if (m->wickets >= 7 && team[i].batsmen_type == BTYPE_TAIL)  score += 5;
        score += rand() % 3;
        if (score > best_score) { best_score = score; best = i; }
    }
    return best;
}
```

**Line 195-203**: `end_over()` - Already correct ✓
```c
void end_over(player team[], int n)
{
    pthread_mutex_lock(&scheduler_mutex);
    int tmp        = striker_id;
    striker_id     = non_striker_id;
    non_striker_id = tmp;
    select_bowler_locked(team, n);  // ← Calls bowler selection
    pthread_mutex_unlock(&scheduler_mutex);
}
```

---

### ✅ src/gantt.c - CAUSE #7
**Line 50-62**: `compute_bowler_times()` - Already correct ✓
```c
static void compute_bowler_times(int innings)
{
    for (int i = 0; i < TEAM_SIZE * 2; i++) bowler_time_ns[i] = 0;
    for (int i = 0; i < gantt_count; i++)
    {
        gantt_event *e = &gantt_log[i];
        if (e->innings != innings) continue;
        int id = e->bowler_id;
        if (id >= 0 && id < TEAM_SIZE * 2)
            bowler_time_ns[id] += e->consumed_ns - e->bowled_ns;  // ← Correct accumulation
    }
}
```

---

### ✅ src/scoreboard.c - CAUSE #6
**Line 63-70**: `reset_for_second_innings()` - Already correct ✓
```c
void reset_for_second_innings()
{
    match.innings  = 1;  // ← Sets to Innings 2
    match.score    = 0;
    match.wickets  = 0;
    match.overs    = 0;
    match.balls    = 0;
    match.extras   = 0;
}
```

---

### ✅ src/players/batsman.c - CAUSE #4
**Line 40-49**: Bowler retrieval - Already correct ✓
```c
pthread_mutex_lock(&scheduler_mutex);
int sid      = striker_id;
int bowler_i = current_bowler_id;  // ← Synchronized retrieval
pthread_mutex_unlock(&scheduler_mutex);

player *batsman = &batting_team[sid];
player *bowler  = &bowling_team[bowler_i];  // ← Consistent bowler
```

---

### ✅ src/main.c - End-of-innings cleanup
**Line 420-450**: Batsman state cleanup - Already correct ✓
```c
if (wickets >= 10)
{
    // 10 wickets fallen: keep 1 as "not out", mark rest as OUT
    int batting_count = 0;
    for (int i = 0; i < TEAM_SIZE; i++)
    {
        if (batting_team[i].played == PLAYER_BATTING)
            batting_count++;
    }
    
    if (batting_count > 1)
    {
        int found = 0;
        for (int i = 0; i < TEAM_SIZE; i++)
        {
            if (batting_team[i].played == PLAYER_BATTING)
            {
                if (found >= 1)  // Keep only 1 as not out
                    batting_team[i].played = PLAYER_OUT;
                found++;
            }
        }
    }
}
else
{
    // Fewer than 10 wickets - can have up to 2 "not out"
    int batting_count = 0;
    for (int i = 0; i < TEAM_SIZE; i++)
    {
        if (batting_team[i].played == PLAYER_BATTING)
            batting_count++;
    }
    
    if (batting_count > 2)
    {
        int found = 0;
        for (int i = 0; i < TEAM_SIZE; i++)
        {
            if (batting_team[i].played == PLAYER_BATTING)
            {
                if (found >= 2)
                    batting_team[i].played = PLAYER_OUT;
                found++;
            }
        }
    }
}
```

---

## Summary of Changes

| Category | Count | Details |
|----------|-------|---------|
| **Fixed** | 3 | Lines modified directly |
| **Verified** | 7+ | Checked and confirmed correct |
| **Total Causes Addressed** | 7 | All root causes covered |

## How to Apply These Changes

The 3 critical fixes have already been applied:
1. ✅ `src/players/batsman.c` - Extra runs logging fix
2. ✅ `src/players/batsman.c` - Aerial ball runs fix  
3. ✅ `src/gantt.c` - Timestamp validation fix

All other issues were already correctly implemented in the codebase.

## Testing

After applying these changes, compile and test:
```bash
gcc -Wall -Wextra -O2 -pthread -Iinclude src/main.c src/pitch.c \
  src/scheduler.c src/scoreboard.c src/gantt.c src/players/batsman.c \
  src/players/bowler.c src/players/fielder.c src/simulation/delivery.c \
  src/simulation/fielding.c src/simulation/shot.c -o moss

./moss -P -IND -NZ
```

**Expected Results**:
- ✓ No compilation warnings
- ✓ Log shows correct bowlers
- ✓ Scorecard totals match log
- ✓ Gantt chart displays correctly
- ✓ No phantom "not out" batsmen

**Last Updated**: March 29, 2026
