CC     = gcc
CFLAGS = -std=gnu11 -Wall -Wextra -MMD -MP
OBJS   = src/main.o src/reader.o

.PHONY: all clean

all: lrg

lrg: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f lrg $(OBJS)

-include $(OBJS:.o=.d)
