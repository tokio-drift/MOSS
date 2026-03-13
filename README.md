# OS-T20

A T20 World Cup Simulator using Pthreads.  
Written in C  

t20-simulator/  
│  
├── README.md  
│    
├── include/  
│   ├── constants.h  
│   ├── types.h  
│   ├── pitch.h  
│   ├── players.h  
│   ├── scoreboard.h  
│   ├── scheduler.h  
│   ├── sync.h  
│  
├── src/  
│   ├── main.c  
│   ├── pitch.c  
│   ├── scoreboard.c  
│   ├── scheduler.c  
│   ├── sync.c  
│  
├── src/players/  
│   ├── bowler.c  
│   ├── batsman.c  
│   ├── fielder.c  
│  
├── src/simulation/  
│   ├── delivery.c  
│   ├── shot.c  
│   ├── fielding.c  
│  
├── logs/  
│   └── match_log.txt  
│  
└── docs/  
    ├── design.pdf  
    ├── gantt_chart.png  