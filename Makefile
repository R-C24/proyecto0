CC = gcc
CFLAGS = -Wall -Wextra -g

TARGET = compilador
SRCS = main.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGET)
	./$(TARGET) $(FILE)

clean:
	rm -f $(OBJS) $(TARGET) *.s *.out

.PHONY: all clean test