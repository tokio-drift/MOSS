# Summary of Changes Needed - All 6 Bugs

## Document: Changes Required for Complete Bug Fix

**Total Bugs**: 6 (1 HIGH severity, 4 MEDIUM, 1 LOW)  
**Lines of Code to Change**: ~50 lines total  
**Time to Implement**: ~20-30 minutes  
**Difficulty**: Low to Medium  

---

## BUG #1: Race Condition in pitch.c
**Severity**: MEDIUM-HIGH  
**File**: `src/pitch.c`  
**Lines**: 58-75  
**Changes**: 1 code block  
**Status**: CRITICAL

### What to change:
Replace complex modulo arithmetic with simple arithmetic in `pitch_read()` function

```c
// CURRENT (risky)
int read_index = (pitch_index - 1 + PITCH_SIZE) % PITCH_SIZE;

// SHOULD BE (safe)
int read_index = pitch_index - 1;
if (read_index < 0) read_index = PITCH_SIZE - 1;
```

---

## BUG #2: catch_taken Not Reset

**Severity**: MEDIUM  
**File**: `src/players/batsman.c`  
**Lines**: 93-113  
**Changes**: 1 line added  
**Status**: IMPORTANT

### What to change:
Add `catch_taken = false;` in the else block of the fielder catch logic

```c
// ADD THIS LINE in the else block:
catch_taken = false;  // Reset flag for next delivery
```

This prevents first catch from affecting all future aerial balls.

---

## BUG #3: Missing Bounds Check

**Severity**: MEDIUM  
**File**: `src/scheduler.c`  
**Lines**: 137  
**Changes**: 1 condition modified  
**Status**: DEFENSIVE

### What to change:
Add `|| next >= TEAM_SIZE` to the condition check in `on_wicket()` function

```c
// CURRENT (incomplete)
if (next == -1)

// SHOULD BE (complete)
if (next == -1 || next >= TEAM_SIZE)
```

---

## BUG #4: Batsman Status Check Too Early ⭐ NEW & CRITICAL

**Severity**: HIGH  
**File**: `src/players/batsman.c`  
**Lines**: 107-130  
**Changes**: 1 logic reordering  
**Status**: CRITICAL - THIS IS THE MAIN BUG YOU REPORTED

### What to change:
Move `is_match_over()` check to AFTER all batsman status updates

```
CURRENT WRONG ORDER:
1. Lock mutex
2. Check if match over ← WRONG PLACE
3. Break if over
4. Update batsman stats
5. Mark batsman out ← Never reached for 10th wicket!
6. Unlock mutex

CORRECT ORDER:
1. Lock mutex
2. Update batsman stats ← FIRST
3. Mark batsman out ← MUST happen for 10th wicket
4. Update wickets counter
5. Increment match.wickets
6. Check if match over ← AFTER all updates
7. Unlock mutex
8. Break if over ← AFTER critical section
```

**Key changes**:
- Delete the early `is_match_over()` block
- Move stat update code to top
- After `next_ball(true);` add:
  ```c
  bool match_over = is_match_over();
  pthread_mutex_unlock(&score_mutex);
  
  if (match_over)
  {
      break;
  }
  ```

**Result**: All 10 batsmen will now correctly show as OUT ✓

---

## BUG #5: Type Naming Inconsistency

**Severity**: LOW  
**Files**: 4 files  
**Lines**: 4 locations  
**Changes**: 4 renames  
**Status**: COSMETIC

### Files to change:

1. **include/types.h** - Line 45
   - Change: `batsmen_type` → `batsman_type`

2. **src/main.c** - Line 385
   - Change: `batsmen_type` → `batsman_type`

3. **src/players/batsman.c** - Line 126
   - Change: `batsmen_type` → `batsman_type`

4. **src/simulation/shot.c** - Line 122
   - Change: `batsmen_type` → `batsman_type`

---

## BUG #6: Weak innings_over Synchronization (Optional)

**Severity**: MEDIUM  
**Files**: 5 files  
**Lines**: ~8 locations  
**Changes**: Use atomic operations instead of volatile  
**Status**: OPTIONAL (improves robustness)

### What to change:

1. Add at top of `src/main.c`:
   ```c
   #include <stdatomic.h>
   ```

2. Change global declaration:
   ```c
   // FROM:
   volatile int innings_over = 0;
   
   // TO:
   _Atomic(int) innings_over = 0;
   ```

3. Change all reads from:
   ```c
   while (!innings_over)  // wrong
   // TO:
   while (!atomic_load(&innings_over))  // correct
   ```

4. Change all writes from:
   ```c
   innings_over = 1;  // wrong
   // TO:
   atomic_store(&innings_over, 1);  // correct
   ```

**Affected files/locations**:
- `src/main.c` - line 307
- `src/pitch.c` - line 42, 61
- `src/players/batsman.c` - line 36
- `src/players/bowler.c` - line 15

---

## Environment Issue: Windows MinGW

**Severity**: BLOCKING (prevents compilation on Windows)  
**Solution**: Install MinGW-w64 with POSIX thread support

### What to do:
1. Uninstall current MinGW
2. Download MinGW-w64 from https://www.mingw-w64.org/
3. During installation, select:
   - Architecture: x86_64
   - **Threads: POSIX** (important!)
   - Rest: defaults
4. Add to PATH
5. Recompile

OR use WSL:
```bash
wsl --install Ubuntu-22.04
```

---

## Summary Table

| Bug # | File | Lines | Severity | Priority | Type |
|-------|------|-------|----------|----------|------|
| 1 | pitch.c | 58-75 | MEDIUM-HIGH | P1 | Race condition |
| 2 | batsman.c | 93-113 | MEDIUM | P2 | Logic |
| 3 | scheduler.c | 137 | MEDIUM | P3 | Bounds check |
| 4 | batsman.c | 107-130 | HIGH | P1 | Logic ⭐ YOUR ISSUE |
| 5 | 4 files | 4 lines | LOW | P4 | Naming |
| 6 | 5 files | 8 lines | MEDIUM | P5 | Sync |

---

## Implementation Checklist

### Must Do (Before Compilation):
- [ ] Fix Bug #4 (batsman.c match-over check) - YOUR ISSUE
- [ ] Fix Bug #1 (pitch.c circular buffer)
- [ ] Fix Bug #2 (batsman.c catch_taken reset)
- [ ] Fix Bug #3 (scheduler.c bounds check)

### Should Do (Before First Run):
- [ ] Fix Bug #5 (type naming) - optional but improves code quality
- [ ] Ensure Windows MinGW has POSIX threads

### Nice to Have (Optional):
- [ ] Fix Bug #6 (atomic synchronization) - improves robustness

---

## Compilation After Fixes

```bash
# Windows (with MinGW-w64 POSIX threads)
gcc -Wall -Wextra -O2 -pthread -Iinclude src/main.c src/**/*.c -o moss

# WSL/Linux/Mac
make clean && make
```

---

## Testing After Fixes

Run these test cases:

1. **Test full innings (10 wickets)**:
   ```bash
   ./moss -R -IND -AUS
   # Check: Final batsman shows "Out", not "Not Out"
   ```

2. **Test different scheduling**:
   ```bash
   ./moss -P -ENG -PAK
   ./moss -S -SRI -SA
   ```

3. **Check output**:
   ```bash
   # View batting card
   grep -A 15 "BATTING" logs/log.txt
   # Should show all 10 out (or 9 out + 1 keeper)
   ```

---

## Most Important Fix

**BUG #4** is the one YOU reported - batsmen showing "Not Out" when all are out.

**Location**: `src/players/batsman.c` lines 107-130  
**Root Cause**: Match-over check happens BEFORE marking batsman out  
**Fix**: Reorder code to check match-over AFTER updating stats

This single fix will solve your reported issue!

---

## Questions About Changes?

Refer to these documents:
- `BUG_FIX_6_BATSMAN_STATUS.md` - Detailed explanation of BUG #4
- `QUICK_FIX_GUIDE.md` - Copy-paste ready code changes
- `DETAILED_FIXES.md` - Technical deep dives
- `COMPLETE_DEBUG_REPORT.txt` - Full analysis

---

**All changes are non-breaking and improve code correctness!**

