CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -pedantic -O2 -Iinclude -Isrc
LDFLAGS := -Llib
AR      := ar
ARFLAGS := rcs

SRC     := $(filter-out src/cli.c, $(wildcard src/*.c))
OBJ     := $(SRC:src/%.c=build/%.o)
LIB     := lib/libtrinary.a
BIN     := bin/trc

.PHONY: all clean test examples

all: $(LIB) $(BIN)

$(LIB): $(OBJ)
	@mkdir -p lib
	$(AR) $(ARFLAGS) $@ $^

$(BIN): src/cli.c $(LIB)
	@mkdir -p bin
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) -ltrinary

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

TESTS := test/test_runner.c test/test_trit.c test/test_gate.c test/test_arith.c test/test_cpu.c test/test_asm.c

test_runner: $(TESTS) $(LIB)
	$(CC) $(CFLAGS) $(TESTS) -o $@ $(LDFLAGS) -ltrinary -lm

test: test_runner
	./test_runner

examples: $(LIB) $(BIN)
	@for ex in examples/*.trc; do \
		echo "--- $$ex ---"; \
		$(BIN) $$ex; \
		echo; \
	done

clean:
	rm -rf build lib bin test_runner
