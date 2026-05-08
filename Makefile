CC = gcc
CFLAGS = -I. -Wall

SRCS = *.c
OBJS = $(SRCS:.c=.o)
TARGET = supplyCat

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -O3 -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean
