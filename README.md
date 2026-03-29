<div align="center">

<pre>
 __       __   ______    ______    ______  
|  \     /  \ /      \  /      \  /      \ 
| $$\   /  $$|  $$$$$$\|  $$$$$$\|  $$$$$$\
| $$$\ /  $$$| $$  | $$| $$___\$$| $$___\$$
| $$$$\  $$$$| $$  | $$ \$$    \  \$$    \ 
| $$\$$ $$ $$| $$  | $$ _\$$$$$$\ _\$$$$$$\
| $$ \$$$| $$| $$__/ $$|  \__| $$|  \__| $$
| $$  \$ | $$ \$$    $$ \$$    $$ \$$    $$
 \$$      \$$  \$$$$$$   \$$$$$$   \$$$$$$ 
</pre>

</div>

---

# About the Project
---

This project maps a **cricket match directly onto Operating System abstractions**.

A delivery is treated as a **shared resource**, passed between:
- a **producer (bowler thread)**  
- a **consumer (batsman thread)**  

through a **bounded buffer (the pitch)**.

Every game event from **catching, fielding, scoring** to **run-outs**  is handled concurrently using:
- **threads**
- **mutexes**
- **condition variables**

This makes the simulation a **live demonstration of synchronization, scheduling, and concurrency concepts** in action.

---

# Features

## Thread Architecture
- **1 Bowler thread**  
  → Generates deliveries and writes them to the pitch buffer  

- **2 Batsman threads**  
  → One active (slot 0 consumes deliveries and drives all game logic)  
  → One dummy (slot 1 waits for innings end)  

- **10 Fielder threads**  
  → One per player  
  → Each sleeps on its own condition variable  
  → Woken only when a **catch opportunity** occurs  

---

## Producer–Consumer via the Pitch Buffer
- Implemented in `pitch.c` as a **bounded buffer**
- Bowler uses `pitch_write()` → blocks until consumed  
- Batsman uses `pitch_read()` → blocks until available  

---

## Synchronization with Mutexes

| Mutex            | Protects |
|------------------|---------|
| `score_mutex`    | Scoreboard, wickets, overs |
| `pitch_mutex`    | Delivery buffer and flags |
| `scheduler_mutex`| Striker, non-striker, bowler IDs |
| `fielder_mutex`  | Catch state, active fielder |
| `runout_mutex`   | Batsman positions |

---

## Three Bowling Scheduling Algorithms

Bowler selection is modeled as a **CPU Scheduling Problem**:

- **Round Robin (`-R`)**
  - Bowlers rotate in order
  - Respect 4-over cap  

- **Shortest Job First (`-S`)**
  - Bowler with fewest balls bowled is selected  

- **Priority (`-P`)**
  - Based on:
    - skill
    - match phase (powerplay / middle / death)
    - run-rate pressure  

User can apply different policies per innings as per his choice. 
Example: `-SP` → SJF (innings 1), Priority (innings 2)

---

## Deadlock Detection 
- Uses a **resource-request graph**
- Resources: `END_1`, `END_2` (creases)

### Logic:
- Batsman A holds X, wants Y  
- Batsman B holds Y, wants X  

This detects a **deadlock** when both batsmen are waiting for each other to cross, simulating a real-life scenario where both batsmen are stranded mid-pitch.

One batsman is eliminated as the **deadlock victim**, simulating a **real run-out**

---

## Gantt Chart Visualization
- Printed after every match innings (with ANSI colors)
- Saved to: `logs/gantt.txt`

### Representation:
- Rows → Bowlers  
- Segments → Balls  

Also shows **thread execution times per bowler**

---

## International Teams 
Nine of the famous international T20 cricket teams included:
- India
- Australia
- England
- Pakistan
- Sri Lanka
- New Zealand
- West Indies
- South Africa
- Afghanistan

Each team has 11 players with unique attributes:
- Batting skill  
- Bowling skill  
- Fielding skill  
- Bowler type (pacer/spinner)  
- Batting position  

Full batting and bowling scorecards are printed at the end of each innings, with each bowlers runs conceded and wickets taken, and each batsman’s runs scored and balls faced. 

---

## Detailed Ball-by-Ball Match Log
Saved in: `logs/log.txt`

### Each entry includes:
- Over number  
- Bowler  
- Batsman  
- Ball type  
- Speed  
- Outcome  

### Covers:
- Runs & extras  
- Dropped catches  
- Catches  
- Bowled / LBW  
- Run-outs (with victim name)

---

<!-- #  Summary
This project is a **complete OS simulation disguised as a cricket match**, demonstrating:

- Thread synchronization  
- Producer–consumer problem  
- CPU scheduling  
- Deadlock detection  
- Concurrent event handling  

All while producing a **realistic and detailed cricket match simulation**.

--- -->