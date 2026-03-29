# T20 Simulator - Detailed Bug Analysis & Fixes

## ✅ VERIFIED: Functions That ARE Implemented

1. ✅ `gantt_print()` - IMPLEMENTED in `src/gantt.c` (lines 289-349)
2. ✅ `print_batting_card()` - IMPLEMENTED in `src/scoreboard.c` (lines 128-162)
3. ✅ `print_bowling_card()` - IMPLEMENTED in `src/scoreboard.c` (lines 164-193)
4. ✅ `update_match_intensity()` - IMPLEMENTED in `src/scheduler.c` (lines 201-204)
5. ✅ `log_event()` - IMPLEMENTED in `src/simulation/shot.c` (lines 149-199)

---

## ❌ ACTUAL BUGS FOUND

### BUG #1: Race Condition in `pipe.c` - `pitch_read()` Logic
**Severity**: MEDIUM-HIGH  
**Probability of Bug**: HIGH  
**Location**: `src/pitch.c` lines 58-75

**Problem**:
The circular buffer read logic assumes the last written element is at `(pitch_index - 1)`, but there's a potential race condition:

```c
// Current problematic code:
int read_index = (pitch_index - 1 + PITCH_SIZE) % PITCH_SIZE;
ball = pitch_buffer[read_index];  // May read wrong element!
```

**Why It's a Bug**:
- The writer updates `pitch_index` AFTER writing `ball_ready = true`
- The reader might read while writer is updating indices
- With `PITCH_SIZE = 6`, modulo arithmetic might wrap incorrectly

**Fix**:
```c
delivery_event pitch_read()
{
    delivery_event ball = {0};
    pthread_mutex_lock(&pitch_mutex);
    while (!ball_ready && !innings_over)
        pthread_cond_wait(&ball_ready_cond, &pitch_mutex);

    if (innings_over)
    {
        pthread_mutex_unlock(&pitch_mutex);
        return ball;
    }

    // Always read from the most recently written element
    // Since writer updated pitch_index AFTER write, correct index is pitch_index-1
    int read_index = (pitch_index == 0) ? PITCH_SIZE - 1 : pitch_index - 1;
    ball = pitch_buffer[read_index];
    ball_ready = false;
    ball_consumed = true;
    pthread_cond_broadcast(&ball_consumed_cond);
    pthread_mutex_unlock(&pitch_mutex);
    return ball;
}
```

---

### BUG #2: Weak Synchronization of `innings_over` Flag
**Severity**: MEDIUM  
**Probability of Bug**: MEDIUM  
**Location**: `src/main.c` line 24

**Problem**:
```c
volatile int innings_over = 0;  // ← Only volatile, no atomic guarantees
```

Used in multiple threads without mutex:
- Read in `pitch.c` lines 42, 61 without locks
- Read in `players/batsman.c`, `players/bowler.c` lines without locks

**Why It's a Bug**:
- Compiler may cache `volatile` variables
- Without `atomic` or mutex, visibility across CPU cores not guaranteed
- May cause livelocks or delayed detection of innings end

**Fix**:
Replace with atomic operations or use mutex:

**Option A - Atomic (better for flags)**:
```c
#include <stdatomic.h>
_Atomic(int) innings_over = 0;  // Use atomic

// Change reads to:
if (atomic_load(&innings_over)) break;

// Change writes to:
atomic_store(&innings_over, 1);
```

**Option B - Mutex (current style)**:
```c
// Wrap all reads/writes with scheduler_mutex
pthread_mutex_lock(&scheduler_mutex);
int over = innings_over;
pthread_mutex_unlock(&scheduler_mutex);
```

---

### BUG #3: Type Name Inconsistency `batsmen_type` vs `batsman_type`
**Severity**: LOW (works but semantically wrong)  
**Probability of Bug**: LOW  
**Location**: `include/types.h` line 45

**Problem**:
```c
int  batsmen_type;  // ← TYPO: plural "batsmen" used for singular field
```

Used throughout `src/main.c:385` and simulation code but confusing naming.

**Fix**:
```c
int  batsman_type;  // Rename to singular form
```

Then update all 3 references:
- `src/main.c` line 385: `team[i].batsmen_type` → `team[i].batsman_type`
- `src/players/batsman.c` line 126: `batsman->batsmen_type` → `batsman->batsman_type`
- `src/simulation/shot.c` line 122: `batsman->batsmen_type` → `batsman->batsman_type`
- And the constants in `gantt.c`

---

### BUG #4: Uninitialized Variables in `batsman.c` - `catch_taken` Race
**Severity**: MEDIUM  
**Probability of Bug**: MEDIUM  
**Location**: `src/players/batsman.c` lines 103-113

**Problem**:
```c
bool edge_to_keeper = (!r.wicket && rand() % 100 < 20) ||
                      (r.wicket  && rand() % 100 < 35);
if (edge_to_keeper)
    fielder_id = 0;
else
    fielder_id = select_fielder(bowling_team, TEAM_SIZE);

notify_fielder(fielder_id, r.aerial);

pthread_mutex_lock(&fielder_mutex);
player *f = &bowling_team[fielder_id];
if (!catch_taken && attempt_catch(f, r.aerial))  // ← catch_taken not reset!
{
    catch_taken = true;
    ...
}
```

**Why It's a Bug**:
- `catch_taken` is never reset to `false` after a catch
- Once a catch is taken, ALL future aerial balls are assumed caught
- `reset_fielder_state()` is called but only resets `active_fielder_id`

**Fix**:
Update `reset_fielder_state()` in `src/players/fielder.c`:

```c
void reset_fielder_state()
{
    pthread_mutex_lock(&fielder_mutex);
    active_fielder_id = -1;
    catch_taken = false;      // ← ADD THIS LINE
    // Wake all fielders so none stays stuck after innings ends
    for (int i = 0; i < TEAM_SIZE; i++)
        pthread_cond_signal(&fielder_cond[i]);
    pthread_mutex_unlock(&fielder_mutex);
}
```

---

### BUG #5: Missing Bounds Check in `scheduler.c`
**Severity**: MEDIUM  
**Probability of Bug**: MEDIUM  
**Location**: `src/scheduler.c` line 137

**Problem**:
```c
int on_wicket()
{
    pthread_mutex_lock(&scheduler_mutex);
    int next = select_next_batsman_locked(batting_team, TEAM_SIZE, &match);
    if (next == -1)
    {
        pthread_mutex_unlock(&scheduler_mutex);
        return -1;  // ✓ CHECKED
    }
    striker_id = next;  // ← But no check if next >= TEAM_SIZE!
    // ...
}
```

When all batsmen are out, `select_next_batsman_locked()` returns valid index but it's never bounds-checked elsewhere it's used.

**Fix**:
Add explicit bounds checking:

```c
int on_wicket()
{
    pthread_mutex_lock(&scheduler_mutex);
    int next = select_next_batsman_locked(batting_team, TEAM_SIZE, &match);
    if (next == -1 || next >= TEAM_SIZE)  // ← Add bounds check
    {
        pthread_mutex_unlock(&scheduler_mutex);
        return -1;
    }
    striker_id = next;
    next_batsman_id = next + 1;
    pthread_mutex_unlock(&scheduler_mutex);
    return next;
}
```

---

### BUG #6: Potential Buffer Overflow in `shot.c`
**Severity**: LOW  
**Probability of Bug**: LOW  
**Location**: `src/simulation/shot.c`

**Problem**:
String operations might not be null-terminated properly in some edge cases with `snprintf()`.

**Not a real issue** since code uses `snprintf()` correctly everywhere.

---

## 🔧 COMPILATION FIXES

### Fix #1: Windows MinGW pthread Support

**Problem**: `pthread.h` not found on Windows

**Solutions**:

1. **Use MinGW-W64 with POSIX threads**:
   ```bash
   # Install MinGW-w64 with POSIX threads (not win32)
   # During installation, choose:
   # - Threads: POSIX (not Win32)
   # - Architecture: x86_64
   ```

2. **Use WSL (Windows Subsystem for Linux)**:
   ```bash
   # Install WSL2 and use native Linux toolchain
   # Clone repo to WSL filesystem
   # Standard make will work
   ```

3. **Use native Linux/macOS**:
   ```bash
   # Standard compilation works out of the box
   gcc -Wall -Wextra -O2 -pthread -Iinclude src/**/*.c -o moss
   ```

4. **Add -lrt flag for some systems**:
   ```bash
   gcc -Wall -Wextra -O2 -pthread -lrt -Iinclude ... -o moss
   ```

---

## 📋 BUG FIX PRIORITY

### CRITICAL (Breaks functionality):
1. None - code compiles and runs (on non-Windows with pthread)

### HIGH (Wrong behavior):
1. **Bug #1**: `pitch_read()` race condition - Can cause wrong balls delivered
2. **Bug #2**: `innings_over` flag - Can cause hangs or delayed termination

### MEDIUM (Incorrect logic):
1. **Bug #4**: `catch_taken` not reset - Makes fielding progressively worse
2. **Bug #5**: Missing bounds check - Remote but possible crash

### LOW (Cosmetic/Semantic):
1. **Bug #3**: Type name inconsistency - Just a naming issue

---

## 📊 Risk Assessment

| Bug | Likelihood | Impact | Urgency |
|-----|-----------|--------|---------|
| pitch_read() race | MEDIUM | Match corruption | HIGH |
| innings_over flag | MEDIUM | Program hangs | HIGH |
| catch_taken not reset | MEDIUM | Gameplay wrong | MEDIUM |
| Bounds check missing | LOW | Rare crash | MEDIUM |
| Type name | LOW | Confusing | LOW |

---

## ✅ VERIFICATION CHECKLIST

- [x] Compilation errors identified (pthread.h missing on Windows)
- [x] Race conditions found (pitch_read, innings_over)
- [x] Memory issues identified (catch_taken, bounds)
- [x] Logic errors found (catch_taken reset missing)
- [x] All functions verified for implementation
- [x] Synchronization reviewed
- [x] Thread safety analyzed

