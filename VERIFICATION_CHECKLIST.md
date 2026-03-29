# Complete Verification Checklist

## Project: T20 Cricket Simulator
**Date**: March 29, 2026  
**Status**: All fixes applied and ready for testing

---

## Phase 1: Code Inspection ✅

- [x] CAUSE #1: Bowler rotation logic - VERIFIED CORRECT
- [x] CAUSE #2: Batsman selection with OUT validation - VERIFIED CORRECT  
- [x] CAUSE #3A: Aerial catch runs handling - FIXED ✓
- [x] CAUSE #3B: Extra runs logging - FIXED ✓
- [x] CAUSE #4: Gantt bowler ID consistency - VERIFIED CORRECT
- [x] CAUSE #5: Timestamp range validation - FIXED ✓
- [x] CAUSE #6: Innings reset logic - VERIFIED CORRECT
- [x] CAUSE #7: Bowler time accumulation - VERIFIED CORRECT
- [x] End-of-innings batsman cleanup - VERIFIED CORRECT
- [x] Initial batsman state setup - VERIFIED CORRECT
- [x] On-wicket batsman transition - VERIFIED CORRECT

---

## Phase 2: Compilation Testing

### Manual Compilation Test
```bash
cd c:\Users\parsh\OS-T20

gcc -Wall -Wextra -O2 -pthread -Iinclude \
  src/main.c src/pitch.c src/scheduler.c src/scoreboard.c src/gantt.c \
  src/players/batsman.c src/players/bowler.c src/players/fielder.c \
  src/simulation/delivery.c src/simulation/fielding.c src/simulation/shot.c \
  -o moss
```

### Expected Result
- ✓ No errors
- ✓ No warnings (or only trivial warnings)
- ✓ Executable "moss" created successfully

### Checklist
- [ ] Command runs without errors
- [ ] moss executable created
- [ ] Size is reasonable (>100KB)
- [ ] Timestamp is current

---

## Phase 3: Runtime Testing

### Test Case 1: Simple Round-Robin Match
```bash
./moss -R -IND -AUS
```

**Verify**:
- [ ] Match starts without crashes
- [ ] Both innings complete
- [ ] No threading errors
- [ ] Output completes cleanly

### Test Case 2: Priority Scheduling  
```bash
./moss -P -NZ -PAK
```

**Verify**:
- [ ] Match runs smoothly
- [ ] Bowlers rotate correctly
- [ ] No deadlocks
- [ ] Final scores displayed

### Test Case 3: Mixed Scheduling (Challenge Test)
```bash
./moss -RP -ENG -SA
```

**Verify**:
- [ ] Innings 1: Round-Robin policy active
- [ ] Innings 2: Priority policy active
- [ ] State reset correctly between innings
- [ ] No state corruption

---

## Phase 4: File Output Verification

### Check Log File Exists
```bash
ls -la logs/log.txt
```
- [ ] File exists
- [ ] Recent timestamp
- [ ] File size > 1KB

### Check Log Content Format
```bash
head -30 logs/log.txt
```

**Expected Format**:
```
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  INNINGS 1  |  Batting: ...
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Over 0.1: Bowler: T. Boult (bowling_team[8]) vs Batsman: V. Kohli  runs: 4
Over 0.2: Bowler: T. Boult ...
```

- [ ] Deliveries logged sequentially
- [ ] Bowler and batsman names shown
- [ ] Runs recorded for each delivery
- [ ] Valid bowler names (not PLAYER_DNB placeholders)

### Check Gantt File Exists
```bash
ls -la logs/gantt.txt
```
- [ ] File exists
- [ ] Recent timestamp

### Check Log Runs Total
```bash
# PowerShell
$total = (Get-Content logs/log.txt | Select-String "runs: \d+" -AllMatches | 
  ForEach-Object { $_.Matches.Value -replace 'runs: ' } | 
  Measure-Object -Sum).Sum
Write-Host "Total runs: $total"
```

---

## Phase 5: Output Display Verification

### Test Match: IND vs NZ with Priority
```bash
./moss -P -IND -NZ 2>&1 | tee match_output.txt
```

### Verify Batting Card (Innings 1)
Look for:
```
── BATTING (India) ───────────────────
  Player              Status    Runs  Balls  ...
  V. Kohli            not out    42     30
  R. Sharma           out        28     22
  ...
```

**Checklist**:
- [ ] All batsmen shown (11 players)
- [ ] Correct names from India squad
- [ ] Status shows "not out" or "out" (not "DNB" unless never batted)
- [ ] Runs and balls are reasonable
- [ ] Maximum 10 "out" (10 wickets fallen)
- [ ] Maximum 2 "not out" if < 10 wickets, 1 if 10 wickets

### Verify Bowling Card (Innings 1)
Look for:
```
── BOWLING (New Zealand) ───────────────────
  Player              Overs  Runs  Wickets  ...
  T. Boult            4.0    28     2
  K. Jamieson         3.0    24     1
  ...
```

**Checklist**:
- [ ] All bowlers shown (11 players)
- [ ] Correct names from New Zealand squad
- [ ] Overs are integers (0, 1, 2, etc.)
- [ ] Runs match log file total
- [ ] Wickets >= 0 and reasonable
- [ ] Overs bowled total ~20 (one full innings)

### Verify Gantt Chart Display
Look for ASCII art like:
```
Innings 1:
  Bowler          |###################...X##########
  T. Boult        |████████████████
  K. Jamieson     |    ██████████
  M. Henry        |        ██
  ...
```

**Checklist**:
- [ ] Chart displays without errors
- [ ] Bowler names are correct (from New Zealand)
- [ ] Bars progress left to right
- [ ] 'X' marks appear for wickets
- [ ] Bars don't overflow the display
- [ ] Coverage is roughly 120 deliveries (20 overs)

---

## Phase 6: Data Consistency Verification

### Test 1: Runs Totality Check
```bash
# Extract Cricket details from output
./moss -P -IND -NZ 2>&1 | grep -E "INNINGS|runs|wickets|overs"

# Manually verify:
# 1. Extract final score from display
# 2. Count total runs in log.txt
# 3. They should match exactly
```

**Checklist**:
- [ ] Display score = sum of all delivered runs in log
- [ ] No missing deliveries
- [ ] Extras counted correctly

### Test 2: Bowler Consistency Check
```bash
# Extract bowlers from first 5 overs in log
grep "Over [01234]\." logs/log.txt | awk '{print $3}' | sort | uniq -c

# Verify each bowler appears ~6 times (6 deliveries per over)
# and bowlers from correct team (New Zealand)
```

**Checklist**:
- [ ] Each bowler in first 5 overs appears ~6 times
- [ ] All bowlers are from bowling team
- [ ] No batsmen appear as bowlers
- [ ] Consistent with Gantt display

### Test 3: Wickets Check
```bash
# Count wickets from log
grep "wicket" logs/log.txt | wc -l

# This should <= 10 (max 10 wickets in T20)
# Should match scorecard wickets count
```

**Checklist**:
- [ ] Wicket count ≤ 10
- [ ] Matches display scorecard
- [ ] Dismissed batsmen marked as "out"

### Test 4: Batsman Status Check
**Expected**:
- Innings 1 (Batting team India):
  - Striker (player 0): "not out" ✓
  - Non-striker (player 1): "not out" ✓  
  - 10 - (wickets fallen): marked "out"
  - Rest: "DNB" (did not bat)

```bash
# Check batting card display carefully
```

**Checklist**:
- [ ] At least 2 "not out" if < 10 wickets
- [ ] At least 1 "not out" if 10 wickets  
- [ ] Exactly X "out" where X = wickets fallen
- [ ] No contradictions

### Test 5: Gantt Innings Separation
```bash
# Check if innings are clearly separated in gantt output
grep -E "Innings|Over" logs/gantt.txt | head -20
```

**Checklist**:
- [ ] Innings 1 section clearly marked
- [ ] Innings 2 section (if applicable) clearly marked
- [ ] Deliveries don't bleed between innings
- [ ] Each section shows ~120 deliveries (20 overs)

---

## Phase 7: Edge Cases & Stress Tests

### Test: Out on Delivery 120 (20 overs complete)
```bash
./moss -R -IND -ENG
```
- [ ] Innings ends after 20 overs
- [ ] Final batsman marked appropriately
- [ ] No index out of bounds errors

### Test: All out before 20 overs
```bash
./moss -P -IND -AUS
```
- [ ] Innings ends on 10 wickets
- [ ] Only 1 "not out" displayed
- [ ] Correct overs/balls shown

### Test: Second Innings Reset
```bash
./moss -S -NZ -PAK -RP
```
- [ ] Innings 1 completes correctly
- [ ] Score properly reset for Innings 2
- [ ] Second innings starts fresh
- [ ] Different scheduling policy (if applicable)

---

## Phase 8: Performance & Stability

### Check CPU Usage
- [ ] No excessive spinning
- [ ] Completes in reasonable time (~10-30 seconds)
- [ ] Responsive to Ctrl+C

### Check Memory
- [ ] No memory leaks (valgrind if available)
- [ ] Handles 22 threads smoothly

### Check Threading
- [ ] All thread joins complete
- [ ] No deadlocks
- [ ] Proper mutex usage

---

## Final Verification Checklist

**Pre-Deployment**:
- [ ] Code compiles with -Wall -Wextra with no errors
- [ ] All 3 key changes applied (batsman.c ×2, gantt.c ×1)
- [ ] All verification phases passed
- [ ] No runtime crashes
- [ ] Output files generated correctly

**Data Quality**:
- [ ] Scorecard totals match log file
- [ ] Bowler names correct (no batsmen as bowlers)
- [ ] Batsman statuses consistent
- [ ] Wickets count correct
- [ ] Gantt chart matches log order

**Functional**:
- [ ] Match completes both innings
- [ ] All threads complete cleanly
- [ ] No infinite loops or hangs
- [ ] Multiple runs produce consistent results

---

## How to Run Full Verification

```bash
# 1. Compile
cd c:\Users\parsh\OS-T20
gcc -Wall -Wextra -O2 -pthread -Iinclude src/main.c src/pitch.c \
  src/scheduler.c src/scoreboard.c src/gantt.c src/players/batsman.c \
  src/players/bowler.c src/players/fielder.c src/simulation/delivery.c \
  src/simulation/fielding.c src/simulation/shot.c -o moss

# 2. Run Tests
./moss -P -IND -NZ        # Priority
./moss -R -AUS -ENG       # Round-Robin
./moss -S -NZ -PAK        # SJF

# 3. Verify Outputs
# Check logs/
# Check final scores
# Check gantt display
# Verify no crashes
```

---

## Success Criteria

✅ **ALL CRITERIA MUST BE MET**:

1. ✓ Code compiles without errors
2. ✓ Code runs without crashes (3 test cases)
3. ✓ Log file shows correct bowler names
4. ✓ Scorecard batting totals match log
5. ✓ Batsman statuses consistent
6. ✓ Gantt chart displays correctly
7. ✓ No "phantom not out" players
8. ✓ Runs totals match exactly
9. ✓ All threads complete cleanly
10. ✓ Output repeatable across runs

---

## Troubleshooting

### If Compilation Fails
- Check all includes are correct
- Verify pthread library linked
- Run: `gcc -Wall -Wextra -O2 -pthread ...`

### If Scores Don't Match
- Check log.txt has all deliveries
- Verify extra runs are being counted
- Look for missing runs in log

### If Gantt Shows Wrong Bowlers
- Compare bowler names in log vs gantt
- Check current_bowler_id synchronization
- Verify select_bowler_locked() is called

### If Threading Hangs
- Check mutex lock/unlock pairs
- Verify condition variables signaled
- Check innings_over flag usage

---

## Sign-Off

- [ ] Developer has reviewed all changes
- [ ] All verification tests passed
- [ ] Ready for production deployment
- [ ] Date: _______________

---

**Status**: 🟢 READY FOR TESTING

**Created**: March 29, 2026  
**By**: GitHub Copilot
