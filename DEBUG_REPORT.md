# T20 Simulator - Debug Report

## Critical Issues Found

### 1. **COMPILATION ERRORS - Windows/MinGW Environment**
**Severity**: CRITICAL
- Missing `pthread.h` and `time.h` headers support
- These are POSIX headers not available by default on Windows
- **Solution**: 
  - Use MinGW-w64 with POSIX thread support
  - Or use Windows Subsystem for Linux (WSL)
  - Or use a Linux/macOS environment

**Error Details**:
```
gcc: src/main.c:4:21: fatal error: pthread.h: No such file or directory
```

---

### 2. **Potential Uninitialized Variables in Batsman Thread**
**Severity**: MEDIUM
- **Location**: `src/players/batsman.c` line 55
- **Issue**: `catch_taken` variable is checked but may not be properly synchronized
- **Problem**: 
```c
if (!catch_taken && attempt_catch(f, r.aerial))  // catch_taken not always synced
```

---

### 3. **Missing Function: `update_match_intensity`**
**Severity**: MEDIUM (ACTUALLY IMPLEMENTED)
- ✅ Implemented in `src/scheduler.c` line 201
- Correctly updates `match.match_intensity` with computed value

---

### 5. **Data Type Inconsistency in types.h**
**Severity**: MEDIUM
- Field name: `batmen_type` should be `batsman_type` (typo)
- **Location**: `include/types.h` line 45
- Used inconsistently throughout code:
  - In `types.h`: defined as `batsmen_type`
  - In constants: `BTYPE_*` constants
  - In code: referenced as `batsman->batsmen_type`

**Current code**:
```c
int  batsmen_type;  // SHOULD BE: batsman_type
```

---

### 6. **Potential Race Condition in pitch.c**
**Severity**: MEDIUM
- **Function**: `pitch_read()`
- **Problem**: Complex circular buffer logic that might fail
- **Location**: `src/pitch.c` lines 58-75

**Issue**:
```c
int read_index = (pitch_index - 1 + PITCH_SIZE) % PITCH_SIZE;
ball = pitch_buffer[read_index];
```
This assumes the last written element is always at `pitch_index - 1`, but the producer might not have written yet.

---

### 7. **Missing Header Files**
**Severity**: MEDIUM
- `include/sync.h` is referenced in README but not present
- Some function declarations might be missing

---

### 8. **Uninitialized Global: `match` variable**
**Severity**: MEDIUM
- Global `match` scoreboard is used in many files
- Must be initialized via `init_scoreboard()` before use
- **Current code** in `scoreboard.c`: Initialization looks correct, but ensure `init_scoreboard()` is called FIRST in `main.c`

---

### 9. **Missing Index Bounds Check**
**Severity**: MEDIUM
- **Location**: `src/scheduler.c` line 137
- Function `select_next_batsman_locked()` can return -1 but caller doesn't always check

```c
int next = select_next_batsman_locked(batting_team, TEAM_SIZE, &match);
if (next == -1) {
    // Already handled ✓
}
```

---

### 10. **Variable: `innings_over` Synchronization**
**Severity**: MEDIUM
- Global `volatile int innings_over = 0;` is used across threads
- Not protected by mutex when read
- **Risk**: Compiler might cache the value
- **Better**: Use `volatile sig_atomic_t` or protect with atomic operations

**Current**:
```c
volatile int innings_over = 0;  // Weak protection
```

---

### 11. **Missing Function Call Verification**
**Severity**: LOW
- `log_event()` parameters match between declaration and definition
- `gantt_record()` parameters match ✓
- Thread functions signatures match ✓

---

### 12. **Incomplete main.c**
**Severity**: UNKNOWN
- Need to see `src/main.c` in full to verify:
  - Proper initialization sequence
  - Thread creation and joining
  - Error handling

---

## Recommended Fix Order

### **PHASE 1: Build Issues**
1. ✅ Set up MinGW with pthread support (or use Linux)
2. ✅ Verify pthread.h and time.h are available

### **PHASE 2: Missing Implementations**
1. Complete `src/gantt.c` file (implement `gantt_print()`)
2. Fix type name: `batsmen_type` → `batsman_type` (or rename consistently)
3. Add missing function implementations

### **PHASE 3: Synchronization**
1. Change `volatile int` to `volatile sig_atomic_t` for `innings_over`
2. Review race conditions in `pitch.c` read/write logic
3. Add bounds checking in `on_wicket()`

### **PHASE 4: Testing**
1. Add error checking to thread creation
2. Verify initialization order in main()
3. Test with thread sanitizer

---

## Files That Need Review

| File | Issues |
|------|--------|
| `src/gantt.c` | Incomplete / Missing `gantt_print()` |
| `include/types.h` | Type name inconsistency |
| `src/pitch.c` | Possible race condition |
| `src/main.c` | Need to review (not shown) |
| `src/scheduler.c` | Bounds checking needed |

---

## How to Fix

See the accompanying fix files or run:
```bash
# After installing MinGW64 with POSIX threads:
gcc -Wall -Wextra -O2 -pthread -lrt -Iinclude src/main.c src/pitch.c src/scheduler.c src/scoreboard.c src/gantt.c src/players/*.c src/simulation/*.c -o moss
```

Note: `-lrt` may be needed for `clock_gettime()` on some systems
