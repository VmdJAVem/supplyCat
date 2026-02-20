CC = gcc
CFLAGS = -std=c99 -O2 -Wall -Wextra -g
LDFLAGS = -lm

SRCS = main.c attack.c board.c movegen.c move.c search.c uci.c
OBJS = $(SRCS:.c=.o)
	HEADERS = types.h attack.h board.h movegen.h move.h search.h uci.h
	TARGET = supplycat

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
