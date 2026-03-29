# Commands Reference - Testing & Verification

## 🏗️ COMPILATION COMMANDS

### Windows (with MinGW-w64 POSIX threads installed)
```bash
# Single command compile
gcc -Wall -Wextra -O2 -pthread -Iinclude src/main.c src/pitch.c src/scheduler.c src/scoreboard.c src/gantt.c src/players/batsman.c src/players/bowler.c src/players/fielder.c src/simulation/delivery.c src/simulation/shot.c src/simulation/fielding.c -o moss

# Using make (if installed)
make clean
make
```

### Windows Subsystem for Linux (WSL)
```bash
# Install build tools
sudo apt update
sudo apt install build-essential

# Navigate to project
cd /mnt/c/Users/parsh/OS-T20

# Compile
make clean
make
```

### Linux/macOS
```bash
# Install dependencies (if needed)
sudo apt install gcc make   # On Ubuntu/Debian
brew install gcc            # On macOS

# Compile
make clean
make
```

---

## 🎮 RUNNING THE SIMULATOR

### Basic Usage
```bash
# Interactive mode (select teams at runtime)
./moss -R

# Specify scheduling policy and teams
./moss -R -IND -AUS           # Round-Robin, India vs Australia
./moss -P -ENG -PAK           # Priority, England vs Pakistan
./moss -S -NZ -SA             # SJF, New Zealand vs South Africa
```

### Scheduling Policies
- `-R` : Round-Robin (same for both innings)
- `-P` : Priority-based
- `-S` : Shortest Job First (SJF)
- `-RP` : Round-Robin for inn1, Priority for inn2
- `-PS` : Priority for inn1, SJF for inn2
- `-SR` : SJF for inn1, Round-Robin for inn2

### Available Teams
- `-IND` : India
- `-AUS` : Australia  
- `-SRI` : Sri Lanka
- `-PAK` : Pakistan
- `-ENG` : England
- `-NZ` : New Zealand
- `-WI` : West Indies
- `-SA` : South Africa
- `-AFG` : Afghanistan

---

## 📊 VIEWING OUTPUT

### Match Log
```bash
# View in real-time
tail -f logs/log.txt

# View entire log
cat logs/log.txt

# Open in editor
nano logs/log.txt
vim logs/log.txt

# Search in log
grep "wicket\|OUT" logs/log.txt

# Get match statistics
grep "^Over" logs/log.txt | wc -l  # Count deliveries
grep "OUT" logs/log.txt | wc -l    # Count wickets
```

### Gantt Chart
```bash
# View Gantt chart
nano logs/gantt.txt
vim logs/gantt.txt

# View latest Gantt chart
tail -100 logs/gantt.txt
```

---

## 🔍 DEBUGGING DURING COMPILATION

### Check for Warnings
```bash
gcc -Wall -Wextra -pedantic -Iinclude src/main.c ... -o moss 2>&1 | grep -i warning
```

### Check for specific errors
```bash
gcc -Wall -Wextra -O2 -pthread -Iinclude src/main.c ... 2>&1 | grep -E "error|race"
```

### Compile with debug symbols
```bash
gcc -g -Wall -Wextra -O0 -pthread -Iinclude src/main.c ... -o moss
```

---

## 🧵 THREADING ANALYSIS

### Run with thread sanitizer (Linux/WSL only)
```bash
gcc -Wall -Wextra -g -fsanitize=thread -pthread -Iinclude src/main.c ... -o moss
./moss -R
```

Shows race conditions and synchronization issues.

### Run with memory sanitizer
```bash
gcc -Wall -Wextra -g -fsanitize=memory -pthread -Iinclude src/main.c ... -o moss
./moss -R  
```

Shows memory leaks and issues.

---

## 🧪 TEST SCENARIOS

### Test 1: Verify All Teams Work
```bash
for team in IND AUS SRI PAK ENG NZ WI SA AFG; do
  for opp in ENG PAK WI; do
    ./moss -R -$team -$opp 2>&1 | tail -5
    echo "---"
  done
done
```

### Test 2: All Scheduling Policies
```bash
./moss -R          # Round-Robin
./moss -P          # Priority
./moss -S          # SJF
./moss -RP         # Mixed
./moss -PS         # Mixed
./moss -SR         # Mixed
```

### Test 3: Performance Test
```bash
# Time execution
time ./moss -R -IND -AUS

# Check output files created
ls -lah logs/

# Count lines in log
wc -l logs/log.txt logs/gantt.txt
```

### Test 4: Stress Test (Run multiple times)
```bash
for i in {1..5}; do
  echo "Run $i:"
  ./moss -R -IND -AUS > /dev/null 2>&1 && echo "OK" || echo "FAILED"
done
```

---

## 🔧 CLEANUP COMMANDS

### Remove executable
```bash
rm moss

# Or using make
make clean
```

### Remove logs
```bash
rm -rf logs/
```

### Remove all generated files
```bash
make clean
rm -rf logs/ *.o moss
```

---

## 📈 VERIFICATION CHECKLIST

```bash
# Step 1: Check compilation
gcc -Wall -Wextra -O2 -pthread -Iinclude src/main.c src/**/*.c -o moss
echo $?  # Should output: 0 (success)

# Step 2: Check executable exists
ls -l moss
file moss

# Step 3: Run quick test
./moss -R -IND -AUS | tail -20

# Step 4: Check logs created
ls logs/

# Step 5: Verify output format
head -20 logs/log.txt
tail -20 logs/gantt.txt

# Step 6: Check for errors in log
grep -i error logs/log.txt
grep -i "out!" logs/log.txt | head -5
```

---

## 🚀 OPTIMIZATION COMMANDS

### Compile with optimization
```bash
gcc -Wall -Wextra -O3 -march=native -pthread -Iinclude src/main.c ... -o moss_opt
```

### Profile execution
```bash
# Linux only
gcc -Wall -Wextra -g -pg -pthread -Iinclude src/main.c ... -o moss_profile
./moss_profile -R
gprof ./moss_profile gmon.out > analysis.txt
cat analysis.txt
```

---

## 🐛 DEBUGGING WITH GDB

### Compile with debug symbols
```bash
gcc -Wall -Wextra -g -O0 -pthread -Iinclude src/main.c ... -o moss_debug
```

### Run with GDB
```bash
gdb ./moss_debug

# In GDB:
(gdb) run -R -IND -AUS
(gdb) bt                    # Print backtrace
(gdb) info threads          # List all threads
(gdb) break src/pitch.c:58  # Set breakpoint
(gdb) c                     # Continue
(gdb) print pitch_index     # Print variable
(gdb) quit
```

### Set breakpoints at functions
```bash
gdb -ex 'break pitch_read' -ex 'break pitch_write' -ex run ./moss_debug -R
```

---

## 📋 COMMON ISSUES & FIXES

### Issue: "pthread.h: No such file"
```bash
# Windows - Install MinGW-w64 with POSIX threads
# Or use WSL
wsl --install Ubuntu-22.04

# Linux/Mac - Install build tools
sudo apt install build-essential   # Ubuntu
brew install gcc                   # macOS
```

### Issue: "undefined reference to 'pthread_*'"
```bash
# Ensure -pthread flag is used
gcc ... -pthread ...

# Or link explicitly
gcc ... -lpthread ...
```

### Issue: Program hangs during execution
```bash
# Kill it with Ctrl+C
# Then check logs to see where it hangs
./moss -R &
sleep 10
kill %1
tail logs/log.txt
```

### Issue: Segmentation fault
```bash
# Compile with debug symbols and run with gdb
gcc -g -O0 -pthread -Iinclude src/main.c ... -o moss_debug
gdb ./moss_debug
(gdb) run -R
(gdb) bt
```

---

## 📞 USEFUL ONE-LINERS

```bash
# Compile and run
gcc -Wall -Wextra -O2 -pthread -Iinclude src/main.c src/**/*.c -o moss && ./moss -R

# Run and see latest score
./moss -R & sleep 15 && tail -5 logs/log.txt

# Count total deliveries
grep -c "^Over" logs/log.txt

# Find all wickets
grep "OUT\|wicket" logs/log.txt

# Get batting summary  
grep "Batsman" logs/log.txt | head -15

# Get bowling analysis
grep "Economy\|Overs" logs/log.txt | head -15

# Compare two runs
diff <(./moss -R 2>&1 | tail -20) <(./moss -R 2>&1 | tail -20)

# Run a tournament (all teams play each other)
for t1 in IND AUS SRI; do
  for t2 in PAK ENG NZ; do
    echo "=== $t1 vs $t2 ===" 
    ./moss -R -$t1 -$t2 | grep "win by"
  done
done
```

---

## ✅ QUICK VERIFICATION SCRIPT

Save as `verify.sh`:
```bash
#!/bin/bash

echo "=== T20 Simulator Verification ==="
echo

echo "1. Check source files..."
if [ -d "include" ] && [ -d "src" ]; then
  echo "   ✓ Directory structure OK"
else
  echo "   ✗ Missing directories"
  exit 1
fi

echo
echo "2. Compiling..."
gcc -Wall -Wextra -O2 -pthread -Iinclude src/main.c src/**/*.c -o moss 2>&1 | head -5
if [ -f "moss" ]; then
  echo "   ✓ Compilation successful"
else
  echo "   ✗ Compilation failed"
  exit 1
fi

echo
echo "3. Running test..."
timeout 30 ./moss -R -IND -AUS > /dev/null 2>&1
if [ $? -eq 0 ] || [ $? -eq 124 ]; then
  echo "   ✓ Program runs successfully"
else
  echo "   ✗ Program failed"
  exit 1
fi

echo
echo "4. Checking output files..."
if [ -f "logs/log.txt" ]; then
  echo "   ✓ Log file created"
  echo "   Lines in log: $(wc -l < logs/log.txt)"
else
  echo "   ✗ No log file"
fi

echo
echo "=== Verification Complete ==="
```

Run with:
```bash
chmod +x verify.sh
./verify.sh
```

