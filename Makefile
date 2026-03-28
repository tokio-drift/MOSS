CC     = gcc
CFLAGS = -Wall -Wextra -O2 -pthread -I./include
SRCS   = src/main.c \
         src/pitch.c \
         src/scheduler.c \
         src/scoreboard.c \
         src/gantt.c \
         src/players/batsman.c \
         src/players/bowler.c \
         src/players/fielder.c \
         src/simulation/shot.c \
         src/simulation/fielding.c \
         src/simulation/delivery.c
TARGET = cricket

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) -lm

clean:
	rm -f $(TARGET)

.PHONY: all clean