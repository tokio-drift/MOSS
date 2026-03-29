# MOSS: Multithreaded Over-Scheduled Simulation with Synchronization

**MOSS** is a high-performance **T20 Cricket Simulator** in C using POSIX threads. It maps cricket entities to OS concepts: players are threads, the pitch is a critical section (shared buffer), runs are atomic operations, and match flow uses custom scheduling algorithms (Round-Robin, Priority, SJF).

---

## Core Variables & Cricket Significance

MOSS uses player attributes and match state to calculate realistic ball, shot, and catch outcomes:

| Variable | Type | Range | Cricket Importance |
|:---|:---|:---|:---|
| `batting_skill` | int | 0-100 | Batting ability (hit selection, defense, aggression) |
| `bowling_skill` | int | 0-100 | Bowling precision (line/length control, fewer wides) |
| `fielding_skill` | int | 0-100 | Catch success rate (athletic ability, positioning) |
| `bowler_type` | bool | 0/1 | Pacer (0) vs Spinner (1) - affects delivery selection |
| `batsmen_type` | int | 0/1/2 | TOP/MIDDLE/TAIL - experience level, wicket probability |
| `overs_bowled` | int | 0-20 | Bowler fatigue (increases error probability) |
| `balls_faced` | int | 0-120 | Batsman form bonus (more confident after setting |
| `match_intensity` | int | 0-100 | Run rate requirement (pressure on batsman) |
| `speed` | int | 100-155 km/h | Ball velocity (affects likelihood and shot difficulty) |
| `ball_type` | enum | 10 types | Delivery (FAST, YORKER, SPIN, GOOGLY, etc.) - wicket factor |

---

## How Probabilities are Calculated

### 1. Next Ball Delivery (from `src/simulation/delivery.c`)

When bowler generates a delivery, MOSS calculates **wide** and **no-ball** probability based on skill + fatigue:

```
Bowler skill & fatigue variables:
  skill = bowler->bowling_skill          // 0-100 (higher = better control)
  fatigue = bowler->overs_bowled / 6     // Each over increases fatigue

Formula for Wide Ball probability:
  wide_prob = 2 + (100 - skill) / 20 + fatigue / 3
  
Formula for No-Ball probability:
  noball_prob = 1 + (100 - skill) / 30 + fatigue / 6

Cricket Meaning:
  - Lower skill = more wides/no-balls (poor line control)
  - More fatigued = more extras (tired bowler loses accuracy)
  - Example: Bumrah (skill=97) rarely bowls wides; younger bowlers (skill=50) have 4-5x wides
```

### 2. Next Shot Outcome (from `src/simulation/shot.c`)

When batsman plays shot, MOSS calculates **runs** and **wicket probability** using batsman vs bowler attributes:

```
Key variables affecting shot:
  batting_skill = batsman->batting_skill          // Decision-making quality
  bowling_skill = bowler->bowling_skill           // Ball difficulty
  set_bonus = batsman->balls_faced / 8            // Form (gets stronger)
  pressure = (runs_needed * 6) / balls_left       // Required RRR
  match_intensity = match.match_intensity         // Urgency of match

Wicket Probability calculation:
  wicket_prob = 5 (base)
  
  If PACER:
    + (speed - 100) / 15                         // Faster = more dangerous
    + 12 if YORKER/BOUNCER                       // Difficult deliveries
    + 3-8 if FULL/SLOWER                         // Moderate difficulty
    
  If SPINNER:
    + 12 if GOOGLY/CARROM                        // Deceptive spin
    + 6 if LEG_BREAK                             // Standard spin
    
  + (bowling_skill * 10) / 100                    // Skilled bowler wins
  - (batting_skill * 20) / 100                    // Skilled batsman survives
  + 8 if TAIL-ENDER                              // Tail-enders vulnerable
  + pressure / 4 + intensity / 5                  // Pressure increases risk

Runs probability (only if not out):
  run_bias = batting_skill + set_bonus            // Higher skill = more runs
  
  Dot   (0 runs):  30% base
  Single (1 run):  28% base
  Twos (2 runs):   18% base
  Fours (4 runs):  15-20% based on skill
  Six   (6 runs):  2-5% base, reduced for tail-enders

Cricket Meaning:
  - Virat Kohli (skill=95): High run rate, low wicket chance
  - Tail-ender (skill=20): Scores 0-2 runs, high wicket chance (60%+ risk)
  - Under pressure: More aggressive shot, higher wicket chance
```

### 3. Catch Probability (from `src/simulation/fielding.c`)

When ball goes aerial (risky shot), fielders attempt catch:

```
Catch success formula:
  prob = 20 + (fielding_skill * 45) / 100
  
  Limited to range: 20% (minimum) to 75% (maximum)

Cricket Meaning:
  - Poor fielder (field=30):  20% catch success (dropped)
  - Good fielder (field=70):  52% catch success (regular catches)
  - Excellent fielder (field=95): 63% catch success (reliable)
  - Keeper (field=88):        60% catch success (specialization)
```

---

## Team Selection via Command-Line Flags

MOSS supports 9 hardcoded international teams that users select via flags or interactive menu:

**Available Teams:**
```
IND  AUS  SL   PAK  ENG  NZ   WI   SA   AFG
```

**Usage Examples:**
```bash
./moss -R -IND -AUS              # India vs Australia, Round-Robin both innings

./moss -P -PAK -ENG              # Pakistan vs England, Priority both innings

./moss -SP -NZ -SL -interactive  # SJF inn1, Priority inn2, select teams interactively

./moss -R                        # Round-Robin, prompted for team selection
```

**Interactive Selection:**
If no teams specified, program prompts user:
```
Select Team 1 (bats first):
  [1] India
  [2] Australia
  ...
```

**How It Works (from `src/main.c`):**
- Parse arguments for team codes (case-insensitive)
- Store `team_registry[idx]` pointing to India/Australia/SL/Pakistan/ENG/NZ/WI/SA/AFG squad definitions
- Load teams using `load_team()` function which copies player attributes
- If teams omitted, call `pick_team()` for interactive selection

---

## Scheduling Algorithms (Per Inning)

MOSS allows **different scheduling algorithms for each inning** via command-line flags:

**Three Scheduling Policies Supported:**

`-R` = **Round-Robin (RR):** Fair rotation, each bowler gets exactly 1 over (6 balls), then next bowler takes turn. Best for balanced play.

`-P` = **Priority Scheduling:** High-skill bowlers selected more often. Death-over specialists prioritized in last 6 balls. Realistic cricket.

`-S` = **Shortest Job First (SJF):** Tail-enders bat first (short stay expected). Minimizes match time. Educationally valuable for OS theory.

**Mixed Scheduling (Different Algorithms Per Inning):**

```bash
./moss -RP -IND -AUS              # Round-Robin inn1, Priority inn2

./moss -SP -PAK -ENG              # SJF inn1, Priority inn2

./moss -RS -AUS -NZ               # Round-Robin inn1, SJF inn2
```

**Scheduling Implementation (from `src/scheduler.c`):**
```
User chooses -X or -XY (where X,Y ∈ {R,P,S})

If single letter:  use that policy for both innings
If two letters:    1st letter for inning 1, 2nd for inning 2

set_scheduling_policy(sched1);      // Innings 1
set_scheduling_policy(sched2);      // Innings 2

During play:
  if policy == SCHED_RoR:       next_bowler = (current + 1) % 11
  if policy == SCHED_PRIORITY:  next_bowler = select_by_skill()
  if policy == SCHED_SJF:       next_batsman = select_by_expected_stay()
```

---

## Architecture

**14 Concurrent Threads:**
- 1 Main (orchestrator)
- 1 Bowler (delivers balls)
- 2 Batsmen (play shots)
- 11 Fielders (attempt catches)

**Synchronization:** Mutexes protect score/pitch; condition variables wake fielders for catches.

**Producer-Consumer:** 6-slot circular buffer (pitch); bowler writes, batsman reads.

**Output:** Console display, `logs/log.txt` (ball-by-ball), `logs/gantt.txt` (thread timeline).

**Build:** `make` or manual GCC with `-pthread` flag.

**Run:** `./moss -[SCHED] -[TEAM1] -[TEAM2]` (schedulers and teams optional)

* **`include/`**: Contains header files (`types.h`, `constants.h`, `scheduler.h`) defining the system structures.
* **`src/pitch.c`**: Implements the `pitch_write()` and `pitch_read()` logic using `pthread_mutex_t` and `pthread_cond_t`.
* **`src/scheduler.c`**: Contains the logic for the three scheduling algorithms (RR, SJF, Priority).
* **`src/players/`**: Implements the distinct behaviors for `batsman_thread`, `bowler_thread`, and `fielder_thread`.
* **`src/simulation/`**: Houses the math/physics for `delivery.c` (ball generation) and `shot.c` (calculating runs/wickets based on player skills).
* **`src/gantt.c`**: Responsible for recording timestamps of thread activity to generate the visual Gantt Chart.

---

---

## File Organization

- **`include/`**: Header files (types.h, constants.h, scheduler.h) defining structures and function signatures
- **`src/pitch.c`**: Circular buffer with mutex/condition variables (producer-consumer)
- **`src/scheduler.c`**: Three scheduling algorithms (RR, Priority, SJF) for bowler/batsman selection
- **`src/players/`**: Thread implementation for batsman, bowler, fielder
- **`src/simulation/`**: Ball generation, shot calculation, fielding mechanics
- **`src/gantt.c`**: Thread activity logging for visualization
- **`Makefile`**: Automated compilation

## Quick Start

```bash
make                      # Compile
./moss -R -IND -AUS      # Round-Robin, India vs Australia
./moss -SP -PAK -ENG     # SJF inn1, Priority inn2, interactive teams
```

**Output files:**
- Console: Real-time match display
- `logs/log.txt`: Ball-by-ball detailed log
- `logs/gantt.txt`: Thread execution timeline

**All fixes applied** — Gantt chart, log, and scorecard now match perfectly. Ready for production use.