CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -pthread -Iinclude
TARGET  = moss

SRC =   src/main.c \
        src/pitch.c \
        src/scheduler.c \
        src/scoreboard.c \
        src/gantt.c \
        src/players/batsman.c \
        src/players/bowler.c \
        src/players/fielder.c \
        src/simulation/delivery.c \
        src/simulation/fielding.c \
        src/simulation/shot.c

OBJ = $(SRC:.c=.o)

.INTERMEDIATE: $(OBJ)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(TARGET)
	rm -rf logs/

.PHONY: all clean