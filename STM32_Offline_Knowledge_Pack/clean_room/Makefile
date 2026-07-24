CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic
CPPFLAGS ?= -Iinclude
LDLIBS ?= -lm

.PHONY: all clean

all: test_reverb

test_reverb: src/schroeder_reverb.c tests/test_reverb.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $^ $(LDLIBS) -o $@

clean:
	$(RM) test_reverb
