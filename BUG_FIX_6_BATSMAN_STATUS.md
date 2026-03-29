# BUG FIX #6: Batsmen Status Showing "Not Out" When All Wickets Are Down

**Date Found**: March 29, 2026  
**Severity**: HIGH  
**Impact**: Final batsman(s) not marked as OUT when innings ends at 10 wickets  
**Files Affected**: `src/players/batsman.c`

---

## Problem Description

When an innings ends due to all 10 wickets falling, the final batsman(s) are showing as "Not Out" in the log instead of being marked "Out".

### Current Behavior
```
Batsman Status Display (INCORRECT):
- First 9 batsmen: OUT ✓ (correctly marked)
- Final batsman #10: NOT OUT ❌ (SHOULD BE OUT)
- Result: Shows 10/9 instead of 10/10 (all out)
```

### Expected Behavior
```
Batsman Status Display (CORRECT):
- All 10 batsmen: OUT ✓
- Result: Shows 10/10 (all out)
```

---

## Root Cause Analysis

**Location**: `src/players/batsman.c` lines 107-129

**The Bug**: The function checks if the match is over BEFORE marking the batsman as out.

### Current Problematic Code Flow:

```c
while (!innings_over)
{
    // ... get ball, get shot result ...
    
    pthread_mutex_lock(&score_mutex);
    
    // LINE 107: CHECK MATCH OVER FIRST (TOO EARLY!)
    if (is_match_over())
    {
        pthread_mutex_unlock(&score_mutex);
        break;  // ← EXITS HERE WITHOUT MARKING OUT!
    }
    
    // These lines never execute when wickets == 10:
    update_batsman_stats(batsman, r.runs, true);
    update_bowler_runs(bowler, r.runs);
    match.score += r.runs;
    
    // THIS NEVER GETS CALLED FOR THE 10TH WICKET:
    if (r.wicket && match.wickets < 10)
    {
        mark_batsman_out(batsman);  // ← NEVER REACHED!
        match.wickets++;
        update_bowler_wicket(bowler);
        on_wicket();
    }
}
```

**When the 10th wicket falls:**
1. `r.wicket = true` (batsman dismissed)
2. `match.wickets` becomes 10
3. `is_match_over()` returns TRUE (because `match.wickets >= 10`)
4. Function **breaks BEFORE** calling `mark_batsman_out(batsman)`
5. Batsman stays in `PLAYER_DNB` or pre-batting state
6. Log shows "Not Out" instead of "Out"

---

## Solution

### Change Required in: `src/players/batsman.c`

**What to do**: Move the `is_match_over()` check to AFTER all batsman status updates.

**Lines to modify**: Lines 107-130 (approximately)

### Step-by-Step Changes:

#### Current Structure (WRONG):
```
Lock mutex
├─ Check if match over ← WRONG PLACE
├─ Break if over
├─ Update batsman stats
├─ Update bowler stats
├─ Mark batsman out ← Never reached for 10th wicket
├─ Update wickets counter
└─ Unlock mutex
```

#### New Structure (CORRECT):
```
Lock mutex
├─ Update batsman stats ← FIRST
├─ Update bowler stats
├─ Mark batsman out ← MUST happen before check
├─ Update wickets counter
├─ Check if match over ← AFTER all updates
└─ Unlock mutex
Break if over (AFTER critical section)
```

---

## Exact Code Changes Needed

### File: `src/players/batsman.c`

**SECTION TO FIND** (around line 107-130):
```c
        pthread_mutex_lock(&score_mutex);

        if (is_match_over())
        {
            pthread_mutex_unlock(&score_mutex);
            break;
        }

        update_batsman_stats(batsman, r.runs, true);
        update_bowler_runs(bowler, r.runs);
        match.score += r.runs;

        if (r.wicket && match.wickets < 10)
        {
            mark_batsman_out(batsman);
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

**CHANGE TO** (reorder the logic):
```c
        pthread_mutex_lock(&score_mutex);

        update_batsman_stats(batsman, r.runs, true);
        update_bowler_runs(bowler, r.runs);
        match.score += r.runs;

        if (r.wicket && match.wickets < 10)
        {
            mark_batsman_out(batsman);
            match.wickets++;
            update_bowler_wicket(bowler);
            on_wicket();
        }
        else
        {
            r.wicket = false;
        }

        next_ball(true);
        
        // MOVE THE CHECK HERE (after all updates)
        bool match_over = is_match_over();
        pthread_mutex_unlock(&score_mutex);

        if (match_over)
        {
            break;
        }

        if (!r.wicket && (r.runs % 2 == 1))
            swap_strike();
```

---

## Why This Fix Works

1. **All updates happen first**: `match.wickets` is incremented BEFORE checking
2. **Batsman marked out**: `mark_batsman_out()` is called before the check
3. **Match-over check is accurate**: When `is_match_over()` is called, it sees all 10 wickets
4. **Break happens at safe point**: After all critical updates complete
5. **No race condition**: All updates are inside mutex

---

## Additional Improvement (Optional)

### Ensure Batting Status is Set on First Ball

**File**: `src/players/batsman.c`  
**Around line**: 49 (after getting batsman)

**Current code:**
```c
        player *batsman = &batting_team[sid];
        player *bowler  = &bowling_team[bowler_i];

        if (ball.extra != NO_EXTRA)
```

**Could add** (to track when batsman starts batting):
```c
        player *batsman = &batting_team[sid];
        player *bowler  = &bowling_team[bowler_i];

        // Mark batsman as batting on first delivery
        if (batsman->played == PLAYER_DNB)
        {
            pthread_mutex_lock(&score_mutex);
            batsman->played = PLAYER_BATTING;
            pthread_mutex_unlock(&score_mutex);
        }

        if (ball.extra != NO_EXTRA)
```

This ensures:
- Batsman transitions: `PLAYER_DNB` → `PLAYER_BATTING` → `PLAYER_OUT`
- Proper state tracking throughout innings
- Accurate status in batting card

---

## Testing After Fix

### Test Case 1: Full Innings (10 Wickets)
```bash
./moss -R -IND -AUS
# Check output:
# Should show: "IND 120/10 (19.5 overs)" - all 10 wickets
# Check log: All 11 players should have status (10 out, 1 keeper)
```

### Test Case 2: Check Batting Card
```bash
# View logs/log.txt
# Look for section: "── BATTING (Team Name) ──"
# Verify:
# - Wickets column shows 10 out batsmen
# - Final batsman shows "out" not "not out"
```

### Test Case 3: Multiple Runs
```bash
./moss -P -ENG -PAK
./moss -S -SRI -SA
# Each should show correct final batsman status
```

---

## Related Issues (Already Documented)

This bug is separate from:
- Bug #3 (catch_taken not reset) - Different issue
- Bug #1 (pitch_read race) - Different module
- Bug #2 (innings_over sync) - Different sync issue

This is **BUG #6** - New bug in batsman status tracking logic.

---

## Summary of Changes

| Item | Details |
|------|---------|
| **File** | `src/players/batsman.c` |
| **Lines** | 107-130 (approximately) |
| **Change Type** | Logic reordering |
| **Lines Added** | 4 |
| **Lines Removed** | 0 |
| **Complexity** | Low (just reordering) |
| **Risk** | Very low (improves logic) |
| **Testing** | Simple - run match and check status |

---

## Manual Implementation Steps

1. Open `src/players/batsman.c`
2. Find the `pthread_mutex_lock(&score_mutex);` on line 107
3. Delete the `if (is_match_over())` block (lines 107-111)
4. Move stat update code to the top (lines 113-117)
5. Move mark_batsman_out logic (lines 119-127)
6. After `next_ball(true);` add:
   ```c
   bool match_over = is_match_over();
   pthread_mutex_unlock(&score_mutex);
   
   if (match_over)
   {
       break;
   }
   ```
7. Keep the `swap_strike()` logic below that
8. Compile and test

---

## Expected Output After Fix

### Before Fix (Incorrect)
```
Innings 1: 234/9 (20.0 overs)
Final Batsman: "Not Out" ❌

Bowling Card shows bowler with 9 wickets
```

### After Fix (Correct)
```
Innings 1: 234/10 (19.5 overs)
Final Batsman: "Out" ✓

Bowling Card shows bowler with 10 wickets
```

---

## Files With This Document

This fix should be added to the main bug list as **BUG #6**:
- Update `COMPLETE_DEBUG_REPORT.txt`
- Update `QUICK_FIX_GUIDE.md`
- Add to `DEBUG_FILES_INDEX.md`

