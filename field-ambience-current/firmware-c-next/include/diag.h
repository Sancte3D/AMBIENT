#ifndef FAM_DIAG_H
#define FAM_DIAG_H

/* Bring-up diagnostics screen (diag_h743.c). Draws live profiler / PSRAM /
 * battery / voice state to the framebuffer. `psram_ran`/`psram_pass` reflect
 * whether the QSPI-PSRAM self-test has been triggered and its result. */
void diag_draw(int psram_pass, int psram_ran);

#ifdef FAM_SOUND_AMBIENT_V3
/* Per-World render load bench (diag_h743.c), entered by holding CELL1 alone
 * at power-on. Cycles the six Worlds under a heavy scene and shows each
 * World's peak and average render load, deadline misses and clips. Never
 * returns (power-cycle to exit). */
void diag_world_load_run(void);
#endif

#endif /* FAM_DIAG_H */
