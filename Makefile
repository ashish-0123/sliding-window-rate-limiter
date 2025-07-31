#CFLAGS= -DDEBUG -g
CFLAGS=

all: rl-st rl-mt rl-st-random rl-mt-random

rl-st: rate-limiter.c
	gcc -o $@ $(CFLAGS) $^

rl-mt: rate-limiter-mt.c
	gcc -o $@ $(CFLAGS) $^ -lpthread

rl-st-random: rate-limiter.c
	gcc -o $@ $(CFLAGS) -DRANDOM $^

rl-mt-random: rate-limiter.c
	gcc -o $@ $(CFLAGS) -DRANDOM $^ -lpthread

.PHONY: clean

clean:
	rm -f rl-st rl-mt rl-st-random rl-mt-random
