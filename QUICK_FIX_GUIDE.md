# Quick Fix Reference - Copy & Paste Solutions

## FIX #1: pitch.c - Simplify circular buffer read

**File**: `src/pitch.c`  
**Lines**: 58-75  
**Change**: Replace complex modulo with simple arithmetic

FROM:
```c
int read_index = (pitch_index - 1 + PITCH_SIZE) % PITCH_SIZE;
ball          = pitch_buffer[read_index];
```

TO:
```c
int read_index = pitch_index - 1;
if (read_index < 0) read_index = PITCH_SIZE - 1;
ball = pitch_buffer[read_index];
```

---

## FIX #2: batsman.c - Reset catch_taken flag

**File**: `src/players/batsman.c`  
**Lines**: 103-113  
**Change**: Add else block to reset catch_taken

FROM:
```c
            pthread_mutex_lock(&fielder_mutex);
            player *f = &bowling_team[fielder_id];
            if (!catch_taken && attempt_catch(f, r.aerial))
            {
                catch_taken       = true;
                r.wicket          = true;
                caught            = 1;
                caught_by_keeper  = f->is_keeper;
            }
            else
            {
                r.wicket = false;
                r.runs   = rand() % 3;
            }
            pthread_mutex_unlock(&fielder_mutex);
```

TO:
```c
            pthread_mutex_lock(&fielder_mutex);
            player *f = &bowling_team[fielder_id];
            if (!catch_taken && attempt_catch(f, r.aerial))
            {
                catch_taken       = true;
                r.wicket          = true;
                caught            = 1;
                caught_by_keeper  = f->is_keeper;
            }
            else
            {
                catch_taken = false;  // ← ADD THIS LINE 
                r.wicket = false;
                r.runs   = rand() % 3;
            }
            pthread_mutex_unlock(&fielder_mutex);
```

---

## FIX #3: scheduler.c - Add bounds check

**File**: `src/scheduler.c`  
**Lines**: 137-147  
**Change**: Check if next >= TEAM_SIZE

FROM:
```c
int on_wicket()
{
    pthread_mutex_lock(&scheduler_mutex);
    int next = select_next_batsman_locked(batting_team, TEAM_SIZE, &match);
    if (next == -1)
    {
        pthread_mutex_unlock(&scheduler_mutex);
        return -1;
    }
    striker_id      = next;
    next_batsman_id = next + 1;
    pthread_mutex_unlock(&scheduler_mutex);
    return next;
}
```

TO:
```c
int on_wicket()
{
    pthread_mutex_lock(&scheduler_mutex);
    int next = select_next_batsman_locked(batting_team, TEAM_SIZE, &match);
    if (next == -1 || next >= TEAM_SIZE)  // ← CHANGE THIS LINE
    {
        pthread_mutex_unlock(&scheduler_mutex);
        return -1;
    }
    striker_id      = next;
    next_batsman_id = next + 1;
    pthread_mutex_unlock(&scheduler_mutex);
    return next;
}
```

---

## FIX #4: batsman.c - Move match-over check (CRITICAL FIX) 🆕

**File**: `src/players/batsman.c`  
**Lines**: 107-130  
**Change**: Move is_match_over() check to AFTER batsman status updates

FROM (current problematic order):
```c
        pthread_mutex_lock(&score_mutex);

        if (is_match_over())
        {
            pthread_mutex_unlock(&score_mutex);
            break;  // ← BUG: exits before marking batsman out
        }

        update_batsman_stats(batsman, r.runs, true);
        update_bowler_runs(bowler, r.runs);
        match.score += r.runs;

        if (r.wicket && match.wickets < 10)
        {
            mark_batsman_out(batsman);  // ← Never reached for 10th wicket!
            match.wickets++;
            update_bowler_wicket(bowler);
            on_wicket();
        }
        else
        {
            r.wicket = false;
        }

        next_ball(true);
        pthread_mutex_unlock(&score_mutex);

        if (!r.wicket && (r.runs % 2 == 1))
            swap_strike();
```

TO (corrected order):
```c
        pthread_mutex_lock(&score_mutex);

        update_batsman_stats(batsman, r.runs, true);
        update_bowler_runs(bowler, r.runs);
        match.score += r.runs;

        if (r.wicket && match.wickets < 10)
        {
            mark_batsman_out(batsman);  // ← NOW always called
            match.wickets++;
            update_bowler_wicket(bowler);
            on_wicket();
        }
        else
        {
            r.wicket = false;
        }

        next_ball(true);
        
        // Move check here (after all updates)
        bool match_over = is_match_over();
        pthread_mutex_unlock(&score_mutex);

        if (match_over)
        {
            break;  // ← Safe to break now
        }

        if (!r.wicket && (r.runs % 2 == 1))
            swap_strike();
```

**Why this works:**
- All batsman stats are updated FIRST
- `match.wickets` is incremented before check
- `mark_batsman_out()` is called before breaking
- 10th batsman will now show as "Out" ✓

---

## FIX #5: Type name rename (4 files)

**Issue**: Field should be "batsman_type" not "batsmen_type"

### File 1: include/types.h - Line 45
FROM: `int  batsmen_type;`  
TO:   `int  batsman_type;`

### File 2: src/main.c - Line 385
FROM: `team[i].batsmen_type   = defs[i].battype;`  
TO:   `team[i].batsman_type   = defs[i].battype;`

### File 3: src/players/batsman.c - Line 126
FROM: `if (batsman->batsmen_type == BTYPE_TAIL)   wicket_prob += 8;`  
TO:   `if (batsman->batsman_type == BTYPE_TAIL)   wicket_prob += 8;`

### File 4: src/simulation/shot.c - Line 122
FROM: `if (batsman->batsmen_type == BTYPE_TAIL) wicket_prob += 8;`  
TO:   `if (batsman->batsman_type == BTYPE_TAIL) wicket_prob += 8;`

---

## FIX #6: innings_over synchronization (Optional - Advanced)

**File**: `src/main.c` + readers  
**Impact**: Prevents thread synchronization issues

### Add at top of main.c:
```c
#include <stdatomic.h>
```

### Change line 24 FROM:
```c
volatile int innings_over = 0;
```

### Change line 24 TO:
```c
_Atomic(int) innings_over = 0;
```

### Change all READS from:
```c
while (!innings_over)  // ← Bad
```

### Change all READS TO:
```c
while (!atomic_load(&innings_over))  // ← Good
```

### Change all WRITES from:
```c
innings_over = 1;  // ← Bad
```

### Change all WRITES TO:
```c
atomic_store(&innings_over, 1);  // ← Good
```

**Files to update**:
- `src/main.c`
- `src/pitch.c` (2 places)
- `src/players/batsman.c` (1 place)
- `src/players/bowler.c` (1 place)

---

## COMPILATION FIX: Windows MinGW

**Problem**: `pthread.h` not found

**Solution**: Reinstall MinGW-w64 with POSIX threads

Steps:
1. Go to https://www.mingw-w64.org/downloads/
2. Download latest installer (msys2 recommended)
3. Run installer
4. During setup, select:
   - Threads: POSIX (important!)
   - Rest: defaults OK
5. Add to PATH
6. Recompile:
   ```bash
   gcc -Wall -Wextra -O2 -pthread -Iinclude src/main.c src/**/*.c -o moss
   ```

OR use WSL:
```bash
wsl --install Ubuntu-22.04
# Then in WSL:
sudo apt install build-essential
make clean && make
```

---

## VERIFICATION

After all fixes:

```bash
# Compile
gcc -Wall -Wextra -O2 -pthread -Iinclude \
    src/main.c src/pitch.c src/scheduler.c src/scoreboard.c src/gantt.c \
    src/players/batsman.c src/players/bowler.c src/players/fielder.c \
    src/simulation/delivery.c src/simulation/shot.c src/simulation/fielding.c \
    -o moss

# Run test
./moss -R
```

Expected output: Match completes without hangs or crashes

