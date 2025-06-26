CC = gcc
CFLAGS = -Wall -Wextra -O2

# List all your source files here
SRCS = main.c server.c commands.c
OBJS = $(SRCS:.c=.o)
DEPS = server.h commands.h

TARGET = ftp_server

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
