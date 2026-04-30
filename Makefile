CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -O2 -Iinclude -pthread
LDFLAGS := -pthread

SRC := \
	src/main.c \
	src/event_queue.c \
	src/sensor.c \
	src/controller.c \
	src/sensors/door.c \
	src/sensors/motion.c

OBJ := $(SRC:.c=.o)
BIN := smart_home

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: clean
