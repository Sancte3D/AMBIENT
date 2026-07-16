CC ?= cc
CFLAGS ?= -O3 -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion
CPPFLAGS := -Ifirmware/include
LDLIBS := -lm

BUILD := build
CORE := firmware/src/ambient_models.c firmware/src/ambient_visuals.c firmware/src/ambient_palettes.c
AUDIO_CORE := firmware/src/ambient_models.c
VISUAL_CORE := firmware/src/ambient_visuals.c firmware/src/ambient_palettes.c

.PHONY: all verify sanitize benchmark visual-benchmark previews audio-previews ui-previews color-ui-previews motion-ui-previews analyze verify-artifacts audit manifest clean

all: verify benchmark

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/verify: tests/verify.c $(CORE) firmware/include/ambient_models.h firmware/include/ambient_visuals.h firmware/include/ambient_palettes.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/verify.c $(CORE) -o $@ $(LDLIBS)

$(BUILD)/verify_sanitize: tests/verify.c $(CORE) | $(BUILD)
	$(CC) $(CPPFLAGS) -O1 -g -std=c11 -Wall -Wextra -Wpedantic -fsanitize=address,undefined -fno-omit-frame-pointer tests/verify.c $(CORE) -o $@ $(LDLIBS)

$(BUILD)/benchmark: tests/benchmark.c $(AUDIO_CORE) firmware/include/ambient_models.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/benchmark.c $(AUDIO_CORE) -o $@ $(LDLIBS)

$(BUILD)/visual_benchmark: tests/visual_benchmark.c firmware/src/ambient_visuals.c firmware/include/ambient_visuals.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/visual_benchmark.c firmware/src/ambient_visuals.c -o $@ $(LDLIBS)

$(BUILD)/render_audio: tools/render_audio.c $(AUDIO_CORE) firmware/include/ambient_models.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tools/render_audio.c $(AUDIO_CORE) -o $@ $(LDLIBS)

$(BUILD)/render_ui: tools/render_ui.c $(VISUAL_CORE) firmware/include/ambient_visuals.h firmware/include/ambient_palettes.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tools/render_ui.c $(VISUAL_CORE) -o $@ $(LDLIBS)

verify: $(BUILD)/verify audit
	./$(BUILD)/verify

sanitize: $(BUILD)/verify_sanitize
	ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ./$(BUILD)/verify_sanitize

benchmark: $(BUILD)/benchmark
	./$(BUILD)/benchmark

visual-benchmark: $(BUILD)/visual_benchmark
	./$(BUILD)/visual_benchmark

audio-previews: $(BUILD)/render_audio
	./tools/generate_audio_previews.sh

ui-previews: $(BUILD)/render_ui
	./tools/generate_ui_previews.sh

color-ui-previews: $(BUILD)/render_ui
	./tools/generate_color_ui_previews.sh

motion-ui-previews: $(BUILD)/render_ui
	./tools/generate_motion_studies.sh

previews: audio-previews ui-previews color-ui-previews motion-ui-previews

analyze: audio-previews
	python3 tools/analyze_previews.py

verify-artifacts:
	./tools/verify_artifacts.sh
	python3 tools/verify_color_previews.py
	python3 tools/verify_motion_studies.py

audit:
	./tools/audit_clean_ip.sh

manifest:
	./tools/update_manifest.sh

clean:
	rm -rf $(BUILD)
