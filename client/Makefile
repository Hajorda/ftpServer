CC=gcc
CFLAGS=-Wall -Wextra -O2

SRCS=client.c connection.c transfer.c
OBJS=$(SRCS:.c=.o)
DEPS=client.h

TARGET=ftp_client

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o $(TARGET)

.PHONY: all clean
