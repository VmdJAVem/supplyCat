CC = clang
CFLAGS = -I. -Wall

SRCS = *.c
OBJS = $(SRCS:.c=.o)
TARGET = supplyCat

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean
