CC = gcc
CFLAGS = -I. -Wall -O3
SRCDIR = src
INCDIR = include
SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(SRCS:.c=.o)
TARGET = supplyCat

$(TARGET): $(OBJS) main.o
	$(CC) $(OBJS) main.o -o $(TARGET)

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

main.o: main.c
	$(CC) $(CFLAGS) -c $< -o main.o

clean:
	rm -f $(OBJS) main.o $(TARGET)

.PHONY: clean