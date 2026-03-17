# T20 World Cup Cricket Simulator

A multi-threaded T20 cricket match simulator built in C++17, designed as part of the Operating Systems (CSC-204) course. Players, the pitch, and the ball are modelled as system resources and concurrent processes — demonstrating mutexes, semaphores, condition variables, scheduling algorithms, and deadlock detection.

---

## Table of Contents

- [Project Structure](#project-structure)
- [Prerequisites](#prerequisites)
- [Running Locally](#running-locally)
- [Team Data Format](#team-data-format)
- [Scheduling Modes](#scheduling-modes)
- [Output](#output)

---

## Project Structure

```
T20_Cricket_Simulator/
├── CMakeLists.txt
├── README.md
├── .gitignore
├── include/
│   ├── player_data.h       # Raw data struct (populated from .txt files)
│   ├── loader.h            # Team file parser
│   ├── pitch.h             # Critical section (mutex-protected)
│   ├── scoreboard.h        # Shared score resource
│   ├── bowler.h            # Bowler thread
│   ├── batsman.h           # Batsman thread
│   ├── fielder.h           # Fielder threads (condition variable)
│   ├── scheduler.h         # RR / SJF / Priority scheduler
│   ├── umpire.h            # Deadlock detection
│   └── match.h             # Match orchestrator
├── src/
│   ├── loader.cpp
│   ├── pitch.cpp
│   ├── scoreboard.cpp
│   ├── bowler.cpp
│   ├── batsman.cpp
│   ├── fielder.cpp
│   ├── scheduler.cpp
│   ├── umpire.cpp
│   ├── match.cpp
│   └── main.cpp
├── stats/
│   ├── india.txt           # India squad data
│   └── australia.txt       # Australia squad data
└── logs/
    └── ball_by_ball.txt    # Generated at runtime
```

---

## Prerequisites

### Arch Linux

```bash
sudo pacman -S cmake gcc base-devel git
```

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install cmake g++ build-essential git
```

### macOS

```bash
xcode-select --install
brew install cmake
```

### Windows

Install [WSL2](https://learn.microsoft.com/en-us/windows/wsl/install), then follow the Ubuntu instructions above inside WSL.

---

## Running Locally

### 1. Clone the repository

```bash
git clone https://github.com/yourname/T20_Cricket_Simulator.git
cd T20_Cricket_Simulator
```

### 2. Configure with CMake (one-time only)

```bash
mkdir build
cd build
cmake ..
cd ..
```

### 3. Build

```bash
cmake --build build
```

### 4. Run

```bash
./build/cricket_sim
```

---

### Rebuilding after code changes

You only need to run `cmake ..` once. After any code change, just:

```bash
cmake --build build
./build/cricket_sim
```

### Clean rebuild (if something breaks)

```bash
rm -rf build
mkdir build && cd build
cmake ..
cd ..
cmake --build build
```

---

## Team Data Format

Teams are loaded from plain `.txt` files in the `stats/` folder. Each player is defined as a `[player]` block:

```ini
TEAM India

# Batsman example
[player]
name=Rohit Sharma
role=BATSMAN
strike_rate=139.2
avg=48.5
expected_balls=30
thread_priority=5
is_death_specialist=false

# Bowler example
[player]
name=Jasprit Bumrah
role=BOWLER
avg=5.0
strike_rate=60.0
expected_balls=4
economy=6.2
max_overs=4
thread_priority=9
is_death_specialist=true
```

**Supported roles:** `BATSMAN`, `BOWLER`, `ALL_ROUNDER`, `WICKETKEEPER`

To add a new team, create a new `.txt` file in `stats/` following the same format and load it in `main.cpp`.

---

## Scheduling Modes

The simulator supports three scheduling strategies, switchable at runtime:

| Mode       | Description                                                                               |
| ---------- | ----------------------------------------------------------------------------------------- |
| `RR`       | Round Robin — bowlers rotated every 6 balls (1 over)                                      |
| `SJF`      | Shortest Job First — tail-enders with fewer `expected_balls` bat earlier                  |
| `PRIORITY` | Priority Scheduling — death over specialist gets highest CPU priority in the last 6 balls |

---

## Output

A ball-by-ball log is written to `logs/ball_by_ball.txt` at runtime, for example:

```
Over 1, Ball 1 | Bumrah -> Rohit Sharma | DOT BALL
Over 1, Ball 2 | Bumrah -> Rohit Sharma | SIX
Over 1, Ball 3 | Bumrah -> Kohli       | WIDE (+1)
Over 1, Ball 4 | Bumrah -> Kohli       | WICKET - RUN OUT
...
Score: 47/2 after 6 overs
```

A Gantt chart visualizing thread usage over time is generated separately via the analysis scripts.

---

## Authors

- Team members listed here
- Course: Operating Systems (CSC-204)
- Deadline: March 22, 2026
