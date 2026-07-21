CC ?= cc
CFLAGS ?= -O3 -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror
CPPFLAGS := -Ifirmware/include
LDLIBS := -lm

BUILD := build
CORE := firmware/src/ambient_effects.c
HEADER := firmware/include/ambient_effects.h

.PHONY: all verify sanitize benchmark previews clean

all: verify benchmark

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/effects_verify: tests/effects_verify.c $(CORE) $(HEADER) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/effects_verify.c $(CORE) -o $@ $(LDLIBS)

$(BUILD)/effects_verify_sanitize: tests/effects_verify.c $(CORE) $(HEADER) | $(BUILD)
	$(CC) $(CPPFLAGS) -O1 -g -std=c11 -Wall -Wextra -Wpedantic \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		tests/effects_verify.c $(CORE) -o $@ $(LDLIBS)

$(BUILD)/effects_benchmark: tests/effects_benchmark.c $(CORE) $(HEADER) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/effects_benchmark.c $(CORE) -o $@ $(LDLIBS)

$(BUILD)/render_effects: tools/render_effects.c $(CORE) $(HEADER) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tools/render_effects.c $(CORE) -o $@ $(LDLIBS)

verify: $(BUILD)/effects_verify
	./$(BUILD)/effects_verify

sanitize: $(BUILD)/effects_verify_sanitize
	ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
	UBSAN_OPTIONS=halt_on_error=1 ./$(BUILD)/effects_verify_sanitize

benchmark: $(BUILD)/effects_benchmark
	./$(BUILD)/effects_benchmark

previews: $(BUILD)/render_effects
	mkdir -p $(BUILD)/effects_wav
	./$(BUILD)/render_effects $(BUILD)/effects_wav

clean:
	rm -rf $(BUILD) build-cmake
